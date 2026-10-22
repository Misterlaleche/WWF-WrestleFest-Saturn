#include <string.h>
#include "wf_r18_video.h"
static unsigned b16(const uint8_t*p){return (p[0]<<8)|p[1];}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-1-i)));}
typedef struct {wf_r18_video_t v;unsigned ticks;} fixture_t;
static wf_status_t readrom(void*u,uint32_t a,void*p,size_t n){memcpy(p,(uint8_t*)u+a,n);return WF_OK;}
static wf_status_t fg(wf_runtime_t*r,const wf_r17_fg0_write_t*x,size_t n,void*u){return wf_r18_video_batch(r,x,n,&((fixture_t*)u)->v);}
static wf_status_t text(wf_runtime_t*r,uint16_t id,void*u){return wf_r18_text_2503c(r,id,&((fixture_t*)u)->v);}
static wf_status_t waitv(wf_runtime_t*r,uint16_t count,void*u){fixture_t*f=u;if(count!=64u||(r->g.global_16c4&255u)!=3u||!(r->g.global_169e&0x1000u))return WF_BAD_STATE;f->ticks+=(unsigned)count+1u;return WF_OK;}
int wf_timeup_oracle(uint8_t*mem,uint32_t*regs,unsigned*ticks,unsigned hooks){
 wf_runtime_t rt={0};wf_r17_post7_ctx_t c={0};fixture_t f={0};uint8_t*g=mem+0x1c0000;
 memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 rt.g.credited_route=b16(g+0x7c);rt.g.global_169e=b16(g+0x169e);rt.g.global_16a0=b16(g+0x16a0);rt.g.global_16a2=b16(g+0x16a2);rt.g.global_16a4=b16(g+0x16a4);rt.g.global_16c4=b16(g+0x16c4);
 memcpy(f.v.fg0,mem+0xc0000,sizeof f.v.fg0);f.v.read_rom=readrom;f.v.read_user=mem;
 c.user=&f;c.fg0_batch=fg;if(hooks&1)c.raw_2503c=text;if(hooks&2)c.wait_21e6=waitv;
 wf_status_t s=wf_r17_owner_262d2(&rt,&c);*ticks=f.ticks;
 memcpy(mem+0xc0000,f.v.fg0,sizeof f.v.fg0);memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
 w(g+0x169e,rt.g.global_169e,2);w(g+0x16a0,rt.g.global_16a0,2);w(g+0x16a2,rt.g.global_16a2,2);w(g+0x16a4,rt.g.global_16a4,2);w(g+0x16c4,rt.g.global_16c4,2);
 return s?-(int)c.blocked_dependency_rom:0;
}
