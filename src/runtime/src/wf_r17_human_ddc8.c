#include "wf_r17_human_ddc8.h"

#define BIT8(n) ((uint8_t)(1u << (n)))

enum {
    O_X=0x06, O_Y=0x0A, O_STATE=0x20, O_TIMER44=0x44, O_PROFILE=0x4C,
    O_SELECTOR=0x60, O_HELD_DIR_LO=0xA9, O_STREAK=0xAC, O_STREAK_LO=0xAD,
    O_ANGLE_WORD=0x2C, O_FLAGS32=0x32, O_FLAGS33=0x33, O_FLAGS34=0x34,
    O_CONTACT=0x36
};

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){
    return (wf_u8(a,o)&BIT8(b))!=0u;
}
static uint8_t state_lo(const wf_actor_raw_t *a){return wf_u8(a,O_STATE+1u);}
static uint8_t selector_lo(const wf_actor_raw_t *a){return wf_u8(a,O_SELECTOR+1u);}
static wf_status_t block(wf_r17_human_ddc8_trace_t *tr,uint32_t pc){
    if(tr)tr->blocked_dependency_rom=pc;
    return WF_BLOCKED;
}
static void trace_clear(wf_r17_human_ddc8_trace_t *tr){
    if(!tr)return;
    tr->blocked_dependency_rom=0u;tr->last_stage_rom=0u;tr->consumed=0u;
    tr->boundary_slot=0xFFu;tr->streak_success=0u;
}

/* $EF6A. D0 is an 8-bit held-direction mask.  Success is the 68000 carry
 * result used by callers: first qualifying invocation seeds $8004 and also
 * decrements the low byte, so success occurs on the fourth qualifying call. */
static int ef6a(wf_actor_raw_t *a,uint8_t mask,wf_r17_human_ddc8_trace_t *tr){
    uint8_t lo;
    if((uint8_t)(mask & wf_u8(a,O_HELD_DIR_LO))==0u){
        wf_set_be16(a,O_STREAK,0u);
        return 0;
    }
    if(!bit(a,O_STREAK,7))wf_set_be16(a,O_STREAK,0x8004u);
    lo=(uint8_t)(wf_u8(a,O_STREAK_LO)-1u);
    wf_set_u8(a,O_STREAK_LO,lo);
    if(lo!=0u)return 0;
    wf_set_be16(a,O_STREAK,0u);
    if(tr)tr->streak_success=1u;
    return 1;
}

/* $EECE. Return 1 exactly when the helper accepts and writes State8; return 0
 * when the requested global owner bit is already occupied. */
static wf_status_t eece(wf_actor_raw_t *a,const wf_r17_human_ddc8_ctx_t *ctx,
                        wf_r17_human_ddc8_trace_t *tr,unsigned index,int *accepted){
    static const uint32_t masks[10]={0x00000001u,0x00000002u,0x00000004u,0x00000008u,0x00000010u,0x00000020u,0x00000040u,0x00000080u,0x00000100u,0x00000200u};
    uint32_t g,mask;
    *accepted=0;
    if(!ctx||!ctx->global_1670)return block(tr,0x0000EED0u);
    g=*ctx->global_1670;
    if(index>=10u)return block(tr,0x0000EED4u);
    mask=masks[index];
    if((g&mask)!=0u)return WF_OK;
    if(wf_be16(a,O_STREAK)==0u)wf_set_be16(a,O_STREAK,0x8004u);
    g|=mask;*ctx->global_1670=g;
    wf_set_be16(a,O_TIMER44,(uint16_t)index);
    wf_set_be16(a,O_PROFILE,0u);
    wf_set_be16(a,O_STATE,8u);
    if(tr){tr->boundary_slot=(uint8_t)index;tr->last_stage_rom=0x0000EECEu;}
    *accepted=1;
    return WF_OK;
}

