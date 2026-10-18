#include <string.h>
#include "wf_saturn_combat_present.h"
void wf_saturn_combat_present_init(wf_saturn_combat_present_t *p,void *hwr_stage,size_t hwr_stage_bytes){
    if(!p)return;
    memset(p,0,sizeof(*p));
    wf_saturn_yaul_backend_init(&p->backend,hwr_stage,hwr_stage_bytes);
}
wf_status_t wf_saturn_combat_present_frame(wf_saturn_combat_present_t *p,wf_runtime_t *rt,wf_visual_state_t *visual,const wf_graphics_blob_t *gfx,const wf_vdp1_assets_t *assets){
    if(!p)return WF_BAD_STATE;
    wf_status_t st=wf_combat_build_render_frame(rt,visual,gfx,assets,&p->frame);
    if(st!=WF_OK)return st;
    return wf_saturn_yaul_present_plan(&p->backend,&p->frame.vdp1,assets);
}
