#ifndef WF_R18_TITLE_PALETTE_H
#define WF_R18_TITLE_PALETTE_H
#include "wf_r18_video.h"
/* Exact title producer $B312 (including B362/B3C0/B476) and IRQ consumer
 * $B4B8. RAM is the shared original1C0000 window (at least8192 bytes).
 * Producer uses live D0-D7; both owners preserve A0-A6 at their boundary.
 * IRQ writes text-bank6/7 colors7..15 through the existing video batch API.
 * No clock/input policy or title timing is added by these two owners. */
typedef struct wf_r18_title_palette_context {
 uint8_t *ram;
 wf_r17_m68k_shadow_t *regs;
 wf_r18_rom_read_fn read_rom;
 wf_r17_fg0_batch_fn write;
 wf_runtime_t *runtime;
 void *user;
} wf_r18_title_palette_context_t;
wf_status_t wf_r18_title_palette_b312(wf_r18_title_palette_context_t *);
wf_status_t wf_r18_title_palette_irq_b4b8(wf_r18_title_palette_context_t *);
#endif
