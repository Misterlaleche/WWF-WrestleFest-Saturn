#ifndef WF_R17_DISPATCH_1C03E_H
#define WF_R17_DISPATCH_1C03E_H
#include "wf_runtime.h"
#include "wf_r17_actions_r11.h"

typedef struct wf_r17_dispatch_1c03e_ctx {
    wf_r17_actions_r11_bridge_t *actions;
    uint32_t blocked_dependency_rom;
} wf_r17_dispatch_1c03e_ctx_t;

/* Bounded exact $1C03E owner.  Closed R11 action families execute; state-4
 * callback-RTS entries $1F/$28 execute; all other callbacks expose their
 * precise ROM owner and stop fail-closed. */
wf_status_t wf_r17_dispatch_1c03e_bounded(wf_runtime_t *rt,wf_actor_raw_t *actor,wf_r17_dispatch_1c03e_ctx_t *ctx);
#endif
