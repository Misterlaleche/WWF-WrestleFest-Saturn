#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static wf_status_t video(wf_runtime_t*rt,const wf_r17_fg0_write_t*w,size_t n,void*user){
 (void)rt;uint8_t*m=user;for(size_t i=0;i<n;i++)for(unsigned j=0;j<w[i].width;j++)m[w[i].addr+j]=(uint8_t)(w[i].value>>(8u*(w[i].width-1u-j)));return WF_OK;
}
static wf_status_t resource(wf_runtime_t*rt,uint16_t id,void*user){
 (void)rt;uint8_t*m=user;uint8_t v=m[0x1c1610u+id];if(v){m[0x1c1610u+id]=0;m[0x1c1600u+(v&15u)]=0;}return WF_OK;
}
static int run(uint8_t*memory,uint32_t*d,unsigned slot,uint32_t*descriptor,unsigned disabled){
 wf_runtime_t r={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t*g=memory+0x1c0000;if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=1;r.g.global_16a7=g[0x16a7];memcpy(r.g.direction_scratch_15e4,g+0x15e4,8);
 actions.video_batch=video;actions.resource_2b58=resource;actions.video_user=memory;dispatch.actions=&actions;
 if(disabled&1u)actions.video_batch=0;if(disabled&2u)actions.resource_2b58=0;
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);g[0x16a7]=r.g.global_16a7;memcpy(g+0x15e4,r.g.direction_scratch_15e4,8);
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
int action7a_oracle(uint8_t*m,uint32_t*d,unsigned s,uint32_t*p){return run(m,d,s,p,0u);}
int action7a_oracle_guard(uint8_t*m,uint32_t*d,unsigned s,uint32_t*p,unsigned disabled){return run(m,d,s,p,disabled);}
