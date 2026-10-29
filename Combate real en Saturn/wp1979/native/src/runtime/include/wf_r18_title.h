#ifndef WF_R18_TITLE_H
#define WF_R18_TITLE_H
#include <stddef.h>
#include "wf_r17_m68k_shadow.h"

#define WF_R18_TITLE_RAM_BYTES 0x4000u
enum { WF_R18_TITLE_MORE=0, WF_R18_TITLE_WAIT=1, WF_R18_TITLE_ATTRACT=2,
       WF_R18_TITLE_BLOCKED=-1, WF_R18_TITLE_BAD_CONTEXT=-2 };
enum { WF_R18_TITLE_CALL=0, WF_R18_TITLE_SCROLL16=1, WF_R18_TITLE_SYNC=2 };
typedef struct wf_r18_title_event {
    uint32_t kind, owner_rom, address, value;
} wf_r18_title_event_t;
typedef int (*wf_r18_title_effect_fn)(void *user, const wf_r18_title_event_t *event,
    uint8_t *ram_1c0000, wf_r17_m68k_shadow_t *regs, uint32_t a[7]);
typedef struct wf_r18_title_context {
    uint8_t *ram_1c0000;
    size_t ram_bytes;
    wf_r17_m68k_shadow_t *regs;
    uint32_t *a; /* Exact A0..A6; A7/68000 temporary stack not represented. */
    wf_r18_title_effect_fn effect;
    void *user;
} wf_r18_title_context_t;
typedef struct wf_r18_title { uint32_t pc, blocked_dependency; } wf_r18_title_t;

/* Original controller $81BA..$848E, plus caller $962..$974 handoff to $AC0.
 * The caller owns title predecessor setup $91C..$95E and global credited
 * START dispatch; START does not become a local skip or confirm here.
 *
 * Compact objects remain at $1C1258 with stride$2A, not wf_actor_raw_t stride.
 * CALL events invoke the exact original dependency with live D/A/RAM:
 * $2A06,$26E66,$1F9E,$2AEA,$27B8,$2836,$2503C,$9ACA,$B312,$2052.
 * Graphics/palette/text effects must preserve or return their original
 * register/RAM effects. $B312 is an external native-owner dependency.
 * $A44E is implemented locally, including full D/A preservation.
 *
 * SYNC events occur at the three original waits, with address$1C006F and
 * value2. The callback may supply actual VBlank/IRQ side effects, including
 * credit HUD and setting ACK bit1. If bit1 stays clear, step returns WAIT.
 * No frame counter, credit handler or elapsed time is invented here.
 * SCROLL16 events commit words to raw board addresses$100000/$100004.
 *
 * MORE means continue until a wait or external boundary. ATTRACT stops at
 * original PC$AC0 after caller bit4-clear and command$3101, without a fade
 * or a synthetic title loop. Exact RAM/records/registers survive this exit.
 * Missing/failed dependencies block; callbacks are never silently skipped.
 */
void wf_r18_title_init(wf_r18_title_t *state);
void wf_r18_title_portrait_divisor_set(unsigned divisor);
int wf_r18_title_step(wf_r18_title_t *state, wf_r18_title_context_t *context);
#endif
