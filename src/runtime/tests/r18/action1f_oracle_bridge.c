#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
int action1f_oracle(uint8_t*mem,uint32_t*d,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t*g=mem+0x1c0000;
 if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(&r.fixed_object0,g+0xf1c,sizeof r.fixed_object0);memcpy(&r.fixed_object1,g+0x1028,sizeof r.fixed_object1);
 memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=1;actions.fixed_object0=&r.fixed_object0;actions.fixed_object1=&r.fixed_object1;dispatch.actions=&actions;
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(g+0xf1c,&r.fixed_object0,sizeof r.fixed_object0);memcpy(g+0x1028,&r.fixed_object1,sizeof r.fixed_object1);memcpy(d,r.m68k_regs.d,32);
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
