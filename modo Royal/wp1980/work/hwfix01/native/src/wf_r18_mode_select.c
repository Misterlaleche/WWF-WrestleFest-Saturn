#include "wf_r18_mode_select.h"
#include <string.h>

/* Intact ROM tables, including bytes adjacent to12/11-byte direction tables
 * because $64E0 can supply all16 contradictory direction nibbles. */
static const uint8_t mode_right[16] = {0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0xffu, 0x00u, 0x00u, 0x01u, 0x00u};
static const uint8_t mode_left[16] = {0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0xffu, 0x00u, 0x00u, 0x00u, 0x00u};
static const uint8_t assignment_upper[16] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u};
static const uint8_t assignment_lower[16] = {0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u};
static const uint8_t pulse_frames[10] = {0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u, 0x00u};
static const uint16_t royal_palette[96] = {
    0x09aau, 0x0a60u, 0x0da0u, 0x0706u, 0x0090u, 0x00f0u, 0x0affu, 0x0fffu, 0x0577u, 0x00bbu, 0x000fu, 0x0ff0u, 0x00ddu, 0x000au, 0x00ffu, 0x0000u,
    0x0bccu, 0x0b82u, 0x0fc2u, 0x0928u, 0x0090u, 0x00f0u, 0x0cffu, 0x0fffu, 0x0799u, 0x02ddu, 0x022fu, 0x0ff3u, 0x02ffu, 0x022cu, 0x03ffu, 0x0000u,
    0x0deeu, 0x0da4u, 0x0fe4u, 0x0b4au, 0x0090u, 0x00f0u, 0x0effu, 0x0fffu, 0x09bbu, 0x04ffu, 0x044fu, 0x0ff6u, 0x04ffu, 0x044fu, 0x06ffu, 0x0000u,
    0x0fffu, 0x0fc6u, 0x0ff6u, 0x0d6cu, 0x0090u, 0x00f0u, 0x0fffu, 0x0fffu, 0x0bddu, 0x06ffu, 0x066fu, 0x0ff9u, 0x06ffu, 0x066fu, 0x09ffu, 0x0000u,
    0x0fffu, 0x0fe8u, 0x0ff8u, 0x0f8eu, 0x0090u, 0x00f0u, 0x0fffu, 0x0fffu, 0x0dffu, 0x08ffu, 0x088fu, 0x0ffcu, 0x08ffu, 0x088fu, 0x0cffu, 0x0000u,
    0x0fffu, 0x0ffau, 0x0ffau, 0x0fafu, 0x0090u, 0x00f0u, 0x0fffu, 0x0fffu, 0x0fffu, 0x0affu, 0x0aafu, 0x0fffu, 0x0affu, 0x0aafu, 0x0fffu, 0x0000u,
};
static const uint16_t tag_palette[96] = {
    0x09aau, 0x0048u, 0x006cu, 0x009eu, 0x02bfu, 0x06dfu, 0x0affu, 0x0fffu, 0x0000u, 0x0204u, 0x0090u, 0x00f0u, 0x00bbu, 0x00ddu, 0x00ffu, 0x0000u,
    0x0bccu, 0x026au, 0x028eu, 0x02bfu, 0x04dfu, 0x08ffu, 0x0cffu, 0x0fffu, 0x0002u, 0x0426u, 0x0090u, 0x02f2u, 0x02ddu, 0x02ffu, 0x02ffu, 0x0000u,
    0x0deeu, 0x048cu, 0x04afu, 0x04dfu, 0x06ffu, 0x0affu, 0x0effu, 0x0fffu, 0x0024u, 0x0648u, 0x0090u, 0x04f4u, 0x04ffu, 0x04ffu, 0x04ffu, 0x0000u,
    0x0fffu, 0x06aeu, 0x06cfu, 0x06ffu, 0x08ffu, 0x0cffu, 0x0fffu, 0x0fffu, 0x0246u, 0x086au, 0x0090u, 0x06f6u, 0x06ffu, 0x06ffu, 0x08ffu, 0x0000u,
    0x0fffu, 0x08cfu, 0x08efu, 0x08ffu, 0x0affu, 0x0effu, 0x0fffu, 0x0fffu, 0x0468u, 0x0a8cu, 0x0090u, 0x08f8u, 0x08ffu, 0x08ffu, 0x0affu, 0x0000u,
    0x0fffu, 0x0aefu, 0x0affu, 0x0bffu, 0x0cffu, 0x0fffu, 0x0fffu, 0x0fffu, 0x068au, 0x0caeu, 0x0090u, 0x0afau, 0x0affu, 0x0affu, 0x0cffu, 0x0000u,
};


