#ifndef WF_R18_MODE_SELECT_H
#define WF_R18_MODE_SELECT_H
#include <stddef.h>
#include "wf_r18_frontend_input.h"

#define WF_R18_MODE_RAM_BYTES 0x2000u
enum {
    WF_R18_MODE_MORE = 0, WF_R18_MODE_WAIT = 1, WF_R18_MODE_ROSTER = 2,
    WF_R18_MODE_BLOCKED = -1, WF_R18_MODE_BAD_CONTEXT = -2
};
typedef struct wf_r18_mode_effect {
    uint32_t owner_rom;       /* $5484/$54CA palette, $51D2/$527E panel, $1E92 credits. */
    uint32_t source_rom;
    uint32_t destination;
    const uint16_t *words;    /* Palette: exactly15 host-endian words; otherwise NULL. */
    unsigned count;
    unsigned descriptor;     /* Original $5224 pointer-table index for panel calls. */
} wf_r18_mode_effect_t;
typedef void (*wf_r18_mode_effect_fn)(void *user, const wf_r18_mode_effect_t *effect,
    uint8_t *ram_1c0000, wf_r17_m68k_shadow_t *regs);
typedef struct wf_r18_mode_select_context {
    uint8_t *ram_1c0000;      /* Complete writable $1C0000..$1C1FFF window. */
    size_t ram_bytes;
    wf_r17_m68k_shadow_t *regs;
    uint16_t input_words[4];  /* $140020/$22/$24/$26, active-low controller words. */
    wf_r18_mode_effect_fn effect;
    void *user;
} wf_r18_mode_select_context_t;
typedef struct wf_r18_mode_select {
    uint32_t pc;
    uint32_t blocked_dependency;
    uint16_t actor_offset;    /* A0 minus $1C0000; not restricted to four by inference. */
    uint8_t assignment_credit;
} wf_r18_mode_select_t;

/* The compositor owns $52BE..$5339 setup. Begin this logic at $533A after
 * that setup, preserving the predecessor mode BYTE $1C0161. init changes
 * only the cursor; step mutates the exact RAM records/globals and D0-D7.
 * Each step performs one original actor quantum, one wait edge, one external
 * credit call, or one gate. MORE means call again with current input state;
 * WAIT means the original sync busy-wait still applies. Sync is BYTE
 * $140026.bit2 (input_words[3]&$0400), independent of its low input byte.
 * No video frame duration or fixed four-actor iteration is synthesized.
 *
 * Effects are mandatory dependencies. Palette effects supply exact data and
 * addresses. Panel effects identify original descriptors for $51D2/$527E;
 * these helpers preserve D0-D2. The $1E92 callback owns credit HUD side effects
 * and its live D-register changes. It is not a generic VBlank callback.
 *
 * ROSTER stops before executing $57E0, after the original $5698..$57DF gates
 * and optional 2P TAG assignment. It does not certify Royal setup/gameplay.
 * No START-confirm/cancel path is added. Unknown input pointers or invalid
 * palette pulse indices block at the dependency PC instead of guessing.
 * A3/A4, CCR and the 68000 temporary stack are outside this native contract.
 */
void wf_r18_mode_select_init(wf_r18_mode_select_t *state);
int wf_r18_mode_select_step(wf_r18_mode_select_t *state,
    wf_r18_mode_select_context_t *context);
#endif
