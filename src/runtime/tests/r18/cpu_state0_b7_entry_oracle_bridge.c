#include <string.h>
#include "wf_cpu_runtime.h"
static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
int wf_cpu_state0_b7_entry_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
    wf_runtime_t rt={0};wf_cpu_trace_t trace={0};uint8_t*g=mem+0x1c0000;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);
    memcpy(rt.g.direction_scratch_15e4,g+0x15e4,8);rt.m68k_regs.valid=1;rt.g.rng_state_valid=1;
    rt.g.rng_state=b32(g+0x5c);rt.g.frame_counter=b32(g+0x80);
    rt.g.credited_route=b16(g+0x7c);rt.g.mode_0161=g[0x161];rt.g.global_1697=g[0x1697];
    wf_status_t status=wf_cpu_actor_step(&rt,slot,&trace);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
    memcpy(g+0x15e4,rt.g.direction_scratch_15e4,8);w(g+0x5c,rt.g.rng_state,4);g[0x1697]=rt.g.global_1697;
    return status?-(int)(trace.blocked_dependency_rom?trace.blocked_dependency_rom:trace.state5.blocked_dependency_rom):0;
}
