#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r18_runtime_handoff.h"
static wf_status_t video(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t count,void*user){
 (void)r;uint8_t*m=user;
 for(size_t i=0;i<count;i++)for(unsigned j=0;j<w[i].width;j++)m[w[i].addr+j]=(uint8_t)(w[i].value>>(8u*(w[i].width-1u-j)));
 return WF_OK;
}
int action3b_oracle(uint8_t*mem,uint32_t*d,unsigned slot,uint32_t*descriptor,unsigned video_enabled){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t a={0};wf_r17_dispatch_1c03e_ctx_t c={0};wf_r17_m68k_shadow_t regs={0};
 memcpy(regs.d,d,32);regs.valid=1;if(wf_r18_runtime_import(&r,mem+0x1c0000,0x4000,&regs)!=WF_OK)return -1;
 a.fixed_object0=&r.fixed_object0;a.fixed_object1=&r.fixed_object1;a.video_batch=video_enabled?video:NULL;a.video_user=mem;c.actions=&a;
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&c);
 if(wf_r18_runtime_export(&r,mem+0x1c0000,0x4000)!=WF_OK)return -2;
 memcpy(d,r.m68k_regs.d,32);*descriptor=a.trace[slot].tick_descriptor_rom;return st?-(int)c.blocked_dependency_rom:0;
}
