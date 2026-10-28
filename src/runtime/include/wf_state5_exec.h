#ifndef WF_STATE5_EXEC_H
#define WF_STATE5_EXEC_H
#include <stdint.h>
#include "wf_runtime.h"

typedef struct wf_state5_trace {
 uint8_t selector;
 uint32_t callback_rom;
 uint8_t forced;
 uint8_t invoked;
 uint32_t blocked_dependency_rom;
} wf_state5_trace_t;

/* Exact $1DFE2 execution gate plus the translated callback prefix/body that is
 * statically proven by WP01. A demonstrated tail owned elsewhere may return
 * WF_BLOCKED after its exact writes; blocked_dependency_rom identifies it. */
wf_status_t wf_state5_execute(wf_runtime_t *rt,unsigned slot,wf_state5_trace_t *trace);

#endif