static uint16_t r16(const uint8_t *p) { return (uint16_t)(((unsigned)p[0] << 8) | p[1]); }
static uint32_t r32(const uint8_t *p) { return ((uint32_t)r16(p) << 16) | r16(p + 2); }
static void w16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void w32(uint8_t *p, uint32_t v) { w16(p, (uint16_t)(v >> 16)); w16(p + 2, (uint16_t)v); }
static int block(wf_r18_mode_select_t *s, uint32_t dependency) {
    s->blocked_dependency = dependency;
    return WF_R18_MODE_BLOCKED;
}
static int input(wf_r18_mode_select_t *s, wf_r18_mode_select_context_t *c) {
    wf_actor_raw_t actor;
    wf_r18_frontend_input_context_t ctx;
    uint32_t rng, port;
    uint16_t sample = 0xffffu;
    uint8_t *ram = c->ram_1c0000;
    if (s->actor_offset > WF_R18_MODE_RAM_BYTES - WF_ACTOR_BYTES) return block(s, 0x64eau);
    memcpy(&actor, ram + s->actor_offset, sizeof actor);
    port = wf_be32(&actor, 0x8au);
    if ((ram[0x15e] & 0x10u) == 0u) {
        if (port < 0x140020u || port > 0x140026u || (port & 1u)) return block(s, 0x64eau);
        sample = c->input_words[(port - 0x140020u) / 2u];
    }
    rng = r32(ram + 0x5c);
    ctx.rng_state_1c005c = &rng; ctx.frame_1c0080 = r32(ram + 0x80); ctx.flags_1c015e = ram[0x15e];
    if (wf_r18_frontend_input_64e0(&actor, sample, &ctx, c->regs)) return block(s, 0x64e0u);
    memcpy(ram + s->actor_offset, &actor, sizeof actor);
    w32(ram + 0x5c, rng);
    return WF_R18_MODE_MORE;
}

static void emit(wf_r18_mode_select_context_t *c, uint32_t owner,
                 uint32_t source, uint32_t destination,
                 const uint16_t *words, unsigned count, unsigned descriptor) {
    wf_r18_mode_effect_t e = {owner, source, destination, words, count, descriptor};
    c->effect(c->user, &e, c->ram_1c0000, c->regs);
}

static int advance_mode(wf_r18_mode_select_t *s, wf_r18_mode_select_context_t *c) {
    uint16_t remaining = (uint16_t)((uint16_t)c->regs->d[0] - 1u);
    s->actor_offset = (uint16_t)(s->actor_offset + 0x10cu);
    wf_r17_m68k_move_w(c->regs, 0u, remaining);
    s->pc = remaining == 0xffffu ? 0x1e92u : 0x534au;
    s->assignment_credit = 0u;
    return WF_R18_MODE_MORE;
}

