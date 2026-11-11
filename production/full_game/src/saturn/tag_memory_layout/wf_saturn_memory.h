#ifndef WF_SATURN_MEMORY_H
#define WF_SATURN_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"

/* Borrowed LWRAM ranges; only CPU-owned data lives here. SCU-DMA staging
 * remains in HWRAM. All original TAG tournament ring packs fit the128KB
 * scene range; the209-page program cache retains all208 measured hot pages. */
#define WF_SATURN_LWRAM_SCENE              0x00201000u
#define WF_SATURN_LWRAM_SCENE_BYTES        0x00020000u
#define WF_SATURN_LWRAM_ROM_CACHE          0x00221000u
#define WF_SATURN_LWRAM_ROM_BYTES          0x00038400u
#define WF_SATURN_LWRAM_VIDEO_WRITES       0x00259400u
#define WF_SATURN_LWRAM_VIDEO_WRITE_BYTES  0x0000c000u
#define WF_SATURN_LWRAM_TILE_CACHE         0x00265400u
#define WF_SATURN_LWRAM_TILE_BYTES         0x0009ac00u
/* Before F8C the combat tile cache is leased to frontend maps and tiles.
 * The16KB original RAM image stays at the top. Release all three leases
 * and reinitialize the full combat cache before loading HOTSPR.BIN. */
#define WF_SATURN_LWRAM_FRONT_SCENE        WF_SATURN_LWRAM_TILE_CACHE
#define WF_SATURN_LWRAM_FRONT_SCENE_BYTES  0x00020000u
#define WF_SATURN_LWRAM_FRONT_TILES        0x00285400u
#define WF_SATURN_LWRAM_FRONT_TILE_BYTES   0x00076c00u
#define WF_SATURN_LWRAM_FRONT_RAM          0x002fc000u
#define WF_SATURN_LWRAM_FRONT_RAM_BYTES    0x00004000u
/* Keep dictionary indices in HWRAM beside the resident LWRAM dictionary.
 * Cold-slot capacity depends on the lossless atlas; the final WP1979
 * 6474-tile atlas leaves53 bounded fallback slots after both traced routes. */
#define WF_SATURN_HWRAM_HOT_INDEX_BYTES    0x0000ca80u

/* WP32 baseline ownership. Offsets are relative to VDP1 VRAM. */
#define WF_SATURN_VDP1_VRAM_BYTES            0x80000u
#define WF_SATURN_VDP1_CMD_RESERVED_BYTES    0x00020u
#define WF_SATURN_VDP1_CMD_OFFSET            0x00020u
#define WF_SATURN_VDP1_CMD_BYTES             0x08100u
#define WF_SATURN_VDP1_CMD_COUNT             1032u
#define WF_SATURN_VDP1_CMD_BANK_COUNT        516u
#define WF_SATURN_VDP1_TEXTURE_BANK_BYTES    0x08000u
/* Immutable crop descriptors: after both palette queues, before textures.
 * Each word is [top/width/side flags:8][height:8] for one sorted HOTSPR ID. */
#define WF_SATURN_VDP1_CROP_OFFSET          0x42000u
#define WF_SATURN_VDP1_CROP_MAX_TILES       6474u
#define WF_SATURN_VDP1_CROP_END             (WF_SATURN_VDP1_CROP_OFFSET + WF_SATURN_VDP1_CROP_MAX_TILES*2u)
#define WF_SATURN_VDP1_DYNAMIC_OFFSET        0x4B580u
#define WF_SATURN_VDP1_DYNAMIC_BYTES         0x10000u
#define WF_SATURN_VDP1_DYNAMIC_END           (WF_SATURN_VDP1_DYNAMIC_OFFSET + WF_SATURN_VDP1_DYNAMIC_BYTES)
/* WP2015 journalv2 owns54048 bytes. The rest of the historical tail
 * reservation may hold immutable secondary sprites after startup.
 * The executable sound sequencer and its state reside in HWRAM. */
#define WF_SATURN_VDP1_JOURNAL_OFFSET        0x5c000u
#define WF_SATURN_VDP1_JOURNAL_END           0x69320u
#define WF_SATURN_VDP1_JOURNAL_RESERVED_END  0x80000u
/* Historical source aliases; no sound-module code occupies this tail. */
#define WF_SATURN_VDP1_MUSIC_OFFSET          WF_SATURN_VDP1_JOURNAL_OFFSET
#define WF_SATURN_VDP1_MUSIC_END             WF_SATURN_VDP1_JOURNAL_RESERVED_END
#define WF_SATURN_VDP1_STREAM_TILE_BYTES     128u
#define WF_SATURN_VDP1_STREAM_TILE_CAPACITY  (WF_SATURN_VDP1_TEXTURE_BANK_BYTES / WF_SATURN_VDP1_STREAM_TILE_BYTES)

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
