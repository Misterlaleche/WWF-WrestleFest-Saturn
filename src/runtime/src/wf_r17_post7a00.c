#include "wf_r17_post7a00.h"
#include "wf_payment.h"
#include "wf_r17_tag_6e34.h"
#include "wf_r17_royal_9132.h"
#include <string.h>


/* ROM $285DA static visibility/source tables. These bytes are copied directly
 * from the canonical main 68000 image at $28696/$286F8/$2875A/$287E0/$287E2. */
static const uint8_t branchb_tbl_28696[]={
 0x04u,0x09u,0x00u,0x05u,0x09u,0x01u,0x06u,0x09u,0x02u,0x07u,0x09u,0x03u,0x08u,0x09u,0x04u,0x09u,
 0x09u,0x05u,0x0au,0x09u,0x06u,0x0bu,0x09u,0x07u,0x0cu,0x09u,0x08u,0x0du,0x09u,0x09u,0x04u,0x08u,
 0x0au,0x05u,0x08u,0x0bu,0x06u,0x08u,0x0cu,0x07u,0x08u,0x0du,0x08u,0x08u,0x0eu,0x09u,0x08u,0x0fu,
 0x0au,0x08u,0x10u,0x0bu,0x08u,0x11u,0x0cu,0x08u,0x12u,0x0du,0x08u,0x13u,0x04u,0x07u,0x14u,0x0du,
 0x07u,0x15u,0x04u,0x06u,0x16u,0x0du,0x06u,0x17u,0x05u,0x07u,0x18u,0x06u,0x07u,0x19u,0x07u,0x07u,
 0x1au,0x08u,0x07u,0x1bu,0x09u,0x07u,0x1cu,0x0au,0x07u,0x1du,0x0bu,0x07u,0x1eu,0x0cu,0x07u,0x1fu,0xffu};
static const uint8_t branchb_tbl_286f8[]={
 0x04u,0x09u,0x00u,0x05u,0x09u,0x01u,0x06u,0x09u,0x02u,0x07u,0x09u,0x03u,0x08u,0x09u,0x04u,0x09u,
 0x09u,0x05u,0x0au,0x09u,0x06u,0x0bu,0x09u,0x07u,0x0cu,0x09u,0x08u,0x0du,0x09u,0x09u,0x04u,0x08u,
 0x0au,0x05u,0x08u,0x0bu,0x06u,0x08u,0x0cu,0x07u,0x08u,0x0du,0x08u,0x08u,0x0eu,0x09u,0x08u,0x0fu,
 0x0au,0x08u,0x10u,0x0bu,0x08u,0x11u,0x0cu,0x08u,0x12u,0x0du,0x08u,0x13u,0x04u,0x07u,0x14u,0x0du,
 0x07u,0x15u,0x04u,0x06u,0x16u,0x0du,0x06u,0x17u,0x05u,0x07u,0x18u,0x06u,0x07u,0x19u,0x0bu,0x07u,
 0x1eu,0x0cu,0x07u,0x1fu,0x07u,0x07u,0x20u,0x08u,0x07u,0x21u,0x09u,0x07u,0x22u,0x0au,0x07u,0x23u,0xffu};
static const uint8_t branchb_tbl_2875a[]={
 0x04u,0x07u,0x00u,0x05u,0x07u,0x01u,0x06u,0x07u,0x02u,0x07u,0x07u,0x03u,0x08u,0x07u,0x04u,0x09u,
 0x07u,0x05u,0x0au,0x07u,0x06u,0x0bu,0x07u,0x07u,0x0cu,0x07u,0x08u,0x0du,0x07u,0x09u,0x0eu,0x07u,
 0x0au,0x0fu,0x07u,0x0bu,0x10u,0x07u,0x0cu,0x11u,0x07u,0x0du,0x12u,0x07u,0x0eu,0x04u,0x06u,0x0fu,
 0x05u,0x06u,0x10u,0x06u,0x06u,0x11u,0x07u,0x06u,0x12u,0x08u,0x06u,0x13u,0x0eu,0x06u,0x14u,0x0fu,
 0x06u,0x15u,0x10u,0x06u,0x16u,0x11u,0x06u,0x17u,0x12u,0x06u,0x18u,0x04u,0x05u,0x19u,0x05u,0x05u,
 0x1au,0x11u,0x06u,0x1bu,0x12u,0x06u,0x1cu,0x04u,0x04u,0x1du,0x05u,0x04u,0x1eu,0x06u,0x04u,0x1fu,
 0x07u,0x04u,0x20u,0x08u,0x04u,0x21u,0x09u,0x04u,0x22u,0x0au,0x04u,0x23u,0x0bu,0x04u,0x24u,0x0cu,
 0x04u,0x25u,0x0du,0x04u,0x26u,0x0eu,0x04u,0x27u,0x0fu,0x04u,0x28u,0x10u,0x04u,0x29u,0x11u,0x04u,
 0x2au,0x12u,0x04u,0x2bu,0xffu};
static const uint8_t branchb_tbl_287e0[]={0xffu};
static const uint8_t branchb_tbl_287e2[]={
 0x04u,0x06u,0x00u,0x05u,0x06u,0x01u,0x07u,0x06u,0x02u,0x08u,0x06u,0x03u,0x04u,0x05u,0x04u,0x05u,
 0x05u,0x05u,0x07u,0x05u,0x06u,0x08u,0x05u,0x07u,0x04u,0x04u,0x08u,0x05u,0x04u,0x09u,0x07u,0x04u,
 0x0au,0x08u,0x04u,0x0bu,0xffu};
static const uint8_t *const branchb_2867a[7]={branchb_tbl_28696,branchb_tbl_28696,branchb_tbl_2875a,branchb_tbl_287e0,branchb_tbl_287e2,branchb_tbl_286f8,branchb_tbl_2875a};

static int branchb_list_contains(const uint8_t list[48],uint8_t id){
    size_t i;for(i=0u;i<48u;i++){if((list[i]&0x80u)!=0u)return 0;if(list[i]==id)return 1;}return 0;
}

