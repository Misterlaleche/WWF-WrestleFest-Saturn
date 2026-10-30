#include "wf_r18_frontend_input.h"

int wf_r18_frontend_input_64e0(wf_actor_raw_t *actor, uint16_t active_low,
    const wf_r18_frontend_input_context_t *ctx, wf_r17_m68k_shadow_t *regs) {
    uint16_t active, held, fresh;
    uint8_t direction;
    if (!actor || !ctx || !regs || !regs->valid || !ctx->rng_state_1c005c)
        return -1;
    if ((ctx->flags_1c015e & 0x10u) != 0u) {
        /* $6534: unsigned word counters wrap before the original compares. */
        uint16_t terminal = (uint16_t)(wf_be16(actor, 0x5eu) + 1u);
        uint16_t timer;
        wf_set_be16(actor, 0x5eu, terminal);
        if (terminal >= 0x40u) {
            wf_set_be16(actor, 0x5eu, 0x20u);
            wf_set_u8(actor, 0xa3u, 1u);
            return 0;
        }
        timer = (uint16_t)(wf_be16(actor, 0x56u) + 1u);
        wf_set_be16(actor, 0x56u, timer);
        wf_r17_rng21b4_live(ctx->rng_state_1c005c, ctx->frame_1c0080, regs);
        wf_r17_m68k_move_w(regs, 0u, (uint16_t)((regs->d[0] & 0x0fu) | 4u));
        if ((uint16_t)regs->d[0] >= timer) return 0;

        wf_set_be16(actor, 0x56u, 0u);
        wf_r17_rng21b4_live(ctx->rng_state_1c005c, ctx->frame_1c0080, regs);
        wf_r17_m68k_move_w(regs, 0u, (uint16_t)(regs->d[0] & 7u));
        /* $6584: 1,2,4,8,1,2,4,8. MOVE.B preserves D1's high 24 bits. */
        direction = (uint8_t)(1u << (regs->d[0] & 3u));
        wf_r17_m68k_move_b(regs, 1u, direction);
        wf_set_u8(actor, 0xa9u, direction);
        return 0;
    }

    active = (uint16_t)((uint16_t)~active_low & 0xffu);
    wf_r17_m68k_move_w(regs, 1u, active);
    wf_set_be16(actor, 0xa8u, active);
    direction = (uint8_t)(active & 0x0fu);
    wf_set_u8(actor, 0xa9u, direction);
    wf_r17_m68k_move_b(regs, 2u, wf_u8(actor, 0x91u));
    if (wf_u8(actor, 0x91u) == direction)
        wf_set_u8(actor, 0xa9u, 0u);
    else
        wf_set_u8(actor, 0x91u, direction);

    held = (uint16_t)((active >> 4) & 3u);
    wf_r17_m68k_move_b(regs, 1u, (uint8_t)held);
    fresh = (uint16_t)(held & (uint16_t)~wf_be16(actor, 0xa6u));
    wf_set_be16(actor, 0xa2u, fresh);
    wf_set_be16(actor, 0xa6u, held);
    return 0;
}
