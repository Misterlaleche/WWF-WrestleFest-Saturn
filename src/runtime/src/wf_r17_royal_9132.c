#include "wf_r17_royal_9132.h"
#include <string.h>
void wf_participant_9132(wf_participant_machine_t *);
static wf_participant_machine_t royal_machine;
static wf_runtime_t royal_snapshot;
static unsigned royal_busy;
wf_status_t wf_r17_owner_9132(wf_runtime_t*rt,wf_actor_raw_t*target,uint16_t paid_index,wf_r17_participant_io_t*ctx){
 wf_status_t status;wf_participant_machine_t*m=&royal_machine;
 (void)target;(void)paid_index;
 if(!rt||!ctx)return WF_BAD_STATE;
 ctx->blocked_dependency_rom=0;
 if(royal_busy)return WF_BAD_STATE;
 if(!ctx->fg0_batch&&!ctx->commit_batch){ctx->blocked_dependency_rom=0x9658;return WF_BLOCKED;}
 royal_busy=1;royal_snapshot=*rt;wf_participant_init(m,rt,ctx->input_word,ctx->user);
 wf_participant_9132(m);status=m->status;
 if(status==WF_OK){
  if(ctx->commit_batch)status=ctx->commit_batch(rt,m->writes,m->write_count,NULL,0,ctx->user);
  else status=ctx->fg0_batch(rt,m->writes,m->write_count,ctx->user);
 }
 if(status!=WF_OK){*rt=royal_snapshot;ctx->blocked_dependency_rom=m->failed_address?m->failed_address:0x9658;}
 /* $9132 MOVEM restores all caller D/A; rt shadow remains untouched. */
 rt->m68k_regs=royal_snapshot.m68k_regs;
 royal_busy=0;return status;
}