wf_status_t wf_r17_owner_285da(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    uint8_t list_snap[48],prev_snap[48],obj_snap[48][4];
    wf_r17_m68k_shadow_t regs_snap;
    const uint8_t *src;size_t si=0u,di=0u,i,j;uint8_t d0,d1,d2,d3;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if(rt->g.global_007e>=7u){ctx->blocked_dependency_rom=0x0002867au;return WF_BLOCKED;}
    memcpy(list_snap,rt->branchb_list_182a,sizeof(list_snap));memcpy(prev_snap,rt->branchb_prev_185a,sizeof(prev_snap));memcpy(obj_snap,rt->branchb_objs_188a,sizeof(obj_snap));regs_snap=rt->m68k_regs;
    d0=(uint8_t)((((uint16_t)(rt->g.global_17da+1u))>>1)-2u);
    d1=(uint8_t)((((uint16_t)(rt->g.global_17da-1u))>>1)+6u);
    d2=(uint8_t)((((uint16_t)(rt->g.global_17dc+1u))>>1)+2u);
    d3=(uint8_t)(d2-8u);
    src=branchb_2867a[rt->g.global_007e];
    while((src[si]&0x80u)==0u){uint8_t x=src[si++],y=src[si++],id=src[si++];if(x>=d0&&x<d1&&y<d2&&y>=d3){if(di>=47u){ctx->blocked_dependency_rom=0x001c182au;goto rollback;}rt->branchb_list_182a[di++]=id;}}
    rt->branchb_list_182a[di++]=0xffu;while(di<48u)rt->branchb_list_182a[di++]=0u;
    /* $2884E: remove previous IDs no longer present. */
    for(i=0u;i<48u&&((rt->branchb_prev_185a[i]&0x80u)==0u);i++)if(!branchb_list_contains(rt->branchb_list_182a,rt->branchb_prev_185a[i])){
        uint8_t id=rt->branchb_prev_185a[i];int found=0;for(j=0u;j<48u;j++){uint8_t b=rt->branchb_objs_188a[j][0];if((b&0x80u)!=0u&&(b&0x3fu)==id){memset(rt->branchb_objs_188a[j],0,4u);found=1;break;}}if(!found){ctx->blocked_dependency_rom=0x00028872u;goto rollback;}
    }
    /* $28808: add current IDs absent from the previous list. */
    for(i=0u;i<48u&&((rt->branchb_list_182a[i]&0x80u)==0u);i++)if(!branchb_list_contains(rt->branchb_prev_185a,rt->branchb_list_182a[i])){
        int found=0;for(j=0u;j<48u;j++)if((rt->branchb_objs_188a[j][0]&0x80u)==0u){rt->branchb_objs_188a[j][0]=(uint8_t)(rt->branchb_list_182a[i]|0x80u);found=1;break;}if(!found){ctx->blocked_dependency_rom=0x0002882cu;goto rollback;}
    }
    /* $285E8..$285FA copies exactly $30 bytes current -> previous. */
    memcpy(rt->branchb_prev_185a,rt->branchb_list_182a,48u);
    /* $28896: two passes over exactly 47 records, bit6 set first then clear. */
    for(unsigned pass=0u;pass<2u;pass++)for(i=0u;i<47u;i++){
        uint8_t b=rt->branchb_objs_188a[i][0];int group=((b&0x40u)!=0u);if((b&0x80u)==0u||group!=(pass==0u))continue;
        if(!ctx->branchb_288f0){ctx->blocked_dependency_rom=0x000288f0u;goto rollback;}
        if(ctx->branchb_288f0(rt,rt->branchb_objs_188a[i],rt->g.global_007f,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000288f0u;goto rollback;}
    }
    /* $285DA/$28608/$28808/$2884E/$28896 preserve D0/D1. Dynamic visual
     * handlers may legitimately leave D2-D7 caller-visible. */
    rt->m68k_regs.d[0]=regs_snap.d[0];rt->m68k_regs.d[1]=regs_snap.d[1];
    return WF_OK;
rollback:
    memcpy(rt->branchb_list_182a,list_snap,sizeof(list_snap));memcpy(rt->branchb_prev_185a,prev_snap,sizeof(prev_snap));memcpy(rt->branchb_objs_188a,obj_snap,sizeof(obj_snap));rt->m68k_regs=regs_snap;return WF_BLOCKED;
}

static wf_status_t branchb_26a60(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,uint8_t side){
    uint16_t sx,sy,tx,ty,direction;
    if(side==0u){sx=rt->g.global_17e6;sy=rt->g.global_17ee;tx=rt->g.global_17da;ty=rt->g.global_17dc;rt->g.global_17fe=(uint16_t)(rt->g.global_17fe&0x7fffu);}else{sx=rt->g.global_17ea;sy=rt->g.global_17f2_word;tx=rt->g.global_17de;ty=rt->g.global_17e0;rt->g.global_17fe=(uint16_t)(rt->g.global_17fe|0x8000u);}
    rt->g.global_17f6=sx;rt->g.global_17fa=sy;rt->g.global_17e2=tx;rt->g.global_17e4=ty;
    rt->g.global_17d6=(uint16_t)(sx>>5);rt->g.global_17d8=(uint16_t)(sy>>5);
    if(rt->g.global_17d6>tx)direction=1u;else if(rt->g.global_17d6<tx)direction=3u;else if(rt->g.global_17d8>ty)direction=0u;else if(rt->g.global_17d8<ty)direction=2u;else{rt->g.global_16d4=0x00ffu;return WF_OK;}
    rt->g.global_16d4=direction;
    if(side==0u)rt->g.global_16d2=(uint16_t)(rt->g.global_16d2|0x8000u);else rt->g.global_16d2=(uint16_t)(rt->g.global_16d2|0x4000u);
    if(!ctx->branchb_26b68){ctx->blocked_dependency_rom=0x00026b68u;return WF_BLOCKED;}
    {
        uint32_t save_d0=rt->m68k_regs.d[0],save_d1=rt->m68k_regs.d[1],save_d2=rt->m68k_regs.d[2];
        if(ctx->branchb_26b68(rt,side,direction,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00026b68u;return WF_BLOCKED;}
        /* $26C8A restores D0-D2 before returning through $26B68. */
        rt->m68k_regs.d[0]=save_d0;rt->m68k_regs.d[1]=save_d1;rt->m68k_regs.d[2]=save_d2;
    }
    /* $26B8E jump table: 0->$26B9E (Y++), 1->$26BDA (X++),
     * 2->$26BBC (Y--), 3->$26BF8 (X--). */
    switch(direction){
        case 0u: if(side==0u)rt->g.global_17dc=(uint16_t)(rt->g.global_17dc+1u);else rt->g.global_17e0=(uint16_t)(rt->g.global_17e0+1u);break;
        case 1u: if(side==0u)rt->g.global_17da=(uint16_t)(rt->g.global_17da+1u);else rt->g.global_17de=(uint16_t)(rt->g.global_17de+1u);break;
        case 2u: if(side==0u)rt->g.global_17dc=(uint16_t)(rt->g.global_17dc-1u);else rt->g.global_17e0=(uint16_t)(rt->g.global_17e0-1u);break;
        case 3u: if(side==0u)rt->g.global_17da=(uint16_t)(rt->g.global_17da-1u);else rt->g.global_17de=(uint16_t)(rt->g.global_17de-1u);break;
        default:return WF_BAD_STATE;
    }
    return WF_OK;
}

wf_status_t wf_r17_owner_26a42(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_globals_raw_t gsnap;wf_r17_m68k_shadow_t rsnap;wf_status_t s;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if(rt->g.global_16d2!=0u)return WF_OK;
    gsnap=rt->g;rsnap=rt->m68k_regs;
    s=branchb_26a60(rt,ctx,0u);if(s!=WF_OK)goto rollback;
    s=branchb_26a60(rt,ctx,1u);if(s!=WF_OK)goto rollback;
    /* $26C1C/$26C6E preserve D0. $26C8A saves/restores D0-D2/A0-A5, but
     * its deeper renderer path may leave D3-D7 caller-visible. Preserve only
     * the D0-D2 subset proven restored by the ROM; retain renderer clobbers D3-D7. */
    rt->m68k_regs.d[0]=rsnap.d[0];
    rt->m68k_regs.d[1]=rsnap.d[1];
    rt->m68k_regs.d[2]=rsnap.d[2];
    rt->m68k_regs.valid=rsnap.valid;
    return WF_OK;
rollback:rt->g=gsnap;rt->m68k_regs=rsnap;return WF_BLOCKED;
}

/* $7548 / $776C / $75EE raw FG0 helpers. */
static int branchb_push_byte(wf_r17_fg0_write_t *w,size_t cap,size_t *n,uint32_t addr,uint8_t v){
    if(*n>=cap)return 0;
    w[(*n)++]=(wf_r17_fg0_write_t){addr,(uint32_t)v,1u};
    return 1;
}
static uint32_t branchb_fg0_base(unsigned slot){
    static const uint16_t off[4]={0x0104u,0x0128u,0x0154u,0x0178u};
    return 0x000c0000u+(uint32_t)off[slot];
}
static int branchb_emit_76e8(wf_r17_fg0_write_t *w,size_t cap,size_t *n,uint32_t a,uint16_t tile){
    if(!branchb_push_byte(w,cap,n,a+1u,(uint8_t)tile))return 0;
    if(!branchb_push_byte(w,cap,n,a+3u,(uint8_t)(((tile>>8)&0xffu)|0x10u)))return 0;
    return 1;
}
static int branchb_emit_76aa(wf_r17_fg0_write_t *w,size_t cap,size_t *n,unsigned slot,uint32_t a,uint16_t level){
    uint16_t step=7u,tile=0x0b33u;
    if(level!=0u){step=11u;tile=0x0ad9u;if((slot&2u)!=0u){step=14u;tile=0x0af2u;}}
    tile=(uint16_t)(tile+level);
    if(!branchb_emit_76e8(w,cap,n,a,tile))return 0;
    if(!branchb_emit_76e8(w,cap,n,a+0x100u,(uint16_t)(tile+step)))return 0;
    return 1;
}
static int branchb_emit_776c_actor(const wf_actor_raw_t *a,unsigned slot,wf_r17_fg0_write_t *w,size_t cap,size_t *n){
    static const uint16_t table[4]={0x0b0fu,0x0b18u,0x0b21u,0x0b2au};
    uint16_t nib=(uint16_t)(wf_be32(a,WF_A_INPUT_SOURCE)&0x0fu),tile;
    uint32_t dst=branchb_fg0_base(slot);
    unsigned i;
    /* Closed input-source domain is $140020/$22/$24/$26 => byte offsets 0/2/4/6. */
    if((nib&1u)!=0u||nib>6u)return 0;
    tile=table[nib>>1];
    for(i=0u;i<3u;i++,tile++,dst+=4u)if(!branchb_emit_76e8(w,cap,n,dst,tile))return 0;
    return 1;
}
static int branchb_emit_75ee_actor(wf_runtime_t *rt,wf_actor_raw_t *a,unsigned slot,wf_r17_fg0_write_t *w,size_t cap,size_t *n){
    uint32_t dst=branchb_fg0_base(slot)+0x20cu;
    uint16_t visible=(uint16_t)(wf_be16(a,0x6eu)&0x00ffu),q,r;
    uint32_t old_d2=rt->m68k_regs.d[2];
    unsigned i;
    for(i=0u;i<6u;i++,dst+=4u)if(!branchb_emit_76aa(w,cap,n,slot,dst,0u))return 0;
    /* MOVE.W #5,D2 + six DBF iterations. */
    rt->m68k_regs.d[2]=(old_d2&0xffff0000u)|0x0000ffffu;
    rt->m68k_regs.d[1]=0u; /* MOVEQ #0,D1, then MOVE.B/ANDI.L #$FF. */
    if(visible==0u)return 1;
    q=(uint16_t)(visible/16u);r=(uint16_t)(visible%16u);
    rt->m68k_regs.d[1]=((uint32_t)r<<16)|q; /* DIVU result before ANDI. */
    rt->m68k_regs.d[2]=rt->m68k_regs.d[1]; /* MOVE.L D1,D2. */
    /* $7624 BSR $7680; $7628 ADDA.L #$20C,A1: clearing advanced A1
     * past all six cells. The fill restarts at the same bar origin. */
    dst=branchb_fg0_base(slot)+0x20cu;
    rt->m68k_regs.d[3]=1u;                 /* MOVEQ #1,D3. */
    rt->m68k_regs.d[1]=(uint32_t)q;        /* ANDI.L #$FFFF,D1. */
    if(q!=0u){
        uint16_t fills=q;
        if(fills>=6u){fills=6u;rt->m68k_regs.d[3]=0u;}
        for(i=0u;i<fills;i++,dst+=4u)if(!branchb_emit_76aa(w,cap,n,slot,dst,8u))return 0;
        rt->m68k_regs.d[1]=0x0000ffffu; /* DBF terminal value. */
        if(rt->m68k_regs.d[3]==0u)return 1;
    }
    /* SWAP D2; ANDI.W #$FF,D2. */
    rt->m68k_regs.d[2]=((uint32_t)q<<16)|r;
    if(r==0u)return 1;
    if((r&1u)!=0u)r--;
    r=(uint16_t)(r>>1);
    rt->m68k_regs.d[2]=((uint32_t)q<<16)|r;
    rt->m68k_regs.d[1]=(rt->m68k_regs.d[1]&0xffff0000u)|r;
    if(!branchb_emit_76aa(w,cap,n,slot,dst,r))return 0;
    return 1;
}

wf_status_t wf_r17_owner_7548(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_actor_raw_t snap[4];wf_r17_m68k_shadow_t rsnap;wf_r17_fg0_write_t w[256];size_t nw=0u;unsigned i;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if((rt->g.global_16c4&0x00ffu)!=0u||rt->g.credited_route==0u)return WF_OK;
    for(i=0u;i<4u;i++)snap[i]=rt->actors[i];
    rsnap=rt->m68k_regs;
    /* $776C: raw three-cell producer for each active non-reserve wrestler. */
    for(i=0u;i<4u;i++){
        wf_actor_raw_t *a=&rt->actors[i];
        if((wf_u8(a,WF_A_ACTIVE)&0x80u)==0u||(wf_u8(a,WF_A_ROLE)&0x80u)!=0u)continue;
        if(!branchb_emit_776c_actor(a,i,w,256u,&nw)){ctx->blocked_dependency_rom=0x000077d0u;goto rollback;}
    }
    /* $776C preserves D0-D4. $7566 MOVEQ #0,D0. */
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
    for(i=0u;i<4u;i++){
        wf_actor_raw_t *a=&rt->actors[i];
        if((wf_u8(a,WF_A_ACTIVE)&0x80u)!=0u&&(wf_u8(a,WF_A_ROLE)&0x80u)==0u){
            uint16_t delta=wf_be16(a,WF_A_VISIBLE_DELTA);
            uint16_t acc;
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,delta);
            acc=(uint16_t)(wf_be16(a,0x6cu)+delta);wf_set_be16(a,0x6cu,acc);wf_set_be16(a,WF_A_VISIBLE_DELTA,0u);
            if(acc!=0u){
                int16_t sacc=(int16_t)acc;uint16_t shown=wf_be16(a,0x6eu);
                if(sacc<0){
                    if(acc!=0xffffu){acc=(uint16_t)(acc+1u);shown=(uint16_t)(shown-1u);}
                    acc=(uint16_t)(acc+1u);shown=(uint16_t)(shown-1u);
                }else{
                    if(acc!=1u){acc=(uint16_t)(acc-1u);shown=(uint16_t)(shown+1u);}
                    acc=(uint16_t)(acc-1u);shown=(uint16_t)(shown+1u);
                }
                wf_set_be16(a,0x6cu,acc);wf_set_be16(a,0x6eu,shown);
                /* $76F8: signed-negative clamp, then unsigned max/equality clamp. */
                if((int16_t)shown<0){wf_set_be16(a,0x6eu,0u);wf_set_be16(a,0x6cu,0u);}
                else if(shown>=wf_be16(a,WF_A_STAMINA_MAX)){wf_set_be16(a,0x6eu,wf_be16(a,WF_A_STAMINA_MAX));wf_set_be16(a,0x6cu,0u);}
                if(!branchb_emit_75ee_actor(rt,a,i,w,256u,&nw)){ctx->blocked_dependency_rom=0x000075eeu;goto rollback;}
            }
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]+1u));
    }
    if(nw){
        wf_r17_m68k_shadow_t final_regs=rt->m68k_regs;
        if(!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c0000u;goto rollback;}
        if(ctx->fg0_batch(rt,w,nw,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c0000u;goto rollback;}
        rt->m68k_regs=final_regs; /* raw stores themselves do not alter D-registers. */
    }
    return WF_OK;
rollback:
    for(i=0u;i<4u;i++)rt->actors[i]=snap[i];
    rt->m68k_regs=rsnap;
    return WF_BLOCKED;
}

static uint16_t low16(uint32_t v){ return (uint16_t)v; }
static int actor_active(const wf_actor_raw_t *a){ return (wf_u8(a,WF_A_ACTIVE)&0x80u)!=0u; }

wf_status_t wf_r17_owner_8ecc(wf_runtime_t *rt){
    unsigned i;
    wf_actor_raw_t actor_snap[4];
    wf_r17_m68k_shadow_t regs_snap;
    if(!rt||!rt->m68k_regs.valid) return WF_BLOCKED;

    /* $8ECC BTST #0,$1C0161 / BEQ $8F4E. No D-register writes on this exit. */
    if((rt->g.mode_0161&0x01u)==0u) return WF_OK;

    for(i=0u;i<4u;i++) actor_snap[i]=rt->actors[i];
    regs_snap=rt->m68k_regs;

    /* $8EDC MOVE.W #$0003,D0. MOVE.W preserves D0 high 16 bits. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u);

    /* A0=$1C05B0, exactly four actor records, stride $10C, DBF D0. */
    for(i=0u;i<4u;i++){
        wf_actor_raw_t *a=&rt->actors[i];
        uint8_t state1d;

        if(actor_active(a)){
            state1d=wf_u8(a,WF_A_LATCHED+1u);
            if(state1d!=0x0bu && state1d!=0x0cu){
                int excluded=0;
                if(state1d==0xffu){
                    unsigned linked_slot=0u;
                    wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),&linked_slot);
                    (void)linked_slot;
                    /* A live ROM pointer is required only on this branch. */
                    if(!linked){
                        unsigned j; for(j=0u;j<4u;j++) rt->actors[j]=actor_snap[j];
                        rt->m68k_regs=regs_snap;
                        return WF_BLOCKED;
                    }
                    if(wf_u8(linked,WF_A_LATCHED+1u)==0x0cu) excluded=1;
                }
                if(!excluded && state1d==0x05u && wf_u8(a,WF_A_SELECTOR_LO)==0x4au) excluded=1;
                if(!excluded){
                    uint16_t live=wf_be16(a,WF_A_STAMINA);
                    /* $8F1C MOVE.W +$66(A0),D1; high 16 of D1 survive. */
                    wf_r17_m68k_move_w(&rt->m68k_regs,1u,live);
                    /* Exact ROM predicate is inequality, not live < max. */
                    if(live!=wf_be16(a,WF_A_STAMINA_MAX)){
                        uint16_t c=(uint16_t)(wf_be16(a,WF_A_RECOVERY_COUNTER)+1u);
                        wf_set_be16(a,WF_A_RECOVERY_COUNTER,c);
                        if(c>=0x0080u){
                            wf_set_be16(a,WF_A_RECOVERY_COUNTER,0u);
                            wf_set_be16(a,WF_A_STAMINA,(uint16_t)(live+1u));
                            wf_set_be16(a,WF_A_VISIBLE_DELTA,(uint16_t)(wf_be16(a,WF_A_VISIBLE_DELTA)+1u));
                            /* No $24EC2 call here: tier remains untouched. */
                        }
                    }
                }
            }
        }

        /* $8F4A DBF D0,$8EE0: decrement low WORD only. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])-1u));
    }
    return WF_OK;
}

wf_status_t wf_phase_owner_8ecc(wf_runtime_t *rt,uint32_t rom_addr,void *user){
    (void)user;
    if(rom_addr!=0x00008eccu) return WF_BAD_STATE;
    return wf_r17_owner_8ecc(rt);
}

static wf_status_t emit_2052(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,uint16_t raw){
    wf_status_t s;
    if(!ctx||!ctx->audio_2052){ if(ctx)ctx->blocked_dependency_rom=0x00002052u; return WF_BLOCKED; }
    s=ctx->audio_2052(rt,raw,ctx->user);
    if(s!=WF_OK)ctx->blocked_dependency_rom=0x00002052u;
    return s;
}

wf_status_t wf_r17_owner_9052(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    static const uint16_t table_0dee[10]={0x3104u,0x310au,0x310cu,0x310fu,0x3106u,0x3104u,0x310au,0x310cu,0x310fu,0x3106u};
    wf_r17_m68k_shadow_t regs_snap;
    uint16_t g0166_snap;
    unsigned i;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;

    /* $9052 TST.W $1C007C / BEQ $90D4 (RTS). */
    if(rt->g.credited_route==0u)return WF_OK;
    /* $9060 BTST #5,$1C169E: byte at even address => WORD mask $2000. */
    if((rt->g.global_169e&0x2000u)!=0u)return WF_OK;

    regs_snap=rt->m68k_regs;
    g0166_snap=rt->g.global_0166;
    /* $906A MOVE.W #3,D1. */
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,3u);
    for(i=0u;i<4u;i++){
        wf_actor_raw_t *a=&rt->actors[i];
        if((wf_u8(a,WF_A_ACTIVE)&0x80u)!=0u && wf_be16(a,WF_A_STAMINA)<0x0018u){
            /* $907E BTST #7,$1C0166; when already latched, return. */
            if((rt->g.global_0166&0x8000u)!=0u)return WF_OK;
            /* $9088 MOVE.W #$310B,D0; $2052 preserves D0 via MOVEM.L. */
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x310bu);
            if(emit_2052(rt,ctx,0x310bu)!=WF_OK){
                rt->m68k_regs=regs_snap;rt->g.global_0166=g0166_snap;return WF_BLOCKED;
            }
            rt->g.global_0166=0x8000u;
            return WF_OK;
        }
        /* $909E DBF D1,$9070: low WORD only. */
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]-1u));
    }

    /* $90A4 BTST #6,$1C0166: byte at even address => WORD mask $4000. */
    if((rt->g.global_0166&0x4000u)!=0u)return WF_OK;
    rt->g.global_0166=0x4000u;

    /* $90BC..$90CC: low byte of progression indexes WORD table $0DEE. */
    {
        uint16_t idx=(uint16_t)(rt->g.progression_0162&0x00ffu);
        uint16_t raw;
        if(idx>=10u){
            rt->m68k_regs=regs_snap;rt->g.global_0166=g0166_snap;ctx->blocked_dependency_rom=0x00000deeu;return WF_BLOCKED;
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,rt->g.progression_0162);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]&0x00ffu));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]<<1));
        raw=table_0dee[idx];
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,raw);
        if(emit_2052(rt,ctx,raw)!=WF_OK){
            rt->m68k_regs=regs_snap;rt->g.global_0166=g0166_snap;return WF_BLOCKED;
        }
    }
    return WF_OK;
}

