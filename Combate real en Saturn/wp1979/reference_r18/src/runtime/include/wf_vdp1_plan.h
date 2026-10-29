#ifndef WF_VDP1_PLAN_H
#define WF_VDP1_PLAN_H
#include <stddef.h>
#include <stdint.h>
#include "wf_graphics.h"
#include "wf_runtime.h"

#define WF_VDP1_TILE_BYTES_16X16_4BPP 128u
#define WF_VDP1_STREAM_TILES_MAX 256u
#define WF_VDP1_STREAM_PALETTES_MAX 16u
#define WF_VDP1_PALETTE_COLORS 16u
#define WF_VDP1_PALETTE_BYTES 32u /* 16 x big-endian xBGR444 source words; backend expands to CRAM mode-2 RGB888 */
#define WF_VDP1_COMMANDS_MAX WF_DRAW_PIECES_MAX

typedef struct wf_vdp1_tile_upload {
    uint16_t source_tile;
    uint16_t stream_slot;
    uint32_t src_byte_offset;
    uint32_t dst_byte_offset;
} wf_vdp1_tile_upload_t;

typedef struct wf_vdp1_palette_upload {
    uint8_t source_resource;
    uint8_t cram_slot;
    uint16_t reserved;
    uint32_t src_byte_offset;
} wf_vdp1_palette_upload_t;

typedef struct wf_vdp1_sprite_cmd {
    int16_t x,y;
    uint16_t stream_tile;
    uint8_t cram_slot;
    uint8_t flip_x,flip_y;
    uint8_t chain_index,chain;
} wf_vdp1_sprite_cmd_t;

typedef struct wf_vdp1_frame_plan {
    wf_vdp1_tile_upload_t tile_upload[WF_VDP1_STREAM_TILES_MAX];
    wf_vdp1_palette_upload_t palette_upload[WF_VDP1_STREAM_PALETTES_MAX];
    wf_vdp1_sprite_cmd_t cmd[WF_VDP1_COMMANDS_MAX];
    unsigned tile_count,palette_count,cmd_count;
    size_t tile_upload_bytes,palette_upload_bytes;
} wf_vdp1_frame_plan_t;

typedef struct wf_vdp1_assets {
    const uint8_t *sprite_tiles; size_t sprite_tile_bytes;
    const uint8_t *palette_resources; size_t palette_bytes;
} wf_vdp1_assets_t;

void wf_vdp1_plan_reset(wf_vdp1_frame_plan_t *p);
wf_status_t wf_vdp1_plan_build(const wf_draw_list_t *draw,const wf_vdp1_assets_t *assets,wf_vdp1_frame_plan_t *out);
wf_status_t wf_vdp1_plan_validate(const wf_vdp1_frame_plan_t *p,const wf_vdp1_assets_t *assets);
/* Keep unchanged source tiles in their resident VRAM slots while preserving
 * every command's tile identity and draw order. resident_bits is a packed
 * bitset; only slots below the current dense tile count can be retained. */
wf_status_t wf_vdp1_plan_retain_tile_slots(wf_vdp1_frame_plan_t *p,
    const uint16_t *resident_tiles,const uint8_t *resident_bits,unsigned resident_count);
#endif
