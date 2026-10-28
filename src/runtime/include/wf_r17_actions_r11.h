#ifndef WF_R17_ACTIONS_R11_H
#define WF_R17_ACTIONS_R11_H
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_r17_raw23.h"
#include "wf_r17_post7a00.h"
#include "wf_r17_contract.h"

typedef struct wf_r17_actions_r11_trace {
    uint32_t callback_rom;
    uint32_t descriptor_rom;
    uint32_t tick_descriptor_rom;
    uint32_t blocked_dependency_rom;
    uint8_t selector;
} wf_r17_actions_r11_trace_t;

typedef struct wf_r17_actions_r11_bridge {
    wf_r17_raw23_ctx_t *raw23_ctx;
    wf_actor_raw_t *fixed_object0; /* arcade $1C0F1C */
    wf_actor_raw_t *fixed_object1; /* arcade $1C1028 */
    wf_r17_fg0_batch_fn video_batch;
    void *video_user;
    wf_r17_render_ctx_t *render;
    wf_actor_raw_t transient_views[11]; /* full native views of compact $1258 records */
    wf_r17_actions_r11_trace_t trace[WF_ACTOR_SLOTS];
    wf_r17_raw23_trace_t raw23_trace[WF_ACTOR_SLOTS];
    wf_r17_raw_word_fn resource_2b58; /* existing allocator owner; uses video_user */
    wf_r17_raw_word_fn resource_2aea; /* action8E's one-time palette load */
} wf_r17_actions_r11_bridge_t;

/* Original $10D3A temporary effect, shared with state-4 callback $1BBC6. */
wf_status_t wf_r17_action_effect_10d3a(wf_runtime_t *, wf_actor_raw_t *,
                                      unsigned id, wf_r17_actions_r11_bridge_t *,
                                      wf_r17_actions_r11_trace_t *);

/* Complete ROM $114B2, shared by main idle and raw $3D. */
wf_status_t wf_r17_action_idle_114b2(wf_runtime_t *, wf_actor_raw_t *,
                                    wf_r17_actions_r11_trace_t *);

/* Shared original partner transaction, also called by raw action $23. */
wf_status_t wf_r17_action_partner_215b6(wf_runtime_t *, wf_actor_raw_t *,
                                       uint32_t *blocked_rom);

/* Shared $21358 cleanup. The caller supplies the original A0 memory view. */
wf_status_t wf_r17_action_cleanup_21358(wf_runtime_t *, wf_actor_raw_t *,
                                       uint32_t *blocked_rom);

/* Shared original $212A0 scene/mode-dependent interruption cleanup. */
wf_status_t wf_r17_action_cleanup_212a0(wf_runtime_t *, wf_actor_raw_t *,
                                       uint32_t *blocked_rom);

/* Bounded ROM-backed $1C03E action implementation. Unimplemented selectors
 * retain the explicit $12614 dependency; no fallback animation is supplied. */
wf_status_t wf_r17_dispatch_1c03e_r11(wf_runtime_t *rt,
                                      unsigned slot,
                                      wf_r17_actions_r11_bridge_t *bridge);
wf_status_t wf_r17_dispatch_1c03e_r11_hook(wf_runtime_t *rt,
                                           wf_actor_raw_t *actor,
                                           void *user);
#endif