wf_status_t wf_r17_owner_899c(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    static const uint16_t row_desc[13][2]={
        {0x0700u,0x0707u},{0x070eu,0x0715u},{0x031cu,0x031fu},{0x0522u,0x0527u},
        {0x052cu,0x0531u},{0x0836u,0x083eu},{0x0846u,0x084eu},{0x0856u,0x085eu},
        {0x0766u,0x076du},{0x0774u,0x077bu},{0x0782u,0x0789u},{0x0590u,0x0596u},
        {0x0195u,0x019bu}
    };
    static const int32_t row_adjust[13]={0,8,-4,0,-8,0,0,4,-4,0,0,-232,0};
    wf_r17_fg0_write_t writes[156];
    wf_r17_m68k_shadow_t regs_snap;
    uint8_t g167a_snap,g167b_snap;
    uint16_t g00aa_snap,g00b2_snap;
    uint32_t a0=0x000c1008u;
    size_t nw=0u;
    unsigned row;

    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    /* $899C BTST #0,$1C167B / BNE return: no D-register writes. */
    if((rt->g.global_167b&0x01u)!=0u)return WF_OK;

    regs_snap=rt->m68k_regs;g167a_snap=rt->g.global_167a;g167b_snap=rt->g.global_167b;
    g00aa_snap=rt->g.global_00aa;g00b2_snap=rt->g.global_00b2;

    /* $89AC: if current bit7 is clear, previous bit7 must be set; then BSET current bit6. */
    if((rt->g.global_167a&0x80u)==0u){
        if((rt->g.global_167b&0x80u)==0u)return WF_OK;
        rt->g.global_167a=(uint8_t)(rt->g.global_167a|0x40u);
    }
    rt->g.global_00aa=(uint16_t)(rt->g.global_00aa+1u);
    if(rt->g.global_00aa>=2u){rt->g.global_00aa=0u;rt->g.global_00b2^=0x0001u;}

    /* $89F2 MOVE.W #0,D0; each row re-enters at $89F6 CLR.L D1. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
    for(row=0u;row<13u;row++){
        uint16_t desc=row_desc[row][(rt->g.global_00b2&0x00ffu)!=0u ? 1u:0u];
        uint16_t tile=(uint16_t)(desc&0x00ffu);
        unsigned count=(unsigned)(desc>>8);
        unsigned col;
        wf_r17_m68k_clr_l(&rt->m68k_regs,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)rt->m68k_regs.d[0]);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<2));
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,desc);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,desc);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((int16_t)(uint16_t)rt->m68k_regs.d[2]>>8));
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((uint16_t)rt->m68k_regs.d[2]-1u));
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,tile);
        for(col=0u;col<count;col++){
            uint32_t addr=a0+(uint32_t)col*4u;
            if((rt->g.global_167a&0x40u)!=0u){
                if(nw>=sizeof(writes)/sizeof(writes[0]))goto internal_block;
                writes[nw++]=(wf_r17_fg0_write_t){addr,0xff00ff00u,4u};
            }else{
                if(nw+2u>sizeof(writes)/sizeof(writes[0]))goto internal_block;
                writes[nw++]=(wf_r17_fg0_write_t){addr+1u,(uint32_t)(uint8_t)rt->m68k_regs.d[1],1u};
                writes[nw++]=(wf_r17_fg0_write_t){addr+3u,0xe4u,1u};
            }
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]+1u));
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((uint16_t)rt->m68k_regs.d[2]-1u));
        }
        a0=(uint32_t)((int64_t)a0+0x100+(int64_t)row_adjust[row]);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)rt->m68k_regs.d[0]);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<2));
        wf_r17_m68k_move_l(&rt->m68k_regs,1u,(uint32_t)row_adjust[row]);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]+1u));
    }
    if(!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c1008u;goto rollback;}
    if(ctx->fg0_batch(rt,writes,nw,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c1008u;goto rollback;}
    rt->g.global_167b=rt->g.global_167a;
    rt->g.global_167a=0u;
    return WF_OK;
internal_block:
    ctx->blocked_dependency_rom=0x0000899cu;
rollback:
    rt->m68k_regs=regs_snap;rt->g.global_167a=g167a_snap;rt->g.global_167b=g167b_snap;
    rt->g.global_00aa=g00aa_snap;rt->g.global_00b2=g00b2_snap;
    return WF_BLOCKED;
}

static uint8_t bcd_dec1(uint8_t v,int *borrow){
    uint8_t hi=(uint8_t)((v>>4)&0x0fu),lo=(uint8_t)(v&0x0fu);
    *borrow=0;
    if(lo>0u){lo--;return (uint8_t)((hi<<4)|lo);}
    lo=9u;
    if(hi>0u){hi--;return (uint8_t)((hi<<4)|lo);}
    *borrow=1;return 0x99u;
}
static int push_long(wf_r17_fg0_write_t *w,size_t cap,size_t *n,uint32_t a,uint32_t v){
    if(*n>=cap){
        return 0;
    }
    w[*n]=(wf_r17_fg0_write_t){a,v,4u};
    (*n)++;
    return 1;
}
static int timer_render_digits(wf_runtime_t *rt,wf_r17_fg0_write_t *w,size_t cap,size_t *n){
    const uint16_t vals[4]={
        (uint16_t)((rt->g.global_16a0>>4)&0x0fu),(uint16_t)(rt->g.global_16a0&0x0fu),
        (uint16_t)((rt->g.global_16a2>>4)&0x0fu),(uint16_t)(rt->g.global_16a2&0x0fu)};
    const uint16_t pos[4]={0u,1u,3u,4u};
    unsigned i;
    for(i=0u;i<4u;i++){
        uint32_t a=0x000c1d1cu+(uint32_t)pos[i]*4u;
        uint32_t up=((uint32_t)(0x00a8u+vals[i])<<16)|0x00e4u;
        uint32_t dn=((uint32_t)(0x00b3u+vals[i])<<16)|0x00e4u;
        if(!push_long(w,cap,n,a,up)||!push_long(w,cap,n,a+0x100u,dn))return 0;
        /* $2647C leaves D1 as byte offset (selector<<2) after ANDI.L #$FF. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,vals[i]);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,pos[i]);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<2));
        wf_r17_m68k_move_l(&rt->m68k_regs,1u,(uint32_t)((uint16_t)rt->m68k_regs.d[1]&0x00ffu));
        wf_r17_m68k_move_l(&rt->m68k_regs,2u,0x00e400a8u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((uint16_t)rt->m68k_regs.d[2]+vals[i]));
        {uint32_t v=rt->m68k_regs.d[2];wf_r17_m68k_move_l(&rt->m68k_regs,2u,(v<<16)|(v>>16));}
        wf_r17_m68k_move_l(&rt->m68k_regs,2u,0x00e400b3u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((uint16_t)rt->m68k_regs.d[2]+vals[i]));
        {uint32_t v=rt->m68k_regs.d[2];wf_r17_m68k_move_l(&rt->m68k_regs,2u,(v<<16)|(v>>16));}
    }
    return 1;
}
wf_status_t wf_r17_owner_262d2(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_r17_fg0_write_t writes[24];
    wf_r17_fg0_write_t video_writes[2];
    size_t nw=0u;
    size_t nvideo=0u;
    int render=0;
    int audio05=0;
    uint8_t a0_snap[WF_ACTOR_SLOTS];uint16_t fe_snap[WF_ACTOR_SLOTS];
    wf_r17_m68k_shadow_t regs_snap;
    wf_globals_raw_t g_snap;
    unsigned i;

    if(!rt||!ctx||!rt->m68k_regs.valid){
        return WF_BLOCKED;
    }
    ctx->blocked_dependency_rom=0u;

    /* $262D2 TST.W $1C007C. */
    if(rt->g.credited_route==0u){
        return WF_OK;
    }
    /* $262DE BTST #4,$1C169E / BNE exit. Byte is at even address. */
    if((rt->g.global_169e&0x1000u)!=0u){
        return WF_OK;
    }
    /* $262E8 BTST #7,$1C169E / BEQ exit. */
    if((rt->g.global_169e&0x8000u)==0u){
        return WF_OK;
    }

    regs_snap=rt->m68k_regs;
    g_snap=rt->g;
    for(i=0u;i<WF_ACTOR_SLOTS;i++){a0_snap[i]=wf_u8(&rt->actors[i],0xa0u);fe_snap[i]=wf_be16(&rt->actors[i],0xfeu);}

    /* $262F2 BSET #6,$1C169E. Initialize only on the first eligible call. */
    if((rt->g.global_169e&0x4000u)==0u){
        uint16_t up=0x009cu;
        uint16_t dn=0x00a2u;
        rt->g.global_169e|=0x4000u;

        /* $26300..$26332: six TIME cells on both text rows. */
        wf_r17_m68k_move_l(&rt->m68k_regs,0u,0x00e4009cu);
        wf_r17_m68k_move_l(&rt->m68k_regs,1u,0x00e400a2u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,5u);
        for(i=0u;i<6u;i++){
            uint32_t v;
            if(!push_long(writes,24u,&nw,0x000c1d04u+(uint32_t)i*4u,((uint32_t)up<<16)|0x00e4u) ||
               !push_long(writes,24u,&nw,0x000c1e04u+(uint32_t)i*4u,((uint32_t)dn<<16)|0x00e4u)){
                goto internal_block;
            }
            v=rt->m68k_regs.d[0];
            wf_r17_m68k_move_l(&rt->m68k_regs,0u,(v<<16)|(v>>16));
            v=rt->m68k_regs.d[1];
            wf_r17_m68k_move_l(&rt->m68k_regs,1u,(v<<16)|(v>>16));
            /* ROM stores the swapped LONGs here. */
            v=rt->m68k_regs.d[0];
            wf_r17_m68k_move_l(&rt->m68k_regs,0u,(v<<16)|(v>>16));
            v=rt->m68k_regs.d[1];
            wf_r17_m68k_move_l(&rt->m68k_regs,1u,(v<<16)|(v>>16));
            wf_r17_m68k_move_l(&rt->m68k_regs,0u,rt->m68k_regs.d[0]+1u);
            wf_r17_m68k_move_l(&rt->m68k_regs,1u,rt->m68k_regs.d[1]+1u);
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((uint16_t)rt->m68k_regs.d[2]-1u));
            up++;
            dn++;
        }
        if(!push_long(writes,24u,&nw,0x000c1d24u,0x00b200e4u) ||
           !push_long(writes,24u,&nw,0x000c1e24u,0x00b200e4u)){
            goto internal_block;
        }

        /* $26362/$2636A are arcade sprite-RAM writes, not FG0. Keep renderer boundary. */
        video_writes[nvideo++]=(wf_r17_fg0_write_t){0x0018071cu,0x00ffu,2u};
        video_writes[nvideo++]=(wf_r17_fg0_write_t){0x0018071eu,0x000fu,2u};

        rt->g.global_16a4=1u;
        rt->g.global_16a2=1u;
        rt->g.global_16a0=0x0030u;
    }

    /* $26370: SUBQ.W #1,$1C16A4. */
    rt->g.global_16a4=(uint16_t)(rt->g.global_16a4-1u);
    if(rt->g.global_16a4!=0u){
        return WF_OK;
    }
    rt->g.global_16a4=4u;

    /* $26382..: clear X/C, SBCD #1 from BCD seconds. */
    {
        int bsec=0;
        uint8_t sec=(uint8_t)rt->g.global_16a2;
        uint8_t nsec;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.global_16a2);
        nsec=bcd_dec1(sec,&bsec);
        wf_r17_m68k_move_b(&rt->m68k_regs,1u,nsec);
        if(!bsec){
            rt->g.global_16a2=(uint16_t)((rt->g.global_16a2&0xff00u)|nsec);
        }else{
            int bmin=0;
            uint8_t min=(uint8_t)rt->g.global_16a0;
            uint8_t nmin;
            rt->g.global_16a2=(uint16_t)((rt->g.global_16a2&0xff00u)|0x59u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,1u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.global_16a0);
            nmin=bcd_dec1(min,&bmin);
            wf_r17_m68k_move_b(&rt->m68k_regs,1u,nmin);
            if(bmin){
                /* $263D8-$2641E: TIME UP precedes the exact hardware wait;
                 * the result flags are written only after all 65 Vblanks. */
                if(!ctx->raw_2503c){ctx->blocked_dependency_rom=0x2503cu;goto rollback;}
                if(!ctx->wait_21e6){ctx->blocked_dependency_rom=0x21e6u;goto rollback;}
                if(!ctx->fg0_batch){ctx->blocked_dependency_rom=0xc1d04u;goto rollback;}
                rt->g.global_169e|=0x1000u;rt->g.global_16a0=0u;rt->g.global_16a2=0u;
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,19u);
                wf_r17_m68k_shadow_t rs=rt->m68k_regs;wf_status_t status=ctx->raw_2503c(rt,19u,ctx->user);rt->m68k_regs=rs;
                if(status!=WF_OK){ctx->blocked_dependency_rom=0x2503cu;goto rollback;}
                rt->g.global_16c4=(uint16_t)((rt->g.global_16c4&0xff00u)|3u);
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,64u);rs=rt->m68k_regs;status=ctx->wait_21e6(rt,64u,ctx->user);rt->m68k_regs=rs;
                if(status!=WF_OK){ctx->blocked_dependency_rom=0x21e6u;goto rollback;}
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
                for(i=0u;i<WF_ACTOR_SLOTS;i++){
                    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(i+1u));
                    if(actor_active(&rt->actors[i])){wf_set_u8(&rt->actors[i],0xa0u,0x80u);wf_set_be16(&rt->actors[i],0xfeu,0x8005u);}
                }
            }else{
                rt->g.global_16a0=(uint16_t)((rt->g.global_16a0&0xff00u)|nmin);
                if(nmin==0x05u){rt->g.global_169e|=0x2000u;audio05=1;}
            }
        }
    }

    render=1;
    if(render&&!timer_render_digits(rt,writes,24u,&nw)){
        goto internal_block;
    }

    /* Preflight every external owner needed by this ROM path before committing side effects. */
    if(nvideo!=0u&&!ctx->video_batch){
        ctx->blocked_dependency_rom=0x0018071cu;
        goto rollback;
    }
    if(nw!=0u&&!ctx->fg0_batch){
        ctx->blocked_dependency_rom=0x000c1d04u;
        goto rollback;
    }
    if(audio05&&!ctx->audio_2052){
        ctx->blocked_dependency_rom=0x00002052u;
        goto rollback;
    }

    /* ROM ordering on the 05-minute transition: audio precedes digit rendering. */
    if(audio05&&emit_2052(rt,ctx,0x000bu)!=WF_OK){
        goto rollback;
    }
    if(nvideo!=0u&&ctx->video_batch(rt,video_writes,nvideo,ctx->user)!=WF_OK){
        ctx->blocked_dependency_rom=0x0018071cu;
        goto rollback;
    }
    if(nw!=0u&&ctx->fg0_batch(rt,writes,nw,ctx->user)!=WF_OK){
        ctx->blocked_dependency_rom=0x000c1d04u;
        goto rollback;
    }
    return WF_OK;

