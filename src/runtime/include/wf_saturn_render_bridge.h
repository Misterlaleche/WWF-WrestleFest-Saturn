#ifndef WF_SATURN_RENDER_BRIDGE_H
#define WF_SATURN_RENDER_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
#include "wf_vdp1_plan.h"

typedef struct wf_saturn_render_ops {
    wf_status_t (*begin_frame)(void *ctx,unsigned tile_count,unsigned palette_count,unsigned cmd_count);
    wf_status_t (*upload_tile_16x16_4bpp)(void *ctx,uint16_t stream_slot,const uint8_t *src,size_t bytes);
    wf_status_t (*upload_palette_xbgr444_16)(void *ctx,uint8_t cram_slot,const uint8_t *src,size_t bytes);
    wf_status_t (*emit_sprite_16x16)(void *ctx,const wf_vdp1_sprite_cmd_t *cmd);
    wf_status_t (*end_frame)(void *ctx);
    void (*abort_frame)(void *ctx);
} wf_saturn_render_ops_t;

wf_status_t wf_saturn_render_submit(const wf_vdp1_frame_plan_t *plan,const wf_vdp1_assets_t *assets,const wf_saturn_render_ops_t *ops,void *ctx);
#endif
