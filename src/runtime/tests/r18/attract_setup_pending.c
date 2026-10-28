#include "wf_r18_attract_setup.h"
#include <string.h>
void wf_r18_attract_setup_init(wf_r18_attract_setup_t*s,uint32_t pc){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}
int wf_r18_attract_setup_step(wf_r18_attract_setup_t*s,wf_r18_prematch_t*c){s->blocked_dependency=c->dependency=s->pc;return WF_R18_ATTRACT_BLOCKED;}
int wf_r18_attract_setup_complete(wf_r18_attract_setup_t*s,wf_r18_prematch_t*c,wf_status_t st,uint16_t v){(void)s;(void)c;(void)st;(void)v;return WF_R18_ATTRACT_BAD_CONTEXT;}
