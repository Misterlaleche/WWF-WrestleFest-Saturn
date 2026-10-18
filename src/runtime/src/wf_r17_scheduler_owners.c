#include "wf_r17_contract.h"

static int active(const wf_actor_raw_t *a){return (wf_u8(a,WF_A_ACTIVE)&0x80u)!=0u;}
static void set_state_new_latch(wf_actor_raw_t *a){
    if((wf_u8(a,WF_A_STATE)&0x80u)==0u){
        wf_set_be16(a,WF_A_PRIOR,wf_be16(a,WF_A_LATCHED));
        wf_set_be16(a,WF_A_LATCHED,wf_be16(a,WF_A_STATE));
        wf_set_u8(a,WF_A_STATE,(uint8_t)(wf_u8(a,WF_A_STATE)|0x80u));
    }
}
static void add_corr(wf_actor_raw_t *a,size_t dst,size_t corr){
    wf_set_be16(a,dst,(uint16_t)(wf_be16(a,dst)+wf_be16(a,corr)));
}

wf_status_t wf_r17_referee_1f914(wf_runtime_t *rt,wf_actor_raw_t *referee,wf_r17_render_ctx_t *render,const wf_r17_referee_hooks_t *hooks,uint8_t mode_0161){
    wf_status_t s;
    if(!rt||!referee||!render||!hooks||!hooks->owner_1f914_logic) return WF_BLOCKED;
    s=hooks->owner_1f914_logic(rt,referee,hooks->user); if(s!=WF_OK)return s;
    s=wf_r17_world_to_screen_247c(referee,render->scroll_a_x,render->scroll_a_y);if(s!=WF_OK)return s;
    s=wf_r17_queue_27b8(render,referee);if(s!=WF_OK)return s;
    if((mode_0161&0x02u)!=0u){if(!hooks->royal_f8e4)return WF_BLOCKED;return hooks->royal_f8e4(rt,referee,hooks->user);}
    return WF_OK;
}

wf_status_t wf_r17_wrestlers_f4c2(wf_runtime_t *rt,wf_r17_render_ctx_t *render,const wf_r17_f4c2_hooks_t *hooks){
    unsigned i;wf_status_t s;
    if(!rt||!render||!hooks||!hooks->dispatcher_1c03e||!hooks->postqueue_2208) return WF_BLOCKED;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u); /* $F4C2 */
    for(i=0;i<WF_ACTOR_SLOTS;i++){
        wf_actor_raw_t *a=&rt->actors[i];
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(i+1u)); /* $250E iterator */
        if(!active(a))continue;
        if((wf_u8(a,WF_A_FLAGS32)&0x80u)!=0u)continue;
        set_state_new_latch(a);
        wf_set_be16(a,WF_A_CONTACT,0u);
        if(wf_u8(a,WF_A_MOTION_MODE)!=0u){
            if(!hooks->resolver_280dc)return WF_BLOCKED;
            s=hooks->resolver_280dc(rt,a,hooks->user);if(s!=WF_OK)return s;
            if(wf_u8(a,WF_A_CONTACT+1u)!=0u){
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,wf_be16(a,WF_A_CORR_X));add_corr(a,WF_A_X,WF_A_CORR_X);
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,wf_be16(a,WF_A_CORR_Y));add_corr(a,WF_A_Y,WF_A_CORR_Y);
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,wf_be16(a,WF_A_CORR_Z));add_corr(a,WF_A_Z,WF_A_CORR_Z);
            }
        }
        {uint32_t saved_d0=rt->m68k_regs.d[0];s=hooks->dispatcher_1c03e(rt,a,hooks->user);rt->m68k_regs.d[0]=saved_d0;if(s!=WF_OK)return s;} /* $1C03E MOVEM D0 */
        s=wf_r17_world_to_screen_247c(a,render->scroll_a_x,render->scroll_a_y);if(s!=WF_OK)return s;
        s=wf_r17_queue_27b8(render,a);if(s!=WF_OK)return s;
        s=hooks->postqueue_2208(rt,a,hooks->user);if(s!=WF_OK)return s;
    }
    /* $24090: separate active-slot rescan after the whole callback pass. */
    for(i=0;i<WF_ACTOR_SLOTS;i++){
        wf_actor_raw_t *a=&rt->actors[i];uint16_t v;
        if(!active(a))continue;
        v=wf_be16(a,WF_A_MIRROR);
        if(v!=wf_be16(a,WF_A_VISUAL_ACTION)){
            wf_set_be16(a,WF_A_COLL_CACHE,0u);
            wf_set_be16(a,WF_A_PROFILE,0u);
            wf_set_be16(a,WF_A_VISUAL_ACTION,v);
        }
    }
    return WF_OK;
}
