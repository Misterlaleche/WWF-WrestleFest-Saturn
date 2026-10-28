#include "wf_r18_prematch.h"
void wf_r18_prematch_scene_init(wf_r18_prematch_scene_t*s){s->pc=0xa654;s->blocked_dependency=0;s->saved_d0=0;}
int wf_r18_prematch_scene_step(wf_r18_prematch_scene_t*s,wf_r18_prematch_t*c,wf_r18_prematch_io_fn io,void*user){
 (void)c;(void)io;(void)user;s->blocked_dependency=0xa654;return WF_R18_PREMATCH_BLOCKED;
}
