#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static unsigned b16(const uint8_t*p){return (p[0]<<8)|p[1];}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w(uint8_t *p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-1-i)));}
int wf_raw71_oracle(uint8_t *mem,uint32_t *regs){
 wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t b={0};wf_r17_dispatch_1c03e_ctx_t ctx={0};ctx.actions=&b;uint8_t*g=mem+0x1c0000;
 memcpy(rt.g.direction_scratch_15e4,g+0x15e4,8);memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 rt.g.global_007e=b16(g+0x7e);rt.g.credited_route=b16(g+0x7c);rt.g.mode_0161=g[0x161];rt.g.frame_counter=b32(g+0x80);rt.g.rng_state=b32(g+0x5c);rt.g.rng_state_valid=1;
 rt.g.global_1697=g[0x1697];rt.g.global_1684=b32(g+0x1684);rt.g.global_1688=b32(g+0x1688);rt.g.global_1682=b16(g+0x1682);rt.g.global_168c=b32(g+0x168c);rt.g.global_1692=b16(g+0x1692);rt.g.global_167a=g[0x167a];rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];
 wf_r17_render_queue_t queue={0};wf_r17_render_ctx_t render={0};
 render.queue=&queue;render.visibility_2188=wf_r17_visibility_2188_exact;
 render.scroll_a_x=b16(g+0x17e6);render.scroll_a_y=b16(g+0x17ee);b.render=&render;
 memcpy(rt.transient_1258,g+0x1258,sizeof rt.transient_1258);
 rt.g.global_1150=b16(g+0x1150);rt.g.global_11b0=b16(g+0x11b0);
 wf_status_t s=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[0],&ctx);
 memcpy(g+0x1258,rt.transient_1258,sizeof rt.transient_1258);
 w(g+0x1150,rt.g.global_1150,2);w(g+0x11b0,rt.g.global_11b0,2);
 for(unsigned k=0;k<6;k++){
  uint32_t list=b32(mem+0x2806+k*8),count=b32(mem+0x280a+k*8);
  w(mem+count,queue.count[k],2);
  for(unsigned j=0;j<queue.count[k];j++)w(mem+list+j*4,0x1c1258u+(uint32_t)(queue.list[k][j]-b.transient_views)*0x2au,4);
 }
 memcpy(g+0x15e4,rt.g.direction_scratch_15e4,8);memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
 g[0x1697]=rt.g.global_1697;w(g+0x5c,rt.g.rng_state,4);w(g+0x1684,rt.g.global_1684,4);w(g+0x1688,rt.g.global_1688,4);w(g+0x1682,rt.g.global_1682,2);w(g+0x168c,rt.g.global_168c,4);w(g+0x1692,rt.g.global_1692,2);g[0x167a]=rt.g.global_167a;g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;
 return s?-(int)ctx.blocked_dependency_rom:0;
}
