#include <string.h>
#include "wf_actor_prepass.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_runtime_handoff.h"
int f18a_facing_oracle(uint8_t *mem,uint32_t *d,uint32_t *blocked){
 wf_runtime_t r={0};wf_gameplay_binding_context_t c={0};wf_r17_m68k_shadow_t regs={0};
 memcpy(regs.d,d,sizeof regs.d);regs.valid=1;
 if(wf_r18_runtime_import(&r,mem+0x1c0000u,0x4000u,&regs)!=WF_OK)return -1;
 wf_status_t st=wf_phase_actor_prepass_f18a(&r,0x00f18au,&c);
 if(blocked)*blocked=c.last_blocked_dependency;
 if(wf_r18_runtime_export(&r,mem+0x1c0000u,0x4000u)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,sizeof r.m68k_regs.d);return (int)st;
}
