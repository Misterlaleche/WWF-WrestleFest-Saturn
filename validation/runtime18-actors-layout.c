/* Compile with the same SH-2 ABI as the executable; read-only observer data. */
#include <stddef.h>
#include "wf_gameplay_bindings.h"
const uint32_t actors_layout[] = {
    offsetof(wf_runtime_t, actors), sizeof(wf_actor_raw_t),
    offsetof(wf_runtime_t, g.rng_state), offsetof(wf_runtime_t, m68k_regs.d),
    offsetof(wf_gameplay_binding_context_t, input_hw_low)
};
