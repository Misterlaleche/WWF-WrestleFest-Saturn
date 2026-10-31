#ifndef WF_R17_HUMAN_RAW23_H
#define WF_R17_HUMAN_RAW23_H
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_r17_human_ddc8.h"
#include "wf_r17_rng1129e.h"

typedef struct wf_r17_human_raw23_ctx {
    uint8_t mode_0161; /* arcade $1C0161 */
    uint32_t *global_1670; /* arcade LONG $1C1670 for $DDC8 boundary owner */
    wf_actor_raw_t *fixed_object0; /* arcade $1C0F1C, WP31 owner */
    wf_actor_raw_t *fixed_object1; /* arcade $1C1028, WP31 owner */
    uint16_t global_0162; /* arcade WORD $1C0162, consumed by $1129E */
    const wf_r17_rng21b4_context_t *rng_1129e; /* legacy exact snapshot path for regression */
    wf_r17_m68k_shadow_t *m68k_live; /* R17 persistent MC68000 D0..D7 shadow */
    uint32_t *rng_state_1c005c;      /* shared RNG state for live path */
    uint32_t global_1c0080;          /* frame-counter term for live path */
} wf_r17_human_raw23_ctx_t;

typedef struct wf_r17_human_raw23_trace {
    uint32_t blocked_dependency_rom;
    uint32_t last_stage_rom;
    uint8_t fresh_mask;
    uint8_t category;
    uint8_t raw_action;
    uint8_t selected_raw23;
    uint8_t consumed_before_raw23;
    uint8_t target_slot;
} wf_r17_human_raw23_trace_t;

/* ROM-exact human-control owner for the closed $DCDE/$DE86 priority path.
 * R10 includes the ordered $E002..$E4BA category classifiers, the complete
 * 12x21x3 $E4FE action matrix, $E926, $EF9A and $E830.  The historical
 * function name is retained for source compatibility with R05-R09 tests.
 * Downstream action-callback execution remains separately owned. */
wf_status_t wf_r17_human_dcde_raw23(wf_runtime_t *rt,
                                    unsigned slot,
                                    const wf_r17_human_raw23_ctx_t *ctx,
                                    wf_r17_human_raw23_trace_t *trace);

#endif
