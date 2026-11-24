#ifndef WF_R18_VIDEO_H
#define WF_R18_VIDEO_H
#include "wf_r17_post7a00.h"
typedef wf_status_t (*wf_r18_rom_read_fn)(void *,uint32_t,void *,size_t);
typedef wf_status_t (*wf_r18_ram_read_fn)(wf_runtime_t *,uint32_t,uint16_t *,void *);
typedef enum wf_r18_background_op {
 WF_R18_BACKGROUND_READ,WF_R18_BACKGROUND_VALIDATE,WF_R18_BACKGROUND_WRITE
} wf_r18_background_op_t;
typedef int (*wf_r18_background_cell_fn)(unsigned,unsigned,uint8_t *,wf_r18_background_op_t);
typedef enum wf_r18_palette_role {
 WF_R18_PALETTE_TEXT=0,WF_R18_PALETTE_SPRITES=1,
 WF_R18_PALETTE_FG=2,WF_R18_PALETTE_BG=3
} wf_r18_palette_role_t;
typedef struct wf_r18_video {
 wf_r18_rom_read_fn read_rom; wf_r18_ram_read_fn read_ram;void *read_user;
 uint8_t fg0[0x4000],background[0x4000],palettes[0x4000];
 /* $188000 FG, $186000 BG: sixteen banks, 128-byte arcade stride. */
 uint8_t fg_palette[0x800],bg_palette[0x800];
 uint8_t palette_map[256],palette_used[16];
 uint8_t hud_dirty[256]; /* 2048 FG0 cells; sprite buffer starts at C2000. */
 uint32_t failed_address;
 uint16_t palette_dirty[4],palette_committed[4];
 wf_r18_background_cell_fn ring_cell;
} wf_r18_video_t;
/* Enable live board-VRAM writes only after direct_boot built the ring. The
 * cleared bootstrap shadow is not a replacement for the ROM-derived map.
 * The ring supplies resident cells for partial writes and validates complete
 * batches before either the shadow or visible overlays are changed. */
void wf_r18_video_ring_bind(wf_r18_video_t *);
/* Internal helpers require a non-NULL video and a valid palette role.
 * Uncommitted banks are pending even when their initial source is all zero.
 * Acknowledge only banks whose physical upload completed successfully. */
static inline uint16_t wf_r18_video_palette_pending(const wf_r18_video_t *v,wf_r18_palette_role_t role){
 return (uint16_t)(v->palette_dirty[role]|(uint16_t)~v->palette_committed[role]);
}
static inline void wf_r18_video_palette_ack(wf_r18_video_t *v,wf_r18_palette_role_t role,uint16_t mask){
 v->palette_dirty[role]&=(uint16_t)~mask;v->palette_committed[role]|=mask;
}
wf_status_t wf_r18_video_batch(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,void*);
wf_status_t wf_r18_video_commit(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,const wf_r17_fg0_write_t*,size_t,void*);
wf_status_t wf_r18_video_peek(wf_runtime_t*,uint32_t,uint32_t*,void*);
wf_status_t wf_r18_palette_2aea(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_palette_2b58(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_text_2503c(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_number_26122(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_strip_9aca(wf_runtime_t*,uint16_t,uint16_t,uint32_t,void*);
#endif
