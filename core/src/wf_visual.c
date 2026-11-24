#include <string.h>
#include "wf_visual.h"
#include "wf_physics.h"
#include "wf_r17_contract.h"

void wf_visual_init(wf_visual_state_t *v){memset(v,0,sizeof(*v));}

static int select_domain(const wf_actor_raw_t *a,wf_anim_domain_t *dom,uint8_t *id){
    uint8_t st=wf_main_state(a);
    if(st==5u){*dom=WF_ANIM_ACTION;*id=wf_u8(a,0x61);return 1;}
    if(st==4u){*dom=WF_ANIM_STATE4;*id=wf_state4_subtype(a);return 1;}
    if(st<13u){*dom=WF_ANIM_MAIN;*id=st;return 1;}
    return 0;
}

wf_status_t wf_visual_actor_step(wf_runtime_t *rt,wf_visual_state_t *v,unsigned slot){
    if(slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
    wf_actor_raw_t *a=&rt->actors[slot]; wf_actor_visual_t *av=&v->actor[slot];
    if(!wf_u8(a,WF_A_ACTIVE)){av->valid=0;return WF_OK;}
    wf_anim_domain_t dom;uint8_t id;if(!select_domain(a,&dom,&id))return WF_BLOCKED;
    if(!av->valid || av->domain!=(uint8_t)dom || av->selector!=id){
        if(!wf_anim_select(a,dom,id,&av->anim))return WF_BLOCKED;
        av->valid=1;av->domain=(uint8_t)dom;av->selector=id;
    }else if(!wf_anim_tick(a,&av->anim)) return WF_BLOCKED;
    av->pose=wf_anim_current_pose(a,&av->anim);
    wf_physics_integrate_common(a);
    return WF_OK;
}

wf_status_t wf_visual_build_screen_positions(wf_runtime_t *rt,wf_visual_state_t *v){
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++) if(wf_u8(&rt->actors[i],WF_A_ACTIVE)){
        wf_actor_raw_t *a=&rt->actors[i];
        if(!v->actor[i].valid)return WF_BLOCKED;
        /* WP35: +$14/+16 are the intermediate $247C actor anchors, not final raster coordinates. */
        wf_status_t st=wf_r17_world_to_screen_247c(a,(uint16_t)v->camera.scroll0_x,(uint16_t)v->camera.scroll0_y);
        if(st!=WF_OK)return st;
        v->actor[i].screen_x=(int16_t)wf_be16(a,WF_A_SCREEN_X);
        v->actor[i].screen_y=(int16_t)wf_be16(a,WF_A_SCREEN_Y);
    }
    return WF_OK;
}
