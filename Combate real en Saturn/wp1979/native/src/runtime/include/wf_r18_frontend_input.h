#ifndef WF_R18_FRONTEND_INPUT_H
#define WF_R18_FRONTEND_INPUT_H
#include "wf_actor.h"
#include "wf_r17_m68k_shadow.h"

typedef struct wf_r18_frontend_input_context {
    uint32_t *rng_state_1c005c; /* Shared live RNG, updated only in automatic path. */
    uint32_t frame_1c0080;     /* Read by original $21B4; not incremented here. */
    uint8_t flags_1c015e;      /* BYTE at $1C015E: bit4 selects automatic path. */
} wf_r18_frontend_input_context_t;

/* Original frontend $64E0..$6582, including timeout automatic input.
 *
 * Resolve actor +$8A to its physical port, then supply that port's active-low
 * WORD as active_low. The routine consumes only its low byte: RIGHT/LEFT/
 * UP/DOWN are bits0/1/2/3, action1/2 are bits4/5. A frontend Saturn adapter
 * with that mapping can supply (uint16_t)(0xFF00u | byte). The existing
 * gameplay controls() mapping has DOWN bit2 / UP bit3: swap its bits2/3
 * before using that byte here, or supply a separate frontend adapter.
 * START bit7 and action3 bit6 do not confirm here. Do not decode through
 * gameplay $514E.
 *
 * actor is the existing $10C-byte selector record. Human input writes +$A8
 * WORD (direction edge), +$91 BYTE (previous held direction), +$A2 WORD
 * (fresh action1/2) and +$A6 WORD (held action1/2). The direction nibble is
 * preserved exactly, including contradictory directions. Equal held direction
 * suppresses +$A9 until it changes or is released; this is not key repeat.
 *
 * Automatic input instead updates +$5E WORD (terminal counter), +$56 WORD
 * (random movement timer), +$A9 BYTE when it moves, or +$A3 BYTE on terminal
 * confirm. Other input bytes and +$91 remain unchanged on that path. At 64
 * actor calls +$5E becomes32 and +$A3 becomes1; the caller owns timeout entry.
 *
 * Mode/roster consume +$A3 first, then direction WORD +$A8. This owner chooses
 * neither mode nor wrestler. It reads no mode/occupancy/credit global and
 * does not advance frame/time, scan active actors, or poll the controller.
 *
 * regs is the exact live D0..D7 shadow at the call: human input changes D1/D2;
 * automatic movement mixes D1..D7 through the existing $21B4 implementation
 * and changes D0/D1, preserving partial-register widths. Keep the updated
 * shadow and RNG for subsequent calls and the eventual combat handoff.
 * A3, CCR and temporary stack writes are outside this C API.
 *
 * Returns0 on success, -1 without mutation if any argument, the RNG pointer,
 * or a valid register shadow is absent. No reset/zero seed is supplied here.
 */
int wf_r18_frontend_input_64e0(wf_actor_raw_t *actor, uint16_t active_low,
    const wf_r18_frontend_input_context_t *ctx, wf_r17_m68k_shadow_t *regs);
#endif
