#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_result_screen.h"
typedef struct {unsigned kind,value;uint32_t d[8],fg,pal;} checkpoint_t;
typedef struct {const uint8_t*rom;wf_runtime_t*r;wf_r18_video_t*v;unsigned events,reads,fail_event,fail_read;wf_status_t failure;checkpoint_t cp[5];} probe_t;
static unsigned tests;
static uint32_t hash(const uint8_t*p,size_t n){uint32_t v=2166136261u;while(n--)v=(v^*p++)*16777619u;return v;}
static wf_status_t reader(void*u,uint32_t at,void*p,size_t n){probe_t*s=u;s->reads++;if(s->reads==s->fail_read)return WF_BLOCKED;if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(p,s->rom+at,n);return WF_OK;}
static wf_status_t event(probe_t*s,unsigned kind,unsigned value){
 assert(s->events<5u);checkpoint_t*c=&s->cp[s->events++];c->kind=kind;c->value=value;memcpy(c->d,s->r->m68k_regs.d,32);
 c->fg=hash(s->v->fg0,sizeof s->v->fg0);c->pal=hash(s->v->palettes,sizeof s->v->palettes)^hash(s->v->fg_palette,sizeof s->v->fg_palette)^hash(s->v->bg_palette,sizeof s->v->bg_palette);
 return s->events==s->fail_event?s->failure:WF_OK;
}
static wf_status_t level(wf_runtime_t*r,unsigned value,void*u){probe_t*s=u;assert(r==s->r&&r->g.global_15fc==1u);return event(s,0,value);}
static wf_status_t wait_frames(wf_runtime_t*r,uint16_t n,void*u){probe_t*s=u;assert(r==s->r&&n==97u&&(uint16_t)r->m68k_regs.d[0]==0x60u);return event(s,2,n);}
static wf_status_t commit(void*u){return event(u,1,0);}
static void init(probe_t*p,wf_runtime_t*r,wf_r18_video_t*v,const uint8_t*rom){
 memset(r,0,sizeof *r);memset(v,0,sizeof *v);memset(p,0,sizeof *p);p->rom=rom;p->r=r;p->v=v;p->failure=WF_BLOCKED;
 r->g.global_15f4=3;r->g.global_15fc=0x9876;r->g.rng_state=0x76543210;r->g.credited_route=0x8000;r->g.global_0092=0xff1c05b0u;r->m68k_regs.valid=1;
 for(unsigned i=0;i<8;i++)r->m68k_regs.d[i]=0xa5001234u+i*0x10101u;
 memset(r->actors,0x9a,sizeof r->actors);memset(v->fg0,0xa5,sizeof v->fg0);memset(v->palettes,0xb6,sizeof v->palettes);memset(v->fg_palette,0xc7,sizeof v->fg_palette);memset(v->bg_palette,0xd8,sizeof v->bg_palette);
 v->read_rom=reader;v->read_user=p;
}
static wf_status_t call(probe_t*p){return wf_r18_result_royal_game_over(p->r,p->v,level,wait_frames,commit,p);}
static void unchanged_globals(const wf_runtime_t*r,const wf_globals_raw_t*before){wf_globals_raw_t expected=*before;expected.global_15fc=1;assert(memcmp(&r->g,&expected,sizeof expected)==0);}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t*rom=malloc(0x80000u);assert(rom&&fread(rom,1,0x80000u,f)==0x80000u);fclose(f);
 wf_runtime_t*r=malloc(sizeof *r);wf_r18_video_t*v=malloc(sizeof *v);assert(r&&v);probe_t p;
 init(&p,r,v,rom);wf_globals_raw_t original=r->g;uint32_t d[8];memcpy(d,r->m68k_regs.d,32);
 assert(call(&p)==WF_OK&&p.events==5u);tests++;checkpoint_t baseline[5];memcpy(baseline,p.cp,sizeof baseline);unsigned total_reads=p.reads;
 const unsigned kinds[5]={0,1,0,1,2},values[5]={1,0,0,0,97};for(unsigned i=0;i<5;i++){assert(p.cp[i].kind==kinds[i]&&p.cp[i].value==values[i]);}
 assert(r->m68k_regs.d[0]==((d[0]&0xffff0000u)|0x60u));assert(r->m68k_regs.d[1]==((d[1]&0xffff0000u)|0x3cu));assert(r->m68k_regs.d[2]==((d[2]&0xffff0000u)|0xffffu));for(unsigned i=3;i<8;i++)assert(r->m68k_regs.d[i]==d[i]);unchanged_globals(r,&original);
 for(unsigned fail=1;fail<=5;fail++)for(unsigned bad=WF_BLOCKED;bad<=WF_BAD_STATE;bad++){
  init(&p,r,v,rom);p.fail_event=fail;p.failure=(wf_status_t)bad;assert(call(&p)==(wf_status_t)bad&&p.events==fail);tests++;
  assert(memcmp(p.cp,baseline,fail*sizeof *baseline)==0);unchanged_globals(r,&original);
 }
 for(unsigned fail=1;fail<=total_reads;fail++){
  init(&p,r,v,rom);p.fail_read=fail;assert(call(&p)==WF_BLOCKED);tests++;assert(p.events==1u||p.events==3u);assert(p.cp[0].fg==baseline[0].fg);
  /* Both palette and text stage their payload before committing it. */
  assert(hash(v->fg0,sizeof v->fg0)==baseline[0].fg);unchanged_globals(r,&original);
  if(p.events==1u){assert(memcmp(r->m68k_regs.d,d,32)==0);assert(p.cp[0].pal==(hash(v->palettes,sizeof v->palettes)^hash(v->fg_palette,sizeof v->fg_palette)^hash(v->bg_palette,sizeof v->bg_palette)));}
  else{assert((uint16_t)r->m68k_regs.d[0]==0x50u);assert(memcmp(p.cp,baseline,3*sizeof *baseline)==0);}
  /* Reader failure must release the shared text staging busy latch. */
  init(&p,r,v,rom);assert(call(&p)==WF_OK);tests++;
 }
 for(unsigned invalid=0;invalid<8;invalid++){
  init(&p,r,v,rom);wf_globals_raw_t before=r->g;uint32_t fgh=hash(v->fg0,sizeof v->fg0);wf_status_t st;
  if(invalid==0)st=wf_r18_result_royal_game_over(0,v,level,wait_frames,commit,&p);
  else if(invalid==1)st=wf_r18_result_royal_game_over(r,0,level,wait_frames,commit,&p);
  else if(invalid==2){r->m68k_regs.valid=0;st=call(&p);}
  else if(invalid==3)st=wf_r18_result_royal_game_over(r,v,0,wait_frames,commit,&p);
  else if(invalid==4)st=wf_r18_result_royal_game_over(r,v,level,0,commit,&p);
  else if(invalid==5)st=wf_r18_result_royal_game_over(r,v,level,wait_frames,0,&p);
  else if(invalid==6){v->read_rom=0;st=call(&p);}
  else{r->g.global_15f4=7;before=r->g;st=call(&p);}
  assert(st==(invalid>=6?WF_BLOCKED:WF_BAD_STATE));assert(!p.events&&!p.reads);assert(memcmp(&r->g,&before,sizeof before)==0);assert(hash(v->fg0,sizeof v->fg0)==fgh);tests++;
 }
 printf("PASS %u Royal GAME OVER contract checks (%u ROM read failure boundaries)\n",tests,total_reads);free(v);free(r);free(rom);return 0;
}
