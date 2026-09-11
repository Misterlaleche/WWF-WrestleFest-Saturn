#ifndef WF_GAMEPLAY_BINDINGS_H
#define WF_GAMEPLAY_BINDINGS_H
#include "wf_cpu_runtime.h"
#include "wf_input.h"
#include "wf_camera.h"

typedef struct wf_gameplay_binding_context {
 uint8_t input_hw_low[WF_ACTOR_SLOTS];
 uint16_t input_valid_mask;
 uint16_t cabinet_word_140020;
 uint16_t cabinet_word_140022;
 uint8_t cabinet_words_valid;
 wf_cpu_trace_t cpu_trace[WF_ACTOR_SLOTS];
 wf_camera_state_t camera;
 uint8_t camera_valid;
 uint32_t last_blocked_phase;
 uint32_t last_blocked_dependency;
} wf_gameplay_binding_context_t;

size_t wf_gameplay_bindings_build(wf_gameplay_binding_context_t *ctx,wf_phase_binding_t *out,size_t cap);
wf_status_t wf_phase_input_real(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_cpu_real(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_refill_8b3a(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_camera_26936(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_camera_2983c(wf_runtime_t*,uint32_t,void*);
#endif
