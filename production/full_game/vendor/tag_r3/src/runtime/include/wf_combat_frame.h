#ifndef WF_COMBAT_FRAME_H
#define WF_COMBAT_FRAME_H
#include "wf_graphics.h"
#include "wf_vdp1_plan.h"

typedef struct wf_combat_frame_output {
    wf_draw_list_t draw;
    wf_vdp1_frame_plan_t vdp1;
} wf_combat_frame_output_t;

wf_status_t wf_combat_build_render_frame(wf_runtime_t *rt,wf_visual_state_t *visual,const wf_graphics_blob_t *gfx,const wf_vdp1_assets_t *assets,wf_combat_frame_output_t *out);
#endif
