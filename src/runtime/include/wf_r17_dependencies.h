#ifndef WF_R17_DEPENDENCIES_H
#define WF_R17_DEPENDENCIES_H
#include "wf_runtime.h"
struct wf_gameplay_binding_context;
/* Core RAM represented in wf_runtime_t for ROM $1F6C/$1FDE. The platform
 * reset callback must separately clear map caches/animation queues, external
 * $19B2..$1CD1 storage and video RAM/ports. Leaves D0.W=$FFFF as $1FDE. */
wf_status_t wf_r17_transition_reset_ram(wf_runtime_t *rt);
wf_status_t wf_r17_f98c(wf_runtime_t *rt,struct wf_gameplay_binding_context *ctx);
wf_status_t wf_r17_a0e0(wf_runtime_t *rt,struct wf_gameplay_binding_context *ctx);
#endif
