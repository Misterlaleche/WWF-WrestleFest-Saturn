#include "wf_postmatch.h"
#include "wf_finale.h"
#include "wf_finale_boundary.h"
#include "wf_r18_match_result.h"
#include "wf_r18_post_tag.h"
#include "wf_r18_referee.h"
#include "wf_r18_result_screen.h"
#include <string.h>

typedef struct {
 wf_session_t *session;
 uint32_t generation,error_owner,error_pc;
 unsigned completed,royal_commits;
} post_context_t;

static uint16_t read_word(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static void write_word(uint8_t *p,uint16_t value){p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;}
static void to_front(wf_session_t *s){memcpy(s->frontend.a,s->match.machine.a,sizeof s->frontend.a);}
static void from_front(wf_session_t *s){memcpy(s->match.machine.a,s->frontend.a,sizeof s->frontend.a);}
static int generation_changed(const post_context_t *c){return c->generation!=c->session->frontend.generation;}
static wf_status_t failed(post_context_t *c,uint32_t owner,uint32_t pc){
 if(!generation_changed(c)&&!c->error_owner){c->error_owner=owner;c->error_pc=pc;}
 return WF_BLOCKED;
}
static wf_status_t owner_failure(post_context_t *c,uint32_t owner,uint32_t pc){
 wf_session_t *s=c->session;
 if(s->match.blocked_dependency)return failed(c,s->match.blocked_dependency,s->match.blocked_pc);
 if(s->frontend.blocked_dependency)return failed(c,s->frontend.blocked_dependency,s->frontend.pc);
 return failed(c,owner,pc);
}
static wf_status_t post_rom(void *user,uint32_t at,void *out,size_t bytes){
 post_context_t *c=user;wf_r18_frontend_driver_t *d=&c->session->frontend;
 return d->platform.read_rom(d->platform.user,at,out,bytes);
}
static wf_status_t next_match(post_context_t *c){
 wf_session_t *s=c->session;to_front(s);
 s->frontend.phase=WF_R18_DRIVER_AC0;s->frontend.pc=0xac0u;
 s->phase=WF_SESSION_MATCH_READY;c->completed=1;return WF_OK;
}
static wf_status_t boot(post_context_t *c){
 wf_session_t *s=c->session;to_front(s);
 if(wf_r18_frontend_driver_resume_boot(&s->frontend,0x6fcu)!=WF_OK)
  return failed(c,0x6fcu,0x6fcu);
 s->phase=WF_SESSION_FRONTEND;c->completed=1;return WF_OK;
}
static wf_status_t run_owner(post_context_t *,uint32_t);
static wf_status_t final_owner(post_context_t *c){
 wf_session_t *s=c->session;wf_r18_frontend_driver_t *d=&s->frontend;
 wf_finale_t cursor;wf_finale_init(&cursor);
 wf_finale_context_t context={&s->match.machine,post_rom,c};
 for(;;){
  d->pc=cursor.pc;int result=wf_finale_step(&cursor,&context);
  if(generation_changed(c))return WF_BLOCKED;
  if(result==WF_FINALE_MORE)continue;
  if(result==WF_FINALE_BOOT)return boot(c);
  if(result!=WF_FINALE_REQUEST)
   return failed(c,cursor.blocked_dependency?cursor.blocked_dependency:cursor.pc,cursor.pc);
  wf_status_t status=WF_BLOCKED;uint32_t value=0;
  if(cursor.request.kind==WF_FINALE_CALL)status=run_owner(c,cursor.request.owner);
  else{
   to_front(s);
   if(cursor.request.kind==WF_FINALE_WRITE16)
    status=d->platform.store_bus(d->platform.user,cursor.request.address,cursor.request.value,2);
   else if(cursor.request.kind==WF_FINALE_READ16||cursor.request.kind==WF_FINALE_READ8)
    status=d->platform.read_bus(d->platform.user,cursor.request.address,&value,
                               cursor.request.kind==WF_FINALE_READ8?1u:2u);
   else if(cursor.request.kind==WF_FINALE_LEVEL){
    status=d->platform.read_bus(d->platform.user,cursor.request.address,&value,1);
    if(!generation_changed(c)&&status==WF_OK&&!!(value&4u)!=cursor.request.value){
     /* Keep the original BYTE poll pending. A successful wait callback is
      * not evidence of a level: reread the physical bus after advancing it. */
     if(wf_finale_complete(&cursor,&context,WF_OK,(uint16_t)value)!=WF_FINALE_WAIT)
      return failed(c,cursor.request.owner,cursor.request.call_pc);
     status=d->platform.wait_level(d->platform.user,d,cursor.request.value);
     if(!generation_changed(c)&&status==WF_OK)
      status=d->platform.read_bus(d->platform.user,cursor.request.address,&value,1);
    }
   }
   if(!generation_changed(c))from_front(s);
  }
  if(generation_changed(c))return WF_BLOCKED;
  if(status!=WF_OK)owner_failure(c,cursor.request.owner,cursor.request.call_pc);
  if(wf_finale_complete(&cursor,&context,status,(uint16_t)value)<0)
   return failed(c,cursor.blocked_dependency?cursor.blocked_dependency:cursor.pc,cursor.pc);
 }
}
static wf_status_t post_owner(post_context_t *c,uint32_t owner){
 wf_session_t *s=c->session;wf_r18_frontend_driver_t *d=&s->frontend;
 wf_r18_post_tag_t cursor;wf_r18_post_tag_init(&cursor,owner);
 wf_r18_post_tag_context_t context={&s->match.machine,post_rom,c};
 for(;;){
  d->pc=cursor.pc;int result=wf_r18_post_tag_step(&cursor,&context);
  if(result==WF_R18_POST_MORE)continue;
  if(result==WF_R18_POST_RETURN)return WF_OK;
  if(result==WF_R18_POST_AC0)return next_match(c);
  if(result==WF_R18_POST_BOUNDARY){
   if(cursor.pc==0x6fcu)return boot(c);
   if(cursor.pc==0x1b4eu)return run_owner(c,cursor.pc);
   return failed(c,cursor.pc,cursor.pc);
  }
  if(result!=WF_R18_POST_REQUEST)
   return failed(c,cursor.blocked_dependency?cursor.blocked_dependency:cursor.pc,cursor.pc);
  wf_status_t status=WF_BLOCKED;uint16_t value=0;
  if(cursor.request.kind==WF_R18_POST_CALL)status=run_owner(c,cursor.request.owner);
  else {
   to_front(s);
   if(cursor.request.kind==WF_R18_POST_WRITE16){
    uint32_t at=cursor.request.address;
    if((at>=0x180000u&&at<0x184000u)||(at>=0x186000u&&at<0x186800u)||
       (at>=0x188000u&&at<0x188800u)){
     wf_r17_fg0_write_t write={at,cursor.request.value,2};
     status=wf_r18_video_batch(&s->runtime,&write,1,&s->video);
    }else status=d->platform.store_bus(d->platform.user,at,cursor.request.value,2);
   }else if(cursor.request.kind==WF_R18_POST_READ16||cursor.request.kind==WF_R18_POST_READ8){
    uint32_t physical=0;
    status=d->platform.read_bus(d->platform.user,cursor.request.address,&physical,
                               cursor.request.kind==WF_R18_POST_READ8?1u:2u);
    if(status==WF_OK)value=(uint16_t)physical;
   }else if(cursor.request.kind==WF_R18_POST_SYNC)
    status=d->platform.wait_vblank(d->platform.user,d);
   if(!generation_changed(c))from_front(s);
  }
  if(generation_changed(c))return WF_BLOCKED;
  if(status!=WF_OK)owner_failure(c,cursor.request.owner,cursor.request.call_pc);
  if(wf_r18_post_tag_complete(&cursor,&context,status,value)<0)
   return failed(c,cursor.blocked_dependency?cursor.blocked_dependency:cursor.pc,cursor.pc);
 }
}
static wf_status_t run_owner(post_context_t *c,uint32_t owner){
 wf_session_t *s=c->session;wf_status_t status;
 switch(owner){
 case 0x1b4e:return final_owner(c);
 case 0x11b6:case 0x1b1e:case 0xb608:case 0xbda6:case 0xae20:
 case 0x15f4:case 0x139c:case 0x160e:
  return post_owner(c,owner);
 case 0x206fe:status=wf_r18_referee_clear_206fe(&s->runtime,&s->video);break;
 case 0x2067c:
  status=wf_r18_referee_number_2067c(&s->runtime,(uint16_t)s->frontend.regs.d[0],&s->video);break;
 default:status=wf_r18_match_driver_owner(&s->match,owner);break;
 }
 if(generation_changed(c))return WF_BLOCKED;
 return status==WF_OK?WF_OK:owner_failure(c,owner,s->frontend.pc);
}

/* Royal's preceding result segment uses the typed runtime. Synchronize only
 * its live D registers and the explicit15FC store; the resumed raw scene is
 * authoritative, so a whole-runtime export here would erase IRQ/record work. */
static void royal_to_front(post_context_t *c){
 wf_session_t *s=c->session;s->frontend.regs=s->runtime.m68k_regs;
 write_word(s->ram+0x15fc,s->runtime.g.global_15fc);to_front(s);
}
static void royal_from_front(post_context_t *c){
 wf_session_t *s=c->session;from_front(s);s->runtime.m68k_regs=s->frontend.regs;
}
static wf_status_t royal_level(wf_runtime_t *runtime,unsigned high,void *user){
 post_context_t *c=user;wf_r18_frontend_driver_t *d=&c->session->frontend;(void)runtime;
 royal_to_front(c);d->pc=high?0x2a0au:0x2a88u;
 wf_status_t status=d->platform.wait_level(d->platform.user,d,high);
 if(generation_changed(c))return WF_BLOCKED;
 royal_from_front(c);
 return status==WF_OK?WF_OK:owner_failure(c,0x2a06u,d->pc);
}
static wf_status_t royal_commit(void *user){
 post_context_t *c=user;wf_r18_frontend_driver_t *d=&c->session->frontend;
 uint32_t owner=c->royal_commits?0x2503cu:0x2a06u;
 royal_to_front(c);d->pc=owner;
 wf_status_t status=d->platform.commit(d->platform.user,d);
 if(generation_changed(c))return WF_BLOCKED;
 royal_from_front(c);
 if(status!=WF_OK)return owner_failure(c,owner,owner);
 c->royal_commits++;return WF_OK;
}
static wf_status_t royal_wait(wf_runtime_t *runtime,uint16_t periods,void *user){
 post_context_t *c=user;
 if(!periods||(unsigned)(uint16_t)runtime->m68k_regs.d[0]+1u!=periods)
  return failed(c,0x21e6u,0x21e6u);
 royal_to_front(c);c->session->frontend.pc=0x21e6u;
 wf_status_t status=run_owner(c,0x21e6u);
 if(!generation_changed(c))royal_from_front(c);
 return status;
}
static wf_status_t royal_loss(post_context_t *c){
 wf_session_t *s=c->session;
 wf_status_t status=wf_r18_result_royal_game_over(&s->runtime,&s->video,
                         royal_level,royal_wait,royal_commit,c);
 if(generation_changed(c))return WF_BLOCKED;
 if(status!=WF_OK){
  uint32_t owner=c->royal_commits?0x2503cu:0x2a06u;
  return owner_failure(c,owner,owner);
 }
 return run_owner(c,0x1b1eu);
}
static wf_status_t attract_exit(post_context_t *c){
 wf_session_t *s=c->session;s->frontend.pc=0x1120u;
 wf_status_t status=run_owner(c,0x8f50u);if(status!=WF_OK)return status;
 wf_r17_m68k_move_w(&s->frontend.regs,0,0x20u);s->frontend.pc=0x112au;
 status=run_owner(c,0x21e6u);if(status!=WF_OK)return status;
 uint16_t sequence=(uint16_t)(read_word(s->ram+0x166)+1u);
 write_word(s->ram+0x166,sequence);
 return sequence>=14u?boot(c):next_match(c);
}
static int bound(const wf_session_t *s){
 const wf_r18_frontend_driver_t *d=&s->frontend;
 const wf_r18_prematch_t *m=&s->match.machine;
 return s->runtime.m68k_regs.valid&&d->regs.valid&&d->ram==s->ram&&d->ram_bytes>=sizeof s->ram&&
  d->runtime==&s->runtime&&d->video==&s->video&&d->phase==WF_R18_DRIVER_AC0&&
  d->control.ram_1c0000==s->ram&&d->control.regs==&d->regs&&d->control.a==d->a&&d->control.effect&&
  !d->irq_active&&!d->coin_active&&!d->call_depth&&!d->active_owner&&!d->fade.active&&
  !d->blocked_dependency&&!s->match.blocked_dependency&&s->match.frontend==d&&
  s->match.generation==d->generation&&m->ram==s->ram&&m->regs==&d->regs&&m->effect&&
  d->platform.read_rom&&d->platform.read_bus&&d->platform.store_bus&&d->platform.wait_level&&
  d->platform.wait_vblank&&d->platform.commit&&d->platform.bind_world&&d->platform.submit_records&&
  s->video.read_rom&&s->video.read_ram;
}
wf_status_t wf_postmatch_run(wf_session_t *s){
 if(!s||!s->initialized)return WF_BAD_STATE;
 if(s->phase==WF_SESSION_FATAL)return WF_BLOCKED;
 if(s->phase!=WF_SESSION_POSTMATCH||!bound(s))return WF_BAD_STATE;
 int attract=wf_r18_attract_timeout(&s->runtime),tag=wf_r18_tag_match_complete(&s->runtime);
 int royal=wf_r18_royal_game_over(&s->runtime),royal_final=wf_royal_finale_ready(&s->runtime);
 if(!attract&&!tag&&!royal&&!royal_final)return WF_BAD_STATE;
 post_context_t context={.session=s,.generation=s->frontend.generation};
 s->error_owner=s->error_pc=0;s->frontend.regs=s->runtime.m68k_regs;
 wf_r18_prematch_speech_divisor_set(1u);
 wf_status_t status=attract?attract_exit(&context):tag?run_owner(&context,0x11b6u):
                    royal_final?run_owner(&context,0x1b4eu):royal_loss(&context);
 if(generation_changed(&context)&&s->frontend.phase==WF_R18_DRIVER_MODE_PLATE&&s->frontend.credited_start){
  s->phase=WF_SESSION_FRONTEND;return WF_OK;
 }
 if(status==WF_OK&&context.completed)return WF_OK;
 if(!context.error_owner)owner_failure(&context,s->frontend.pc,s->frontend.pc);
 s->phase=WF_SESSION_FATAL;
 s->error_owner=context.error_owner?context.error_owner:s->frontend.pc;
 s->error_pc=context.error_pc?context.error_pc:s->frontend.pc;
 return WF_BLOCKED;
}
