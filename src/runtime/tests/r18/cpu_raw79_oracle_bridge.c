#include <string.h>
#include "wf_cpu_runtime.h"
#include "wf_r18_runtime_handoff.h"
int cpu_raw79_oracle(uint8_t*mem,uint32_t*d,unsigned slot){
 wf_runtime_t r={0};wf_r17_m68k_shadow_t regs={0};wf_cpu_trace_t tr={0};
 memcpy(regs.d,d,32);regs.valid=1;
 if(wf_r18_runtime_import(&r,mem+0x1c0000,0x4000,&regs)!=WF_OK)return -1;
 wf_status_t st=wf_cpu_actor_step(&r,slot,&tr);
 if(wf_r18_runtime_export(&r,mem+0x1c0000,0x4000)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,32);
 return st?-(int)(tr.blocked_dependency_rom?tr.blocked_dependency_rom:tr.state5.blocked_dependency_rom):0;
}
