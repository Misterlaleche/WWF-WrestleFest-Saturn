#include <stddef.h>
#include <string.h>
#include "wf_r17_human_raw23.h"
#include "wf_r18_runtime_handoff.h"

/* The natural fixture is a HOST dump, not a native SH2 object layout. */
_Static_assert(sizeof(wf_runtime_t)==7664u,"frozen HOST runtime layout");
int human_ec34_capture(const uint8_t *dump,uint8_t *ram,uint32_t *d,size_t *layout){
 wf_runtime_t r;memcpy(&r,dump,sizeof r);memset(ram,0,0x4000u);
 if(layout){layout[0]=sizeof r;layout[1]=offsetof(wf_runtime_t,actors);layout[2]=offsetof(wf_runtime_t,m68k_regs);}
 memcpy(d,r.m68k_regs.d,32u);
 return (int)wf_r18_runtime_export(&r,ram,0x4000u);
}
int human_ec34_oracle(uint8_t *memory,uint32_t *d,unsigned slot,uint32_t *stage){
 wf_runtime_t r={0};wf_r17_m68k_shadow_t regs={0};wf_r17_human_raw23_ctx_t c={0};wf_r17_human_raw23_trace_t tr={0};
 memcpy(regs.d,d,32u);regs.valid=1u;
 if(wf_r18_runtime_import(&r,memory+0x1c0000u,0x4000u,&regs)!=WF_OK)return -1;
 c.mode_0161=r.g.mode_0161;c.global_1670=&r.g.global_1670;
 c.fixed_object0=&r.fixed_object0;c.fixed_object1=&r.fixed_object1;
 c.global_0162=r.g.progression_0162;c.m68k_live=&r.m68k_regs;
 c.rng_state_1c005c=&r.g.rng_state;c.global_1c0080=r.g.frame_counter;
 wf_status_t status=wf_r17_human_dcde_raw23(&r,slot,&c,&tr);
 if(wf_r18_runtime_export(&r,memory+0x1c0000u,0x4000u)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,32u);if(stage)*stage=tr.last_stage_rom;
 return status?-(int)tr.blocked_dependency_rom:0;
}
