#include "wf_r18_card_scene.h"
#include <string.h>
void wf_r18_card_scene_init(wf_r18_card_scene_t*s,uint32_t pc){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}
int wf_r18_card_scene_step(wf_r18_card_scene_t*s,wf_r18_prematch_t*c){s->blocked_dependency=c->dependency=s->pc;return WF_R18_CARD_BLOCKED;}
int wf_r18_card_scene_complete(wf_r18_card_scene_t*s,wf_r18_prematch_t*c,wf_status_t st){(void)s;(void)c;(void)st;return WF_R18_CARD_BAD_CONTEXT;}
