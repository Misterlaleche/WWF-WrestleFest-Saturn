#include "wf_r18_world.h"
#include <string.h>
static unsigned b16(const uint8_t*p){return (p[0]<<8)|p[1];}
static void w16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,v>>16);w16(p+2,v);}
static wf_status_t rd(void*u,uint32_t at,void*d,size_t n){if(at+n>0x80000)return WF_BLOCKED;memcpy(d,(uint8_t*)u+at,n);return WF_OK;}
int wf_r18_world_oracle(unsigned op,uint8_t *mem,uint32_t *regs,unsigned obj){
 wf_runtime_t rt;wf_r18_world_ctx_t c;memset(&rt,0,sizeof rt);memset(&c,0,sizeof c);c.read_rom=rd;c.read_user=mem;
 uint8_t*g=mem+0x1c0000;rt.g.global_007e=(uint16_t)b16(g+0x7e);rt.g.global_007f=g[0x7f];rt.g.global_17e2=(uint16_t)b16(g+0x17e2);rt.g.global_17e4=(uint16_t)b16(g+0x17e4);rt.g.global_17fe=(uint16_t)b16(g+0x17fe);rt.g.global_16d4=(uint16_t)b16(g+0x16d4);rt.m68k_regs.valid=1;memcpy(rt.m68k_regs.d,regs,32);memcpy(rt.branchb_objs_188a,g+0x188a,192);memcpy(c.animation_queue[0],g+0x194a,48);memcpy(c.animation_queue[1],g+0x197a,48);c.animation_count[0]=(uint16_t)b16(g+0x19ac);c.animation_count[1]=(uint16_t)b16(g+0x19aa);
 unsigned side=!!(rt.g.global_17fe&0x8000);wf_status_t status;
 if(op==0){status=wf_r18_world_288f0(&rt,rt.branchb_objs_188a[obj],rt.g.global_007f,&c);memcpy(g+0x188a,rt.branchb_objs_188a,192);memcpy(g+0x194a,c.animation_queue[0],48);memcpy(g+0x197a,c.animation_queue[1],48);w16(g+0x19ac,c.animation_count[0]);w16(g+0x19aa,c.animation_count[1]);}
 else{status=wf_r18_world_26b68(&rt,(uint8_t)side,rt.g.global_16d4,&c);for(unsigned i=0;i<16;i++){w32(g+(side?0x1756:0x16d6)+i*8,c.cache[side][i].source);w32(g+(side?0x1756:0x16d6)+i*8+4,c.cache[side][i].destination);}}
 memcpy(regs,rt.m68k_regs.d,32);return status;
}
