#ifndef WF_R17_CONTRACT_H
#define WF_R17_CONTRACT_H
#include <stdint.h>
#include <stddef.h>
#include "wf_runtime.h"

typedef enum wf_r17_tri { WF_R17_NO=0, WF_R17_YES=1, WF_R17_UNKNOWN=2 } wf_r17_tri_t;
typedef wf_r17_tri_t (*wf_r17_visibility_2188_fn)(wf_actor_raw_t *actor, void *user);
typedef wf_status_t (*wf_r17_actor_hook_fn)(wf_runtime_t *rt, wf_actor_raw_t *actor, void *user);
typedef wf_status_t (*wf_r17_referee_owner_fn)(wf_runtime_t *rt, wf_actor_raw_t *referee, void *user);

#define WF_R17_RENDER_LISTS 6u
#define WF_R17_RENDER_LIST_CAP 32u

typedef struct wf_r17_render_queue {
    wf_actor_raw_t *list[WF_R17_RENDER_LISTS][WF_R17_RENDER_LIST_CAP];
    uint16_t count[WF_R17_RENDER_LISTS];
} wf_r17_render_queue_t;

typedef struct wf_r17_render_ctx {
    uint16_t scroll_a_x; /* arcade $1C17E6 */
    uint16_t scroll_a_y; /* arcade $1C17EE */
    wf_r17_render_queue_t *queue;
    wf_r17_visibility_2188_fn visibility_2188;
    void *visibility_user;
} wf_r17_render_ctx_t;

typedef struct wf_r17_f4c2_hooks {
    wf_r17_actor_hook_fn resolver_280dc;
    wf_r17_actor_hook_fn dispatcher_1c03e; /* callback + $1C0C8 tick, inseparable */
    wf_r17_actor_hook_fn postqueue_2208;
    void *user;
} wf_r17_f4c2_hooks_t;

typedef struct wf_r17_referee_hooks {
    wf_r17_referee_owner_fn owner_1f914_logic; /* $1F952 dispatch + $1FAF0 */
    wf_r17_actor_hook_fn royal_f8e4;
    void *user;
} wf_r17_referee_hooks_t;

void wf_r17_render_queue_clear(wf_r17_render_queue_t *q);
wf_r17_tri_t wf_r17_visibility_2188_exact(wf_actor_raw_t *actor, void *user);
wf_status_t wf_r17_world_to_screen_247c(wf_actor_raw_t *a, uint16_t scroll_a_x, uint16_t scroll_a_y);
wf_status_t wf_r17_queue_27b8(wf_r17_render_ctx_t *ctx, wf_actor_raw_t *a);
size_t wf_r17_emit_actor_order_2836(wf_r17_render_queue_t *q, wf_actor_raw_t **out, size_t out_cap);
wf_status_t wf_r17_referee_1f914(wf_runtime_t *rt, wf_actor_raw_t *referee, wf_r17_render_ctx_t *render, const wf_r17_referee_hooks_t *hooks, uint8_t mode_0161);
wf_status_t wf_r17_wrestlers_f4c2(wf_runtime_t *rt, wf_r17_render_ctx_t *render, const wf_r17_f4c2_hooks_t *hooks);
#endif
