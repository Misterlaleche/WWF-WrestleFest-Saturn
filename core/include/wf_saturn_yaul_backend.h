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
    uint8_t resident[WF_VDP1_STREAM_TILES_MAX/8u]; /* Jo: prepared this frame */
    uint8_t dirty[WF_VDP1_STREAM_TILES_MAX/8u];
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
#ifdef WF_JO_ENGINE
/* Pin every resident immutable source before allocating any misses. Prepared
 * slots remain distinct from consumed slots until upload/skip. */
wf_status_t wf_jo_backend_protect_rom_tiles(wf_saturn_yaul_backend_t *,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count);
/* Private presenter optimization. unchanged proves the immediately preceding
 * completed plan retained this entire source array and backend metadata.
 * generation identifies the immutable asset binding; zero disables reuse.
 * Active slots are still checked, and normal consumption/commands follow. */
wf_status_t wf_jo_backend_prepare_rom_tiles(wf_saturn_yaul_backend_t *,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count,
    unsigned unchanged,uint32_t generation);
/* Call immediately after private preparation, before any callback or change
 * to its arrays/backend. Returns1 only when a restored whole preparation was
 * consumed without callbacks; returns0 for the ordinary consumption path.
 * Cache mutations and public tile consumers revoke this one-use permission. */
int wf_jo_backend_consume_prepared_rom_tiles(wf_saturn_yaul_backend_t *);
/* Complete logical-slot batch. Read callbacks retain their scalar order and
 * failures; context/limits are revalidated after each callback. */
wf_status_t wf_jo_backend_consume_rom_tiles(wf_saturn_yaul_backend_t *,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count,
    wf_status_t (*read_tile)(uint16_t,const uint8_t **));
/* Consume an unused persistent slot without reading or uploading pixels. */
wf_status_t wf_jo_backend_skip_tile(wf_saturn_yaul_backend_t *,uint16_t slot);
/* Ordered batch after tile preparation; checks context/count once and each
 * sprite's active slot and palette before emitting its cropped command. */
wf_status_t wf_jo_backend_emit_sprites(wf_saturn_yaul_backend_t *,
    const wf_vdp1_sprite_cmd_t *,unsigned count);
#endif
void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t *b, void *hwr_stage, size_t hwr_stage_bytes);
const wf_saturn_render_ops_t *wf_saturn_yaul_render_ops(void);

/* Hardware setup/submission exist only in the Saturn build. Host builds retain the contract for audit/tests. */
wf_status_t wf_saturn_yaul_video_init(void);
wf_status_t wf_saturn_yaul_present_plan(wf_saturn_yaul_backend_t *b,const wf_vdp1_frame_plan_t *plan,const wf_vdp1_assets_t *assets);
#endif
