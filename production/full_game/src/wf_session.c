#include "wf_session.h"
#include "wf_r18_frontend_world.h"
#include "wf_r18_match_support.h"
#include "wf_r18_runtime_handoff.h"
#include <string.h>

static wf_status_t fail(wf_session_t *s,uint32_t owner,uint32_t pc){
 s->phase=WF_SESSION_FATAL;s->error_owner=owner;s->error_pc=pc;
 return WF_BLOCKED;
}
static wf_status_t rom(void *u,uint32_t at,uint8_t *out,size_t n){
 wf_session_t *s=u;
 return s->frontend.platform.read_rom(s->frontend.platform.user,at,out,n);
}
static wf_status_t palette(void *u,uint32_t at,uint16_t value){
 wf_session_t *s=u;wf_r17_fg0_write_t write={at,value,2};
 return wf_r18_video_batch(&s->runtime,&write,1,&s->video);
}
static wf_status_t extension(void *u,uint32_t owner,wf_r18_prematch_t *machine){
 wf_session_t *s=u;
 if(owner==0x285dau){
  wf_r18_frontend_world_t world={.ram=machine->ram,.ram_bytes=sizeof s->ram,
   .regs=machine->regs,.a=machine->a,.read_rom=s->frontend.platform.read_rom,
   .user=s->frontend.platform.user};
  wf_status_t status=wf_r18_frontend_world_call(&world,owner);
  if(status!=WF_OK)machine->dependency=world.blocked_dependency;
  return status;
 }
 wf_r18_match_support_t support={.machine=machine,.fg_0c0000=s->video.fg0,
  .fg_bytes=sizeof s->video.fg0,.read_rom=rom,.write_palette=palette,.user=s};
 return wf_r18_match_support_call(&support,owner);
}
wf_status_t wf_session_init(wf_session_t *s,const wf_session_config_t *c){
 if(!s||!c||!c->cold_ram||c->cold_ram_bytes<sizeof s->ram||!(c->sr&0x2000u)||
  (c->binding_count&&!c->bindings)||!c->platform.read_rom||!c->platform.read_bus||
  !c->platform.store_bus||!c->platform.wait_level||!c->platform.wait_vblank||
  !c->platform.commit||!c->platform.bind_world||!c->platform.submit_records)return WF_BAD_STATE;
 /* Reject overlapping cold images before clearing the destination. */
 uintptr_t src=(uintptr_t)c->cold_ram,dst=(uintptr_t)s;
 if((src>=dst&&src-dst<sizeof *s)||(src<dst&&dst-src<sizeof s->ram))return WF_BAD_STATE;
 memset(s,0,sizeof *s);memcpy(s->ram,c->cold_ram,sizeof s->ram);
 wf_runtime_init(&s->runtime,c->bindings,c->binding_count);
 wf_r18_frontend_driver_config_t config={.ram=s->ram,.ram_bytes=sizeof s->ram,
  .runtime=&s->runtime,.video=&s->video,.platform=c->platform,
  .ssp=c->ssp,.usp=c->usp,.sr=c->sr};
 wf_status_t status=wf_r18_frontend_driver_init(&s->frontend,&config);
 if(status!=WF_OK)return status;
 s->initialized=1;s->phase=WF_SESSION_FRONTEND;return WF_OK;
}
wf_status_t wf_session_step(wf_session_t *s){
 if(!s||!s->initialized)return WF_BAD_STATE;
 if(s->phase==WF_SESSION_FATAL)return WF_BLOCKED;
 if(s->phase!=WF_SESSION_FRONTEND)return WF_BAD_STATE;
 int result=wf_r18_frontend_driver_step(&s->frontend);
 if(result<0)return fail(s,s->frontend.blocked_dependency,s->frontend.pc);
 if(result==WF_R18_DRIVER_HANDOFF)s->phase=WF_SESSION_MATCH_READY;
 return WF_OK;
}
wf_status_t wf_session_bind_match(wf_session_t *s){
 if(!s||!s->initialized||s->frontend.phase!=WF_R18_DRIVER_AC0)return WF_BAD_STATE;
 return wf_r18_match_driver_init(&s->match,&s->frontend,extension,s);
}
wf_status_t wf_session_prepare_match(wf_session_t *s){
 if(!s||!s->initialized)return WF_BAD_STATE;
 if(s->phase==WF_SESSION_FATAL)return WF_BLOCKED;
 if(s->phase!=WF_SESSION_MATCH_READY)return WF_BAD_STATE;
 if(wf_session_bind_match(s)!=WF_OK)
  return fail(s,0xac0u,s->frontend.pc);
 wf_status_t status=wf_r18_match_driver_run(&s->match);
 if(s->match.generation!=s->frontend.generation&&
    s->frontend.phase!=WF_R18_DRIVER_FATAL){
  s->phase=WF_SESSION_FRONTEND;return WF_OK;
 }
 if(status!=WF_OK)return fail(s,s->match.blocked_dependency,s->match.blocked_pc);
 status=wf_r18_runtime_handoff_f8c(&s->runtime,s->ram,sizeof s->ram,&s->frontend.regs);
 if(status!=WF_OK)return fail(s,0xf8cu,s->match.flow.pc);
 s->phase=WF_SESSION_GAMEPLAY;return WF_OK;
}
