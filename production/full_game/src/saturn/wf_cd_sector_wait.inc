/* Opt-in derived wait candidate. Uses the reader's unchanged command(), so
 * every query retains the existing rate guard and command completion checks.
 * This fixes a host-modeled stale-count decision, not a proven silicon fault. */
#include "wf_cd_wait_diagnostics.h"
#ifndef WP2015_CD_CSCT_READ
#define WP2015_CD_CSCT_READ() (HIRQ)
#endif
#ifndef WP2015_CD_CSCT_ACK
#define WP2015_CD_CSCT_ACK(flags) (HIRQ=(uint16_t)~(flags))
#endif
extern volatile uint32_t wf_fields __attribute__((weak));
volatile wf_cd_wait_diagnostics_t wf_cd_wait_diagnostics;
static uint32_t wf_cd_wait_start_fields, wf_cd_wait_remaining;
static void wf_cd_wait_reset(void){
 wf_cd_wait_diagnostics_t empty={0};
 empty.version=1;empty.fields_available=(&wf_fields!=0);
 wf_cd_wait_start_fields=empty.fields_available?wf_fields:0;
 wf_cd_wait_remaining=0;wf_cd_wait_diagnostics=empty;
}
static int wf_cd_wait_finish(wf_cd_wait_diagnostics_t *d,int rc){
 d->result=rc;
 d->fields_elapsed=d->fields_available?wf_fields-wf_cd_wait_start_fields:0;
 /* Only this helper publishes a wait. Cleanup commands cannot overwrite it. */
 wf_cd_wait_diagnostics=*d;
 return rc;
}
static int wf_cd_wait_query(response_t *response,uint32_t *available,
                            wf_cd_wait_diagnostics_t *d){
 WP2015_CD_CSCT_ACK(CSCT);
 int rc=command(0x5100u,0,0,0,0,response);
 if(rc)return rc;
 d->count=response->cr4;
 if(d->count>200u)return WP1979_CD_BUFFER_COUNT;
 if(d->count)*available=d->count;
 return 0;
}
static int sector_count_when_ready(response_t *response,uint32_t *available){
 wf_cd_wait_diagnostics_t d={0};
 d.version=1;d.fields_available=wf_cd_wait_diagnostics.fields_available;
 d.remaining_sectors=wf_cd_wait_remaining;
 d.read_requeries=wf_cd_wait_diagnostics.read_requeries;
 d.read_recoveries=wf_cd_wait_diagnostics.read_recoveries;
 for(uint32_t notices=0;notices<EMPTY_COMMAND_LIMIT;notices++){
  int rc=wf_cd_wait_query(response,available,&d);
  d.notices=notices+1;
  if(rc){d.reason=rc==WP1979_CD_BUFFER_COUNT?WF_CD_WAIT_COUNT:WF_CD_WAIT_COMMAND;return wf_cd_wait_finish(&d,rc);}
  if(d.count)return wf_cd_wait_finish(&d,0);
  uint32_t polls;
  for(polls=0;polls<POLL_LIMIT;polls++){
   uint16_t flags=WP2015_CD_CSCT_READ();
   d.last_hirq=flags;d.seen_hirq|=flags;
   /* CSCT retains priority, including on the last allowed poll. */
   if(flags&CSCT)break;
   if(flags&PEND){d.reason=WF_CD_WAIT_PEND;break;}
  }
  uint32_t sampled=polls<POLL_LIMIT?polls+1:POLL_LIMIT;
  d.polls=UINT32_MAX-d.polls<sampled?UINT32_MAX:d.polls+sampled;
  if(polls==POLL_LIMIT)d.reason=WF_CD_WAIT_POLLS;
  if(d.reason){
   /* Exactly one terminal requery. A valid current count alone rescues this
    * wait. No retry of the read, no larger budgets, no assumed availability. */
   d.requery_attempt=1;++d.read_requeries;
   rc=wf_cd_wait_query(response,available,&d);
   if(!rc && !d.count)rc=WP1979_CD_SECTOR_TIMEOUT;
   d.requery_result=rc;
   if(!rc){d.requery_recovered=1;++d.read_recoveries;}
   return wf_cd_wait_finish(&d,rc);
  }
 }
 /* Repeated empty notices keep the original strict query bound. No final
  * requery here: EMPTY_COMMAND_LIMIT is already the command budget. */
 d.reason=WF_CD_WAIT_NOTICES;
 return wf_cd_wait_finish(&d,WP1979_CD_SECTOR_TIMEOUT);
}
