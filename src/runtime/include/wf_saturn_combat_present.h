#ifndef WF_SATURN_COMBAT_PRESENT_H
#define WF_SATURN_COMBAT_PRESENT_H
#include "wf_combat_frame.h"
#include "wf_saturn_yaul_backend.h"

typedef struct wf_saturn_combat_present {
    wf_combat_frame_output_t frame;
    wf_saturn_yaul_backend_t backend;
} wf_saturn_combat_present_t;

void wf_saturn_combat_present_init(wf_saturn_combat_present_t *p,void *hwr_stage,size_t hwr_stage_bytes);
wf_status_t wf_saturn_combat_present_frame(wf_saturn_combat_present_t *p,wf_runtime_t *rt,wf_visual_state_t *visual,const wf_graphics_blob_t *gfx,const wf_vdp1_assets_t *assets);
#endif
