#ifndef WF_R18_MATCH_SUPPORT_H
#define WF_R18_MATCH_SUPPORT_H
#include "wf_r18_match_hud.h"
typedef wf_r18_match_hud_t wf_r18_match_support_t;
/* Native $98BA TAG presentation join: nine actor records, exact $993A
 * layout, $99D0 special tiles and $9A78 palette stores. $9ACA remains a
 * mandatory original strip effect through machine.effect; it must preserve
 * all D/A as in ROM and write the exact supplied strip/address. Palette
 * storage can use HUD's optional write_palette callback or raw36KiB buffer.
 * TAG with no active actors inherits A3 for its final strip; callers must
 * supply that original producer state. No fallback position is invented.
 * Native $8EB6 emits $2052 twice with D0.W=$3100 then$3120, preserving D0
 * high16 and other registers according to the required command effect.
 * No fake audio, RNG, input, actor reset or rendering approximation.
 * Failure retains executed prefix; dependency names the unavailable owner
 * or ROM/storage address. CCR/A7 temporary stack are outside prematch ABI.
 */
wf_status_t wf_r18_match_support_call(wf_r18_match_support_t *context,uint32_t owner_rom);
#endif
