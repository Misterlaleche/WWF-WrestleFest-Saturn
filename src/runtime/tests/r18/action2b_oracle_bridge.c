#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r17_rng1129e.h"
#include "wf_r17_resolver_280dc.h"
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
int action2b_oracle(uint8_t*memory,uint32_t*d,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};
 uint8_t*g=memory+0x1c0000;if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=r.g.rng_state_valid=1;
 r.g.mode_0161=g[0x161];r.g.progression_0162=r16(g+0x162);r.g.credited_route=r16(g+0x7c);r.g.global_007e=r16(g+0x7e);
 r.g.rng_state=r32(g+0x5c);r.g.frame_counter=r32(g+0x80);r.g.global_1800=r16(g+0x1800);
 r.g.global_15d2=g[0x15d2];r.g.global_15d3=g[0x15d3];
 r.g.global_1150=r16(g+0x1150);r.g.global_1180=r16(g+0x1180);r.g.global_11b0=r16(g+0x11b0);r.g.global_11e0=r16(g+0x11e0);
 dispatch.actions=&actions;wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);
 w(g+0x5c,r.g.rng_state,4);w(g+0x1800,r.g.global_1800,2);g[0x15d2]=r.g.global_15d2;g[0x15d3]=r.g.global_15d3;
 w(g+0x1150,r.g.global_1150,2);w(g+0x1180,r.g.global_1180,2);w(g+0x11b0,r.g.global_11b0,2);w(g+0x11e0,r.g.global_11e0,2);
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
int action2b_helper_oracle(uint8_t*memory,uint32_t*d,unsigned slot,unsigned helper,int*carry){
 wf_runtime_t r={0};uint8_t*g=memory+0x1c0000;if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=r.g.rng_state_valid=1;
 r.g.mode_0161=g[0x161];r.g.progression_0162=r16(g+0x162);r.g.global_007e=r16(g+0x7e);
 r.g.rng_state=r32(g+0x5c);r.g.frame_counter=r32(g+0x80);
 wf_status_t st;uint32_t blocked=0u;
 if(helper==0x1129eu){
  wf_r17_1129e_trace_t trace;
  st=wf_r17_1129e_live(&r,slot,r.g.mode_0161,r.g.progression_0162,&r.g.rng_state,r.g.frame_counter,&r.m68k_regs,&trace,carry);
  blocked=trace.blocked_dependency_rom;
 }else if(helper==0x280dcu){st=wf_r17_resolver_280dc(&r,&r.actors[slot]);blocked=0x280dcu;}
 else return -1;
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);w(g+0x5c,r.g.rng_state,4);
 return st?-(int)blocked:0;
}
