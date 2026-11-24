#ifndef WF_R18_MATCH_HUD_H
#define WF_R18_MATCH_HUD_H
#include <stddef.h>
#include "wf_r18_prematch.h"
typedef wf_status_t (*wf_r18_match_hud_read_fn)(void *user,uint32_t address,uint8_t *bytes,size_t count);
typedef wf_status_t (*wf_r18_match_hud_palette_fn)(void *user,uint32_t address,uint16_t word);
typedef struct wf_r18_match_hud {
    wf_r18_prematch_t *machine;
    uint8_t *fg_0c0000;
    size_t fg_bytes;
    uint8_t *palette_180000;
    size_t palette_bytes;
    wf_r18_match_hud_read_fn read_rom;
    void *user;
    wf_r18_match_hud_palette_fn write_palette; /* Optional raw storage instead of palette buffer. */
} wf_r18_match_hud_t;
/* Native original $7506 health/player HUD, $782C portrait for live A0,
 * and $C1EA active/inactive TAG palettes. Exact helpers $75EE/$7680/$76AA/
 * $76E8/$7720/$776C/$77D8/$C23A are local. Data reads use intact ROM tables,
 * not generated UI. FG16KiB/palette36KiB include original padding bytes.
 * Optional write_palette uses the same user pointer and receives exact raw
 * WORD stores. With that callback, palette buffer/size may be NULL/zero.
 * Live RAM16KiB/D/A and original MOVEM preservation are authoritative.
 * No clock/input/RNG, actor reset, tile conversion or synthetic health added.
 * Unknown owner or invalid ROM/buffer/actor address blocks with dependency.
 * On failure the executed prefix remains, as for wf_r18_prematch_t.
 * CCR/68000 temporary stack are outside this machine ABI.
 */
wf_status_t wf_r18_match_hud_call(wf_r18_match_hud_t *context,uint32_t owner_rom);
#endif
