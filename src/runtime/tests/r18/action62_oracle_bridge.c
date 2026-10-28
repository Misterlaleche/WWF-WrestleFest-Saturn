#include <stddef.h>
#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r18_runtime_handoff.h"
_Static_assert(sizeof(wf_runtime_t)==7664u,"frozen HOST runtime layout");
int action62_capture(const uint8_t *dump,uint8_t *ram,uint32_t *d,size_t *layout){
 wf_runtime_t r;memcpy(&r,dump,sizeof r);memset(ram,0,0x4000u);
 if(layout){layout[0]=sizeof r;layout[1]=offsetof(wf_runtime_t,actors);layout[2]=offsetof(wf_runtime_t,m68k_regs);}
 memcpy(d,r.m68k_regs.d,32u);return (int)wf_r18_runtime_export(&r,ram,0x4000u);
}
int action62_oracle(uint8_t *memory,uint32_t *d,unsigned slot,uint32_t *descriptor){
 wf_runtime_t r={0};wf_r17_m68k_shadow_t regs={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};
 memcpy(regs.d,d,32u);regs.valid=1u;
 if(wf_r18_runtime_import(&r,memory+0x1c0000u,0x4000u,&regs)!=WF_OK)return -1;
 dispatch.actions=&actions;
 wf_status_t status=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 if(wf_r18_runtime_export(&r,memory+0x1c0000u,0x4000u)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,32u);if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return status?-(int)dispatch.blocked_dependency_rom:0;
}
