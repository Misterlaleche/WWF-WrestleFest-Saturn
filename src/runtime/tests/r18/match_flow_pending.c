#include "wf_r18_match_flow.h"
void wf_r18_match_flow_init(wf_r18_match_flow_t*s,uint32_t pc){s->pc=pc;s->entry_pc=pc;s->pending=0;s->fatal=0;s->blocked_dependency=0;}
int wf_r18_match_flow_step(wf_r18_match_flow_t*s,wf_r18_prematch_t*c){(void)c;s->blocked_dependency=s->pc;return WF_R18_MATCH_BLOCKED;}
int wf_r18_match_flow_complete(wf_r18_match_flow_t*s,wf_r18_prematch_t*c,wf_status_t status,uint32_t pc){(void)s;(void)c;(void)status;(void)pc;return WF_R18_MATCH_BLOCKED;}
