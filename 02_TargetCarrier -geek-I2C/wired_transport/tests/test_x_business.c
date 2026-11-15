
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "robot_uart.h"
#include "robot_wire.h"
#include "robot_peer.h"
#include "robot_bridge.h"
#include "control_feedback.h"
#include "motor_motion.h"
#include "motor_config.h"
#include "wl_stm.h"
#include "wl_codec.h"
TIM_TypeDef test_tim1;static uint32_t tick,mask;static wl_codec master,slave;
static uint8_t pending[128],feedback[32];static bool have_rx,have_feedback;static unsigned sent;
uint32_t HAL_GetTick(void){return tick;}
void HAL_GPIO_WritePin(void*p,uint32_t n,GPIO_PinState v){(void)p;(void)n;(void)v;}
uint32_t __get_PRIMASK(void){return mask;}void __disable_irq(void){mask=1;}void __set_PRIMASK(uint32_t p){mask=p;}
bool wl_stm_init(void){wl_codec_init(&slave,false,1,0);return true;}
bool wl_stm_bound(void){return slave.bound;}void wl_stm_poll(void){}
unsigned wl_stm_read(uint8_t*out,unsigned cap){if(!have_rx)return 0;have_rx=false;unsigned n;if(!wl_decode(&slave,pending,out,&n))return 0;assert(n<=cap);return n;}
bool wl_stm_send(const uint8_t*d,unsigned n){uint8_t b[128];unsigned got;if(!wl_encode(&slave,d,n,false,tick,b))return false;assert(wl_decode(&master,b,feedback,&got)&&got==32);have_feedback=true;sent++;return true;}
uint32_t wl_stm_errors(void){return slave.rejected;}
static void Frame(const wire_frame_t*f,bool safe){uint8_t d[32];wire_encode(f,d);assert(wl_encode(&master,d,32,safe,tick,pending));have_rx=true;have_feedback=false;RobotUART_Poll();}
static wire_frame_t Reply(void){wire_frame_t r;assert(have_feedback&&wire_decode(feedback,&r));return r;}
static void Advance(unsigned n){while(n--){tick++;RobotUART_Poll();Motor_Process();}}

static wire_frame_t Exchange(robot_bridge_t *bridge)
{
    wire_frame_t frame;
    assert(bridge_tick(bridge,tick,&frame));
    Frame(&frame,frame.type!=WIRE_PING && frame.payload[0]==0);
    wire_frame_t reply=Reply();
    assert(bridge_receive(bridge,&reply,tick));
    return frame;
}

static void ProfileRequest(robot_bridge_t *b, uint32_t seq, uint8_t op, bool reset)
{
    bridge_profile(b,4,reset?X_RESET_ACCEL:200,reset?X_RESET_DECEL:200);
    bridge_parameters(b,0,0,0);
    bridge_request(b,seq,op,op?40:0,seq,tick);
}

static void CheckLimit(robot_bridge_t *b, uint8_t inputs)
{
    assert(b->online && b->error==BRIDGE_OK && b->peer_error==X_PEER_LIMIT);
    assert(b->output_status==(X_OUTPUT_PRESENT|X_LIMIT_LATCHED|inputs));
    assert(!b->output_hz && !Motor_OutputActive() && Motor_OutputStopped());
}

