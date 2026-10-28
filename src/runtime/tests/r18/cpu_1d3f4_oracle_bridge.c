#include <stddef.h>
#include <string.h>
#include "wf_cpu_runtime.h"
#include "wf_r18_runtime_handoff.h"
_Static_assert(sizeof(wf_runtime_t)==7664u,"frozen HOST runtime layout");
int cpu_1d3f4_capture(const uint8_t *dump,uint8_t *ram,uint32_t *d,size_t *layout){
 wf_runtime_t r;memcpy(&r,dump,sizeof r);memset(ram,0,0x4000u);
 if(layout){layout[0]=sizeof r;layout[1]=offsetof(wf_runtime_t,actors);layout[2]=offsetof(wf_runtime_t,m68k_regs);}
 memcpy(d,r.m68k_regs.d,32u);return (int)wf_r18_runtime_export(&r,ram,0x4000u);
}
int cpu_1d3f4_oracle(uint8_t *memory,uint32_t *d,unsigned slot,unsigned valid){
 wf_runtime_t r={0};wf_r17_m68k_shadow_t regs={0};wf_cpu_trace_t tr={0};
 memcpy(regs.d,d,32u);regs.valid=1u;
 if(wf_r18_runtime_import(&r,memory+0x1c0000u,0x4000u,&regs)!=WF_OK)return -1;
 r.g.rng_state_valid=(uint8_t)((valid&2u)!=0u);r.m68k_regs.valid=(uint8_t)((valid&1u)!=0u);
 wf_status_t status=wf_cpu_actor_step(&r,slot,&tr);
 if(wf_r18_runtime_export(&r,memory+0x1c0000u,0x4000u)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,32u);
 return status?-(int)(tr.blocked_dependency_rom?tr.blocked_dependency_rom:tr.state5.blocked_dependency_rom):0;
}
