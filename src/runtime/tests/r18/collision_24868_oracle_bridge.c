#include <string.h>
#include "wf_runtime.h"
#include "wf_collision_dispatch.h"

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

int collision_24868_oracle(uint8_t *memory, uint32_t *d)
{
    (void)d;
    wf_runtime_t runtime = {0};
    uint8_t *ram = memory + 0x1c0000;
    memcpy(runtime.actors, ram + 0x5b0, sizeof runtime.actors);
    wf_actor_raw_t *source = &runtime.actors[5];
    wf_actor_raw_t *target = &runtime.actors[4];
    wf_collision_pair_ctx_t pair = {
        source, target,
        wf_actor_from_rom_ptr(&runtime, be32(target->b + WF_A_LINK), NULL),
        5u, 17u, 9u, NULL
    };
    int result = wf_collision_profile_pair_apply_rows(17u, 9u, &pair);
    memcpy(ram + 0x5b0, runtime.actors, sizeof runtime.actors);
    return result;
}
