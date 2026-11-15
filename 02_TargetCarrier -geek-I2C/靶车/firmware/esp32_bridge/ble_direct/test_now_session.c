#include "now_session.h"
#include "ble_guard.h"
#include "armor_wire.h"
#include <assert.h>
#include <stdio.h>
static now_request_t req(uint32_t seq,uint16_t flags){now_request_t q={.kind=5,.node=2,.boot=7,.epoch=1,.session=11,.seq=seq};ctrl_target_t t={0};proto_encode_ctrl(q.command,&t,flags,MOTOR_MODE_MIT,(uint8_t)seq);return q;}
int main(void){
 tn_session_t s={0};now_request_t q=req(1,CTRL_FLAG_ENABLE|CTRL_FLAG_ARM);
 assert(tn_accept(&s,&q,7,1,0,0)==NOW_NEED_STOP&&!s.active);
 q=req(2,CTRL_FLAG_ESTOP);assert(tn_accept(&s,&q,7,1,0,0)==NOW_OK&&s.active);
 assert(tn_accept(&s,&q,7,1,1,1)==NOW_REPLAY);q.seq=3;q.session=22;assert(tn_accept(&s,&q,7,1,1,1)==NOW_SESSION);
 q.session=11;q.epoch=0;assert(tn_accept(&s,&q,7,1,1,1)==NOW_CHALLENGE);q.epoch=1;assert(tn_accept(&s,&q,7,1,0,101)==NOW_STALE);
 q.node=1;assert(tn_accept(&s,&q,7,1,101,101)==NOW_BAD_COMMAND);
 q=req(3,CTRL_FLAG_ESTOP);s.active=false;s.session=0;assert(tn_accept(&s,&q,7,2,1,1)==NOW_CHALLENGE);
 q.epoch=2;assert(tn_accept(&s,&q,7,2,1,1)==NOW_OK);
 tb_guard_t g;tb_guard_init(&g);tb_guard_connection(&g,true);assert(tb_guard_accept(&g,q.command,1,false));
 q=req(4,proto_flags_set_token(CTRL_FLAG_ENABLE|CTRL_FLAG_ARM,8));q.epoch=2;
 assert(tn_accept(&s,&q,7,2,2,2)==NOW_OK);assert(!tb_guard_accept(&g,q.command,2,false));
 q=req(5,proto_flags_set_token(CTRL_FLAG_ENABLE|CTRL_FLAG_ARM,8));assert(tb_guard_accept(&g,q.command,3,true));
 assert(tb_guard_tick(&g,4,false)&&g.failsafe&&!g.active);
 now_request_t lamp={.kind=5,.node=2,.boot=7,.epoch=2,.session=11,.seq=6,
                     .command={PROTO_HEAD,13,ARMOR_SET,0,2,42}};
 lamp.command[15]=(uint8_t)proto_crc16(lamp.command+1,14);
 lamp.command[16]=PROTO_TAIL;
 tn_session_t none={0};
 assert(tb_frame_valid(lamp.command,17));
 assert(tn_accept(&none,&lamp,7,2,10,10)==NOW_NEED_STOP && !none.active);
 assert(tn_accept(&s,&lamp,7,2,10,10)==NOW_OK);
 assert(tn_accept(&s,&lamp,7,2,10,10)==NOW_REPLAY);
 lamp.seq++;lamp.command[4]=3;lamp.command[15]=(uint8_t)proto_crc16(lamp.command+1,14);
 assert(tn_accept(&s,&lamp,7,2,10,10)==NOW_BAD_COMMAND);
 puts("PASS armor frames use target session checks and reject invalid colors");
 puts("PASS target session STOP acquisition / node / duplicate / stale / old epoch / owner / STM freshness");return 0;
}
