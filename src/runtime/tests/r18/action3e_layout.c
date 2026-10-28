#include <stddef.h>
#include "wf_runtime.h"
/* Scalar prefix only: verify against the exact frozen Stage7b SH2 layout. */
const uint32_t wf_action3e_layout[] = {
 offsetof(wf_runtime_t,actors),sizeof(wf_actor_raw_t),
 offsetof(wf_runtime_t,g.rng_state),offsetof(wf_runtime_t,m68k_regs),
 offsetof(wf_runtime_t,g),offsetof(wf_runtime_t,g.frame_counter),
 offsetof(wf_runtime_t,g.mode_0161),offsetof(wf_runtime_t,g.credited_route),
 offsetof(wf_runtime_t,g.global_007e),offsetof(wf_runtime_t,g.global_1670),
 offsetof(wf_runtime_t,g.global_1150),offsetof(wf_runtime_t,g.global_1180),
 offsetof(wf_runtime_t,g.global_11b0),offsetof(wf_runtime_t,g.global_11e0)
};
