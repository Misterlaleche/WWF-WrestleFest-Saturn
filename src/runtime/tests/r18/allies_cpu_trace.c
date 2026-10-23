/* Test-only observation of the real CPU owner, without changing its inputs. */
#define wf_cpu_actor_step allies_cpu_actor_step_unobserved
#include "../../src/wf_cpu_runtime.c"
#undef wf_cpu_actor_step
#include "allies_trace.h"
wf_status_t wf_cpu_actor_step(wf_runtime_t *rt,unsigned slot,wf_cpu_trace_t *tr){
 uint32_t pc=arcade_cpu_main_handler(wf_main_state(&rt->actors[slot]));
 allies_trace_begin(rt,slot,0u,pc);
 wf_status_t status=allies_cpu_actor_step_unobserved(rt,slot,tr);
 allies_trace_end(rt,slot,0u,pc,status);
 return status;
}
