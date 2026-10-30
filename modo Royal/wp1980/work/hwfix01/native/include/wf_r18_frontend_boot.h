#ifndef WF_R18_FRONTEND_BOOT_H
#define WF_R18_FRONTEND_BOOT_H
#include "wf_r18_title.h"

/* Same RAM16KiB/D0-D7/A0-A6/event ABI as the title controller. Event kind1
 * is a raw WORD board write here, including command port$14000C. */
typedef wf_r18_title_context_t wf_r18_frontend_boot_context_t;
typedef struct wf_r18_frontend_boot { uint32_t pc,blocked_dependency,hold_return; } wf_r18_frontend_boot_t;
enum { WF_R18_BOOT_MORE=0, WF_R18_BOOT_WAIT=1, WF_R18_BOOT_TITLE=2,
       WF_R18_BOOT_BLOCKED=-1, WF_R18_BOOT_BAD_CONTEXT=-2 };

/* Re-entry $6FC is distinct from hardware reset$654. This owner preserves
 * credits, coinage, packed config, rankings and RNG; clears only the original
 * $1F52 domain and explicit globals, then follows FBI/legal/Technos/title.
 * CALL callbacks own exact graphics/palette/fade/text/command dependencies.
 * $1E6C hold128, $1F38/$1F52 clears and Technos $A0AE wrapper are native.
 * SYNC callbacks at$1E7C supply real VBlank/IRQ ACK in$1C006F.bit1; absent
 * ACK returns WAIT. Global credited START diversion belongs to the IRQ
 * driver, not a local plate-skip. Stop TITLE before executing$81BA and pass
 * the same RAM/registers directly to wf_r18_title without reinitialization.
 * $654 platform reset/SR/USP/DIP/coinage/factory records are outside this API.
 */
void wf_r18_frontend_boot_init(wf_r18_frontend_boot_t *state);
int wf_r18_frontend_boot_step(wf_r18_frontend_boot_t *state,wf_r18_frontend_boot_context_t *context);

/* Original exact clear ranges, preserving D0-D7/A0-A6 and all other RAM. */
int wf_r18_frontend_clear_1f38(wf_r18_frontend_boot_context_t *context);
int wf_r18_frontend_clear_1f52(wf_r18_frontend_boot_context_t *context);

/* $055A for selectors0 and4 only, selected by live D0 as in ROM. Return1
 * means carry set (credit consumed),0 means carry clear; negative means an
 * unsupported selector/invalid context. No free-play/coinage policy added. */
int wf_r18_frontend_credit_055a(wf_r18_frontend_boot_context_t *context);

/* Exact$978 through returns$9E4/$AB0/$AB6. ports[4] are words at$140020..26.
 * Return carries as above; accepted_mask identifies physical ports assigned.
 * Generic scans four ports and accepts first success for one unit. Dedicated
 * config&$0800 tries P1 for one unit, then P2 for two units activating P1/P2.
 * No local $76/$7C gating is added: IRQ caller checks those globals first.
 * Generic success clears WORD$76; dedicated success preserves it until the
 * original IRQ caller$8BE clears it. $1F38 preserves predecessor mode$160.
 * This does not implement caller$8AE..$8F2 stack/SR reset and scene clears,
 * which must precede its JMP$52BE. CCR except returned carry is not modeled.
 */
int wf_r18_frontend_start_0978(wf_r18_frontend_boot_context_t *context,
    const uint16_t ports[4],unsigned *accepted_mask);
#endif
