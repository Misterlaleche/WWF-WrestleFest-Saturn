#ifndef WF_R17_FIXED_FOUR_10120_H
#define WF_R17_FIXED_FOUR_10120_H
#include "wf_runtime.h"
#include "wf_r17_contract.h"

typedef wf_status_t (*wf_r17_fixed_four_audio_fn)(wf_runtime_t *rt,uint16_t raw_command,void *user);

typedef struct wf_r17_fixed_four_trace {
 uint32_t blocked_dependency_rom;
 uint8_t processed_objects;
} wf_r17_fixed_four_trace_t;

/* Direct ROM initializer at $1004C.  This writes only fields written by ROM. */
void wf_r17_fixed_four_init_1004c(wf_runtime_t *rt);

/* Direct fixed-four scheduler owner at $10120. */
wf_status_t wf_r17_fixed_four_10120(wf_runtime_t *rt,wf_r17_render_ctx_t *render,
                                    wf_r17_fixed_four_audio_fn audio,void *audio_user,
                                    wf_r17_fixed_four_trace_t *trace);
#endif
