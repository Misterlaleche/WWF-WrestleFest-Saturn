#ifndef WF_R18_FRONTEND_EFFECT_H
#define WF_R18_FRONTEND_EFFECT_H
#include "wf_runtime.h"
typedef enum wf_r18_frontend_edge {
 WF_R18_FRONTEND_EFFECT_DONE=0,WF_R18_FRONTEND_WAIT_HIGH=1,
 WF_R18_FRONTEND_WAIT_LOW=2,WF_R18_FRONTEND_EFFECT_ERROR=3
} wf_r18_frontend_edge_t;
typedef wf_status_t (*wf_r18_frontend_palette_io_fn)(void *,unsigned,
                                                  uint16_t colors[256]);
typedef struct wf_r18_frontend_effect {
 uint8_t *ram; /* 16KB, original $1C0000..$1C3FFF incl. palette scratch. */
 wf_r17_m68k_shadow_t *regs;
 wf_r18_frontend_palette_io_fn read_palette,write_palette;
 void *user;
 uint32_t saved_d[8];
 uint16_t saved_flags;
 uint8_t owner,phase,step,active;
} wf_r18_frontend_effect_t;
/* Original $26772 snapshot/clear, $26844 reveal, or $264E2 fade-out.
 * The caller feeds the actual VBlank-status level; requested HIGH/LOW pairs
 * preserve original entry alignment and three complete periods per phase.
 * Palette IO order: text,sprites,BG,FG. Colors use host-order $0RGB words.
 * $26844 consumes the snapshot previously prepared at RAM+$1CD4; it does
 * not capture current colors. Zero-initialize the cursor before first begin.
 * No title fade-in policy, clock increment, input poll or RNG is supplied. */
wf_status_t wf_r18_frontend_effect_begin(wf_r18_frontend_effect_t *,uint32_t);
wf_r18_frontend_edge_t wf_r18_frontend_effect_resume(wf_r18_frontend_effect_t *,
                                                   unsigned high);
#endif