static int mode_palette(wf_r18_mode_select_t *s, wf_r18_mode_select_context_t *c) {
    uint8_t *actor = c->ram_1c0000 + s->actor_offset;
    int changed = s->pc == 0x53cau || s->pc == 0x5436u;
    int royal = s->pc == 0x539au || s->pc == 0x53cau;
    unsigned offset = 0u;
    if (!changed) {
        uint16_t timer = (uint16_t)(r16(actor + 0xau) + 1u);
        w16(actor + 0xau, timer);
        if (timer != 5u) return WF_R18_MODE_MORE;
        w16(actor + 0xau, 0u);
        uint16_t frame = r16(actor + 6u);
        if (frame >= 10u) return block(s, 0x54a0u);
        wf_r17_m68k_move_w(c->regs, 0u, frame);
        offset = (unsigned)pulse_frames[frame] * 32u;
        w16(actor + 6u, (uint16_t)(frame == 9u ? 0u : frame + 1u));
    }
    wf_r17_m68k_moveq(c->regs, 1u, 0);
    wf_r17_m68k_move_w(c->regs, 1u, (uint16_t)offset);
    wf_r17_m68k_moveq(c->regs, 2u, 14);
    wf_r17_m68k_moveq(c->regs, 4u, 0);
    emit(c, changed ? 0x54cau : 0x5484u,
        (royal ? 0x5518u : 0x55d8u) + offset,
        royal ? 0x188602u : 0x188482u,
        (royal ? royal_palette : tag_palette) + offset / 2u, 15u, 0u);
    wf_r17_m68k_move_w(c->regs, 1u, (uint16_t)(offset + 30u));
    wf_r17_m68k_move_w(c->regs, 2u, 0xffffu);
    wf_r17_m68k_move_w(c->regs, 4u, 30u);
    if (changed) { w16(actor + 6u, 0u); w16(actor + 0xau, 0u); }
    return WF_R18_MODE_MORE;
}

static void panel(wf_r18_mode_select_context_t *c, unsigned descriptor, int clear) {
    static const uint32_t sources[3] = {0x5230u, 0x5256u, 0x526au};
    wf_r17_m68k_moveq(c->regs, 0u, (int8_t)descriptor);
    emit(c, clear ? 0x527eu : 0x51d2u, sources[descriptor], 0xc1770u, 0, 0u, descriptor);
}

static int assignment_finish(wf_r18_mode_select_t *s, wf_r18_mode_select_context_t *c) {
    uint8_t *ram = c->ram_1c0000;
    panel(c, 0u, 1); panel(c, 1u, 1); panel(c, 2u, 1);
    if (ram[0x160] & 0x40u) {
        w16(ram + 0x6bc, 0u);
        w32(ram + 0x746, 0u);
        w16(ram + 0x7c8, 0x8000u);
        w32(ram + 0x852, 0x140022u);
    }
    s->pc = 0x57e0u;
    return WF_R18_MODE_ROSTER;
}

void wf_r18_mode_select_init(wf_r18_mode_select_t *state) {
    if (!state) return;
    state->pc = 0x533au;
    state->blocked_dependency = 0u;
    state->actor_offset = 0x5b0u;
    state->assignment_credit = 0u;
}

