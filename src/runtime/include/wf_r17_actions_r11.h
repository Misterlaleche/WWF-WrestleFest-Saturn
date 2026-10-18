#ifndef WF_R17_ACTIONS_R11_H
#define WF_R17_ACTIONS_R11_H
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_r17_raw23.h"

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
    wf_r17_actions_r11_trace_t trace[WF_ACTOR_SLOTS];
    wf_r17_raw23_trace_t raw23_trace[WF_ACTOR_SLOTS];
} wf_r17_actions_r11_bridge_t;

/* Bounded ROM-exact extension of $1C03E.  R11 executes raw $23, $70, $72 and
 * $7C only. Every other selector remains fail-closed at the $12614 owner. */
wf_status_t wf_r17_dispatch_1c03e_r11(wf_runtime_t *rt,
                                      unsigned slot,
                                      wf_r17_actions_r11_bridge_t *bridge);
wf_status_t wf_r17_dispatch_1c03e_r11_hook(wf_runtime_t *rt,
                                           wf_actor_raw_t *actor,
                                           void *user);
#endif
