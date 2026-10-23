#ifndef WF_R18_VIDEO_H
#define WF_R18_VIDEO_H
#include "wf_r17_post7a00.h"
typedef wf_status_t (*wf_r18_rom_read_fn)(void *,uint32_t,void *,size_t);
typedef wf_status_t (*wf_r18_ram_read_fn)(wf_runtime_t *,uint32_t,uint16_t *,void *);
typedef struct wf_r18_video {
 wf_r18_rom_read_fn read_rom; wf_r18_ram_read_fn read_ram;void *read_user;
 uint8_t fg0[0x4000],background[0x4000],palettes[0x4000];
 /* $188000 FG, $186000 BG: sixteen banks, 128-byte arcade stride. */
 uint8_t fg_palette[0x800],bg_palette[0x800];
 uint8_t palette_map[256],palette_used[16];
 uint8_t hud_dirty[256]; /* 2048 FG0 cells; sprite buffer starts at C2000. */
 uint32_t failed_address;
} wf_r18_video_t;
wf_status_t wf_r18_video_batch(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,void*);
wf_status_t wf_r18_video_commit(wf_runtime_t*,const wf_r17_fg0_write_t*,size_t,const wf_r17_fg0_write_t*,size_t,void*);
wf_status_t wf_r18_video_peek(wf_runtime_t*,uint32_t,uint32_t*,void*);
wf_status_t wf_r18_palette_2aea(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_palette_2b58(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_text_2503c(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_number_26122(wf_runtime_t*,uint16_t,void*);
wf_status_t wf_r18_strip_9aca(wf_runtime_t*,uint16_t,uint16_t,uint32_t,void*);
#endif
