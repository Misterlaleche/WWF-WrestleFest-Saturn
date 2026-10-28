#include <string.h>
#include "wf_r17_candidate_2187e.h"
#include "wf_cpu_runtime.h"
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
int wf_royal_candidate_oracle(uint8_t*mem,uint32_t*regs,unsigned slot,unsigned direct,uint32_t*candidate,int*found){
 wf_runtime_t r={0};uint8_t*g=mem+0x1c0000;wf_cpu_trace_t trace={0};uint32_t dependency=0;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,regs,32);r.m68k_regs.valid=r.g.rng_state_valid=1;
 r.g.rng_state=r32(g+0x5c);r.g.frame_counter=r32(g+0x80);r.g.mode_0161=g[0x161];r.g.credited_route=r16(g+0x7c);
 r.g.progression_0162=r16(g+0x162);r.g.global_0166=r16(g+0x166);r.g.global_1692=r16(g+0x1692);r.g.global_168c=r32(g+0x168c);
 r.g.global_1697=g[0x1697];r.g.global_16a0=r16(g+0x16a0);r.g.global_007e=r16(g+0x7e);memcpy(r.g.direction_scratch_15e4,g+0x15e4,8);
 wf_status_t st=direct==1?wf_r17_candidate_2187e(&r,slot,candidate,found,&dependency):direct==2?wf_state5_execute(&r,slot,&trace.state5):wf_cpu_actor_step(&r,slot,&trace);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(regs,r.m68k_regs.d,32);
 w(g+0x1692,r.g.global_1692,2);w(g+0x168c,r.g.global_168c,4);w(g+0x5c,r.g.rng_state,4);g[0x1697]=r.g.global_1697;
 memcpy(g+0x15e4,r.g.direction_scratch_15e4,8);
 return st?-(int)(dependency?dependency:trace.blocked_dependency_rom?trace.blocked_dependency_rom:trace.state5.blocked_dependency_rom):0;
}
