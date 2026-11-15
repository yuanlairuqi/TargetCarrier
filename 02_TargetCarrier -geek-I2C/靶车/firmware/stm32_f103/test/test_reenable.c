/* Compile the actual 1 kHz control loop with deterministic CAN and RTOS mocks. */
#include <assert.h>
#include <stdio.h>
#include "../src/app_main.c"

static uint32_t tick;
static bool healthy, recovering, can_busy, hold_ack;
static can_tx_done_t completion;
static uint8_t last_can[8];
static unsigned sent, enables, disables;
static int mutex;

TickType_t xTaskGetTickCount(void) { return tick; }
bool can_drv_lock(uint32_t timeout) { return timeout == 0; }
void can_drv_unlock(void) {}
bool can_drv_is_healthy(void) { return healthy; }
bool can_drv_is_recovering(void) { return recovering; }
bool can_drv_tx_idle(void) { return !can_busy; }
void can_drv_abort_all(void) { can_busy = false; completion = (can_tx_done_t){0}; }
can_tx_done_t can_drv_poll_tx(void)
{
    can_tx_done_t done = completion;
    completion = (can_tx_done_t){0};
    if (done.completed) can_busy = false;
    return done;
}
bool can_drv_send(uint32_t id, const uint8_t *data, uint8_t len)
{
    assert(len == 8);
    ++sent;
    memcpy(last_can, data, len);
    if (id == MOTOR_CAN_ID && !memcmp(data, "\xff\xff\xff\xff\xff\xff\xff", 7)) {
        enables += data[7] == DM_CMD_ENABLE;
        disables += data[7] == DM_CMD_DISABLE;
    }
    if (s_arm_phase >= ARM_PRIME && s_arm_phase <= ARM_ENABLE_SENT) {
        assert(!can_busy);
        can_busy = true;
        if (!hold_ack) completion = (can_tx_done_t){1, 1, 0};
    }
    return true;
}
BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t timeout)
{
    (void)q; (void)item; (void)timeout;
    return pdFALSE;
}
BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t timeout)
{
    (void)q; (void)item; (void)timeout;
    return pdTRUE;
}

static void feedback(uint16_t raw, uint8_t state, float speed)
{
    const uint8_t data[8] = { (uint8_t)((state << 4) | MOTOR_CAN_ID),
        (uint8_t)(raw >> 8), (uint8_t)raw, 0x7f, 0xf7, 0xff, 25, 25 };
    dm_feedback_t fb = {0};
    assert(dm_parse_feedback(MOTOR_CAN_ID, data, sizeof(data), &fb));
    fb.vel_rad_s = speed;
    fb_update(&fb);
}

static void step(void) { ctrl_step(tick++); }
static void cmd(uint8_t seq, uint16_t flags, int16_t yaw, int16_t rate, bool safety)
{
    g_last_ctrl_ms = tick;
    g_have_ctrl = true;
    lc_cmd_t c = { .seq = seq, .flags = flags, .yaw_q = yaw, .yawrate_q = rate,
                   .rx_ms = tick, .safety = safety };
    assert(lc_cmdq_push(&s_cmdq, &c));
}

static void reset(void)
{
    tick = 1000;
    healthy = true;
    recovering = can_busy = hold_ack = false;
    sent = enables = disables = 0;
    completion = (can_tx_done_t){0};
    g_fb = (fb_state_t){0};
    g_last_ctrl_ms = 0;
    g_have_ctrl = false;
    g_failsafe_events = s_seen_failsafe = 0;
    g_can_recover_events = s_seen_recover = 0;
    s_fault = s_estop = s_abort_mailboxes = s_need_disable_burst = false;
    s_link_lost = true;
    s_fail_streak = s_prev_submit_fail = 0;
    s_motion_at = s_fault_mask_until_ms = 0;
    s_cmdq_mtx = &mutex;
    lc_cmdq_init(&s_cmdq);
    lc_can_tx_init(&s_can_tx);
    lc_seq_gate_init(&s_applied_ctrl);
    lc_arm_init(&s_arm);
    can_submit_disable();
    cmd(1, 0, 0, 0, true);
    for (unsigned i = 0; i < 25; ++i) step();
    s_target_kp = 200.0f;
    s_target_kd = 3.0f;
}

static void check_hold(uint16_t raw, bool zero_gain)
{
    assert(((uint16_t)last_can[0] << 8 | last_can[1]) == raw);
    assert(((unsigned)last_can[2] << 4 | last_can[3] >> 4) == 2047u);
    assert(((unsigned)(last_can[6] & 0x0f) << 8 | last_can[7]) == 2047u);
    if (zero_gain) {
        assert(!(last_can[3] & 0x0f) && !last_can[4] &&
               !last_can[5] && !(last_can[6] >> 4));
    }
}

static uint16_t arm(uint8_t token)
{
    return proto_flags_set_token(CTRL_FLAG_ENABLE | CTRL_FLAG_ARM, token);
}

