#ifndef WF_R17_RESOLVER_280DC_H
#define WF_R17_RESOLVER_280DC_H
#include "wf_runtime.h"

/* Exact $280DC ring/contact resolver.  Numeric actor fields are kept at their
 * original widths; no semantic re-labelling is introduced here. */
wf_status_t wf_r17_resolver_280dc(wf_runtime_t *rt,wf_actor_raw_t *actor);
wf_status_t wf_r17_resolver_280dc_hook(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user);
#endif
