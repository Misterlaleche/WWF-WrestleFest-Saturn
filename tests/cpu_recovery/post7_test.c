#include "wf_r17_post7a00.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
wf_status_t reference_wf_r17_owner_bb14(wf_runtime_t *,wf_r17_post7_ctx_t *);
wf_status_t reference_wf_r17_owner_7366(wf_runtime_t *,wf_r17_post7_ctx_t *);
typedef struct event {unsigned kind;uint16_t raw;wf_runtime_t state;uint16_t counts[6];} event_t;
typedef struct fixture {wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_render_ctx_t render;wf_r17_post7_ctx_t ctx;unsigned calls,fail_at,mutation,visibility;event_t events[5];} fixture_t;
static uint32_t seed=0xbb147366;static unsigned bb_cases,scan_cases,blocked,callbacks;
static uint32_t rng(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void bind(fixture_t *f);
static void copy(fixture_t *d,const fixture_t *s){*d=*s;bind(d);for(unsigned k=0;k<6;k++)for(unsigned i=0;i<32;i++)if(s->q.list[k][i])d->q.list[k][i]=(wf_actor_raw_t*)((uint8_t*)&d->rt+((uint8_t*)s->q.list[k][i]-(uint8_t*)&s->rt));}
static wf_status_t effect(fixture_t *f,unsigned kind,uint16_t value){
 assert(f->calls<5);event_t *e=&f->events[f->calls++];e->kind=kind;e->raw=value;e->state=f->rt;memcpy(e->counts,f->q.count,sizeof e->counts);
 if(f->mutation){f->rt.actors[8].b[200]^=0x35;f->rt.g.global_1694^=0x1234;f->rt.g.rng_state^=0x76543210;f->rt.m68k_regs.d[7]^=0x8734;f->q.list[5][0]=&f->rt.fixed_object1;f->q.count[5]=1;}
 return f->calls==f->fail_at?WF_BLOCKED:WF_OK;
}
static wf_status_t resource_a(wf_runtime_t *rt,uint16_t v,void *p){fixture_t *f=p;assert(rt==&f->rt);return effect(f,1,v);}
static wf_status_t resource_b(wf_runtime_t *rt,uint16_t v,void *p){fixture_t *f=p;assert(rt==&f->rt);return effect(f,2,v);}
static wf_r17_tri_t visibility(wf_actor_raw_t *a,void *p){fixture_t *f=p;(void)a;if(effect(f,3,0)!=WF_OK)return WF_R17_UNKNOWN;return (wf_r17_tri_t)f->visibility;}
static void bind(fixture_t *f){f->ctx.user=f;if(f->ctx.render)f->ctx.render=&f->render;if(f->render.queue)f->render.queue=&f->q;f->render.visibility_user=f;}
static void init(fixture_t *f){memset(f,0,sizeof *f);for(size_t i=0;i<sizeof f->rt.g;i++)((uint8_t*)&f->rt.g)[i]=(uint8_t)rng();for(unsigned k=0;k<8;k++)f->rt.m68k_regs.d[k]=rng();f->rt.m68k_regs.valid=1;memset(f->rt.g.royal_slots_1586,0,sizeof f->rt.g.royal_slots_1586);memset(f->rt.g.royal_occupancy,0,sizeof f->rt.g.royal_occupancy);f->rt.g.rng_state_valid=1;f->rt.g.mode_0161=1;f->ctx.resource_2aea=resource_a;f->ctx.resource_2b58=resource_b;f->ctx.render=&f->render;f->render.queue=&f->q;f->render.visibility_2188=visibility;f->visibility=WF_R17_YES;f->ctx.blocked_dependency_rom=rng();bind(f);}
static void compare(fixture_t *start,unsigned which){
 static fixture_t a,b;copy(&a,start);copy(&b,start);
 wf_status_t ar=which?reference_wf_r17_owner_7366(&a.rt,&a.ctx):reference_wf_r17_owner_bb14(&a.rt,&a.ctx);
 wf_status_t br=which?wf_r17_owner_7366(&b.rt,&b.ctx):wf_r17_owner_bb14(&b.rt,&b.ctx);
 assert(ar==br);assert(!memcmp(&a.rt,&b.rt,sizeof a.rt));assert(!memcmp(a.q.count,b.q.count,sizeof a.q.count));
 for(unsigned k=0;k<6;k++)for(unsigned i=0;i<32;i++){uintptr_t ap=a.q.list[k][i]?(uintptr_t)((uint8_t*)a.q.list[k][i]-(uint8_t*)&a.rt)+1:0;uintptr_t bp=b.q.list[k][i]?(uintptr_t)((uint8_t*)b.q.list[k][i]-(uint8_t*)&b.rt)+1:0;assert(ap==bp);}
 assert(a.ctx.blocked_dependency_rom==b.ctx.blocked_dependency_rom);assert(a.calls==b.calls&&!memcmp(a.events,b.events,a.calls*sizeof a.events[0]));
 callbacks+=a.calls;if(ar!=WF_OK)blocked++;if(which)scan_cases++;else bb_cases++;
}
static void bb14(void){fixture_t f;for(unsigned timer=0;timer<=65535;timer++){init(&f);f.rt.g.global_1694=(uint16_t)timer;f.rt.g.global_16a7=1;compare(&f,0);}
 const uint16_t timers[]={0,1,0x124,0x125,0x126,0xfffe,0xffff};
 for(unsigned t=0;t<sizeof timers/sizeof timers[0];t++)for(unsigned valid=0;valid<2;valid++)for(unsigned active=0;active<=9;active++)for(unsigned mode=0;mode<5;mode++){
  init(&f);f.rt.g.global_1694=timers[t];f.rt.g.global_16a7=mode==1?0:mode==2?255:1;f.rt.g.rng_state_valid=(uint8_t)valid;
  for(unsigned i=0;i<active;i++){wf_set_u8(&f.rt.actors[i],WF_A_ACTIVE,0x80);wf_set_u8(&f.rt.actors[i],WF_A_ROLE,(uint8_t)((mode&1)?0x20:(i&1)*0x20));}
  if(mode>=3)for(unsigned i=0;i<12;i++)f.rt.g.royal_occupancy[i]=(uint16_t)(mode==4||i<6);
  compare(&f,0);
 }
 for(unsigned slot=0;slot<9;slot++){init(&f);f.rt.g.royal_slots_1586[slot]=1;compare(&f,0);}
 init(&f);f.rt.m68k_regs.valid=0;compare(&f,0);assert(reference_wf_r17_owner_bb14(NULL,NULL)==wf_r17_owner_bb14(NULL,NULL));
}
static void owner7366(void){fixture_t f;
 for(unsigned target=0;target<10;target++)for(unsigned pass=0;pass<2;pass++)for(unsigned phase=0;phase<22;phase++)for(unsigned latched=0;latched<2;latched++)for(unsigned fail=0;fail<12;fail++){
  init(&f);f.rt.g.royal_slots_1586[target]=(uint16_t)(pass?0x4000:0x2000);wf_actor_raw_t *a=target<9?&f.rt.actors[target]:&f.rt.fixed_object0;
  wf_set_u8(a,WF_A_ACTIVE,0x80);wf_set_u8(a,WF_A_LATCHED,latched?0x80:0);wf_set_be16(a,WF_A_CHARACTER,0x10);wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)(phase==21?65535:phase));wf_set_be16(a,WF_A_ANIM_COUNTER,fail==10?1:fail==11?0x8001:0);wf_set_u8(a,WF_A_ROLE,phase&1?0x80:0);f.mutation=1;
  if(fail==1)f.ctx.resource_2aea=NULL;if(fail==2)f.ctx.resource_2b58=NULL;if(fail==3)f.fail_at=1;if(fail==4)f.fail_at=2;if(fail==5)f.rt.g.rng_state_valid=0;if(fail==6)f.render.visibility_2188=NULL;if(fail==7)f.visibility=WF_R17_UNKNOWN;if(fail==8)f.q.count[0]=32;if(fail==9)wf_set_be16(a,WF_A_RENDER_LIST,6);
  compare(&f,1);
 }
 for(unsigned invalid=0;invalid<6;invalid++){init(&f);if(invalid==1)f.ctx.render=NULL;if(invalid==2)f.render.queue=NULL;if(invalid==3)f.rt.m68k_regs.valid=0;if(invalid==4)f.rt.g.mode_0161=0;if(invalid==5)for(unsigned i=0;i<10;i++)f.rt.g.royal_slots_1586[i]=0x8000;compare(&f,1);}
 assert(reference_wf_r17_owner_7366(NULL,NULL)==wf_r17_owner_7366(NULL,NULL));init(&f);assert(reference_wf_r17_owner_7366(&f.rt,NULL)==wf_r17_owner_7366(&f.rt,NULL));
}
int main(void){bb14();owner7366();printf("PASS BB14=%u 7366=%u blocked=%u callback states=%u\n",bb_cases,scan_cases,blocked,callbacks);return 0;}
