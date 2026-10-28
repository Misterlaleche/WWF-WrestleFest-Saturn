#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint32_t r32(const uint8_t*p){return((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
int action3e_oracle(uint8_t*memory,uint32_t*d,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t*g=memory+0x1c0000;if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=1;
 r.g.rng_state=r32(g+0x5c);r.g.rng_state_valid=1;r.g.frame_counter=r32(g+0x80);
 dispatch.actions=&actions;wf_status_t status=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return status?-(int)dispatch.blocked_dependency_rom:0;
}