internal_block:
    ctx->blocked_dependency_rom=0x000262d2u;
rollback:
    rt->m68k_regs=regs_snap;
    rt->g=g_snap;
    for(i=0u;i<WF_ACTOR_SLOTS;i++){wf_set_u8(&rt->actors[i],0xa0u,a0_snap[i]);wf_set_be16(&rt->actors[i],0xfeu,fe_snap[i]);}
    return WF_BLOCKED;
}


/* ---- Post-$7A00 owners recovered from the canonical main68k ROM. ---- */
static wf_status_t post7_call_word_preserve_regs(wf_runtime_t *rt,wf_r17_raw_word_fn fn,uint16_t raw,void *user){
    wf_r17_m68k_shadow_t snap;
    wf_status_t s;
    if(!rt||!fn)return WF_BLOCKED;
    snap=rt->m68k_regs;
    s=fn(rt,raw,user);
    rt->m68k_regs=snap; /* $2AEA/$2B58/$2503C do not donate caller-visible D-register clobbers here. */
    return s;
}

wf_status_t wf_r17_owner_bcfe(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    uint16_t status_snap[4];wf_r17_m68k_shadow_t rsnap;wf_r17_fg0_write_t fg[48],video[1];size_t nfg=0u,nvideo=0u;unsigned i;
    uint32_t d1_hi;uint16_t raw2503c=0u;uint8_t do2503c=0u;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if((rt->g.mode_0161&0x01u)==0u)return WF_OK;
    memcpy(status_snap,rt->g.global_0156,sizeof(status_snap));rsnap=rt->m68k_regs;d1_hi=rt->m68k_regs.d[1]&0xffff0000u;
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
    for(i=0u;i<4u;i++){
        uint16_t v=rt->g.global_0156[i];
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)rt->m68k_regs.d[0]);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<1));
        if((v&0x0001u)!=0u){
            uint8_t hi=(uint8_t)(v>>8);hi=(uint8_t)(hi-1u);v=(uint16_t)(((uint16_t)hi<<8)|(v&0x00ffu));rt->g.global_0156[i]=v;
            if(hi==0u){
                /* $BD5A: CLR.W status; $BD5E writes $180012 before $BD6A clears FG0. */
                rt->g.global_0156[i]=0u;video[nvideo++]=(wf_r17_fg0_write_t){0x00180012u,0x06ffu,2u};
                for(unsigned row=0u;row<2u;row++)for(unsigned j=0u;j<24u;j++)fg[nfg++]=(wf_r17_fg0_write_t){0x000c0520u+(uint32_t)row*0x100u+(uint32_t)j*4u,0u,4u};
                wf_r17_m68k_moveq(&rt->m68k_regs,0u,1);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu);rt->m68k_regs.d[1]=d1_hi|0xffffu;
            }else{
                /* $BD36/$BD88 writes $180012 before either $2503C or $BD6A. */
                video[nvideo++]=(wf_r17_fg0_write_t){0x00180012u,((rt->g.pipeline_mode&0x02u)!=0u)?0x0fffu:0x000fu,2u};
                if((rt->g.pipeline_mode&0x08u)==0u){
                    raw2503c=(uint16_t)((uint16_t)rt->m68k_regs.d[0]+0x004cu);do2503c=1u;
                }else{
                    for(unsigned row=0u;row<2u;row++)for(unsigned j=0u;j<24u;j++)fg[nfg++]=(wf_r17_fg0_write_t){0x000c0520u+(uint32_t)row*0x100u+(uint32_t)j*4u,0u,4u};
                    wf_r17_m68k_moveq(&rt->m68k_regs,0u,1);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu);rt->m68k_regs.d[1]=d1_hi|0xffffu;
                }
            }
            break;
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]+1u));
    }
    /* Preflight every external owner reached by the selected ROM path. */
    if(nvideo&&!ctx->video_batch){ctx->blocked_dependency_rom=0x00180012u;goto rollback;}
    if(nfg&&!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c0520u;goto rollback;}
    if(do2503c&&!ctx->raw_2503c){ctx->blocked_dependency_rom=0x0002503cu;goto rollback;}
    {
        wf_r17_m68k_shadow_t final_regs=rt->m68k_regs;
        /* Exact observable order: raw $180012 first, then helper/FG0 side effect. */
        if(nvideo&&ctx->video_batch(rt,video,nvideo,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00180012u;goto rollback;}
        rt->m68k_regs=final_regs;
        if(do2503c){
            if(post7_call_word_preserve_regs(rt,ctx->raw_2503c,raw2503c,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x0002503cu;goto rollback;}
            rt->m68k_regs=final_regs; /* $BD42 MOVEM D0 around $2503C. */
        }
        if(nfg&&ctx->fg0_batch(rt,fg,nfg,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c0520u;goto rollback;}
        rt->m68k_regs=final_regs;
    }
    return WF_OK;
rollback:
    memcpy(rt->g.global_0156,status_snap,sizeof(status_snap));rt->m68k_regs=rsnap;return WF_BLOCKED;
}


static uint8_t post7_word_hi(uint16_t v){return (uint8_t)(v>>8);}
static uint8_t post7_word_lo(uint16_t v){return (uint8_t)v;}
static uint16_t post7_word_set_hi(uint16_t v,uint8_t b){return (uint16_t)(((uint16_t)b<<8)|(v&0x00ffu));}
static uint16_t post7_word_set_lo(uint16_t v,uint8_t b){return (uint16_t)((v&0xff00u)|b);}

/* ROM $C404 credit/config predicate. Return value is the 68000 carry result. */
static int post7_c404_carry(wf_runtime_t *rt,uint16_t code){
    uint8_t c=(uint8_t)code;
    uint8_t config_lo=(uint8_t)rt->g.config_0066;
    if(c==0u)return rt->g.credit_count_004f!=0u;
    if(c==2u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u);}
    else if(c==5u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,2u);}
    else if(c==1u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,4u);}
    else if(c==3u){
        if((config_lo&0x10u)!=0u)return rt->g.credit_count_004f>=2u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(rt->g.config_0066&0x0003u));
        if(low16(rt->m68k_regs.d[0])==0u)return rt->g.credit_count_004f>=2u;
        return rt->g.credit_remainder_004e>=2u || rt->g.credit_count_004f!=0u;
    }else return 0;
    if((config_lo&(uint8_t)(1u<<(low16(rt->m68k_regs.d[0])&7u)))!=0u)
        return rt->g.credit_count_004f!=0u;
    return rt->g.credit_remainder_004e!=0u || rt->g.credit_count_004f!=0u;
}

static size_t post7_c172_build_clear(wf_runtime_t *rt,wf_r17_fg0_write_t *w,size_t cap){
    size_t n=0u;unsigned row,col;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x000cu);
    for(row=0u;row<13u;row++){
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0x0014u);
        for(col=0u;col<21u;col++){
            if(n>=cap)return 0u;
            w[n++]=(wf_r17_fg0_write_t){0x000c1004u+(uint32_t)row*0x100u+(uint32_t)col*4u,0u,4u};
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(low16(rt->m68k_regs.d[1])-1u));
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])-1u));
    }
    /* $C198 CLR.B $1C16CC: clear high-byte flags, preserve $16CD. */
    rt->g.global_16cc=(uint16_t)(rt->g.global_16cc&0x00ffu);
    return n;
}

static size_t post7_c23a_selector4(wf_runtime_t *rt,wf_r17_fg0_write_t *w,size_t cap){
    static const uint16_t src_2cc2[16]={
        0x0000u,0x0000u,0x0003u,0x0027u,0x004au,0x027du,0x04afu,0x06cfu,
        0x09efu,0x0069u,0x0eeeu,0x03dfu,0x000du,0x0888u,0x009du,0x0fd0u
    };
    size_t n=0u;int i;
    /* $C114 MOVEQ #4,D2; $C23A ANDI.L #$F,D2; selector4 -> ROM ptr $2CC2. */
    wf_r17_m68k_moveq(&rt->m68k_regs,2u,4);
    wf_r17_m68k_move_l(&rt->m68k_regs,2u,rt->m68k_regs.d[2]&0x0000000fu);
    wf_r17_m68k_move_l(&rt->m68k_regs,3u,rt->m68k_regs.d[2]);
    wf_r17_m68k_move_w(&rt->m68k_regs,3u,(uint16_t)(low16(rt->m68k_regs.d[3])<<7));
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(low16(rt->m68k_regs.d[2])<<2));
    wf_r17_m68k_moveq(&rt->m68k_regs,2u,15);
    for(i=15;i>=0;i--){
        uint16_t off=(uint16_t)((uint16_t)i<<1);
        wf_r17_m68k_move_w(&rt->m68k_regs,3u,off);
        if(n>=cap)return 0u;
        w[n++]=(wf_r17_fg0_write_t){0x00180300u+(uint32_t)off,src_2cc2[i],2u};
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(low16(rt->m68k_regs.d[2])-1u));
    }
    return n;
}

static wf_status_t post7_call_9aca_preserve_regs(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,uint16_t d2,uint16_t d7,uint32_t dest){
    wf_r17_m68k_shadow_t rs;wf_status_t s;
    if(!ctx->raw_9aca){ctx->blocked_dependency_rom=0x00009acau;return WF_BLOCKED;}
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);wf_r17_m68k_move_w(&rt->m68k_regs,7u,d7);
    rs=rt->m68k_regs;s=ctx->raw_9aca(rt,d2,d7,dest,ctx->user);rt->m68k_regs=rs;
    if(s!=WF_OK){ctx->blocked_dependency_rom=0x00009acau;return WF_BLOCKED;}
    return WF_OK;
}

