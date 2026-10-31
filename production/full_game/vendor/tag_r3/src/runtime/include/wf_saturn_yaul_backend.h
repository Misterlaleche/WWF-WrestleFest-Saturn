#ifndef WF_SATURN_YAUL_BACKEND_H
#define WF_SATURN_YAUL_BACKEND_H
#include <stddef.h>
#include <stdint.h>
#include "wf_saturn_render_bridge.h"
#include "wf_saturn_memory.h"

typedef struct wf_saturn_yaul_backend {
    uint8_t *hwr_stage;
    size_t hwr_stage_bytes;
    unsigned expected_tiles, expected_palettes, expected_cmds;
    unsigned emitted_tiles, emitted_palettes, emitted_cmds;
    uint16_t rom_source[WF_VDP1_STREAM_TILES_MAX];
    uint8_t rom_identity[WF_VDP1_STREAM_TILES_MAX/8u];
    uint8_t resident[WF_VDP1_STREAM_TILES_MAX/8u];
    uint8_t dirty[WF_VDP1_STREAM_TILES_MAX/8u];
    /* A logical staged slot may be current in either physical VRAM bank.
     * Changes stale both; only DMA to that inactive bank clears its bit. */
    uint8_t bank_stale[2][WF_VDP1_STREAM_TILES_MAX/8u];
    uint8_t palette_stage[16][32];
    uint16_t palette_mask;
    uint32_t texture_bytes, texture_transfers, texture_reused;
    uint8_t active;
    uint8_t defer_sync; /* caller commits VDP1 and VDP2 together before the next frame */
    /* Bounds describe staged bytes only while the slot is resident or dirty.
     * A zero height consumes the logical request without a hardware sprite. */
    /* Low5 bits: original top row; bit5: packed8px width; bit6: right
     * source half. Reuses the former top byte: backend size is unchanged. */
    uint8_t crop_meta[WF_VDP1_STREAM_TILES_MAX];
    uint8_t crop_height[WF_VDP1_STREAM_TILES_MAX];
} wf_saturn_yaul_backend_t;

/* Source IDs refer to immutable decoded ROM tiles for this backend lifetime.
 * Reinitialization invalidates residency before any asset-set replacement. */
wf_status_t wf_saturn_yaul_upload_rom_tile(wf_saturn_yaul_backend_t*,uint16_t slot,uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t**));
/* Optional prevalidated immutable crop hint. 0xffff requests the generic
 * pixel scan. Other values pack existing crop_meta in the high byte and
 * height in the low byte; read_crop refers to the just-decoded source. */
wf_status_t wf_saturn_yaul_upload_rom_tile_crop(wf_saturn_yaul_backend_t*,uint16_t slot,uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t**),uint16_t (*read_crop)(uint16_t));
/* Dense plan prefix in slots 0..count-1; source_tile is the only upload
 * field consumed, matching the presenter's scalar loop. Identity hits do
 * not call either reader. Count zero is an unconditional no-op. */
wf_status_t wf_saturn_yaul_upload_rom_tiles(wf_saturn_yaul_backend_t*,const wf_vdp1_tile_upload_t*,size_t count,wf_status_t (*read_tile)(uint16_t,const uint8_t**),uint16_t (*read_crop)(uint16_t));
void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t *b, void *hwr_stage, size_t hwr_stage_bytes);
const wf_saturn_render_ops_t *wf_saturn_yaul_render_ops(void);
/* Identical to count scalar emit calls, including the successful prefix on
 * error and the no-op for count zero. Native builds share loop invariants. */
wf_status_t wf_saturn_yaul_emit_sprites(wf_saturn_yaul_backend_t*,const wf_vdp1_sprite_cmd_t*,size_t count);


/* Hardware setup/submission exist only in the Saturn build. Host builds retain the contract for audit/tests. */
wf_status_t wf_saturn_yaul_video_init(void);
wf_status_t wf_saturn_yaul_present_plan(wf_saturn_yaul_backend_t *b,const wf_vdp1_frame_plan_t *plan,const wf_vdp1_assets_t *assets);
/* Direct TAG presentation transaction: queue after all layers are ready,
 * then wait for the VBlankOut after the actual VBlankIn root publication. */
wf_status_t wf_saturn_yaul_frame_sync(void);
void wf_saturn_yaul_frame_wait(void);
void wf_saturn_yaul_display_drain(void);
void wf_saturn_yaul_vblank_out(void);
/* Publications, acknowledgements, busy begin, texture/command ownership
 * conflicts, late-VBlank deferrals, CEF-busy deferrals. */
extern volatile uint32_t wp2015_vdp1_bank_observer[7];
#endif
