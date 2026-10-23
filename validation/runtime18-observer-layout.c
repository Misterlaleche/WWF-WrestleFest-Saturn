/* Cross-compile only: extract .rodata as big-endian uint32 values for the
 * read-only Mednafen observer. Uses the executable's actual SH-2 C ABI. */
#include <stddef.h>
#include "wf_gameplay_bindings.h"
const uint32_t wf_observer_layout[] = {
 offsetof(wf_runtime_t,g.frame_counter),
 offsetof(wf_runtime_t,g.global_17e6),
 offsetof(wf_runtime_t,g.global_17ea),
 offsetof(wf_runtime_t,g.global_17ee),
 offsetof(wf_runtime_t,g.global_17f2_word),
 offsetof(wf_gameplay_binding_context_t,camera.scroll0_x),
 offsetof(wf_gameplay_binding_context_t,camera.scroll1_x),
 offsetof(wf_gameplay_binding_context_t,camera.scroll0_y),
 offsetof(wf_gameplay_binding_context_t,camera.scroll1_y),
 offsetof(wf_runtime_t,frame_ready)
};
