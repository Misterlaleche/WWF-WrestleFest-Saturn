#include <string.h>
#include "wf_cpu_runtime.h"
static uint16_t b16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return((uint32_t)b16(p)<<16)|b16(p+2);}
static void word(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
int wf_cpu_1d032_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
 wf_runtime_t rt={0};wf_cpu_trace_t trace={0};uint8_t*g=mem+0x1c0000;
 memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.fixed_object0.b,g+0xf1c,0x10c);memcpy(rt.fixed_object1.b,g+0x1028,0x10c);
 memcpy(rt.m68k_regs.d,regs,32);memcpy(rt.g.direction_scratch_15e4,g+0x15e4,8);rt.m68k_regs.valid=1;rt.g.rng_state_valid=1;
 rt.g.rng_state=b32(g+0x5c);rt.g.frame_counter=b32(g+0x80);rt.g.credited_route=b16(g+0x7c);rt.g.global_007e=b16(g+0x7e);rt.g.mode_0161=g[0x161];rt.g.progression_0162=b16(g+0x162);rt.g.global_0166=b16(g+0x166);rt.g.global_1697=g[0x1697];
 wf_status_t status=wf_cpu_actor_step(&rt,slot,&trace);
 memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(g+0xf1c,rt.fixed_object0.b,0x10c);memcpy(g+0x1028,rt.fixed_object1.b,0x10c);
 memcpy(regs,rt.m68k_regs.d,32);memcpy(g+0x15e4,rt.g.direction_scratch_15e4,8);g[0x1697]=rt.g.global_1697;word(g+0x5c,(uint16_t)(rt.g.rng_state>>16));word(g+0x5e,(uint16_t)rt.g.rng_state);
 return status?-(int)(trace.blocked_dependency_rom?trace.blocked_dependency_rom:trace.state5.blocked_dependency_rom):0;
}
