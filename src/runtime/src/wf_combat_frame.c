#include <string.h>
#include "wf_combat_frame.h"
wf_status_t wf_combat_build_render_frame(wf_runtime_t *rt,wf_visual_state_t *v,const wf_graphics_blob_t *g,const wf_vdp1_assets_t *a,wf_combat_frame_output_t *o){
    if(!rt||!v||!g||!a||!o)return WF_BAD_STATE;
    memset(o,0,sizeof(*o));
    wf_status_t st=wf_visual_build_screen_positions(rt,v);if(st!=WF_OK)return st;
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        if(!wf_u8(&rt->actors[i],WF_A_ACTIVE))continue;
        st=wf_graphics_emit_actor(&rt->actors[i],&v->actor[i],g,&o->draw);if(st!=WF_OK)return st;
    }
    return wf_vdp1_plan_build(&o->draw,a,&o->vdp1);
}
