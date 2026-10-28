#include <string.h>
#include "wf_r18_result_screen.h"
#include "wf_r18_match_result.h"
typedef struct { const uint8_t *rom; wf_r18_video_t *v; wf_runtime_t *r;
 uint32_t *events; uint8_t *snapshots; unsigned count; } probe_t;
static wf_status_t read_rom(void *u,uint32_t a,void *p,size_t n){
 probe_t *s=u;if(a>0x80000u||n>0x80000u-a)return WF_BLOCKED;
 memcpy(p,s->rom+a,n);return WF_OK;
}
static void event(probe_t*s,unsigned kind,unsigned value){
 uint32_t *e=s->events+10u*s->count;e[0]=kind;e[1]=value;
 memcpy(e+2,s->r->m68k_regs.d,32);
 uint8_t*p=s->snapshots+0xd000u*s->count++;
 memcpy(p,s->v->fg0,0x4000);memcpy(p+0x4000,s->v->palettes,0x4000);
 memcpy(p+0x8000,s->v->fg_palette,0x800);memcpy(p+0x8800,s->v->bg_palette,0x800);
}
static wf_status_t level(wf_runtime_t*r,unsigned high,void*u){(void)r;event(u,0,high);return WF_OK;}
static wf_status_t wait_frames(wf_runtime_t*r,uint16_t n,void*u){(void)r;event(u,2,n);return WF_OK;}
static wf_status_t commit(void*u){event(u,1,0);return WF_OK;}
int wf_royal_result_oracle(const uint8_t*rom,uint8_t*ram,uint32_t*regs,
 uint8_t*fg,uint8_t*pal,uint32_t*events,uint8_t*snapshots,unsigned*count){
 wf_runtime_t r;wf_r18_video_t v;probe_t p={rom,&v,&r,events,snapshots,0};
 memset(&r,0,sizeof r);memset(&v,0,sizeof v);r.m68k_regs.valid=1;
 memcpy(r.m68k_regs.d,regs,32);r.g.global_15f4=(uint16_t)((ram[0x15f4]<<8)|ram[0x15f5]);
 r.g.global_15fc=(uint16_t)((ram[0x15fc]<<8)|ram[0x15fd]);
 memcpy(v.fg0,fg,0x4000);memcpy(v.palettes,pal,0x4000);
 memcpy(v.fg_palette,pal+0x8000,0x800);memcpy(v.bg_palette,pal+0x6000,0x800);
 v.read_rom=read_rom;v.read_user=&p;
 int st=wf_r18_result_royal_game_over(&r,&v,level,wait_frames,commit,&p);
 ram[0x15fc]=(uint8_t)(r.g.global_15fc>>8);ram[0x15fd]=(uint8_t)r.g.global_15fc;
 memcpy(regs,r.m68k_regs.d,32);memcpy(fg,v.fg0,0x4000);
 memcpy(pal,v.palettes,0x4000);memcpy(pal+0x8000,v.fg_palette,0x800);memcpy(pal+0x6000,v.bg_palette,0x800);
 *count=p.count;return st;
}