static void CheckLimits(void)
{
    robot_bridge_t b;
    robot_control_t r;
    robot_init(&r);
    r.uart_configured=true;
    r.require_stm_limits=true;
    robot_connection(&r,true);
    bridge_init(&b,true,73);
    (void)Exchange(&b);
    CheckLimit(&b,3);
    control_feedback(&r,&b);
    assert(r.limit_latched && !robot_can_move(&r));

    ProfileRequest(&b,1,1,false);
    (void)Exchange(&b);
    CheckLimit(&b,3);
    ProfileRequest(&b,2,0,true);
    wire_frame_t denied=Exchange(&b);
    CheckLimit(&b,3);
    assert(b.peer_flags&PEER_APPLIED);

    Motor_TestLimitPins(UINT16_MAX,false);
    Advance(120);
    Frame(&denied,true); /* same token cannot clear a denied reset */
    assert(Reply().payload[15]==(X_OUTPUT_PRESENT|X_LIMIT_LATCHED));
    ProfileRequest(&b,3,0,false);
    (void)Exchange(&b);
    CheckLimit(&b,0);

    ProfileRequest(&b,4,0,true);
    (void)Exchange(&b);
    assert(b.reset_confirmed && b.output_status==X_OUTPUT_PRESENT && !b.output_hz);
    r.protocol=4;r.revision=b.revision;r.ack=b.ack;r.accel=X_RESET_ACCEL;r.decel=X_RESET_DECEL;
    control_feedback(&r,&b);
    assert(!r.limit_latched && robot_can_move(&r));
    uint8_t status[ROBOT_STATUS_SIZE];
    robot_status(&r,tick,status);
    assert(status[0]==4 && status[1]==0 && status[19]==0x80 && (status[2]&32));
    Advance(150);
    assert(Motor_OutputStopped()); /* no queued start or lease after reset */

    ProfileRequest(&b,5,2,false);
    wire_frame_t old_move=Exchange(&b);
    Advance(140);
    assert(Motor_OutputActive());
    assert(bridge_tick(&b,tick+BRIDGE_PING_MS,&denied));
    Frame(&denied,false);
    wire_frame_t reply=Reply();
    assert(bridge_receive(&b,&reply,tick));
    assert(b.output_hz>0 && ((b.output_status>>X_OUTPUT_DIR_SHIFT)&3)==2);

    Motor_TestLimitPins(UINT16_MAX & ~(1U << 12),true);
    assert(Motor_OutputActive() && Motor_LimitStatus()==0); /* old PB12 is ignored */
    Motor_TestLimitPins(UINT16_MAX & ~(1U << 13),true); /* PB13 -> limit 2 */
    assert(!(test_tim1.CR1&TIM_CR1_CEN)); /* before the RTOS task runs */
    assert((test_tim1.CCMR1&TIM_CCMR1_OC1M)==(4U<<TIM_CCMR1_OC1M_Pos));
    assert(Motor_OutputStopped());
    ProfileRequest(&b,6,0,false);
    (void)Exchange(&b);
    CheckLimit(&b,X_LIMIT_RIGHT);
    control_feedback(&r,&b);
    assert(r.limit_latched && !robot_can_move(&r));
    robot_status(&r,tick,status);
    assert(status[7]==ROBOT_LIMIT_STOP && status[19]==0xe0 && status[3]==0);
    bridge_tx_failed(&b);
    control_feedback(&r,&b);
    robot_status(&r,tick,status);
    assert(!(status[2]&8) && status[19]==0 && status[7]==ROBOT_LIMIT_STOP);

    Motor_TestLimitPins(UINT16_MAX,false);
    Advance(20);
    ProfileRequest(&b,7,0,true);
    (void)Exchange(&b);
    CheckLimit(&b,0); /* release must remain stable for 100 ms */
    Advance(100);
    ProfileRequest(&b,8,0,true);
    (void)Exchange(&b);
    assert(b.output_status==X_OUTPUT_PRESENT && Motor_OutputStopped());
    Frame(&old_move,false);
    Advance(140);
    assert(Motor_OutputStopped()); /* an earlier movement token cannot replay */

    ProfileRequest(&b,9,1,false);
    (void)Exchange(&b);
    Advance(140);
    assert(Motor_OutputActive());
    Motor_TestLimitPins(UINT16_MAX & ~(1U << 14),true); /* PB14 -> limit 1 */
    assert(Motor_OutputStopped());
    assert(Motor_LimitStatus()==(X_LIMIT_LEFT|X_LIMIT_LATCHED));
    Motor_TestLimitPins(UINT16_MAX,false); /* brief edge still latches */
    Advance(110);
    ProfileRequest(&b,10,0,true);
    (void)Exchange(&b);
    assert(b.output_status==X_OUTPUT_PRESENT && Motor_OutputStopped());
    puts("PASS PB14/PB13 mapping, unused PB12, boot limits, EXTI-equivalent cut, latch/release, explicit reset, stale motion and offline status");
}

int main(void){UART_HandleTypeDef unused={USART3,0};TIM_HandleTypeDef timer={TIM1};test_tim1.CCMR1=TIM_OCMODE_PWM2;wl_codec_init(&master,true,1,0x12345678);RobotUART_Init(&unused);Motor_TestLimitPins(0,false);Motor_StartOnce(&timer);Motor_Stop();assert(Motor_OutputStopped());CheckLimits();robot_bridge_t bridge;wire_frame_t f,r;bridge_init(&bridge,true,42);assert(bridge_tick(&bridge,tick,&f));Frame(&f,true);r=Reply();assert(bridge_receive(&bridge,&r,tick)&&bridge.confirmed&&(bridge.peer_flags&PEER_APPLIED));
 bridge_request(&bridge,1,1,40,101,tick);assert(bridge_tick(&bridge,tick,&f));Frame(&f,false);r=Reply();assert(bridge_receive(&bridge,&r,tick));Advance(15);assert(Motor_OutputActive());uint8_t old[128];memcpy(old,pending,128);
 bridge_request(&bridge,2,0,0,102,tick);assert(bridge_tick(&bridge,tick,&f));Frame(&f,true);r=Reply();assert(bridge_receive(&bridge,&r,tick));assert(Motor_OutputStopped());
 memcpy(pending,old,128);have_rx=true;RobotUART_Poll();assert(Motor_OutputStopped());Advance(700);assert(!g_robot_uart_status.linked&&Motor_OutputStopped());memcpy(pending,old,128);have_rx=true;RobotUART_Poll();assert(Motor_OutputStopped());
 wl_codec_init(&master,true,1,77);wire_frame_t move={.type=WIRE_COMMAND,.session=77,.token=1};move.payload[0]=1;move.payload[1]=40;move.payload[2]=0xf4;move.payload[3]=1;Frame(&move,false);assert(Motor_OutputStopped());wire_frame_t stop={.type=WIRE_COMMAND,.session=77,.token=2};stop.payload[2]=0xf4;stop.payload[3]=1;Frame(&stop,true);assert(Motor_OutputStopped());r=Reply();assert(r.session==77&&r.payload[0]==0&&(r.payload[2]&PEER_APPLIED));
 assert(sent>=3);puts("PASS production ESP bridge + new STM wired adapter + motor model: STOP handshake, explicit start/stop, old packet, timeout, new owner STOP-first");return 0;}
