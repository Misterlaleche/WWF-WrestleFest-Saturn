#include <stddef.h>
#include "wf_gameplay_bindings.h"
/* Compile with the frozen Stage4 SH2 headers. Data only; never executed. */
const uint32_t wf_action04_layout[] = {
 sizeof(wf_gameplay_binding_context_t),
 offsetof(wf_gameplay_binding_context_t,f4c2_actions),
 offsetof(wf_gameplay_binding_context_t,f4c2_actions.trace),
 sizeof(wf_r17_actions_r11_trace_t),
 offsetof(wf_r17_actions_r11_trace_t,blocked_dependency_rom),
 offsetof(wf_r17_actions_r11_trace_t,selector),
 offsetof(wf_runtime_t,actors),
 offsetof(wf_runtime_t,g.rng_state),
 offsetof(wf_runtime_t,g.frame_counter),
 offsetof(wf_runtime_t,m68k_regs),
 sizeof(wf_runtime_t)
};
/* This pointer-free prefix has identical HOST/SH2 offsets. */
const uint32_t wf_action04_globals[] = {
 offsetof(wf_runtime_t,g.mode_0161),offsetof(wf_runtime_t,g.credited_route),
 offsetof(wf_runtime_t,g.global_007e),offsetof(wf_runtime_t,g.global_1670),
 offsetof(wf_runtime_t,g.global_1150),offsetof(wf_runtime_t,g.global_1180),
 offsetof(wf_runtime_t,g.global_11b0),offsetof(wf_runtime_t,g.global_11e0)
};
