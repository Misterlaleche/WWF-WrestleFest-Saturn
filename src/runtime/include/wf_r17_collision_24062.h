#ifndef WF_R17_COLLISION_24062_H
#define WF_R17_COLLISION_24062_H
#include "wf_runtime.h"
typedef struct wf_r17_collision_24062_trace {
 uint32_t blocked_dependency_rom;
 uint16_t source_pairs_tested;
 uint16_t accepted_pairs;
} wf_r17_collision_24062_trace_t;
wf_status_t wf_r17_collision_24062(wf_runtime_t *rt,wf_r17_collision_24062_trace_t *trace);
#endif