static void use_hold_then_move(uint16_t raw)
{
    reset();
    feedback(raw, 0, 0);
    cmd(2, arm(1), 0, 1800, true);
    hold_ack = true;
    step();
    assert(s_arm_phase == ARM_PRIME_SENT && !enables);
    check_hold(raw, true);
    step();
    assert(s_arm_phase == ARM_PRIME_SENT && !enables);
    completion = (can_tx_done_t){1, 1, 0};
    step();
    assert(s_arm_phase == ARM_ENABLE && !enables);
    step();
    assert(s_arm_phase == ARM_ENABLE_SENT && enables == 1);
    feedback(raw, 1, 0);
    cmd(3, arm(1) | CTRL_FLAG_MOTION | CTRL_FLAG_VELOCITY, 0, -100, false);
    step();
    assert(!g_snap.armed && !g_snap.enabled && !s_motion_requested);
    completion = (can_tx_done_t){1, 1, 0};
    step();
    assert(s_arm_phase == ARM_CONFIRM);
    step();
    assert(!g_snap.armed && !g_snap.enabled);
    check_hold(raw, true);
    feedback(raw, 1, 0);
    step();
    assert(s_arm_phase == ARM_ACTIVE && g_snap.armed && g_snap.enabled);
    check_hold(raw, true);
    for (unsigned i = 0; i < GAIN_RAMP_STEPS + 1; ++i) {
        feedback(raw, 1, 0);
        cmd((uint8_t)(4 + i), arm(1) | CTRL_FLAG_VELOCITY, 0, -1800, false);
        step();
        check_hold(raw, false);
        assert(s_motion.velocity == 0 && g_snap.kp <= ARM_HOLD_KP_MAX &&
               g_snap.kd <= ARM_HOLD_KD_MAX);
    }
    uint8_t seq = (uint8_t)(5 + GAIN_RAMP_STEPS);
    cmd(seq++, arm(1) | CTRL_FLAG_MOTION | CTRL_FLAG_VELOCITY, 0, 100, false);
    step();
    assert(s_motion_requested && s_motion.velocity > 0 && g_snap.target_vel_dps > 0);
    cmd(seq++, arm(1) | CTRL_FLAG_MOTION | CTRL_FLAG_VELOCITY, 0, -100, false);
    cmd(seq++, CTRL_FLAG_ESTOP, 0, 0, true);
    step();
    assert(!s_motion_requested && !s_arm.armed && !g_snap.enabled && s_ramp == 0);
    check_hold(raw, true);
    /* Even with an explicitly new token the previous enabled feedback and
     * outstanding DISABLE burst cannot be used to enable in the next tick. */
    cmd(seq++, arm(2), 0, 0, true);
    step();
    assert(enables == 1 && !s_arm.armed && s_arm_phase == ARM_IDLE);
    feedback(raw, 0, 0);
    for (unsigned i = 0; i < 25; ++i) step();
    cmd(seq++, arm(2), 0, 0, true);
    step();
    assert(enables == 1 && !s_arm.armed);
    hold_ack = false;
    cmd(seq++, arm(3), 0, 0, true);
    step(); step(); step();
    assert(enables == 2);
}

static void invalid_preconditions(void)
{
    for (unsigned i = 0; i < 7; ++i) {
        reset();
        if (i != 0) feedback(41234, i == 3 ? 1 : i == 4 ? 10 : 0, i == 2 ? .2f : 0);
        if (i == 1) tick += ARM_FB_MAX_AGE_MS + 1;
        healthy = i != 5;
        recovering = i == 6;
        cmd(2, arm(1), 0, 0, true);
        step(); step(); step();
        assert(!enables && !s_arm.armed && !g_snap.enabled);
    }
    reset();
    feedback(41234, 0, 0);
    cmd(2, arm(1) | CTRL_FLAG_MOTION, 100, 0, true);
    step(); step(); step();
    assert(!enables && !s_arm.armed);
}

static void delayed_motion_before_confirmation(void)
{
    reset();
    feedback(41234, 0, 0);
    cmd(2, arm(1), 0, 0, true);
    step(); step(); step(); step();
    feedback(41234, 1, 0);
    step();
    assert(s_arm_phase == ARM_ACTIVE);
    cmd(3, arm(1) | CTRL_FLAG_MOTION | CTRL_FLAG_VELOCITY, 0, -100, false);
    s_cmdq.q[0].rx_ms = s_arm_confirmed_at;
    step();
    assert(!s_motion_requested);
    cmd(4, arm(1) | CTRL_FLAG_MOTION | CTRL_FLAG_VELOCITY, 0, 100, false);
    step();
    assert(s_motion_requested && s_motion.velocity > 0);
}

static void interrupted_enable(void)
{
    for (unsigned i = 0; i < 4; ++i) {
        reset();
        feedback(41234, 0, 0);
        hold_ack = true;
        cmd(2, arm(1), 0, 0, true);
        step();
        if (i == 0) {
            completion = (can_tx_done_t){1, 0, 1};
        } else if (i == 1) {
            cmd(3, CTRL_FLAG_ESTOP, 0, 0, true);
        } else if (i == 2) {
            g_failsafe_events++;
        } else {
            feedback(42000, 0, 0);
        }
        step();
        assert(!enables && !s_arm.armed && !g_snap.enabled);
    }
    reset();
    feedback(41234, 0, 0);
    cmd(2, arm(1), 0, 0, true);
    step(); step();
    hold_ack = true;
    step();
    assert(enables == 1 && s_arm_phase == ARM_ENABLE_SENT);
    completion = (can_tx_done_t){1, 0, 1};
    step();
    assert(!s_arm.armed && !g_snap.enabled);
}

int main(void)
{
    const float rad = 180.0f / (float)M_PI;
    for (uint32_t code = 0; code <= UINT16_MAX; code++) {
        float pos = dm_uint_to_float((uint16_t)code, -MOTOR_PMAX, MOTOR_PMAX, 16);
        float deg = pos * rad;
        float orientation = deg - roundf(deg / 360.0f) * 360.0f;
        float quantized = roundf(orientation * 100.0f) * 0.01f / rad;
        assert(fabsf(gm_nearest(quantized, pos, MOTOR_PMAX) - pos) <= 0.0001f);
    }
    use_hold_then_move(0);
    use_hold_then_move(4095);
    use_hold_then_move(61439);
    use_hold_then_move(65535);
    invalid_preconditions();
    interrupted_enable();
    delayed_motion_before_confirmation();
    puts("Re-enable CAN frames, full position range, stale or moving feedback, and STOP barriers passed");
    return 0;
}
