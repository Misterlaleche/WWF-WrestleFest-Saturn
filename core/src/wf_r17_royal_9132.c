#include "wf_r17_royal_9132.h"
#include <string.h>
void wf_participant_9132(wf_participant_machine_t *);
wf_status_t wf_r17_owner_9132(wf_runtime_t*rt,wf_actor_raw_t*target,uint16_t paid_index,wf_r17_participant_io_t*ctx){
 wf_status_t status;wf_participant_workspace_t*workspace;
 (void)target;(void)paid_index;
 if(!rt||!ctx)return WF_BAD_STATE;
 status=wf_participant_workspace_acquire(WF_PARTICIPANT_ROYAL,&workspace);if(status!=WF_OK)return status;
 ctx->blocked_dependency_rom=0;
 if(!ctx->fg0_batch&&!ctx->commit_batch){ctx->blocked_dependency_rom=0x9658;(void)wf_participant_workspace_release(WF_PARTICIPANT_ROYAL,workspace);return WF_BLOCKED;}
 wf_participant_machine_t*m=&workspace->machine;wf_runtime_t*snapshot=&workspace->snapshot;
 *snapshot=*rt;wf_participant_init(m,rt,ctx->input_word,ctx->user);
 wf_participant_9132(m);status=m->status;
 if(status==WF_OK){
  if(ctx->commit_batch)status=ctx->commit_batch(rt,m->writes,m->write_count,NULL,0,ctx->user);
  else status=ctx->fg0_batch(rt,m->writes,m->write_count,ctx->user);
 }
 if(status!=WF_OK){*rt=*snapshot;ctx->blocked_dependency_rom=m->failed_address?m->failed_address:0x9658;}
 /* $9132 MOVEM restores all caller D/A; rt shadow remains untouched. */
 rt->m68k_regs=snapshot->m68k_regs;
 (void)wf_participant_workspace_release(WF_PARTICIPANT_ROYAL,workspace);return status;
}
