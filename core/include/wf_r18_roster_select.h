#ifndef WF_R18_ROSTER_SELECT_H
#define WF_R18_ROSTER_SELECT_H
#include <stddef.h>
#include "wf_r18_frontend_input.h"

#define WF_R18_ROSTER_RAM_BYTES 0x2000u
enum { WF_R18_ROSTER_OK=0, WF_R18_ROSTER_BLOCKED=-1,
       WF_R18_ROSTER_BAD_CONTEXT=-2, WF_R18_ROSTER_WAIT=1,
       WF_R18_ROSTER_LIVE=2, WF_R18_ROSTER_COMBAT=3 };
typedef int (*wf_r18_roster_read_rom_fn)(void *user,uint32_t address,
    void *destination,size_t bytes);
typedef int (*wf_r18_roster_effect_fn)(void *user,uint32_t owner_rom,
    uint32_t a[7],uint8_t *ram_1c0000,wf_r17_m68k_shadow_t *regs);
typedef int (*wf_r18_roster_palette_fn)(void *user,uint32_t store_rom,
    uint32_t original_address,uint16_t value);
typedef struct wf_r18_roster_select_context {
    uint8_t *ram_1c0000;
    size_t ram_bytes;
    wf_r17_m68k_shadow_t *regs;
    uint32_t a[7]; /* Live A0..A6, for external-owner arguments; no host pointers. */
    uint16_t input_words[4];
    wf_r18_roster_read_rom_fn read_rom;
    wf_r18_roster_effect_fn effect;
    void *user;
    uint32_t blocked_dependency;
    /* $5EEA/$5EF2 original WORD stores to palette RAM. Return0 on success;
     * transport only: the instruction changes no D/A register. */
    wf_r18_roster_palette_fn write_palette;
} wf_r18_roster_select_context_t;

/* Exact original subroutine $658C. Changes WORD $1C17E6 by +/-8, calls
 * external $26A42 using the live register context, then clears BYTE
 * $1C015E.bit3 exactly at target $0140/$0210. Caller retains frame timing.
 * The effect callback is mandatory and must implement the original callee's
 * RAM/register semantics; this owner does not guess world-scroll behavior.
 */
int wf_r18_roster_scroll_658c(wf_r18_roster_select_context_t *ctx);

/* Complete original $600E..$60A0 page-toggle reconciliation subroutine.
 * Uses exact ROM lists $62AA/$62B0/$62B6, actor order and occupancy writes.
 * No physical input, RNG, render callback or synthetic fallback is used.
 */
int wf_r18_roster_reconcile_600e(wf_r18_roster_select_context_t *ctx);

/* Exact navigation block $5B3E..$5CCE, stopping BEFORE position/render owner
 * $5CD0. actor_offset is A0-$1C0000 and must name an existing $10C-byte
 * record. Normalizer $64E0 must already have supplied +$A8. Updates previous
 * and current slot, reservations and page request flags; reads original
 * $62BC/$6314/$6382 matrices and $63F0 fallback pointers directly from ROM.
 * Does not confirm a player or select CPU partners/opponents.
 * All APIs preserve byte/word D-register writes. Invalid context rejects
 * without mutation; failed external/ROM dependencies return BLOCKED at their
 * exact address, retaining the caller-visible state reached at that boundary.
 */
int wf_r18_roster_navigate_5b3e(wf_r18_roster_select_context_t *ctx,
    unsigned actor_offset);

typedef struct wf_r18_roster_select {uint32_t pc;} wf_r18_roster_select_t;
/* Original SELECT PLAYERS $57E0 through JMP $AC0, split at original owner
 * boundaries. Initialize the cursor only; preserve raw RAM and D/A transferred
 * from GAME SELECT. An existing 16KiB bus may be passed directly (8KiB minimum).
 * No heap, static machine, independent RNG seed or frame timing is created.
 *
 * Each step executes one original block/actor. Return OK means step again;
 * WAIT means external hardware has not met the original gate. Supply $140020,
 * $140022,$140024,$140026 in input_words; init polls $140026 high-byte bit2
 * (WORD bit10). Live $5D1E waits until external VBlank sets RAM $006F.bit1.
 * Original rendering $2836 and $006F.bit0 precede that wait. The driver must
 * resume steps within the same frame until this gate, then service VBlank;
 * calling one step per frame would introduce invented delays.
 *
 * effect is mandatory for original $2AEA,$26E66,$1F9E,$2A06,$21E6,$26A42,
 * $247C,$27B8,$2836,$51D2,$527E. It must perform each callee's RAM and live
 * D/A semantics synchronously, or return nonzero to fail closed. $21E6 sees
 * D0.W=$0010 at init and $0020 at exit; its original DBRA owner determines
 * the wait (17/33 ticks), not a synthetic selector countdown. write_palette
 * transports the exact original $5EEA/$5EF2 writes in confirmation flashing.
 *
 * Timeout $1C015C >=1024 enters original automatic $64E0 (terminal counter64,
 * occupied-choice retry resets it to32). Confirmed actors count +$58 to76,
 * restoring +$AE on tick75. TAG slot8 can confirm its partner immediately.
 * Original joins $60A2, $055A mode0 credit payment, assignment $7304 and
 * activation $5F28 execute internally with exact ROM tables and partial D
 * writes. No cancel, START-confirm or opponent roster policy is added.
 *
 * LIVE means init reached $5990 BEFORE the live loop; continue calling step.
 * COMBAT means selection reached $AC0 BEFORE the next combat/bootstrap owner,
 * retaining selected identities/history, raw actor pointers and live D/A.
 * CPU team generation belongs to that later owner. Missing ROM/effects or
 * unsupported entry PCs block with blocked_dependency set to the exact PC/
 * bus address, retaining the state reached there. Such a failed synchronous
 * call is a fatal dependency boundary, not a replayable transaction.
 */
void wf_r18_roster_select_init(wf_r18_roster_select_t *state);
int wf_r18_roster_select_step(wf_r18_roster_select_t *state,
    wf_r18_roster_select_context_t *ctx);
#endif
