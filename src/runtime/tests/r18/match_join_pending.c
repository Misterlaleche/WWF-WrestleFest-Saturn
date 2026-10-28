#include "wf_r18_match_setup.h"
wf_status_t wf_r18_match_setup_d1c(wf_r18_prematch_t*c){c->dependency=0xd1c;return WF_BLOCKED;}
wf_status_t wf_r18_match_setup_ffd2(wf_r18_prematch_t*c){c->dependency=0xffd2;return WF_BLOCKED;}
wf_status_t wf_r18_match_setup_1004a(wf_r18_prematch_t*c){c->dependency=0x1004a;return WF_BLOCKED;}
void wf_r18_match_join_init(wf_r18_match_join_t*s){s->pc=0xc98;s->blocked_dependency=0;}
int wf_r18_match_join_step(wf_r18_match_join_t*s,wf_r18_prematch_t*c){(void)c;s->blocked_dependency=s->pc;return WF_R18_PREMATCH_BLOCKED;}
