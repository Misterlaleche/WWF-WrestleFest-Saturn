#ifndef WP1980_ROYAL_INIT_H
#define WP1980_ROYAL_INIT_H
#include "wf_r18_runtime_handoff.h"

/* One verified original paid-Royal initial condition, not a frame stream.
 * Sections retain their original byte order. The scratch image may be reused
 * after apply: all live state is imported into runtime/video/world owners. */
enum {
 WP1980_ROYINIT_HEADER=128,
 WP1980_ROYINIT_RAM=128,
 WP1980_ROYINIT_REGS=128+16384,
 WP1980_ROYINIT_TEXT=128+16384+32,
 WP1980_ROYINIT_SPRITES=128+16384+32+8192,
 WP1980_ROYINIT_FGMAP=128+16384+32+8192+8192,
 WP1980_ROYINIT_BGMAP=128+16384+32+8192+8192+4096,
 WP1980_ROYINIT_PALETTE=128+16384+32+8192+8192+4096+4096,
 WP1980_ROYINIT_SCROLL=128+16384+32+8192+8192+4096+4096+65536,
 WP1980_ROYINIT_BYTES=128+16384+32+8192+8192+4096+4096+65536+8
};
typedef struct wp1980_royal_init_info {
 uint32_t source_pc,source_rng;
 uint16_t source_sr,scroll[4];
 uint8_t priority;
} wp1980_royal_init_info_t;
wf_status_t wp1980_royal_init_apply(wf_runtime_t *,wf_r18_video_t *,
 wf_r18_world_ctx_t *,void *image,size_t bytes,wp1980_royal_init_info_t *);
#endif
