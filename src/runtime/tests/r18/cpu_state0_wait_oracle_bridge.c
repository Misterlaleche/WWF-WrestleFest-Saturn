/* This test is copied into the frozen f88acad source tree by the runner. */
#include "../../src/wf_cpu_runtime.c"
static uint16_t b16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w(uint8_t*m,unsigned at,uint32_t value,unsigned n){for(unsigned i=0;i<n;i++)m[at+i]=(uint8_t)(value>>(8u*(n-1u-i)));}
int cpu_state0_wait_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
 wf_runtime_t r={0};wf_cpu_trace_t trace={0};uint8_t*g=mem+0x1c0000;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.g.direction_scratch_15e4,g+0x15e4,8);memcpy(r.m68k_regs.d,regs,32);r.m68k_regs.valid=1;r.g.rng_state_valid=1;
#define G(at,field,n) r.g.field=n==4?b32(g+at):n==2?b16(g+at):g[at]
 G(0x5c,rng_state,4);G(0x80,frame_counter,4);G(0x7c,credited_route,2);G(0x7e,global_007e,2);G(0x161,mode_0161,1);G(0x162,progression_0162,2);G(0x166,global_0166,2);
 G(0x16a0,global_16a0,2);G(0x16c4,global_16c4,2);G(0x1670,global_1670,4);G(0x167a,global_167a,1);G(0x1682,global_1682,2);G(0x1684,global_1684,4);G(0x1688,global_1688,4);G(0x168c,global_168c,4);G(0x1692,global_1692,2);G(0x1697,global_1697,1);G(0x1800,global_1800,2);G(0x1802,global_1802,2);
#undef G
 wf_status_t status=wf_cpu_actor_step(&r,slot,&trace);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(g+0x15e4,r.g.direction_scratch_15e4,8);memcpy(regs,r.m68k_regs.d,32);
#define G(at,field,n) w(g,at,r.g.field,n)
 G(0x5c,rng_state,4);G(0x1670,global_1670,4);G(0x167a,global_167a,1);G(0x1682,global_1682,2);G(0x1684,global_1684,4);G(0x1688,global_1688,4);G(0x168c,global_168c,4);G(0x1692,global_1692,2);G(0x1697,global_1697,1);G(0x1800,global_1800,2);G(0x1802,global_1802,2);
#undef G
 return status?-(int)(trace.blocked_dependency_rom?trace.blocked_dependency_rom:trace.state5.blocked_dependency_rom):0;
}
