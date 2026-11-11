#include "wf_combat_handoff.h"
#include "wf_r18_match_result.h"
#include "wf_finale_boundary.h"
#include <stddef.h>
#include <string.h>
enum { MAGIC=0x57464348u,VERSION=2u,TO_COMBAT=1u,TO_FRONTEND=2u };
#ifdef WF_ALPHA_RETURN
#define WF_HANDOFF_COLD __attribute__((cold,noinline))
#else
#define WF_HANDOFF_COLD
#endif
static uint32_t checksum(const wf_combat_handoff_t*h){
 const uint8_t*p=(const uint8_t*)h;uint32_t hash=2166136261u;
 for(size_t i=0;i<sizeof *h;i++)if(i<offsetof(wf_combat_handoff_t,checksum)||i>=offsetof(wf_combat_handoff_t,checksum)+4u)hash=(hash^p[i])*16777619u;
 return hash;
}
void wf_combat_handoff_seal(wf_combat_handoff_t*h){if(h)h->checksum=checksum(h);}
static bool checkpoint_valid(const wf_frontend_checkpoint_t *f){
 return f->phase==WF_R18_DRIVER_AC0&&f->pc<0x80000u&&!(f->pc&1u)&&
  f->ssp>=0x1c0000u&&f->ssp<=0x1c4000u&&f->usp>=0x1c0000u&&f->usp<=0x1c4000u&&
  !((f->ssp|f->usp)&1u)&&f->credited_start<=1u&&f->world_visible<=1u&&!f->reserved;
}
static bool terminal(const wf_runtime_t *r){
 return wf_r18_tag_match_complete(r)||wf_r18_royal_game_over(r)||wf_r18_attract_timeout(r)||wf_royal_finale_ready(r);
}
static WF_HANDOFF_COLD bool pack(wf_combat_handoff_t*h,const wf_session_t*s,const wf_board_t*b,const wf_pacer_t*p,unsigned direction){
 if(!h||!s||!b||!p||!s->initialized||s->phase!=WF_SESSION_GAMEPLAY||
    s->runtime.frame_stage!=(direction==TO_COMBAT?3u:2u)||p->clock.failed||!p->load_depth)return false;
 const wf_r18_frontend_driver_t *f=&s->frontend;
 if(f->irq_active||f->coin_active||f->call_depth||f->active_owner||f->fade.active)return false;
 wf_frontend_checkpoint_t saved={.ssp=f->control.ssp,.usp=f->control.usp,.sr=f->control.sr,
  .pc=f->pc,.generation=f->generation,.phase=f->phase,.credited_start=f->credited_start,.world_visible=f->world_visible};
 memcpy(saved.a,f->a,sizeof saved.a);
 if(!checkpoint_valid(&saved)||(direction==TO_FRONTEND&&!terminal(&s->runtime)))return false;
 memset(h,0,sizeof *h);h->magic=MAGIC;h->version=VERSION;h->bytes=sizeof *h;
 memcpy(h->ram,s->ram,sizeof h->ram);h->runtime=s->runtime;h->video=s->video;
 h->runtime.bindings=NULL;h->runtime.binding_count=0;
 h->video.read_rom=NULL;h->video.read_ram=NULL;h->video.read_user=NULL;h->video.ring_cell=NULL;
 h->board=*b;h->pacer=*p;h->fields=p->last_field;
 h->direction=direction;h->frontend=saved;
 wf_combat_handoff_seal(h);return true;
}
static WF_HANDOFF_COLD bool restore(const wf_combat_handoff_t*h,wf_session_t*s,wf_board_t*b,wf_pacer_t*p,unsigned direction){
 if(!h||!s||!b||!p||h->magic!=MAGIC||h->version!=VERSION||h->bytes!=sizeof *h||h->checksum!=checksum(h)||
  h->direction!=direction||!checkpoint_valid(&h->frontend)||
  h->runtime.frame_stage!=(direction==TO_COMBAT?3u:2u)||
  (direction==TO_FRONTEND&&!terminal(&h->runtime))||
  h->runtime.bindings||h->runtime.binding_count||h->video.read_rom||h->video.read_ram||h->video.read_user||h->video.ring_cell||h->pacer.clock.failed||!h->pacer.load_depth)return false;
 /* Validate before touching any output: a failed read cannot partially
  * replace an existing live session. Frontend pointers are never copied. */
 memset(s,0,sizeof *s);memcpy(s->ram,h->ram,sizeof s->ram);s->runtime=h->runtime;s->video=h->video;
 s->phase=direction==TO_COMBAT?WF_SESSION_GAMEPLAY:WF_SESSION_POSTMATCH;s->initialized=1;
 s->frontend.pc=h->frontend.pc;s->frontend.phase=h->frontend.phase;s->frontend.generation=h->frontend.generation;
 memcpy(s->frontend.a,h->frontend.a,sizeof s->frontend.a);
 s->frontend.control.ssp=h->frontend.ssp;s->frontend.control.usp=h->frontend.usp;s->frontend.control.sr=h->frontend.sr;
 s->frontend.credited_start=h->frontend.credited_start;s->frontend.world_visible=h->frontend.world_visible;
 s->frontend.regs=s->runtime.m68k_regs;
 *b=h->board;*p=h->pacer;return true;
}
bool wf_combat_handoff_pack(wf_combat_handoff_t*h,const wf_session_t*s,const wf_board_t*b,const wf_pacer_t*p){return pack(h,s,b,p,TO_COMBAT);}
bool wf_combat_handoff_restore(const wf_combat_handoff_t*h,wf_session_t*s,wf_board_t*b,wf_pacer_t*p){return restore(h,s,b,p,TO_COMBAT);}
bool wf_combat_handoff_pack_return(wf_combat_handoff_t*h,const wf_session_t*s,const wf_board_t*b,const wf_pacer_t*p){return pack(h,s,b,p,TO_FRONTEND);}
bool wf_combat_handoff_restore_return(const wf_combat_handoff_t*h,wf_session_t*s,wf_board_t*b,wf_pacer_t*p){return restore(h,s,b,p,TO_FRONTEND);}
