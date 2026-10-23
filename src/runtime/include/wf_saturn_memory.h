#ifndef WF_SATURN_MEMORY_H
#define WF_SATURN_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"

/* Borrowed LWRAM ranges; only CPU-owned data lives here. SCU-DMA staging
 * remains in HWRAM. The 208KB ROM cache holds172 hot pages plus36 spare. */
#define WF_SATURN_LWRAM_ROM_CACHE          0x00220000u
#define WF_SATURN_LWRAM_ROM_BYTES          0x00034000u
#define WF_SATURN_LWRAM_VIDEO_WRITES       0x00254000u
#define WF_SATURN_LWRAM_VIDEO_WRITE_BYTES  0x0000c000u
#define WF_SATURN_LWRAM_TILE_CACHE         0x00260000u
#define WF_SATURN_LWRAM_TILE_BYTES         0x000a0000u

/* WP32 baseline ownership. Offsets are relative to VDP1 VRAM. */
#define WF_SATURN_VDP1_VRAM_BYTES            0x80000u
#define WF_SATURN_VDP1_CMD_RESERVED_BYTES    0x00020u
#define WF_SATURN_VDP1_CMD_OFFSET            0x00020u
#define WF_SATURN_VDP1_CMD_BYTES             0x08000u
#define WF_SATURN_VDP1_CMD_COUNT             1024u
#define WF_SATURN_VDP1_DYNAMIC_OFFSET        0x4B580u
#define WF_SATURN_VDP1_DYNAMIC_BYTES         0x10000u
#define WF_SATURN_VDP1_DYNAMIC_END           (WF_SATURN_VDP1_DYNAMIC_OFFSET + WF_SATURN_VDP1_DYNAMIC_BYTES)
#define WF_SATURN_VDP1_STREAM_TILE_BYTES     128u
#define WF_SATURN_VDP1_STREAM_TILE_CAPACITY  (WF_SATURN_VDP1_DYNAMIC_BYTES / WF_SATURN_VDP1_STREAM_TILE_BYTES)

/* Four independent arcade palette domains in CRAM mode 2 (entry indices). */
#define WF_SATURN_CRAM_MODE                  2u
#define WF_SATURN_CRAM_TEXT_FIRST            0x200u
#define WF_SATURN_CRAM_SPRITE_FIRST          0x100u
#define WF_SATURN_CRAM_FG_FIRST              0x000u
#define WF_SATURN_CRAM_BG_FIRST              0x300u
#define WF_SATURN_CRAM_WINDOW_ENTRIES        0x100u
#define WF_SATURN_CRAM_BANK_COLORS           16u
#define WF_SATURN_CRAM_SPRITE_BANKS          16u
#define WF_SATURN_CRAM_RGB888_BYTES          4u
#define WF_SATURN_CRAM_BANK_BYTES_MODE2      (WF_SATURN_CRAM_BANK_COLORS * WF_SATURN_CRAM_RGB888_BYTES)

/* Native VDP2 ownership: world CPD banks0/2, all names bank1, font bank3.
 * One-word HUD auxiliary-mode1 covers4096 glyphs without flip bits. */
#define WF_SATURN_VDP2_FG_NAMES 0x0000u
#define WF_SATURN_VDP2_BG_NAMES 0x2000u
#define WF_SATURN_VDP2_HUD_NAMES 0x4000u
#define WF_SATURN_VDP2_BACK_COLOR 0x1fffeu
#define WF_SATURN_FONT_BYTES 0x20000u

/* WP26 canonical 320x240 composition. */
#define WF_SATURN_VIEW_W 320u
#define WF_SATURN_VIEW_H 240u

wf_status_t wf_saturn_memory_contract_validate(void);
wf_status_t wf_saturn_stream_tile_offset(uint16_t slot, uint32_t *out_vdp1_offset);
wf_status_t wf_saturn_sprite_cram_entry(uint8_t bank, uint8_t color, uint16_t *out_entry);
void wf_saturn_cram_reset(void);
wf_status_t wf_saturn_cram_write(uint16_t entry,uint16_t xbgr444);
wf_status_t wf_saturn_cram_write_bank(uint16_t first,const uint8_t *big_endian_16_colors);
extern uint32_t wf_saturn_cram_writes;
uint32_t wf_saturn_xbgr444_to_rgb888_raw(uint16_t xbgr444);
#endif
