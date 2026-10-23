#ifndef WF_CPU_RUNTIME_H
#define WF_CPU_RUNTIME_H
#include "wf_state5_exec.h"
typedef struct wf_cpu_trace {uint8_t state;uint32_t handler_rom;uint32_t blocked_dependency_rom;wf_state5_trace_t state5;} wf_cpu_trace_t;
wf_status_t wf_cpu_actor_step(wf_runtime_t *rt,unsigned slot,wf_cpu_trace_t *trace);
wf_status_t wf_cpu_relation_1c228(wf_runtime_t *,unsigned);
int wf_cpu_arrived_1f15c(wf_runtime_t *,wf_actor_raw_t *);
wf_status_t wf_cpu_continue_1c5b6(wf_runtime_t *,unsigned,wf_state5_trace_t *);
wf_status_t wf_cpu_decide_1f084(wf_runtime_t *,unsigned,unsigned,wf_state5_trace_t *);
#endif
