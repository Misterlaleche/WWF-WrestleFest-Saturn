#include <stddef.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_animation.h"

static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}
static wf_status_t blocked(wf_r17_dispatch_1c03e_ctx_t *c,uint32_t pc){if(c)c->blocked_dependency_rom=pc;return WF_BLOCKED;}
static void exact_tail_1c0b4(wf_actor_raw_t *a){
    if((wf_u8(a,WF_A_FIELD18)&0x80u)!=0u){wf_set_be16(a,WF_A_FIELD18,0u);wf_set_be16(a,WF_A_FIELD1A,0u);}
}

wf_status_t wf_r17_dispatch_1c03e_bounded(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    uint8_t domain,state,id;uint16_t di;const wf_anim_desc_t *d;wf_status_t s;ptrdiff_t slot;
    if(!rt||!a||!ctx)return WF_BAD_STATE;
    ctx->blocked_dependency_rom=0u;
    domain=low(a,WF_A_LATCHED);state=low(a,WF_A_STATE);
    /* $1C084: current low state $FF exits before callback, tick and +18/+1A tail. */
    if(state==0xffu)return WF_OK;
    if(domain==5u){
        if(!ctx->actions)return blocked(ctx,0x00012614u);
        slot=a-rt->actors;if(slot<0||slot>=(ptrdiff_t)WF_ACTOR_SLOTS)return WF_BAD_STATE;
        s=wf_r17_dispatch_1c03e_r11(rt,(unsigned)slot,ctx->actions);
        if(s!=WF_OK){
            uint32_t dep=ctx->actions->trace[(unsigned)slot].blocked_dependency_rom;
            if(dep==0u)dep=ctx->actions->raw23_trace[(unsigned)slot].blocked_dependency_rom;
            return blocked(ctx,dep?dep:0x00012614u);
        }
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(domain==4u){
        id=wf_u8(a,WF_A_STATE4_LO);if(id>=43u)return blocked(ctx,0x0001afd4u);
        di=wf_anim_state4_dispatch[id];if(di>=wf_anim_desc_count)return blocked(ctx,0x0001afd4u);d=&wf_anim_descs[di];
        /* The two demonstrated callback-RTS entries are subtypes $1F/$28. */
        if(d->callback_rom!=0x00012612u)return blocked(ctx,d->callback_rom);
        /* $1C0C8 sees a kind-0 descriptor here and performs no animation mutation. */
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(domain==0xffu)return blocked(ctx,0x000125c0u);
    id=domain;if(id>=13u)return blocked(ctx,0x00011478u);
    di=wf_anim_main_dispatch[id];if(di>=wf_anim_desc_count)return blocked(ctx,0x00011478u);d=&wf_anim_descs[di];
    return blocked(ctx,d->callback_rom);
}
