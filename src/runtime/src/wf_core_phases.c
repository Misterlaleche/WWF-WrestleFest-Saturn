#include "wf_runtime.h"
/* Exact central damage phase implementation. Other phase owners bind independently. */
static void damage_one(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,void *u){(void)rt;(void)slot;(void)u;wf_apply_pending_damage_and_tier(a);}
wf_status_t wf_phase_damage(wf_runtime_t *rt,uint32_t addr,void *user){(void)addr;(void)user;wf_for_each_active(rt,damage_one,0);return WF_OK;}
wf_status_t wf_phase_vblank_stub(wf_runtime_t *rt,uint32_t addr,void *user){(void)addr;(void)user;rt->vblank_ack=1;return WF_OK;}
