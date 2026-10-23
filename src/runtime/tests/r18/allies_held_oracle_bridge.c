#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
int allies_held_oracle(uint8_t *mem,uint32_t *regs,unsigned slot){
 wf_runtime_t rt={0};wf_r17_dispatch_1c03e_ctx_t ctx={0};
 memcpy(rt.actors,mem+0x1c05b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&ctx);
 memcpy(mem+0x1c05b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
 return status?-(int)ctx.blocked_dependency_rom:0;
}
