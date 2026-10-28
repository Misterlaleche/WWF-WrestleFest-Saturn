#include <stddef.h>
#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r18_runtime_handoff.h"
_Static_assert(sizeof(wf_runtime_t)==7664u,"frozen HOST runtime layout");
int action20_capture(const uint8_t*dump,uint8_t*ram,uint32_t*d,size_t*layout){
 wf_runtime_t r;memcpy(&r,dump,sizeof r);memset(ram,0,0x4000u);
 layout[0]=sizeof r;layout[1]=offsetof(wf_runtime_t,actors);layout[2]=offsetof(wf_runtime_t,m68k_regs);
 memcpy(d,r.m68k_regs.d,32u);return(int)wf_r18_runtime_export(&r,ram,0x4000u);
}
int action20_oracle(uint8_t*mem,uint32_t*d,unsigned slot,uint32_t*descriptor){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};
 wf_r17_dispatch_1c03e_ctx_t dispatch={0};wf_r17_m68k_shadow_t regs={0};
 memcpy(regs.d,d,32);regs.valid=1;
 if(wf_r18_runtime_import(&r,mem+0x1c0000,0x4000,&regs)!=WF_OK)return -1;
 actions.fixed_object0=&r.fixed_object0;actions.fixed_object1=&r.fixed_object1;dispatch.actions=&actions;
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 if(wf_r18_runtime_export(&r,mem+0x1c0000,0x4000)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,32);*descriptor=actions.trace[slot].tick_descriptor_rom;
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
