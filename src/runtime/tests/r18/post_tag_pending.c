#include "wf_r18_post_tag.h"
#include <string.h>
void wf_r18_post_tag_init(wf_r18_post_tag_t*s,uint32_t pc){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}
int wf_r18_post_tag_step(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*c){s->blocked_dependency=c->machine->dependency=s->pc;return WF_R18_POST_BLOCKED;}
int wf_r18_post_tag_complete(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*c,wf_status_t st,uint16_t v){(void)s;(void)c;(void)st;(void)v;return WF_R18_POST_BAD_CONTEXT;}
