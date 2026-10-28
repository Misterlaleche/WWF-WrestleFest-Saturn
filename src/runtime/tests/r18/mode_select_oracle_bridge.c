#include <string.h>
#include "wf_r18_mode_select.h"

static uint32_t r16(const uint8_t *p) { return ((uint32_t)p[0] << 8) | p[1]; }
static void put(uint8_t *p, uint32_t v, unsigned width) {
    for (unsigned n = 0; n < width; ++n) p[n] = (uint8_t)(v >> (8u * (width - n - 1u)));
}
typedef struct effect_context { uint8_t *memory; uint32_t *events; unsigned count; } effect_context_t;
static void effect(void *user, const wf_r18_mode_effect_t *e, uint8_t *ram,
                   wf_r17_m68k_shadow_t *regs) {
    effect_context_t *context = user;
    uint8_t *memory = context->memory;
    uint32_t *record = context->events + context->count++ * 5u;
    record[0] = e->owner_rom; record[1] = e->source_rom;
    record[2] = e->destination; record[3] = e->count; record[4] = e->descriptor;
    if (e->words) {
        for (unsigned i = 0; i < e->count; ++i) put(memory + e->destination + 2u * i, e->words[i], 2u);
    } else if (e->owner_rom == 0x51d2u || e->owner_rom == 0x527eu) {
        /* Independent test adapter executes the descriptor format in the ROM.
         * The production owner emits the exact dependency, not this renderer. */
        uint32_t source = e->source_rom;
        while (r16(memory + source) != 0xfffu) {
            uint32_t destination = 0xc1770u + r16(memory + source);
            uint32_t tile = r16(memory + source + 2u);
            unsigned count = (unsigned)r16(memory + source + 4u) + 1u;
            for (unsigned i = 0; i < count; ++i)
                put(memory + destination + 4u * i,
                    e->owner_rom == 0x51d2u ? ((tile + i) << 16) | 0x1eu : 0u, 4u);
            source += 6u;
        }
    } else {
        /* Credit fixtures hold the original $1E92 cache clean. */
        if (e->owner_rom != 0x1e92u || r16(ram + 0x72) != r16(ram + 0x4e) ||
            (r16(ram + 0x72) & 0x8000u)) context->count = 0xffffffffu;
    }
    (void)regs;
}

int mode_select_oracle(uint8_t *memory, uint32_t *d, uint32_t *cursor,
                       const uint16_t *ports, uint32_t *events) {
    wf_r17_m68k_shadow_t regs;
    wf_r18_mode_select_t state;
    wf_r18_mode_select_context_t ctx;
    effect_context_t effects = {memory, events + 1, 0u};
    state.pc = cursor[0]; state.actor_offset = (uint16_t)cursor[1];
    state.assignment_credit = (uint8_t)cursor[2]; state.blocked_dependency = 0;
    memcpy(regs.d, d, sizeof regs.d); regs.valid = 1u;
    ctx.ram_1c0000 = memory + 0x1c0000; ctx.ram_bytes = 0x2000;
    ctx.regs = &regs; ctx.effect = effect; ctx.user = &effects;
    memcpy(ctx.input_words, ports, sizeof ctx.input_words);
    int result = wf_r18_mode_select_step(&state, &ctx);
    memcpy(d, regs.d, sizeof regs.d);
    cursor[0] = state.pc; cursor[1] = state.actor_offset;
    cursor[2] = state.assignment_credit; cursor[3] = state.blocked_dependency;
    events[0] = effects.count;
    return result;
}