/* $EDC0. Return consumed=1 for the caller's carry-set result at $EEC8. */
static wf_status_t edc0(wf_actor_raw_t *a,const wf_r17_human_ddc8_ctx_t *ctx,
                        wf_r17_human_ddc8_trace_t *tr,int *consumed){
    uint16_t x,y,contact;int accepted;wf_status_t s;
    *consumed=0;
    if(bit(a,O_FLAGS34,0)||bit(a,O_FLAGS33,2))return WF_OK;
    contact=(uint16_t)(wf_be16(a,O_CONTACT)&3u);
    if(contact==0u)return WF_OK;
    if(!ctx||!ctx->global_1670)return block(tr,0x0000EDE0u);
    if(*ctx->global_1670!=0u)return WF_OK;
    x=wf_be16(a,O_X);y=wf_be16(a,O_Y);
    if((ctx->mode_0161&2u)!=0u){
        if(x>=0x03CFu){
            s=eece(a,ctx,tr,9u,&accepted);if(s!=WF_OK)return s;
            if(accepted){*consumed=1;return WF_OK;}
        }
        if(x<0x0271u){
            s=eece(a,ctx,tr,8u,&accepted);if(s!=WF_OK)return s;
            if(accepted){*consumed=1;return WF_OK;}
        }
        return WF_OK;
    }
    if(x<0x01C5u&&y<0x011Au&&ef6a(a,0x08u,tr)){
        s=eece(a,ctx,tr,2u,&accepted);if(s!=WF_OK)return s;
        if(accepted){*consumed=1;return WF_OK;}
    }
    if(x>=0x032Bu&&y<0x011Au&&ef6a(a,0x08u,tr)){
        s=eece(a,ctx,tr,3u,&accepted);if(s!=WF_OK)return s;
        if(accepted){*consumed=1;return WF_OK;}
    }
    if(x<0x01F1u&&y>=0x0197u&&ef6a(a,0x04u,tr)){
        s=eece(a,ctx,tr,0u,&accepted);if(s!=WF_OK)return s;
        if(accepted){*consumed=1;return WF_OK;}
    }
    if(x>=0x02FCu&&y>=0x0197u&&ef6a(a,0x04u,tr)){
        s=eece(a,ctx,tr,1u,&accepted);if(s!=WF_OK)return s;
        if(accepted){*consumed=1;return WF_OK;}
    }
    return WF_OK;
}

/* $EF0A. */
static wf_status_t ef0a(wf_actor_raw_t *a,const wf_r17_human_ddc8_ctx_t *ctx,
                        wf_r17_human_ddc8_trace_t *tr,int *consumed){
    *consumed=0;
    if(!ctx||(ctx->mode_0161&2u)==0u)return WF_OK;
    if(!bit(a,O_FLAGS33,0))return WF_OK;
    if(wf_u8(a,O_CONTACT)!=4u)return WF_OK;
    if(!bit(a,O_FLAGS33,2)){
        if(!ef6a(a,0x08u,tr))return WF_OK;
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x006Cu);
    }else{
        if(!ef6a(a,0x04u,tr))return WF_OK;
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0069u);
    }
    if(tr)tr->last_stage_rom=0x0000EF0Au;
    *consumed=1;
    return WF_OK;
}

wf_status_t wf_r17_human_ddc8(wf_runtime_t *rt,unsigned slot,const wf_r17_human_ddc8_ctx_t *ctx,
                              wf_r17_human_ddc8_trace_t *tr){
    wf_actor_raw_t *a;uint8_t st,sel,held;int consumed;wf_status_t s;
    trace_clear(tr);
    if(!rt||!ctx||slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
    a=&rt->actors[slot];
    if(tr)tr->last_stage_rom=0x0000DDC8u;
    if(bit(a,O_FLAGS32,0)||bit(a,O_FLAGS34,1))return WF_OK;
    st=state_lo(a);
    if(st==0u||st==1u){
        held=wf_u8(a,O_HELD_DIR_LO);
        if(held!=0u){
            s=edc0(a,ctx,tr,&consumed);if(s!=WF_OK)return s;
            if(consumed){if(tr)tr->consumed=1u;return WF_OK;}
            s=ef0a(a,ctx,tr,&consumed);if(s!=WF_OK)return s;
            if(consumed){if(tr)tr->consumed=1u;return WF_OK;}
            if(state_lo(a)!=1u)wf_set_be16(a,O_STATE,1u);
            return WF_OK;
        }
        if(st!=0u)wf_set_be16(a,O_STATE,0u);
        return WF_OK;
    }
    if(st!=5u)return WF_OK;
    sel=selector_lo(a);held=wf_u8(a,O_HELD_DIR_LO);
    if(sel==0x15u){
        if(held!=0u){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0016u);}
        return WF_OK;
    }
    if(sel==0x16u){
        if(held==0u){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_TIMER44,1u);
            wf_set_be16(a,O_SELECTOR,0x0015u);wf_set_be16(a,O_ANGLE_WORD,0u);
        }
        return WF_OK;
    }
    return WF_OK;
}
