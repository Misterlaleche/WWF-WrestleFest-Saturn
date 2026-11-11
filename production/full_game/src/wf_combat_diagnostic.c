#include "wf_combat_diagnostic.h"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize ("Oz")
#endif
__attribute__((cold,noinline))
void wf_failure_combat_from_runtime(wf_failure_combat_t *out,const wf_runtime_t *r){
 out->valid=0;if(!r)return;
 out->royal=r->g.mode_0161&1u;out->frame=r->g.frame_counter;out->rng=r->g.rng_state;
 out->scene=r->g.global_007e;out->frame_phase=r->frame_stage;
 for(unsigned i=0;i<9;i++){
  const wf_actor_raw_t *a=r->actors+i;wf_failure_actor_t *p=out->actors+i;
  p->id=wf_be16(a,WF_A_CHARACTER);p->active=wf_u8(a,WF_A_ACTIVE);
  p->state=wf_be16(a,WF_A_STATE);p->animation=wf_be16(a,WF_A_ANIM_COUNTER);
  p->action=wf_u8(a,WF_A_ACTION_PHASE);p->x=wf_be16(a,WF_A_X);p->y=wf_be16(a,WF_A_Y);
  p->stamina=wf_be16(a,WF_A_STAMINA);
 }
 out->valid=1;
}