static wf_status_t post7_c0f8(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_r17_fg0_write_t fg[273],video[16];size_t nfg=0u,nvideo=0u;
    uint8_t hi=post7_word_hi(rt->g.global_16cc),lo=(uint8_t)(post7_word_lo(rt->g.global_16cc)-1u);
    int need_clear,need_video;
    rt->g.global_16cc=post7_word_set_lo(rt->g.global_16cc,lo);
    if((int8_t)lo<0){
        if(!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c1004u;return WF_BLOCKED;}
        nfg=post7_c172_build_clear(rt,fg,273u);if(nfg!=273u){ctx->blocked_dependency_rom=0x0000c172u;return WF_BLOCKED;}
        if(ctx->fg0_batch(rt,fg,nfg,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c1004u;return WF_BLOCKED;}
        rt->g.global_167b=(uint8_t)(rt->g.global_167b&~0x01u);
        return WF_OK;
    }
    need_clear=(hi&0x80u)==0u;
    need_video=need_clear;
    if(need_clear&&!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c1004u;return WF_BLOCKED;}
    if(need_video&&!ctx->video_batch){ctx->blocked_dependency_rom=0x00180300u;return WF_BLOCKED;}
    if(!ctx->raw_9aca){ctx->blocked_dependency_rom=0x00009acau;return WF_BLOCKED;}
    if(need_clear){
        nfg=post7_c172_build_clear(rt,fg,273u);if(nfg!=273u){ctx->blocked_dependency_rom=0x0000c172u;return WF_BLOCKED;}
        if(ctx->fg0_batch(rt,fg,nfg,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c1004u;return WF_BLOCKED;}
        nvideo=post7_c23a_selector4(rt,video,16u);if(nvideo!=16u){ctx->blocked_dependency_rom=0x0000c23au;return WF_BLOCKED;}
        if(ctx->video_batch(rt,video,nvideo,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00180300u;return WF_BLOCKED;}
        hi=post7_word_hi(rt->g.global_16cc); /* C172 cleared it. */
    }
    {
        uint8_t old6=(uint8_t)(hi&0x40u);hi=(uint8_t)(hi|0x40u);rt->g.global_16cc=post7_word_set_hi(rt->g.global_16cc,hi);
        if(old6==0u){if(post7_call_9aca_preserve_regs(rt,ctx,0x0011u,0x0010u,0x000c1804u)!=WF_OK)return WF_BLOCKED;}
    }
    if(post7_call_9aca_preserve_regs(rt,ctx,(lo&0x04u)?0x0012u:0x0013u,0x0060u,0x000c1224u)!=WF_OK)return WF_BLOCKED;
    hi=post7_word_hi(rt->g.global_16cc);rt->g.global_16cc=post7_word_set_hi(rt->g.global_16cc,(uint8_t)(hi|0x80u));
    return WF_OK;
}

wf_status_t wf_r17_owner_c04a(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_r17_m68k_shadow_t rsnap;uint8_t g167b_snap;uint16_t g16ca_snap,g16cc_snap;unsigned i;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if((rt->g.global_16c4&0x00ffu)!=0u)return WF_OK;
    if(wf_be16(&rt->actors[0],WF_A_RESULT_ROLE)!=0u)return WF_OK; /* $1C06AE = actor0+$FE */
    if(wf_be16(&rt->actors[2],WF_A_RESULT_ROLE)!=0u)return WF_OK; /* $1C08C6 = actor2+$FE */
    if((rt->g.global_167b&0x80u)!=0u || (rt->g.global_167b&0x02u)!=0u)return WF_OK;
    rsnap=rt->m68k_regs;g167b_snap=rt->g.global_167b;g16ca_snap=rt->g.global_16ca;g16cc_snap=rt->g.global_16cc;
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,2);
    if(post7_c404_carry(rt,2u)){
        if((post7_word_hi(rt->g.global_16cc)&0x80u)!=0u){
            wf_r17_fg0_write_t fg[273];size_t n;
            if(!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c1004u;goto rollback;}
            n=post7_c172_build_clear(rt,fg,273u);if(n!=273u){ctx->blocked_dependency_rom=0x0000c172u;goto rollback;}
            if(ctx->fg0_batch(rt,fg,n,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c1004u;goto rollback;}
        }
        return WF_OK;
    }
    if((rt->g.global_167b&0x01u)==0u){
        rt->g.global_16ca=(uint16_t)(rt->g.global_16ca-1u);
        if((int8_t)post7_word_lo(rt->g.global_16ca)>=0)return WF_OK;
        wf_r17_m68k_moveq(&rt->m68k_regs,0u,3);
        for(i=0u;i<4u;i++){
            wf_actor_raw_t *a=&rt->actors[i];
            if((wf_u8(a,WF_A_ACTIVE)&0x80u)!=0u && (wf_u8(a,WF_A_ROLE)&0x80u)==0u && wf_u8(a,WF_A_STAMINA+1u)<0x24u){
                rt->g.global_167b=(uint8_t)(rt->g.global_167b|0x01u);rt->g.global_16ca=0x0080u;rt->g.global_16cc=0x0030u;
                if(post7_c0f8(rt,ctx)!=WF_OK)goto rollback;
                return WF_OK;
            }
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])-1u));
        }
        return WF_OK;
    }
    if(post7_c0f8(rt,ctx)!=WF_OK)goto rollback;
    return WF_OK;
rollback:
    rt->m68k_regs=rsnap;rt->g.global_167b=g167b_snap;rt->g.global_16ca=g16ca_snap;rt->g.global_16cc=g16cc_snap;return WF_BLOCKED;
}


/* ROM $C710 message/status family. $7680 maps slot 0..3 to the same
 * four FG0 bases used by the closed WP18 $7548 contract. */
static void post7_c710_inc_slot(wf_runtime_t *rt){rt->g.global_16ce=(uint16_t)(rt->g.global_16ce+1u);}
static wf_status_t post7_c710_emit(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,wf_actor_raw_t *a,unsigned slot,size_t cache_off,uint16_t selector,uint32_t delta){
    uint32_t dest;
    if(wf_u8(a,cache_off)==(uint8_t)selector)return WF_OK;
    if(!ctx->raw_9aca){ctx->blocked_dependency_rom=0x00009acau;return WF_BLOCKED;}
    wf_set_u8(a,cache_off,(uint8_t)selector);
    /* $7680 preserves D0, so the caller-visible D0 is the selected slot. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,rt->g.global_16ce);
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,selector);
    wf_r17_m68k_move_w(&rt->m68k_regs,7u,0x0010u);
    dest=branchb_fg0_base(slot)+delta;
    return post7_call_9aca_preserve_regs(rt,ctx,selector,0x0010u,dest);
}
static wf_status_t post7_c710_c892(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,wf_actor_raw_t *a,unsigned slot){
    static const uint8_t swap[4]={1u,0u,3u,2u};
    uint16_t d2;uint32_t input=wf_be32(a,WF_A_INPUT_SOURCE);int flag=(wf_u8(a,WF_A_FLAGS33)&0x02u)!=0;
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,rt->g.config_0066);
    d2=(uint16_t)(low16(rt->m68k_regs.d[2])&0x0c00u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);
    if(d2==0x0400u){
        if(rt->g.global_16ce>=2u)wf_r17_m68k_moveq(&rt->m68k_regs,3u,2);
        else if((rt->g.mode_0161&1u)==0u){
            uint32_t expected=0x00140020u+(uint32_t)slot*2u;int remap=flag?(input!=expected):(input==expected);
            if(remap){uint8_t idx=(uint8_t)rt->m68k_regs.d[3];if(idx>=4u){ctx->blocked_dependency_rom=0x0000cb98u;return WF_BLOCKED;}wf_r17_m68k_move_b(&rt->m68k_regs,3u,swap[idx]);}
        }
    }else if(d2==0x0800u){
        if((rt->g.mode_0161&1u)==0u){
            if((rt->g.mode_0160&0x80u)!=0u){
                if(rt->g.global_16ce>=2u)wf_r17_m68k_moveq(&rt->m68k_regs,3u,input==0x00140020u?0:1);
                else wf_r17_m68k_moveq(&rt->m68k_regs,3u,input==0x00140022u?1:0);
            }else if((rt->g.mode_0160&0x40u)!=0u){
                wf_r17_m68k_moveq(&rt->m68k_regs,3u,rt->g.global_16ce>=2u?1:0);
            }else{
                if(rt->g.global_16ce>=2u)wf_r17_m68k_moveq(&rt->m68k_regs,3u,flag?1:0);
                else wf_r17_m68k_moveq(&rt->m68k_regs,3u,flag?0:1);
            }
        }
    }else if((rt->g.mode_0161&1u)==0u){
        uint32_t expected=0x00140020u+(uint32_t)slot*2u;int remap=flag?(input!=expected):(input==expected);
        if(remap){uint8_t idx=(uint8_t)rt->m68k_regs.d[3];if(idx>=4u){ctx->blocked_dependency_rom=0x0000cb98u;return WF_BLOCKED;}wf_r17_m68k_move_b(&rt->m68k_regs,3u,swap[idx]);}
    }
    wf_r17_m68k_move_w(&rt->m68k_regs,3u,(uint16_t)(low16(rt->m68k_regs.d[3])+0x0022u));
    return post7_c710_emit(rt,ctx,a,slot,0xc2u,low16(rt->m68k_regs.d[3]),0x000cu);
}
static wf_status_t post7_c710_c9be(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,wf_actor_raw_t *a,unsigned slot){
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.config_0066);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(low16(rt->m68k_regs.d[1])&0x0800u));
    if(low16(rt->m68k_regs.d[1])==0x0800u && (rt->g.mode_0161&1u)==0u)
        wf_r17_m68k_moveq(&rt->m68k_regs,3u,low16(rt->m68k_regs.d[3])>=2u?1:0);
    wf_r17_m68k_move_w(&rt->m68k_regs,3u,(uint16_t)(low16(rt->m68k_regs.d[3])+0x001cu));
    return post7_c710_emit(rt,ctx,a,slot,0xc2u,low16(rt->m68k_regs.d[3]),0u);
}
static wf_status_t post7_c710_fixed(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,wf_actor_raw_t *a,unsigned slot,size_t cache_off,uint16_t selector,uint32_t delta){
    wf_r17_m68k_move_w(&rt->m68k_regs,3u,selector);
    return post7_c710_emit(rt,ctx,a,slot,cache_off,selector,delta);
}
static wf_status_t post7_c710_ca10(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,wf_actor_raw_t *a,unsigned slot){
    return post7_c710_fixed(rt,ctx,a,slot,0xc3u,(wf_u8(a,0xc2u)&1u)?0x001au:0x001bu,0x0204u);
}
static wf_status_t post7_c710_cb42(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,unsigned slot){
    wf_r17_fg0_write_t w[36];uint32_t base=branchb_fg0_base(slot),v=0u;size_t n=0u;unsigned row,col;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,rt->g.global_16ce);
    if(!ctx->fg0_peek_long){ctx->blocked_dependency_rom=base+0x000cu;return WF_BLOCKED;}
    if(ctx->fg0_peek_long(rt,base+0x000cu,&v,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=base+0x000cu;return WF_BLOCKED;}
    if(v==0u)return WF_OK;
    if(!ctx->fg0_batch){ctx->blocked_dependency_rom=base;return WF_BLOCKED;}
    wf_r17_m68k_moveq(&rt->m68k_regs,2u,3);
    for(row=0u;row<4u;row++){
        wf_r17_m68k_moveq(&rt->m68k_regs,3u,8);
        for(col=0u;col<9u;col++){
            w[n++]=(wf_r17_fg0_write_t){base+(uint32_t)row*0x100u+(uint32_t)col*4u,0u,4u};
            wf_r17_m68k_move_w(&rt->m68k_regs,3u,(uint16_t)(low16(rt->m68k_regs.d[3])-1u));
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(low16(rt->m68k_regs.d[2])-1u));
    }
    if(ctx->fg0_batch(rt,w,n,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=base;return WF_BLOCKED;}
    return WF_OK;
}

wf_status_t wf_r17_owner_c710(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_r17_m68k_shadow_t rsnap;wf_actor_raw_t asnap[4];uint16_t slot_snap;unsigned slot,i;wf_status_t st;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if(rt->g.global_16c4!=0u || rt->g.credited_route==0u)return WF_OK;
    rsnap=rt->m68k_regs;slot_snap=rt->g.global_16ce;for(i=0u;i<4u;i++)asnap[i]=rt->actors[i];
    if(post7_word_lo(rt->g.global_16ce)>=4u)rt->g.global_16ce=0u;
    if(rt->g.global_16ce>=4u){ctx->blocked_dependency_rom=0x0000c740u;goto rollback;}
    wf_r17_m68k_move_w(&rt->m68k_regs,3u,rt->g.global_16ce);slot=(unsigned)rt->g.global_16ce;
    if((wf_u8(&rt->actors[slot],0xceu)&0x80u)!=0u){post7_c710_inc_slot(rt);return WF_OK;}
    if((wf_u8(&rt->actors[slot],WF_A_ACTIVE)&0x80u)==0u){
        uint16_t d1;
        if((rt->g.mode_0161&1u)!=0u){
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.config_0066);d1=(uint16_t)(low16(rt->m68k_regs.d[1])&0x0c00u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
            if(d1==0x0800u && rt->g.global_16ce>=2u)goto clear_slot;
            if(d1!=0x0800u && d1==0x0400u && rt->g.global_16ce==3u)goto clear_slot;
        }else{
            if(rt->g.progression_0162==4u||rt->g.progression_0162==9u)goto clear_slot;
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.config_0066);d1=(uint16_t)(low16(rt->m68k_regs.d[1])&0x0c00u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
            if(d1==0x0800u){if((rt->g.mode_0160&0x80u)!=0u)goto clear_slot;}
            else if(d1==0x0400u && rt->g.global_16ce==3u)goto clear_slot;
        }
        wf_r17_m68k_moveq(&rt->m68k_regs,0u,5);
        if(post7_c404_carry(rt,5u)){
            wf_r17_m68k_move_w(&rt->m68k_regs,3u,rt->g.global_16ce);
            st=post7_c710_c9be(rt,ctx,&rt->actors[slot],slot);if(st!=WF_OK)goto rollback;
            st=post7_c710_ca10(rt,ctx,&rt->actors[slot],slot);if(st!=WF_OK)goto rollback;
        }else{
            st=post7_c710_fixed(rt,ctx,&rt->actors[slot],slot,0xc2u,0x0018u,0u);if(st!=WF_OK)goto rollback;
            st=post7_c710_fixed(rt,ctx,&rt->actors[slot],slot,0xc3u,0x0019u,0x0204u);if(st!=WF_OK)goto rollback;
        }
        post7_c710_inc_slot(rt);return WF_OK;
    }
    if((wf_u8(&rt->actors[slot],WF_A_FLAGS33)&0x02u)==0u){
        uint16_t d1;wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.config_0066);d1=(uint16_t)(low16(rt->m68k_regs.d[1])&0x0c00u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
        if(d1==0x0400u){if(post7_word_lo(rt->g.global_16ce)>=2u)goto active_code2;}
        else if(d1==0x0800u && (rt->g.mode_0160&0x40u)!=0u)goto active_code2;
        wf_r17_m68k_moveq(&rt->m68k_regs,0u,5);
        if(post7_c404_carry(rt,5u)){
            wf_r17_m68k_move_w(&rt->m68k_regs,3u,rt->g.global_16ce);
            st=post7_c710_c892(rt,ctx,&rt->actors[slot],slot);if(st!=WF_OK)goto rollback;
        }else{
            st=post7_c710_fixed(rt,ctx,&rt->actors[slot],slot,0xc2u,0x0020u,0x000cu);if(st!=WF_OK)goto rollback;
        }
        st=post7_c710_fixed(rt,ctx,&rt->actors[slot],slot,0xc3u,0x0021u,0x010cu);if(st!=WF_OK)goto rollback;
        post7_c710_inc_slot(rt);return WF_OK;
    }
active_code2:
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,2);
    if(post7_c404_carry(rt,2u)){
        wf_r17_m68k_move_w(&rt->m68k_regs,3u,rt->g.global_16ce);
        st=post7_c710_c892(rt,ctx,&rt->actors[slot],slot);if(st!=WF_OK)goto rollback;
    }else{
        st=post7_c710_fixed(rt,ctx,&rt->actors[slot],slot,0xc2u,0x0020u,0x000cu);if(st!=WF_OK)goto rollback;
    }
    st=post7_c710_fixed(rt,ctx,&rt->actors[slot],slot,0xc3u,0x0026u,0x010cu);if(st!=WF_OK)goto rollback;
    post7_c710_inc_slot(rt);return WF_OK;
clear_slot:
    st=post7_c710_cb42(rt,ctx,slot);if(st!=WF_OK)goto rollback;post7_c710_inc_slot(rt);return WF_OK;
rollback:
    rt->m68k_regs=rsnap;rt->g.global_16ce=slot_snap;for(i=0u;i<4u;i++)rt->actors[i]=asnap[i];return WF_BLOCKED;
}

wf_status_t wf_r17_owner_88a2(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    static const uint32_t dst[4]={0x000c0110u,0x000c0134u,0x000c0160u,0x000c0184u};
    static const uint32_t src[4]={0xffb40013u,0xffc00013u,0xffcc0013u,0xffd80013u};
    wf_actor_raw_t asnap[4]; wf_r17_m68k_shadow_t rsnap;
    wf_r17_fg0_write_t w[48]; size_t nw=0u; unsigned i,j;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    for(i=0;i<4u;i++){asnap[i]=rt->actors[i];}
    rsnap=rt->m68k_regs;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u); /* $88A8 CLR.W D0 */
    for(i=0;i<4u;i++){
        wf_actor_raw_t *a=&rt->actors[i];
        uint8_t role=wf_u8(a,WF_A_ROLE);
        int draw=0;
        if((role&0x80u)!=0u){
            /* $88D6: role bit7 bypasses local HUD animation. */
        }else if((wf_u8(a,WF_A_FLAGS33)&0x20u)!=0u || (wf_u8(a,WF_A_C6)&0x40u)!=0u){
            draw=1;
        }else{
            uint8_t ce=wf_u8(a,0xceu);
            wf_set_u8(a,0xceu,(uint8_t)(ce&0x7fu));
            if((ce&0x80u)!=0u){wf_set_be16(a,0xc2u,0u);wf_set_be16(a,0x96u,0u);}
            wf_set_u8(a,0xcfu,0u);
        }
        if(draw){
            uint16_t idx=(uint16_t)(wf_be16(a,0xceu)&0x007fu);
            uint32_t d1=src[idx&3u],d3=d1+0x00060000u;
            wf_set_u8(a,0xceu,0x80u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(i<<2));
            wf_r17_m68k_move_l(&rt->m68k_regs,1u,d1);
            wf_r17_m68k_move_l(&rt->m68k_regs,3u,d3);
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,5u);
            for(j=0u;j<6u;j++){
                w[nw++]=(wf_r17_fg0_write_t){dst[i]+(uint32_t)j*4u,d1,4u};
                w[nw++]=(wf_r17_fg0_write_t){dst[i]+0x100u+(uint32_t)j*4u,d3,4u};
                d1+=0x00010000u;d3+=0x00010000u;
                wf_r17_m68k_move_l(&rt->m68k_regs,1u,d1);
                wf_r17_m68k_move_l(&rt->m68k_regs,3u,d3);
                wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(low16(rt->m68k_regs.d[2])-1u));
            }
            {
                uint16_t cc=(uint16_t)(wf_be16(a,0xccu)+1u);
                wf_set_be16(a,0xccu,cc);
                if(cc>=2u){uint8_t cf;wf_set_be16(a,0xccu,0u);cf=(uint8_t)(wf_u8(a,0xcfu)+1u);if(cf>=4u)cf=0u;wf_set_u8(a,0xcfu,cf);}
            }
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])+1u));
    }
    if(nw){
        if(!ctx->fg0_batch){ctx->blocked_dependency_rom=0x000c0110u;goto rollback;}
        if(ctx->fg0_batch(rt,w,nw,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x000c0110u;goto rollback;}
    }
    return WF_OK;
rollback:
    for(i=0;i<4u;i++){rt->actors[i]=asnap[i];}
    rt->m68k_regs=rsnap;
    return WF_BLOCKED;
}

