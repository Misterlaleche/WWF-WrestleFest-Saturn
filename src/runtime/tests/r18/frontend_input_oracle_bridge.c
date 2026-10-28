#include <string.h>
#include "wf_r18_frontend_input.h"

static uint32_t read32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

/* Test-only adapter. ram is the complete $1C0000..$1C1FFF window. */
int frontend_input_oracle(uint8_t *ram, uint32_t *d, unsigned slot,
                          uint16_t active_low) {
    wf_actor_raw_t actor;
    wf_r17_m68k_shadow_t regs;
    wf_r18_frontend_input_context_t ctx;
    uint32_t rng = read32(ram + 0x5c);
    int status;
    if (slot >= 4u) return -2;
    memcpy(&actor, ram + 0x5b0 + slot * WF_ACTOR_BYTES, sizeof actor);
    memcpy(regs.d, d, sizeof regs.d);
    regs.valid = 1u;
    ctx.rng_state_1c005c = &rng;
    ctx.frame_1c0080 = read32(ram + 0x80);
    ctx.flags_1c015e = ram[0x15e];
    status = wf_r18_frontend_input_64e0(&actor, active_low, &ctx, &regs);
    memcpy(ram + 0x5b0 + slot * WF_ACTOR_BYTES, &actor, sizeof actor);
    memcpy(d, regs.d, sizeof regs.d);
    ram[0x5c] = (uint8_t)(rng >> 24);
    ram[0x5d] = (uint8_t)(rng >> 16);
    ram[0x5e] = (uint8_t)(rng >> 8);
    ram[0x5f] = (uint8_t)rng;
    return status;
}

/* Missing live RNG/register context must fail without changing caller state. */
int frontend_input_invalid_contexts(void) {
    for (unsigned which = 0; which < 5u; ++which) {
        wf_actor_raw_t actor, before_actor;
        wf_r17_m68k_shadow_t regs, before_regs;
        uint32_t rng = 0x12345678u;
        wf_r18_frontend_input_context_t ctx = {&rng, 42u, 0x10u};
        int status;
        memset(&actor, 0xa5, sizeof actor);
        memset(&regs, 0x5a, sizeof regs);
        regs.valid = which == 4u ? 0u : 1u;
        if (which == 3u) ctx.rng_state_1c005c = 0;
        before_actor = actor;
        before_regs = regs;
        status = wf_r18_frontend_input_64e0(which == 0u ? 0 : &actor, 0u,
            which == 1u ? 0 : &ctx, which == 2u ? 0 : &regs);
        if (status != -1 || memcmp(&actor, &before_actor, sizeof actor) != 0 ||
            memcmp(&regs, &before_regs, sizeof regs) != 0 || rng != 0x12345678u)
            return (int)which + 1;
    }
    return 0;
}
