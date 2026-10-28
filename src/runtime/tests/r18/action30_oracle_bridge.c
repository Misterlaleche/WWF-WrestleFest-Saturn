#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t rw(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t rl(const uint8_t*p){return((uint32_t)rw(p)<<16)|rw(p+2);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void wl(uint8_t*p,uint32_t v){ww(p,(uint16_t)(v>>16));ww(p+2,(uint16_t)v);}
int action30_oracle(uint8_t*mem,uint32_t*d,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t*g=mem+0x1c0000;
 dispatch.actions=&actions;memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=r.g.rng_state_valid=1;
 r.g.mode_0161=g[0x161];r.g.progression_0162=rw(g+0x162);r.g.global_007e=rw(g+0x7e);r.g.credited_route=rw(g+0x7c);
 r.g.rng_state=rl(g+0x5c);r.g.frame_counter=rl(g+0x80);r.g.global_15d2=g[0x15d2];r.g.global_15d3=g[0x15d3];
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);wl(g+0x5c,r.g.rng_state);g[0x15d2]=r.g.global_15d2;g[0x15d3]=r.g.global_15d3;
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
