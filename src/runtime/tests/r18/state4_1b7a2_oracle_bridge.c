#include <string.h>
#include "../../src/wf_r17_dispatch_1c03e.c"
int wf_state4_1b7a2_oracle(uint8_t*mem,uint32_t*regs,unsigned slot,unsigned direct,uint32_t*descriptor){
 wf_runtime_t rt={0};wf_r17_dispatch_1c03e_ctx_t ctx={0};uint8_t*g=mem+0x1c0000;
 memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 unsigned alternate=0;wf_status_t st;
 if(direct){st=state4_1b7a2(&rt,&rt.actors[slot],&ctx,&alternate);if(st==WF_OK){state4_1b7a2_tick(&rt.actors[slot],alternate);exact_tail_1c0b4(&rt.actors[slot]);}}
 else st=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&ctx);
 memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);if(descriptor)*descriptor=alternate?0x1b796:0x1b78a;
 return st?-(int)ctx.blocked_dependency_rom:0;
}
