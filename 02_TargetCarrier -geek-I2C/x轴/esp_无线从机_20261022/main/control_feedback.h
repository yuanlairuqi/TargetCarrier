#ifndef CONTROL_FEEDBACK_H
#define CONTROL_FEEDBACK_H
#include "robot_control.h"
#include "robot_bridge.h"
// A reused per-source sequence is not proof that the current revision reached STM32.
static inline void control_feedback(robot_control_t *r,const robot_bridge_t *b) {
    bool current=b->online&&b->confirmed&&!b->waiting&&b->revision==r->revision&&b->completed_phone==r->ack;
    r->current_hz=b->online?b->current_hz:0;r->target_hz=b->online?b->target_hz:0;
    r->stm_mode=b->peer_mode;r->stm_phase=b->peer_phase;r->stm_online=b->online;
    r->stm_motion=b->peer_motion;r->stm_error=b->peer_error;
    r->stm_flags=current?b->peer_flags:(uint8_t)(b->peer_flags&~PEER_APPLIED);
    r->stm_ack=current?b->ack:0;
    if (r->require_stm_limits) {
        r->output_status=b->online?b->output_status:0;
        bool valid=(r->output_status & X_OUTPUT_PRESENT)!=0;
        r->output_direction=valid ? (r->output_status >> X_OUTPUT_DIR_SHIFT)&3U : 0;
        r->current_hz=valid?b->output_hz:0;
        r->stm_motion=r->output_direction;
        if (valid && (r->output_status & X_LIMIT_LATCHED)) {
            robot_latch_limit(r);
        } else if (valid && r->limit_latched) {
            /* Clear only on a current STOP ACK. A restart or unrelated
             * heartbeat does not release the ESP-side motion barrier. */
            bool reset_pending=r->accel==X_RESET_ACCEL && r->decel==X_RESET_DECEL;
            if (current && reset_pending && b->reset_confirmed && !b->peer_error && !r->motion &&
                (b->peer_flags&PEER_APPLIED) && !b->output_hz) {
                r->limit_latched=false;
                r->error=ROBOT_OK;
                r->accel=r->decel=200;
            }
        }
        if (!valid || !current) {
            r->stm_flags&=(uint8_t)~PEER_APPLIED;
            r->stm_ack=0;
        }
    }
}
static inline bool control_limit_reset_ready(const robot_control_t *r,const robot_bridge_t *b,uint32_t now) {
    return r->uart_configured && b->enabled && b->online && !b->waiting && b->confirmed &&
        b->revision==r->revision && b->completed_phone==r->ack &&
        (uint32_t)(now-b->last_rx)<BRIDGE_ONLINE_MS && !b->error &&
        (b->peer_flags&PEER_APPLIED) && !(b->peer_flags&(PEER_DEMO|PEER_LOCAL_BUSY)) &&
        !b->peer_motion && !b->peer_phase && !b->peer_error && !b->current_hz && !b->target_hz;
}
#endif