static uint8_t post7_8856_any_active(const wf_runtime_t *rt){
    unsigned i;
    /* $8856: exactly four compact records from $1C14CE, stride $010C.
     * D1 becomes 1 if any +$00.bit7 is set. No unrelated globals participate. */
    for(i=0u;i<4u;i++) if((wf_u8(&rt->compact_14ce[i],0u)&0x80u)!=0u) return 1u;
    return 0u;
}
static uint16_t post7_8884_d5(const wf_actor_raw_t *actor){
    uint32_t p=wf_be32(actor,WF_A_INPUT_SOURCE); int i;
    for(i=0;i<4;i++)if(p==(0x00140026u-(uint32_t)(i*2)))return (uint16_t)(3-i);
    return 0u;
}

wf_status_t wf_r17_owner_8710(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_actor_raw_t actors_snap[4],compact_snap[4];wf_r17_m68k_shadow_t rsnap;wf_r17_render_queue_t qsnap;unsigned i;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    if(rt->g.credited_route==0u)return WF_OK;
    if(!ctx->render)return WF_BLOCKED;
    for(i=0;i<4u;i++){actors_snap[i]=rt->actors[i];compact_snap[i]=rt->compact_14ce[i];}rsnap=rt->m68k_regs;qsnap=*ctx->render->queue;
    wf_r17_m68k_moveq(&rt->m68k_regs,6u,3);
    for(i=0;i<4u;i++){
        wf_actor_raw_t *a=&rt->actors[i],*c=&rt->compact_14ce[i];
        if(actor_active(a)){
            uint8_t f=wf_u8(a,WF_A_PINFLAGS);
            int trigger3=(f&0x08u)!=0;
            wf_set_u8(a,WF_A_PINFLAGS,(uint8_t)(f&~0x08u));
            if(trigger3){
                wf_set_u8(c,0u,(uint8_t)(wf_u8(c,0u)|0x80u));wf_set_be16(c,2u,0x001eu);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0035u);
                if(!ctx->resource_2aea){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}
                if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,0x0035u,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}
                wf_set_be16(c,4u,0u);wf_set_be16(c,0x1eu,0u);wf_set_be16(c,0x1cu,0u);wf_set_be16(c,0x24u,0u);wf_set_be16(c,0x22u,0u);
            }else{
                int trigger5=(wf_u8(a,WF_A_PINFLAGS)&0x20u)!=0;
                wf_set_u8(a,WF_A_PINFLAGS,(uint8_t)(wf_u8(a,WF_A_PINFLAGS)&~0x20u));
                if(trigger5){
                    uint8_t any;
                    wf_set_u8(c,0u,(uint8_t)(wf_u8(c,0u)&~0x80u));
                    any=post7_8856_any_active(rt);
                    wf_r17_m68k_moveq(&rt->m68k_regs,0u,3);
                    wf_r17_m68k_moveq(&rt->m68k_regs,1u,any?1:0);
                    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu);
                    if(!any){
                        if(!ctx->resource_2b58){ctx->blocked_dependency_rom=0x00002b58u;goto rollback;}
                        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0035u);
                        if(post7_call_word_preserve_regs(rt,ctx->resource_2b58,0x0035u,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00002b58u;goto rollback;}
                    }
                }
                {
                    int trigger4=(wf_u8(a,WF_A_PINFLAGS)&0x10u)!=0;
                    wf_set_u8(a,WF_A_PINFLAGS,(uint8_t)(wf_u8(a,WF_A_PINFLAGS)&~0x10u));
                    if(trigger4){
                    uint16_t d5=post7_8884_d5(a);
                    wf_set_u8(c,0u,(uint8_t)(wf_u8(c,0u)|0x80u));wf_set_be16(c,2u,0x001eu);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0035u);
                    if(!ctx->resource_2aea){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}
                    if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,0x0035u,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}
                    wf_set_be16(c,0x1cu,0u);wf_set_be16(c,0x24u,0u);wf_set_be16(c,0x22u,0u);wf_set_be16(c,0x1eu,(uint16_t)(3u+d5));wf_set_be16(c,4u,wf_be16(c,0x1eu));
                    wf_r17_m68k_moveq(&rt->m68k_regs,5u,(int8_t)d5);
                    }
                }
            }
            if((wf_u8(c,0u)&0x80u)!=0u){
                uint16_t life=(uint16_t)(wf_be16(c,0x1cu)+1u);
                wf_set_be16(c,0x1cu,life);
                if(life>=0x80u){
                    wf_set_u8(c,0u,(uint8_t)(wf_u8(c,0u)&~0x80u));
                    {
                    uint8_t any=post7_8856_any_active(rt);
                    wf_r17_m68k_moveq(&rt->m68k_regs,0u,3);wf_r17_m68k_moveq(&rt->m68k_regs,1u,any?1:0);
                    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu);
                    if(!any){
                        if(!ctx->resource_2b58){ctx->blocked_dependency_rom=0x00002b58u;goto rollback;}
                        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0035u);
                        if(post7_call_word_preserve_regs(rt,ctx->resource_2b58,0x0035u,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00002b58u;goto rollback;}
                    }
                    }
                }else{
                    int16_t x=(int16_t)wf_be16(a,WF_A_X),y=(int16_t)wf_be16(a,WF_A_Y),z=(int16_t)wf_be16(a,WF_A_Z);
                    x=(int16_t)(x+(((wf_u8(a,WF_A_FACING)&0x80u)!=0u)?8:-8));
                    wf_set_be16(c,WF_A_X,(uint16_t)x);wf_set_be16(c,WF_A_Y,(uint16_t)y);
                    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)z);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])+0x68u));wf_set_be16(c,WF_A_Z,low16(rt->m68k_regs.d[0]));
                    wf_set_be16(c,0x22u,(uint16_t)(wf_be16(c,0x22u)+1u));
                    if(wf_be16(c,0x22u)>=2u){wf_set_be16(c,0x22u,0u);wf_set_be16(c,0x24u,(uint16_t)(wf_be16(c,0x24u)+1u));if(wf_be16(c,0x24u)>=3u){wf_set_be16(c,0x24u,0u);wf_set_be16(c,4u,wf_be16(c,0x1eu));}else if(wf_be16(c,0x1eu)==0u){wf_set_be16(c,4u,(uint16_t)(wf_be16(c,4u)+1u));}}
                    if(wf_r17_world_to_screen_247c(c,ctx->render->scroll_a_x,ctx->render->scroll_a_y)!=WF_OK)goto rollback;
                    if(wf_r17_queue_27b8(ctx->render,c)!=WF_OK)goto rollback;
                }
            }
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,6u,(uint16_t)(low16(rt->m68k_regs.d[6])-1u));
    }
    return WF_OK;
rollback:
    for(i=0;i<4u;i++){rt->actors[i]=actors_snap[i];rt->compact_14ce[i]=compact_snap[i];}rt->m68k_regs=rsnap;*ctx->render->queue=qsnap;return WF_BLOCKED;
}

static uint16_t post7_royal_first_free(const uint16_t *occ){unsigned i;for(i=0u;i<6u;i++)if(occ[i]==0u)return (uint16_t)i;return 0xffffu;}
static int post7_royal_has_free(const uint16_t *occ){unsigned i;for(i=0u;i<6u;i++)if(occ[i]==0u)return 1;return 0;}
static wf_status_t post7_24cc(wf_runtime_t *rt,uint16_t *candidate){
    static const uint8_t weights[6]={0x11u,0x11u,0x11u,0x11u,0x10u,0x10u};
    uint32_t save_d1,save_d2;uint16_t v=50u;unsigned tries,i;uint16_t sum=0u;
    if(!rt||!candidate||!rt->g.rng_state_valid||!rt->m68k_regs.valid)return WF_BLOCKED;
    save_d1=rt->m68k_regs.d[1];save_d2=rt->m68k_regs.d[2];
    wf_r17_m68k_moveq(&rt->m68k_regs,1u,3);
    for(tries=0u;tries<4u;tries++){
        wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);
        wf_r17_m68k_move_l(&rt->m68k_regs,0u,rt->m68k_regs.d[0]&0x000000ffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])>>1));
        if(low16(rt->m68k_regs.d[0])<100u){v=low16(rt->m68k_regs.d[0]);break;}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(low16(rt->m68k_regs.d[1])-1u));
    }
    if(tries==4u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);v=50u;}
    if(v==0u){v=1u;wf_r17_m68k_move_w(&rt->m68k_regs,0u,1u);}
    for(i=0u;i<6u;i++){sum=(uint16_t)(sum+weights[i]);if(sum>=v)break;}
    if(i>=6u){i=5u;}
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)i);
    rt->m68k_regs.d[1]=save_d1;rt->m68k_regs.d[2]=save_d2;*candidate=(uint16_t)i;return WF_OK;
}

