#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
int wf_action12_oracle(uint8_t*mem,uint32_t*regs,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t*g=mem+0x1c0000;dispatch.actions=&actions;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,regs,32);r.m68k_regs.valid=r.g.rng_state_valid=1;
 r.g.mode_0161=g[0x161];r.g.rng_state=r32(g+0x5c);r.g.frame_counter=r32(g+0x80);r.g.global_1670=r32(g+0x1670);r.g.global_15d2=g[0x15d2];r.g.global_15d3=g[0x15d3];
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(regs,r.m68k_regs.d,32);
 w(g+0x1670,r.g.global_1670,4);g[0x15d2]=r.g.global_15d2;g[0x15d3]=r.g.global_15d3;
 *descriptor=actions.trace[slot].tick_descriptor_rom;return st?-(int)dispatch.blocked_dependency_rom:0;
}