int wf_r18_mode_select_step(wf_r18_mode_select_t *state,
    wf_r18_mode_select_context_t *context) {
    uint8_t *ram;
    uint16_t counter;
    if (!state || !context || !context->ram_1c0000 ||
        context->ram_bytes < WF_R18_MODE_RAM_BYTES || !context->regs ||
        !context->regs->valid || !context->effect) return WF_R18_MODE_BAD_CONTEXT;
    ram = context->ram_1c0000;
    if (state->pc == 0x57e0u) return WF_R18_MODE_ROSTER;
    if (state->pc == 0x1e92u) {
        emit(context, 0x1e92u, 0u, 0u, 0, 0u, 0u);
        state->actor_offset = 0x5b0u;
        wf_r17_m68k_moveq(context->regs, state->assignment_credit ? 3u : 0u,
                          state->assignment_credit ? 1 : 3);
        state->pc = state->assignment_credit ? 0x56e2u : 0x534au;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x533au) {
        w16(ram + 0x15c, 0u);
        wf_r17_m68k_moveq(context->regs, 0u, 3);
        state->actor_offset = 0x5b0u;
        state->pc = 0x534au;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x5698u) {
        uint16_t config = (uint16_t)(r16(ram + 0x66) & 0x800u);
        wf_r17_m68k_move_w(context->regs, 1u, config);
        if (config != 0x800u || (ram[0x6bc] & 0x80u) == 0u) {
            state->pc = 0x57e0u;
            return WF_R18_MODE_ROSTER;
        }
        if (ram[0x161] & 1u) {
            ram[0x160] |= 0x80u;
            state->pc = 0x57e0u;
            return WF_R18_MODE_ROSTER;
        }
        panel(context, 0u, 0); panel(context, 1u, 0);
        ram[0x160] |= 0x40u;
        wf_r17_m68k_moveq(context->regs, 3u, 1);
        state->actor_offset = 0x5b0u;
        state->pc = 0x56e2u;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x56e2u) {
        counter = (uint16_t)(r16(ram + 0x15c) + 1u);
        w16(ram + 0x15c, counter);
        if (counter < 0x200u && input(state, context) != 0) return WF_R18_MODE_BLOCKED;
        if (counter >= 0x200u || ram[state->actor_offset + 0xa3u] != 0u)
            return assignment_finish(state, context);
        uint16_t direction = r16(ram + state->actor_offset + 0xa8u);
        wf_r17_m68k_move_w(context->regs, 0u, direction);
        if (direction >= 16u) return block(state, 0x5716u);
        int upper = (ram[0x160] & 0x40u) != 0u;
        int lower = upper ? assignment_upper[direction] != 0u : assignment_lower[direction] == 0u;
        if (lower) {
            panel(context, 1u, 1); panel(context, 2u, 0);
            ram[0x160] = (uint8_t)((ram[0x160] | 0x80u) & 0xbfu);
        } else {
            panel(context, 2u, 1); panel(context, 1u, 0);
            ram[0x160] = (uint8_t)((ram[0x160] | 0x40u) & 0x7fu);
        }
        state->actor_offset = (uint16_t)(state->actor_offset + 0x10cu);
        uint16_t remaining = (uint16_t)((uint16_t)context->regs->d[3] - 1u);
        wf_r17_m68k_move_w(context->regs, 3u, remaining);
        state->pc = remaining == 0xffffu ? 0x5778u : 0x56e2u;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x5778u) {
        if ((context->input_words[3] & 0x0400u) == 0u) return WF_R18_MODE_WAIT;
        state->pc = 0x5782u;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x5782u) {
        if (context->input_words[3] & 0x0400u) return WF_R18_MODE_WAIT;
        state->assignment_credit = 1u;
        state->pc = 0x1e92u;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x534au) {
        if (state->actor_offset > WF_R18_MODE_RAM_BYTES - WF_ACTOR_BYTES) return block(state, 0x534au);
        if ((ram[state->actor_offset] & 0x80u) == 0u) return advance_mode(state, context);
        w32(ram + 0x80, r32(ram + 0x80) + 1u);
        w16(ram + 0x6e, 0u);
        counter = (uint16_t)(r16(ram + 0x15c) + 1u);
        w16(ram + 0x15c, counter);
        if (counter < 0x200u && input(state, context) != 0) return WF_R18_MODE_BLOCKED;
        if (counter >= 0x200u || ram[state->actor_offset + 0xa3u] != 0u) {
            w16(ram + 0x15c, 0u); w16(ram + 0x15e, 0u);
            state->pc = 0x5698u;
            return WF_R18_MODE_MORE;
        }
        uint16_t direction = r16(ram + state->actor_offset + 0xa8u);
        wf_r17_m68k_move_w(context->regs, 0u, direction);
        if (direction >= 16u) return block(state, 0x5394u);
        if (ram[0x161] & 1u) {
            if (mode_right[direction]) { ram[0x161] &= 0xfeu; state->pc = 0x5436u; }
            else state->pc = 0x5406u;
        } else {
            if (mode_left[direction]) { ram[0x161] |= 1u; state->pc = 0x53cau; }
            else state->pc = 0x539au;
        }
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x539au || state->pc == 0x53cau ||
        state->pc == 0x5406u || state->pc == 0x5436u) {
        if ((context->input_words[3] & 0x0400u) == 0u) return WF_R18_MODE_WAIT;
        uint32_t fall = state->pc == 0x539au ? 0x53b4u :
            state->pc == 0x53cau ? 0x53ecu : state->pc == 0x5406u ? 0x5420u : 0x5456u;
        if (mode_palette(state, context) != 0) return WF_R18_MODE_BLOCKED;
        state->pc = fall;
        return WF_R18_MODE_MORE;
    }
    if (state->pc == 0x53b4u || state->pc == 0x53ecu ||
        state->pc == 0x5420u || state->pc == 0x5456u) {
        if (context->input_words[3] & 0x0400u) return WF_R18_MODE_WAIT;
        return advance_mode(state, context);
    }
    return block(state, state->pc);
}