static wf_status_t post7_royal_activate(wf_runtime_t *rt,uint16_t id){
    unsigned slot;uint16_t d2=0u;
    wf_r17_m68k_moveq(&rt->m68k_regs,1u,4);wf_r17_m68k_moveq(&rt->m68k_regs,2u,0);
    for(slot=4u;slot<9u;slot++){
        wf_actor_raw_t *a=&rt->actors[slot];
        if(!actor_active(a)){
            memset(a,0,sizeof(*a));wf_r17_m68k_moveq(&rt->m68k_regs,4u,0);wf_r17_m68k_move_w(&rt->m68k_regs,4u,0x010cu);
            wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_ROLE,id);wf_set_be16(a,WF_A_CHARACTER,id);wf_set_u8(a,WF_A_CHARACTER+1u,(uint8_t)(wf_u8(a,WF_A_CHARACTER+1u)+0x10u));
            if(id>=6u)wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)|0x20u));
            wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)|0x80u));wf_set_be16(a,WF_A_FLAGS32,0x8005u);
            wf_set_be16(a,WF_A_STAMINA,0x0087u);wf_set_be16(a,WF_A_STAMINA_MAX,0x0087u);wf_set_be16(a,WF_A_TIER,0u);
            rt->g.royal_slots_1586[4u+d2]=0x4000u;rt->g.royal_occupancy[id]=1u;
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(d2<<1));wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(id<<1));
            return WF_OK;
        }
        d2++;wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(low16(rt->m68k_regs.d[1])-1u));
    }
    return WF_OK;
}

wf_status_t wf_r17_owner_bb14(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_runtime_t snap;unsigned i;uint16_t active=0u,lower=0u,upper=0u;uint16_t base=0u,cand=0u;int preferred_upper;
    (void)ctx;if(!rt||!rt->m68k_regs.valid)return WF_BLOCKED;snap=*rt;
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,8);wf_r17_m68k_moveq(&rt->m68k_regs,1u,0);
    for(i=9u;i-- >0u;){wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(i<<1));if(rt->g.royal_slots_1586[i]!=0u)return WF_OK;wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])-1u));}
    if(rt->g.global_16a7==0u)return WF_OK;
    rt->g.global_1694=(uint16_t)(rt->g.global_1694+1u);if(rt->g.global_1694<0x0126u)return WF_OK;rt->g.global_1694=0u;
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,8);wf_r17_m68k_moveq(&rt->m68k_regs,1u,0);
    for(i=0u;i<9u;i++){if(actor_active(&rt->actors[i])){active++;wf_r17_m68k_move_w(&rt->m68k_regs,1u,active);}wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])-1u));}
    if(active>=6u)return WF_OK;
    rt->g.global_16a7--;
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,8);wf_r17_m68k_moveq(&rt->m68k_regs,1u,0);wf_r17_m68k_moveq(&rt->m68k_regs,2u,0);
    for(i=0u;i<9u;i++)if(actor_active(&rt->actors[i])){if((wf_u8(&rt->actors[i],WF_A_ROLE)&0x20u)!=0u)upper++;else lower++;}
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,lower);wf_r17_m68k_move_w(&rt->m68k_regs,2u,upper);
    preferred_upper=(upper<lower); /* CMP.B D1,D2 / BCS $BBC4 */
    if(preferred_upper){if(post7_royal_has_free(&rt->g.royal_occupancy[6]))base=6u;else if(post7_royal_has_free(&rt->g.royal_occupancy[0]))base=0u;else return WF_OK;}
    else{if(post7_royal_has_free(&rt->g.royal_occupancy[0]))base=0u;else if(post7_royal_has_free(&rt->g.royal_occupancy[6]))base=6u;else return WF_OK;}
    wf_r17_m68k_moveq(&rt->m68k_regs,1u,3);wf_r17_m68k_moveq(&rt->m68k_regs,2u,0);
    for(i=0u;i<4u;i++){
        if(post7_24cc(rt,&cand)!=WF_OK){*rt=snap;return WF_BLOCKED;}
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,cand);wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(cand<<1));
        if(rt->g.royal_occupancy[base+cand]==0u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(base+cand));return post7_royal_activate(rt,(uint16_t)(base+cand));}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(low16(rt->m68k_regs.d[1])-1u));
    }
    /* ROM $10AF0/$10B26 fallback: first free slot 0 -> 5, not highest free. */
    cand=post7_royal_first_free(&rt->g.royal_occupancy[base]);
    if(cand!=0xffffu){wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(base+cand));return post7_royal_activate(rt,(uint16_t)(base+cand));}
    base=(uint16_t)(base?0u:6u);cand=post7_royal_first_free(&rt->g.royal_occupancy[base]);
    if(cand!=0xffffu){wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(base+cand));return post7_royal_activate(rt,(uint16_t)(base+cand));}
    return WF_OK;
}

static wf_actor_raw_t *post7_royal_actor10(wf_runtime_t *rt,unsigned idx){return idx<9u?&rt->actors[idx]:(idx==9u?&rt->fixed_object0:0);}
wf_status_t wf_r17_owner_7366(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    static const uint8_t tab[30]={0,4,1,4,0,4,1,4,0,4,1,4,0,4,1,4,0,4,1,4,0,4,1,4,2,0x30,3,0x20,0,4};
    wf_runtime_t snap;wf_r17_render_queue_t qsnap;unsigned pass,idx;wf_actor_raw_t *a=0;uint16_t phase;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    if((rt->g.mode_0161&1u)==0u)return WF_OK;
    if(!ctx->render||!ctx->render->queue)return WF_BLOCKED;
    snap=*rt;
    qsnap=*ctx->render->queue;
    wf_r17_m68k_moveq(&rt->m68k_regs,2u,1);wf_r17_m68k_move_w(&rt->m68k_regs,3u,0x2000u);
    for(pass=0u;pass<2u&&!a;pass++){
        uint16_t target=pass?0x4000u:0x2000u;wf_r17_m68k_move_w(&rt->m68k_regs,3u,target);wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
        for(idx=0u;idx<10u;idx++){
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,low16(rt->m68k_regs.d[0]));wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(low16(rt->m68k_regs.d[1])<<1));wf_r17_m68k_move_w(&rt->m68k_regs,4u,rt->g.royal_slots_1586[idx]);
            if(rt->g.royal_slots_1586[idx]==target){a=post7_royal_actor10(rt,idx);break;}
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])+1u));
        }
        if(!a)wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(low16(rt->m68k_regs.d[2])-1u));
    }
    if(!a)return WF_OK;
    rt->g.royal_slots_1586[idx]=0x2000u;wf_set_be16(a,WF_A_STATE,(uint16_t)idx);
    {uint8_t old=wf_u8(a,WF_A_LATCHED);wf_set_u8(a,WF_A_LATCHED,(uint8_t)(old|0x80u));if((old&0x80u)==0u){
        wf_set_be16(a,WF_A_MIRROR,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_RENDER_LIST,0u);wf_set_be16(a,0x4au,3u);
        if(!ctx->resource_2aea){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,WF_A_CHARACTER));if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,low16(rt->m68k_regs.d[0]),ctx->user)!=WF_OK)goto rollback;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x001cu);if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,0x001cu,ctx->user)!=WF_OK)goto rollback;
    }}
    wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u));
    if((int16_t)wf_be16(a,WF_A_ANIM_COUNTER)<0){
        phase=wf_be16(a,WF_A_ANIM_INDEX);wf_r17_m68k_move_w(&rt->m68k_regs,2u,phase);wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(phase<<1));
        if(phase<15u){wf_set_u8(a,5u,tab[phase*2u]);wf_set_be16(a,WF_A_MIRROR,(uint16_t)(wf_be16(a,WF_A_MIRROR)&0x00ffu));wf_set_u8(a,0x23u,tab[phase*2u+1u]);wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)&0x00ffu));}
        if(phase<12u){if(!rt->g.rng_state_valid){ctx->blocked_dependency_rom=0x000021b4u;goto rollback;}wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])&0x000fu));wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)+low16(rt->m68k_regs.d[0])));}
        wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)(phase+1u));
        if(wf_be16(a,WF_A_ANIM_INDEX)==15u){if(!ctx->resource_2b58){ctx->blocked_dependency_rom=0x00002b58u;goto rollback;}wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,WF_A_CHARACTER));if(post7_call_word_preserve_regs(rt,ctx->resource_2b58,low16(rt->m68k_regs.d[0]),ctx->user)!=WF_OK)goto rollback;wf_set_be16(a,WF_A_CHARACTER,0x001cu);}
        if(wf_be16(a,WF_A_MIRROR)==0x00ffu){
            uint16_t id;rt->g.royal_slots_1586[idx]=0u;wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,WF_A_ROLE));wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])&0x000fu));id=low16(rt->m68k_regs.d[0]);wf_set_be16(a,WF_A_CHARACTER,id);
            if(!ctx->resource_2aea||!ctx->resource_2b58){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,id,ctx->user)!=WF_OK)goto rollback;if(wf_u8(a,WF_A_INTERNAL_ID)>=6u)wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)|0x20u));wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x001cu);if(post7_call_word_preserve_regs(rt,ctx->resource_2b58,0x001cu,ctx->user)!=WF_OK)goto rollback;
            wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0x000bu);wf_set_be16(a,WF_A_ANGLE_WORD,0u);wf_set_be16(a,WF_A_X,0x0350u);wf_set_be16(a,WF_A_Y,0x0020u);wf_set_be16(a,WF_A_Z,0x0100u);wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&0x7fu));if((wf_u8(a,WF_A_ROLE)&0x80u)==0u)wf_set_be16(a,WF_A_FLAGS32,0x0007u);wf_set_be16(a,WF_A_FACING,0u);wf_set_u8(a,WF_A_MOTION_MODE,(uint8_t)(wf_u8(a,WF_A_MOTION_MODE)|1u));wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)|0x40u));return WF_OK;
        }
    }
    wf_set_be16(a,WF_A_SCREEN_X,0x0100u);wf_set_be16(a,WF_A_SCREEN_Y,0x0090u);wf_set_be16(a,WF_A_Y,0u);if(wf_r17_queue_27b8(ctx->render,a)!=WF_OK)goto rollback;return WF_OK;
rollback:*rt=snap;*ctx->render->queue=qsnap;return WF_BLOCKED;
}

wf_status_t wf_r17_owner_8cf8(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    static const uint16_t tab_8e9a[14]={0xffffu,0xffffu,0x0000u,0x0001u,0x0002u,0x0003u,0x0006u,0x0007u,0x0008u,0x0009u,0x000au,0x000bu,0x0004u,0x0005u};
    wf_runtime_t snap;wf_r17_render_queue_t qsnap;wf_actor_raw_t *a;uint8_t g167;uint16_t word82;uint16_t rid;
    uint16_t d2=0x0015u,d7=0x0070u;uint32_t dest=0x000c131cu;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    if(rt->g.credited_route!=0u)return WF_OK;
    g167=(uint8_t)rt->g.global_0166;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0029u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);wf_r17_m68k_move_w(&rt->m68k_regs,7u,d7);
    if(g167!=0u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x002au);d2=0x0016u;d7=0x0060u;dest=0x000c1604u;wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);wf_r17_m68k_move_w(&rt->m68k_regs,7u,d7);}
    if(g167==0u||g167==1u){
        wf_r17_m68k_shadow_t rs=rt->m68k_regs;wf_status_t s;
        if(!ctx->raw_2503c){ctx->blocked_dependency_rom=0x0002503cu;return WF_BLOCKED;}
        s=ctx->raw_2503c(rt,low16(rt->m68k_regs.d[0]),ctx->user);rt->m68k_regs=rs;if(s!=WF_OK){ctx->blocked_dependency_rom=0x0002503cu;return WF_BLOCKED;}
        if(!ctx->raw_9aca){ctx->blocked_dependency_rom=0x00009acau;return WF_BLOCKED;}
        rs=rt->m68k_regs;s=ctx->raw_9aca(rt,d2,d7,dest,ctx->user);rt->m68k_regs=rs;if(s!=WF_OK){ctx->blocked_dependency_rom=0x00009acau;return WF_BLOCKED;}
        return WF_OK;
    }
    if(g167<2u)return WF_OK;
    a=&rt->compact_14ce[0];if((wf_u8(a,0u)&0x40u)!=0u)return WF_OK;
    if(!ctx->render||!ctx->render->queue)return WF_BLOCKED;
    snap=*rt;
    qsnap=*ctx->render->queue;
    if((wf_u8(a,0u)&0x80u)==0u){
        uint16_t raw[3];unsigned i;raw[0]=0x0006u;raw[1]=(uint16_t)(rt->g.global_0166-2u+7u);raw[2]=(uint16_t)(rt->g.global_0166-2u+0x40u);
        if(!ctx->raw_2503c){ctx->blocked_dependency_rom=0x0002503cu;goto rollback;}
        for(i=0;i<3u;i++){wf_r17_m68k_shadow_t rs=rt->m68k_regs;wf_status_t s;wf_r17_m68k_move_w(&rt->m68k_regs,0u,raw[i]);rs=rt->m68k_regs;s=ctx->raw_2503c(rt,raw[i],ctx->user);rt->m68k_regs=rs;if(s!=WF_OK){ctx->blocked_dependency_rom=0x0002503cu;goto rollback;}}
    }
    {uint8_t old=wf_u8(a,WF_A_MOTION_MODE);wf_set_u8(a,WF_A_MOTION_MODE,(uint8_t)(old|1u));if((old&1u)==0u){
        uint16_t idx=rt->g.global_0166;
        wf_set_u8(a,0u,0x80u);wf_set_be16(a,WF_A_SCREEN_X,0x0028u);wf_set_be16(a,WF_A_SCREEN_Y,0x00a8u);wf_set_be16(a,WF_A_ANIM_COUNTER,8u);wf_set_be16(a,WF_A_ANIM_INDEX,0u);
        if(!ctx->resource_2aea){ctx->blocked_dependency_rom=0x00002aeau;goto rollback;}wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x001eu);if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,0x001eu,ctx->user)!=WF_OK)goto rollback;
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,idx);wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(idx<<1));if(idx>=14u){ctx->blocked_dependency_rom=0x00008e9au;goto rollback;}wf_set_be16(a,WF_A_CHARACTER,(uint16_t)(tab_8e9a[idx]+0x30u));
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(wf_be16(a,WF_A_CHARACTER)-0x20u));wf_r17_m68k_move_w(&rt->m68k_regs,0u,low16(rt->m68k_regs.d[1]));rid=low16(rt->m68k_regs.d[0]);if(post7_call_word_preserve_regs(rt,ctx->resource_2aea,rid,ctx->user)!=WF_OK)goto rollback;
    }}
    wf_set_be16(a,WF_A_MIRROR,wf_be16(a,WF_A_ANIM_INDEX));word82=(uint16_t)(((uint16_t)rt->g.global_0082<<8)|rt->g.pipeline_mode);if(word82>=0x0100u)wf_set_be16(a,WF_A_MIRROR,2u);
    wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u));if(wf_be16(a,WF_A_ANIM_COUNTER)==0u){if(!rt->g.rng_state_valid){ctx->blocked_dependency_rom=0x000021b4u;goto rollback;}wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])&0x000fu));wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])+8u));wf_set_be16(a,WF_A_ANIM_COUNTER,low16(rt->m68k_regs.d[0]));wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)^1u));}
    if(wf_r17_queue_27b8(ctx->render,a)!=WF_OK)goto rollback;
    if(word82<0x0200u)return WF_OK;
    wf_set_u8(a,0u,(uint8_t)(wf_u8(a,0u)|0x40u));if(!ctx->resource_2b58||!ctx->raw_2503c){ctx->blocked_dependency_rom=0x00002b58u;goto rollback;}
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(wf_be16(a,WF_A_CHARACTER)-0x20u));rid=low16(rt->m68k_regs.d[0]);if(post7_call_word_preserve_regs(rt,ctx->resource_2b58,rid,ctx->user)!=WF_OK)goto rollback;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x001eu);if(post7_call_word_preserve_regs(rt,ctx->resource_2b58,0x001eu,ctx->user)!=WF_OK)goto rollback;
    {uint16_t raw[3]={0x8006u,(uint16_t)(rt->g.global_0166-2u+0x8007u),(uint16_t)(rt->g.global_0166-2u+0x8040u)};unsigned i;for(i=0u;i<3u;i++){wf_r17_m68k_shadow_t rs;wf_status_t s;wf_r17_m68k_move_w(&rt->m68k_regs,0u,raw[i]);rs=rt->m68k_regs;s=ctx->raw_2503c(rt,raw[i],ctx->user);rt->m68k_regs=rs;if(s!=WF_OK){ctx->blocked_dependency_rom=0x0002503cu;goto rollback;}}}
    return WF_OK;
