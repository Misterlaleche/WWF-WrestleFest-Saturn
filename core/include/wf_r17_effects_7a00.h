#ifndef WF_R17_EFFECTS_7A00_H
#define WF_R17_EFFECTS_7A00_H
#include "wf_runtime.h"
#include "wf_r17_contract.h"

#define WF_R17_EFFECT_1450_COUNT 3u

typedef struct wf_r17_effects_7a00_trace {
 uint32_t blocked_dependency_rom;
 uint8_t processed_objects;
 uint8_t rng_calls;
 uint8_t rendered_objects;
} wf_r17_effects_7a00_trace_t;

/* Direct ROM owner $007A00 for three compact effect records rooted at
 * $1C1450 with arcade stride $2A. Native storage uses full raw actor records
 * so the already-closed $247C/$27B8 contracts can consume the common prefix
 * without out-of-bounds aliasing. */
wf_status_t wf_r17_effects_7a00(wf_runtime_t *rt,wf_r17_render_ctx_t *render,
                                wf_r17_effects_7a00_trace_t *trace);
#endif
