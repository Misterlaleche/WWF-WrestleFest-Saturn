#ifndef WF_R17_HUMAN_DDC8_H
#define WF_R17_HUMAN_DDC8_H
#include <stdint.h>
#include "wf_runtime.h"

typedef struct wf_r17_human_ddc8_ctx {
    uint8_t mode_0161;          /* arcade $1C0161 */
    uint32_t *global_1670;      /* arcade LONG $1C1670, mutable owner bitmap */
} wf_r17_human_ddc8_ctx_t;

typedef struct wf_r17_human_ddc8_trace {
    uint32_t blocked_dependency_rom;
    uint32_t last_stage_rom;
    uint8_t consumed;
    uint8_t boundary_slot;
    uint8_t streak_success;
} wf_r17_human_ddc8_trace_t;

/* Exact $DDC8 held-direction owner, including the $EDC0/$EECE boundary
 * consumers, $EF0A special directional owner and $EF6A four-invocation
 * streak primitive.  No semantic RUN/TAG aliases are introduced. */
wf_status_t wf_r17_human_ddc8(wf_runtime_t *rt,
                              unsigned slot,
                              const wf_r17_human_ddc8_ctx_t *ctx,
                              wf_r17_human_ddc8_trace_t *trace);

#endif
