#ifndef WF_R17_RINGSIDE_FDEE_H
#define WF_R17_RINGSIDE_FDEE_H
#include "wf_runtime.h"
#include "wf_r17_contract.h"

typedef struct wf_r17_fdee_trace {
 uint32_t blocked_dependency_rom;
 uint8_t processed_objects;
} wf_r17_fdee_trace_t;

wf_status_t wf_r17_ringside_fdee(wf_runtime_t *rt,wf_r17_render_ctx_t *render,wf_r17_fdee_trace_t *trace);
#endif
