/* Compile using the same SH2 ABI as the supplied, unchanged game. */
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"
const uint32_t audio2_game_layout[] = {
    offsetof(wf_runtime_t, actors), sizeof(wf_actor_raw_t), WF_ACTOR_SLOTS,
    offsetof(wf_runtime_t, g.frame_counter), offsetof(wf_runtime_t, g.mode_0161),
    offsetof(wf_runtime_t, g.progression_0162), sizeof(wf_runtime_t)
};
