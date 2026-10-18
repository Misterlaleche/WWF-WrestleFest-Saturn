#ifndef WF_R17_RNG1129E_H
#define WF_R17_RNG1129E_H
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_r17_m68k_shadow.h"

typedef struct wf_r17_rng21b4_context {
    uint32_t *state_1c005c;      /* arcade LONG $1C005C, updated by $21B4 */
    uint32_t global_1c0080;      /* arcade LONG $1C0080 */
    uint32_t d[8];               /* exact D0..D7 snapshot at JSR $1129E entry */
    uint8_t valid;
} wf_r17_rng21b4_context_t;

typedef struct wf_r17_1129e_trace {
    uint32_t blocked_dependency_rom;
    uint32_t rng_before;
    uint32_t rng_after;
    uint32_t d1_at_rng;
    uint32_t d2_at_rng;
    uint16_t mode_entry;
    uint8_t threshold;
    uint8_t rng_called;
    uint8_t carry;
} wf_r17_1129e_trace_t;

/* ROM-exact $1129E helper.  The caller supplies the exact D0..D7 snapshot at
 * JSR entry; this is mandatory because $21B4 mixes full LONG D1..D7, including
 * inherited high words that the caller's MOVE.W writes do not clear. */
wf_status_t wf_r17_1129e(wf_runtime_t *rt,
                         unsigned actor_slot,
                         uint8_t global_0161,
                         uint16_t global_0162,
                         const wf_r17_rng21b4_context_t *rng,
                         wf_r17_1129e_trace_t *trace,
                         int *carry_out);

wf_status_t wf_r17_1129e_live(wf_runtime_t *rt,
                              unsigned actor_slot,
                              uint8_t global_0161,
                              uint16_t global_0162,
                              uint32_t *state_1c005c,
                              uint32_t global_1c0080,
                              const wf_r17_m68k_shadow_t *regs,
                              wf_r17_1129e_trace_t *trace,
                              int *carry_out);

#endif
