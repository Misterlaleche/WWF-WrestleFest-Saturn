#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t b16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return((uint32_t)b16(p)<<16)|b16(p+2);}
static void word(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static wf_status_t video(wf_runtime_t*rt,const wf_r17_fg0_write_t*writes,size_t count,void*user){
 (void)rt;uint8_t*mem=user;for(size_t i=0;i<count;i++)for(unsigned k=0;k<writes[i].width;k++)mem[writes[i].addr+k]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-k)));return WF_OK;
}
int wf_action25_oracle(uint8_t*mem,uint32_t*regs,unsigned slot,uint32_t*descriptor){
 wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};dispatch.actions=&actions;
 actions.video_batch=video;actions.video_user=mem;uint8_t*g=mem+0x1c0000;memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.referee.b,g+0x11f4,sizeof rt.referee.b);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 rt.g.config_0066=b16(g+0x66);rt.g.global_1682=b16(g+0x1682);rt.g.global_169e=b16(g+0x169e);rt.g.global_007e=b16(g+0x7e);rt.g.mode_0161=g[0x161];rt.g.credited_route=b16(g+0x7c);rt.g.progression_0162=b16(g+0x162);rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];rt.g.global_1800=b16(g+0x1800);rt.g.rng_state=b32(g+0x5c);rt.g.rng_state_valid=1;rt.g.frame_counter=b32(g+0x80);
 rt.g.global_1150=b16(g+0x1150);rt.g.global_1180=b16(g+0x1180);rt.g.global_11b0=b16(g+0x11b0);rt.g.global_11e0=b16(g+0x11e0);
 wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
 memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);memcpy(regs,rt.m68k_regs.d,32);word(g+0x1682,rt.g.global_1682);word(g+0x169e,rt.g.global_169e);
 g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;word(g+0x1800,rt.g.global_1800);
 word(g+0x1150,rt.g.global_1150);word(g+0x1180,rt.g.global_1180);word(g+0x11b0,rt.g.global_11b0);word(g+0x11e0,rt.g.global_11e0);
 word(g+0x5c,(uint16_t)(rt.g.rng_state>>16));word(g+0x5e,(uint16_t)rt.g.rng_state);
 if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
 return status?-(int)(dispatch.blocked_dependency_rom?dispatch.blocked_dependency_rom:status):0;
}
