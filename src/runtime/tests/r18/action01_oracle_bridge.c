#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
int action01_oracle(uint8_t*memory,uint32_t*d,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t*g=memory+0x1c0000;if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=1;r.g.mode_0161=g[0x161];r.g.global_1670=r32(g+0x1670);
 r.g.global_1150=r16(g+0x1150);r.g.global_1180=r16(g+0x1180);r.g.global_11b0=r16(g+0x11b0);r.g.global_11e0=r16(g+0x11e0);
 dispatch.actions=&actions;wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);w(g+0x1670,r.g.global_1670,4);
 w(g+0x1150,r.g.global_1150,2);w(g+0x1180,r.g.global_1180,2);w(g+0x11b0,r.g.global_11b0,2);w(g+0x11e0,r.g.global_11e0,2);
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
