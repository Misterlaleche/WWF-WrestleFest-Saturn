#ifndef WF_R17_BOOTSTRAP_H
#define WF_R17_BOOTSTRAP_H
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_r17_m68k_shadow.h"

typedef struct wf_r17_f8c_state {
    uint16_t control_1c0076;
    uint16_t sync_1c006e;
    uint32_t counter_1c0080;
    uint8_t branch_1c0083;
} wf_r17_f8c_state_t;

typedef struct wf_r17_f8c_trace {
    uint32_t rng_before;
    uint32_t rng_after;
    uint32_t d0_before;
    uint32_t d0_after;
    uint8_t rng_calls;
    uint8_t branch;
} wf_r17_f8c_trace_t;

/* Exact Gameplay boundary common to paid direct match and attract after $8AE6.
 * This does not recreate SceneFlow/Prematch; it consumes their CLOSED handoff. */
wf_status_t wf_r17_f8c_enter(wf_r17_f8c_state_t *s,uint32_t *state_1c005c,wf_r17_m68k_shadow_t *regs,wf_r17_f8c_trace_t *tr);
/* Exact attract timeout comparison after synchronized post-frame increment. */
int wf_r17_attract_timeout_after_increment(uint32_t counter,uint16_t threshold);
#endif
