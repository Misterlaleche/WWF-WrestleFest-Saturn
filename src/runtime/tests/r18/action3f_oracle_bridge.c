#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
/* The adapter invokes the bounded dispatcher with the unchanged pre-call RAM.
 * Newly queued compact sprites append to existing ROM list entries. */
int wf_action3f_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
 wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t b={0};wf_r17_dispatch_1c03e_ctx_t ctx={0};ctx.actions=&b;
 wf_r17_render_queue_t queue={0};wf_r17_render_ctx_t render={0};uint8_t*g=mem+0x1c0000;
 memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 memcpy(rt.transient_1258,g+0x1258,sizeof rt.transient_1258);
 rt.g.global_1150=b16(g+0x1150);rt.g.global_11b0=b16(g+0x11b0);
 render.queue=&queue;render.visibility_2188=wf_r17_visibility_2188_exact;
 render.scroll_a_x=b16(g+0x17e6);render.scroll_a_y=b16(g+0x17ee);b.render=&render;
 wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&ctx);
 memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
 memcpy(g+0x1258,rt.transient_1258,sizeof rt.transient_1258);
 w(g+0x1150,rt.g.global_1150,2);w(g+0x11b0,rt.g.global_11b0,2);
 for(unsigned k=0;k<6;k++)if(queue.count[k]){
  uint32_t list=b32(mem+0x2806+k*8),counter=b32(mem+0x280a+k*8);uint16_t first=b16(mem+counter);
  for(unsigned j=0;j<queue.count[k];j++)w(mem+list+(first+j)*4,0x1c1258u+(uint32_t)(queue.list[k][j]-b.transient_views)*0x2au,4);
  w(mem+counter,first+queue.count[k],2);
 }
 return status?-(int)ctx.blocked_dependency_rom:0;
}