rollback:*rt=snap;*ctx->render->queue=qsnap;return WF_BLOCKED;
}

wf_status_t wf_r17_owner_2836(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_r17_render_queue_t qcopy;
    wf_actor_raw_t *ordered[WF_R17_RENDER_LISTS*WF_R17_RENDER_LIST_CAP];
    wf_r17_m68k_shadow_t regs_snap;
    uint32_t keep_d[6];
    size_t n;
    unsigned i;
    if(!rt||!ctx||!ctx->render||!ctx->render->queue||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;
    if(!ctx->render_2836_batch){ctx->blocked_dependency_rom=0x0000d1fcu;return WF_BLOCKED;}

    /* $2836 starts with MOVEM.L preserving D0-D5 at the caller boundary.  Build
     * the exact 5,4,3,2,1,0 order on a copy so a blocked WP26 join cannot
     * partially consume the live queues.  $2962/$2966 use signed WORD Y. */
    qcopy=*ctx->render->queue;
    n=wf_r17_emit_actor_order_2836(&qcopy,ordered,sizeof(ordered)/sizeof(ordered[0]));
    regs_snap=rt->m68k_regs;
    for(i=0u;i<6u;i++)keep_d[i]=rt->m68k_regs.d[i];

    /* WP26 owns the repeated $D1FC render handoff/stale-tail output.  D6/D7
     * are intentionally allowed to leak from that path ($D216 can CLR.L D7). */
    if(ctx->render_2836_batch(rt,ordered,n,ctx->user)!=WF_OK){
        rt->m68k_regs=regs_snap;
        ctx->blocked_dependency_rom=0x0000d1fcu;
        return WF_BLOCKED;
    }
    for(i=0u;i<6u;i++)rt->m68k_regs.d[i]=keep_d[i];
    for(i=0u;i<WF_R17_RENDER_LISTS;i++)ctx->render->queue->count[i]=0u;
    return WF_OK;
}

wf_status_t wf_r17_owner_298ec(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx){
    wf_globals_raw_t gsnap;wf_r17_m68k_shadow_t rsnap;wf_r17_fg0_write_t w[18u*16u];size_t nw=0u;uint16_t count;unsigned qi,row,col;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    count=rt->g.transfer_count_1828;
    if(count==0u)return WF_OK;
    if(count>16u){ctx->blocked_dependency_rom=0x000298ecu;return WF_BLOCKED;}
    if(!ctx->video_batch){ctx->blocked_dependency_rom=0x0008218au;return WF_BLOCKED;}
    gsnap=rt->g;rsnap=rt->m68k_regs;
    wf_r17_m68k_move_w(&rt->m68k_regs,4u,count);
    wf_r17_m68k_move_l(&rt->m68k_regs,4u,rt->m68k_regs.d[4]-1u); /* SUBQ.L #1,D4 */
    for(qi=0u;qi<count;qi++){
        uint8_t id=rt->g.transfer_queue_1808[qi];uint16_t off;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);wf_r17_m68k_move_b(&rt->m68k_regs,0u,id);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])-1u));
        if(id!=1u&&id!=2u){ctx->blocked_dependency_rom=0x000299f4u;goto rollback;}
        off=0x018au;wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])<<1)); /* $29912 LSL.W #1,D0 */
        wf_r17_m68k_move_l(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,off);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])<<1)); /* $29926 LSL.W #1,D0 */
        for(row=0u;row<3u;row++)for(col=0u;col<6u;col++){
            uint16_t pair=0u;
            if(id==2u){static const uint8_t lo[3]={0xbbu,0x8au,0x9au};pair=(uint16_t)(0x0200u|lo[row]);}
            w[nw++]=(wf_r17_fg0_write_t){0x0008218au+(uint32_t)row*0x40u+(uint32_t)col*2u,pair,2u};
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,4u,(uint16_t)(low16(rt->m68k_regs.d[4])-1u)); /* DBF low WORD */
    }
    if(ctx->video_batch(rt,w,nw,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x0008218au;goto rollback;}
    rt->g.transfer_count_1828=0u;memset(rt->g.transfer_queue_1808,0,sizeof(rt->g.transfer_queue_1808));return WF_OK;
rollback:rt->g=gsnap;rt->m68k_regs=rsnap;return WF_BLOCKED;
}

/* ROM $18C4. Native participant owners supply the TAG tail and Royal child.
 * Explicit callback overrides remain available for isolated contract tests. */
wf_status_t wf_r17_owner_18c4(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,const uint8_t input_hw_low[WF_ACTOR_SLOTS],uint16_t input_valid_mask){
    wf_r17_m68k_shadow_t regs_snap;
    uint8_t credit_count_snap,credit_rem_snap;
    uint16_t table_snap[4];
    uint16_t marker_snap;
    unsigned i;
    int has_bit7=0;
    if(!rt||!ctx||!rt->m68k_regs.valid)return WF_BLOCKED;
    ctx->blocked_dependency_rom=0u;

    /* $18C4 TST.W $1C007C / BEQ $19B8. */
    if(rt->g.credited_route==0u)return WF_OK;

    /* $18CE BTST #0,$1C0161 / BEQ.W $6E34. This is a tail branch. */
    if((rt->g.mode_0161&0x01u)==0u){
        if(ctx->tag_6e34)return ctx->tag_6e34(rt,ctx->user);
        if(ctx->participants){
            wf_status_t s=wf_r17_owner_6e34(rt,ctx->participants);
            ctx->blocked_dependency_rom=ctx->participants->blocked_dependency_rom;
            return s;
        }
        ctx->blocked_dependency_rom=0x00006e34u;return WF_BLOCKED;
    }

    /* $18DA TST.B $1C16C5 / BNE $19B8. */
    if((rt->g.global_16c4&0x00ffu)!=0u)return WF_OK;

    regs_snap=rt->m68k_regs;
    credit_count_snap=rt->g.credit_count_004f;
    credit_rem_snap=rt->g.credit_remainder_004e;
    marker_snap=rt->g.global_15ce;
    for(i=0u;i<4u;i++)table_snap[i]=rt->g.royal_slots_1586[i];

    /* $18EA MOVE.W #3,D0; $18EE..$1900 scans first four table WORDs 0->3. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u);
    for(i=0u;i<4u;i++){
        if((rt->g.royal_slots_1586[i]&0x8000u)!=0u){has_bit7=1;break;}
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]-1u));
    }

    if(!has_bit7){
        /* $1902 MOVE.W #0,D4; $1906 MOVEQ #0,D3. */
        wf_r17_m68k_move_w(&rt->m68k_regs,4u,0u);
        wf_r17_m68k_move_l(&rt->m68k_regs,3u,0u);
        for(i=0u;i<4u;i++){
            wf_actor_raw_t *a=&rt->actors[i];
            /* $1914 active actors skip both the table and hardware reads. */
            if(!actor_active(a)){
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)rt->m68k_regs.d[4]);
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<1));
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.royal_slots_1586[i]);
                if((uint16_t)rt->m68k_regs.d[1]==0u){
                    uint16_t raw;
                    uint32_t address=0x00140020u+(uint32_t)i*2u;
                    if(ctx->participants){
                        wf_status_t input_status;
                        wf_r17_m68k_shadow_t input_regs=rt->m68k_regs;
                        if(!ctx->participants->input_word){ctx->blocked_dependency_rom=address;goto rollback;}
                        input_status=ctx->participants->input_word(rt,address,&raw,ctx->participants->user);
                        rt->m68k_regs=input_regs;
                        if(input_status!=WF_OK){ctx->blocked_dependency_rom=address;goto rollback;}
                    }else{
                        /* Explicit legacy-owner fixtures may supply a sampled input image. */
                        if(!input_hw_low||(input_valid_mask&(uint16_t)(1u<<i))==0u){ctx->blocked_dependency_rom=address;goto rollback;}
                        raw=input_hw_low[i];
                    }
                    if((raw&0x80u)==0u){
                        int paid;
                        wf_r17_m68k_move_l(&rt->m68k_regs,0u,5u);
                        paid=wf_payment_consume_055a(rt,5u);
                        wf_r17_m68k_move_w(&rt->m68k_regs,0u,2u); /* $0588 */
                        if(paid){
                            wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)rt->m68k_regs.d[4]);
                            wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<1));
                            rt->g.royal_slots_1586[i]=0x8000u;
                            break; /* $1950 BRA $196A: exactly one successful payment per call. */
                        }
                    }
                }
            }
            /* $195E ADDQ.B #1,D3; $1960 ADDI.W #1,D4. */
            wf_r17_m68k_move_b(&rt->m68k_regs,3u,(uint8_t)((uint8_t)rt->m68k_regs.d[3]+1u));
            wf_r17_m68k_move_w(&rt->m68k_regs,4u,(uint16_t)((uint16_t)rt->m68k_regs.d[4]+1u));
        }
    }

    /* $196A..$19B6 scans exact $8000 markers in reverse index order 3->0. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u);
    for(;;){
        uint16_t paid=(uint16_t)rt->m68k_regs.d[0];
        /* $1974 MOVE.W D0,D1; ASL.W #1,D1; table lookup leaves D1=table value. */
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,paid);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<1));
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,rt->g.royal_slots_1586[paid]);
        if((uint16_t)rt->m68k_regs.d[1]==0x8000u){
            wf_actor_raw_t *target=0;
            unsigned reserve;
            rt->g.global_15ce=0u; /* $1982 CLR.W $1C15CE */
            /* $1988 A0=actor4; D1=4; scan actors4..8 for first inactive. */
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,4u);
            for(reserve=4u;reserve<=8u;reserve++){
                if(!actor_active(&rt->actors[reserve])){target=&rt->actors[reserve];break;}
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((uint16_t)rt->m68k_regs.d[1]-1u));
            }
            if(!target){
                /* DBF exhausted: A0 has advanced to $1C0F1C, then marker is set. */
                rt->g.global_15ce=0x8000u;
                target=&rt->fixed_object0;
            }
            if(ctx->royal_9132){
                if(ctx->royal_9132(rt,target,paid,ctx->user)!=WF_OK){ctx->blocked_dependency_rom=0x00009132u;goto rollback;}
            }else if(ctx->participants){
                if(wf_r17_owner_9132(rt,target,paid,ctx->participants)!=WF_OK){
                    ctx->blocked_dependency_rom=ctx->participants->blocked_dependency_rom;
                    goto rollback;
                }
            }else{ctx->blocked_dependency_rom=0x00009132u;goto rollback;}
            return WF_OK;
        }
        if((uint16_t)rt->m68k_regs.d[0]==0u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu);break;}
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((uint16_t)rt->m68k_regs.d[0]-1u));
    }
    return WF_OK;

rollback:
    rt->m68k_regs=regs_snap;
    rt->g.credit_count_004f=credit_count_snap;
    rt->g.credit_remainder_004e=credit_rem_snap;
    rt->g.global_15ce=marker_snap;
    for(i=0u;i<4u;i++)rt->g.royal_slots_1586[i]=table_snap[i];
    return WF_BLOCKED;
}
