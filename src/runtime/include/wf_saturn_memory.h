#ifndef WF_SATURN_MEMORY_H
#define WF_SATURN_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"

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

/* WP26 authoritative CRAM mode-2 logical palette windows (entry indices). */
#define WF_SATURN_CRAM_MODE                  2u
#define WF_SATURN_CRAM_TEXT_FIRST            0x000u
#define WF_SATURN_CRAM_SPRITE_FIRST          0x100u
#define WF_SATURN_CRAM_FG_FIRST              0x200u
#define WF_SATURN_CRAM_BG_FIRST              0x300u
#define WF_SATURN_CRAM_WINDOW_ENTRIES        0x100u
#define WF_SATURN_CRAM_BANK_COLORS           16u
#define WF_SATURN_CRAM_SPRITE_BANKS          16u
#define WF_SATURN_CRAM_RGB888_BYTES          4u
#define WF_SATURN_CRAM_BANK_BYTES_MODE2      (WF_SATURN_CRAM_BANK_COLORS * WF_SATURN_CRAM_RGB888_BYTES)

/* WP26 canonical 320x240 composition. */
#define WF_SATURN_VIEW_W 320u
#define WF_SATURN_VIEW_H 240u

wf_status_t wf_saturn_memory_contract_validate(void);
wf_status_t wf_saturn_stream_tile_offset(uint16_t slot, uint32_t *out_vdp1_offset);
wf_status_t wf_saturn_sprite_cram_entry(uint8_t bank, uint8_t color, uint16_t *out_entry);
uint32_t wf_saturn_xbgr444_to_rgb888_raw(uint16_t xbgr444);
#endif
