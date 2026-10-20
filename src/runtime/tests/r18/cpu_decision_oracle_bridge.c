#include "../../src/wf_cpu_runtime.c"
#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static unsigned b16(const uint8_t*p){return (p[0]<<8)|p[1];}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static wf_status_t palette(wf_runtime_t*rt,const wf_r17_fg0_write_t*w,size_t n,void*user){
 uint8_t*mem=user;(void)rt;for(size_t i=0;i<n;i++){mem[w[i].addr]=(uint8_t)(w[i].value>>8);mem[w[i].addr+1]=(uint8_t)w[i].value;}return WF_OK;
}
int wf_cpu_oracle(uint8_t *mem,uint32_t *regs,unsigned slot){
 wf_runtime_t rt;wf_cpu_trace_t tr;memset(&rt,0,sizeof rt);
 uint8_t*g=mem+0x1c0000;
 memcpy(rt.actors,g+0x5b0,sizeof rt.actors);
 rt.g.global_1670=b32(g+0x1670);rt.g.global_167a=g[0x167a];
 rt.g.global_007e=b16(g+0x7e);rt.g.global_1802=b16(g+0x1802);
 rt.g.credited_route=b16(g+0x7c);rt.g.mode_0161=g[0x161];rt.g.global_0166=b16(g+0x166);
 rt.g.global_16a0=b16(g+0x16a0);rt.g.progression_0162=b16(g+0x162);
 rt.g.frame_counter=b32(g+0x80);rt.g.rng_state=b32(g+0x5c);rt.g.rng_state_valid=1;
 memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 wf_status_t status;
 memset(&tr,0,sizeof tr);
 if(slot==256u){int carry=0;status=arbitrate_1f7f0(&rt,&rt.actors[0],&carry,&tr);regs[8]=(uint32_t)carry;}
 else if(slot==257u){
  wf_r17_actions_r11_bridge_t actions;wf_r17_dispatch_1c03e_ctx_t ctx;
  wf_r17_render_queue_t queue={0};wf_r17_render_ctx_t render={0};
  memset(&actions,0,sizeof actions);memset(&ctx,0,sizeof ctx);ctx.actions=&actions;
  actions.video_batch=palette;actions.video_user=mem;
  render.queue=&queue;render.visibility_2188=wf_r17_visibility_2188_exact;
  render.scroll_a_x=b16(g+0x17e6);render.scroll_a_y=b16(g+0x17ee);actions.render=&render;
  memcpy(rt.transient_1258,g+0x1258,sizeof rt.transient_1258);
  rt.g.global_1684=b32(g+0x1684);rt.g.global_1688=b32(g+0x1688);
  rt.g.global_1150=b16(g+0x1150);rt.g.global_1180=b16(g+0x1180);rt.g.global_11b0=b16(g+0x11b0);rt.g.global_11e0=b16(g+0x11e0);
  status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[0],&ctx);tr.blocked_dependency_rom=ctx.blocked_dependency_rom;
  memcpy(g+0x1258,rt.transient_1258,sizeof rt.transient_1258);
  for(unsigned k=0;k<6;k++){
   uint32_t list=b32(mem+0x2806+k*8),count=b32(mem+0x280a+k*8);
   mem[count]=(uint8_t)(queue.count[k]>>8);mem[count+1]=(uint8_t)queue.count[k];
   for(unsigned j=0;j<queue.count[k];j++){
    uint32_t address=0x1c1258u+(uint32_t)(queue.list[k][j]-actions.transient_views)*0x2au;
    for(unsigned v=0;v<4;v++)mem[list+j*4+v]=(uint8_t)(address>>(24-v*8));
   }
  }
  unsigned at[4]={0x1150,0x1180,0x11b0,0x11e0};uint16_t v[4]={rt.g.global_1150,rt.g.global_1180,rt.g.global_11b0,rt.g.global_11e0};
  for(unsigned i=0;i<4;i++){g[at[i]]=(uint8_t)(v[i]>>8);g[at[i]+1]=(uint8_t)v[i];}
  for(unsigned i=0;i<4;i++){g[0x1684+i]=(uint8_t)(rt.g.global_1684>>(24u-i*8u));g[0x1688+i]=(uint8_t)(rt.g.global_1688>>(24u-i*8u));}
 }
 else status=wf_cpu_actor_step(&rt,slot,&tr);
 memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
 for(unsigned i=0;i<4;i++)g[0x1670+i]=(uint8_t)(rt.g.global_1670>>(24u-i*8u));g[0x167a]=rt.g.global_167a;
 g[0x1802]=(uint8_t)(rt.g.global_1802>>8);g[0x1803]=(uint8_t)rt.g.global_1802;
 g[0x1697]=rt.g.global_1697;g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;
 for(unsigned i=0;i<4;i++)g[0x5c+i]=(uint8_t)(rt.g.rng_state>>(24u-i*8u));
 return status?-(int)(tr.blocked_dependency_rom?tr.blocked_dependency_rom:tr.state5.blocked_dependency_rom):0;
}
