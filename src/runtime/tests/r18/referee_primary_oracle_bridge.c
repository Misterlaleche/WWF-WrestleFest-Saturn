/* Include the owner to observe its primary stage before action dispatch. */
#include "../../src/wf_r17_referee.c"
#include <string.h>

static uint32_t read_be(const uint8_t *p,unsigned n){
 uint32_t v=0;while(n--)v=(v<<8)|*p++;return v;
}
static void write_be(uint8_t *p,uint32_t v,unsigned n){
 for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-i-1u)));
}
__attribute__((visibility("default")))
int referee_primary_oracle(uint8_t *mem,uint32_t *regs){
 wf_runtime_t rt={0};wf_r17_referee_ctx_t ctx={0};uint8_t *ram=mem+0x1c0000;
 memcpy(rt.actors,ram+0x5b0,sizeof rt.actors);
 memcpy(rt.referee.b,ram+0x11f4,0x64);
 memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 rt.g.credited_route=read_be(ram+0x7c,2);
 rt.g.mode_0161=ram[0x161];rt.g.global_1682=read_be(ram+0x1682,2);
 rt.g.global_1684=read_be(ram+0x1684,4);rt.g.global_1688=read_be(ram+0x1688,4);
 wf_status_t status=primary_dispatch(&rt,&rt.referee,&ctx);
 memcpy(ram+0x5b0,rt.actors,sizeof rt.actors);
 memcpy(ram+0x11f4,rt.referee.b,0x64);memcpy(regs,rt.m68k_regs.d,32);
 write_be(ram+0x1682,rt.g.global_1682,2);
 return status?-(int)(ctx.blocked_dependency_rom?ctx.blocked_dependency_rom:status):0;
}
