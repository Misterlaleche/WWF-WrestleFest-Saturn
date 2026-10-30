#include "wf_r17_human_raw23.h"
#include "wf_r17_human_ddc8.h"
#include "wf_r17_rng1129e.h"
#include "wf_r17_human_tables.h"

#define BIT8(n) ((uint8_t)(1u << (n)))
#define ACTOR_BASE 0x001C05B0u
#define ACTOR_STRIDE 0x10Cu

enum {
    O_CHAR=0x02, O_X=0x06, O_Y=0x0A, O_OBJ_STATE=0x1C, O_STATE=0x20, O_LINK=0x26,
    O_FACING=0x2E, O_FLAGS32=0x32, O_FLAGS33=0x33, O_FLAGS34=0x34,
    O_SELECTOR=0x60, O_STATE4_LO=0x65, O_TIER=0x70, O_REL_FLAGS=0x74,
    O_OBJLINK=0x76, O_TARGET=0x7A, O_TARGET2=0x82, O_TEAM=0x86, O_AUX9C=0x9C, O_INPUT_FRESH_LO=0xA3,
    O_TIMER44=0x44, O_MASH=0xAA, O_AE=0xAE, O_AF=0xAF, O_FE=0xFE
};

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){
    return (wf_u8(a,o)&BIT8(b))!=0u;
}
static uint8_t state_lo(const wf_actor_raw_t *a){
    return wf_u8(a,O_STATE+1u);
}
static wf_status_t block(wf_r17_human_raw23_trace_t *tr,uint32_t pc){
    if(tr)tr->blocked_dependency_rom=pc;
    return WF_BLOCKED;
}
static void clear_trace(wf_r17_human_raw23_trace_t *tr){
    if(!tr)return;
    tr->blocked_dependency_rom=0u;
    tr->last_stage_rom=0u;
    tr->fresh_mask=0u;
    tr->category=0u;
    tr->raw_action=0u;
    tr->selected_raw23=0u;
    tr->consumed_before_raw23=0u;
    tr->target_slot=0u;
}
static uint32_t actor_ptr(unsigned slot){
    return slot<WF_ACTOR_SLOTS?ACTOR_BASE+(uint32_t)slot*ACTOR_STRIDE:0u;
}
static wf_actor_raw_t *actor_from_ptr(wf_runtime_t *rt,uint32_t p,unsigned *slot){
    uint32_t d;unsigned s;
    if(!rt||p<ACTOR_BASE)return 0;
    d=p-ACTOR_BASE;
    if((d%ACTOR_STRIDE)!=0u)return 0;
    s=(unsigned)(d/ACTOR_STRIDE);
    if(s>=WF_ACTOR_SLOTS)return 0;
    if(slot)*slot=s;
    return &rt->actors[s];
}

/* $E958. Only table selectors 0 and 6 are needed by the demonstrated path:
 * selector 0 for the pre-classifier +$7A/+$82 target arbitration and selector 6
 * for raw $23's $EF9A action gate. CMP.W+BCC/BCS is unsigned WORD logic. */
static int e958_carry(const wf_actor_raw_t *a,const wf_actor_raw_t *t,unsigned code){
    /* ROM $E9BA: 17 records x 4 signed WORD bounds, indexed by D0*8. */
    static const int16_t bounds[17][4]={
        {-32,128,-56,40},{80,160,-24,24},{-48,56,-24,24},{8,96,-30,30},
        {-24,96,-48,24},{-64,192,-56,8},{-32,96,-56,24},{-64,112,-56,24},
        {-48,32,-24,24},{32,112,-40,16},{-8,104,-72,24},{-32,96,-40,24},
        {56,160,-56,32},{-32,64,-52,32},{-64,80,-56,16},{-32,64,-40,40},
        {-32,64,-40,40}
    };
    const int16_t *b;uint16_t lo,hi,top,bot,ax,ay,tx,ty,tmp;
    if(!a||!t||code>=17u)return 0;
    b=bounds[code];
    lo=(uint16_t)b[0];hi=(uint16_t)b[1];top=(uint16_t)b[2];bot=(uint16_t)b[3];
    if(bit(t,O_FACING,7)){
        tmp=lo;lo=hi;hi=tmp;
        lo=(uint16_t)(0u-lo);hi=(uint16_t)(0u-hi);
    }
    ax=wf_be16(a,O_X);ay=wf_be16(a,O_Y);tx=wf_be16(t,O_X);ty=wf_be16(t,O_Y);
    lo=(uint16_t)(lo+tx);hi=(uint16_t)(hi+tx);
    if(lo>=ax)return 0;
    if(hi<ax)return 0;
    top=(uint16_t)(top+ty);bot=(uint16_t)(bot+ty);
    if(top>=ay)return 0;
    if(bot<ay)return 0;
    return 1;
}

/* $DD02 exact special-fresh priority consumer. Returns 1 when it consumes. */
static wf_status_t dd02(wf_runtime_t *rt,wf_actor_raw_t *a,const wf_r17_human_raw23_ctx_t *ctx,
                        wf_r17_human_raw23_trace_t *tr,int *consumed){
    wf_actor_raw_t *p;uint16_t ae,x,y;uint8_t st,sel;
    *consumed=0;
    if((wf_u8(a,O_INPUT_FRESH_LO)&3u)==0u)return WF_OK;
    if((ctx->mode_0161&3u)!=0u)return WF_OK;
    if(bit(a,O_FLAGS32,0))return WF_OK;
    p=actor_from_ptr(rt,wf_be32(a,O_TEAM),0);
    if(!p)return block(tr,0x0000DD2Au);
    if(state_lo(p)!=1u)return WF_OK;
    ae=(uint16_t)(wf_be16(p,O_AE)&0x80FFu);
    if(ae!=0x8002u)return WF_OK;
    x=wf_be16(a,O_X);y=wf_be16(a,O_Y);
    if(!bit(a,O_FLAGS33,7)){
        if(x>=0x0220u||y<0x0186u)return WF_OK;
    }else{
        if(x<0x02F0u||y>=0x012Eu)return WF_OK;
    }
    st=state_lo(a);
    if(st==0u||st==1u){
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x004Cu);sel=0x4Cu;
    }else if(st==5u){
        sel=wf_u8(a,O_SELECTOR+1u);
        if(sel==0x15u||sel==0x16u){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0066u);sel=0x66u;
        }else return WF_OK;
    }else return WF_OK;
    if(tr){
        tr->raw_action=sel;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000DD02u;
    }
    *consumed=1;
    return WF_OK;
}


/* $EBC4 mash-completion/special pre-action owner. Carry-set in ROM means the
 * fresh-button route was consumed; carry-clear means DE86 continues. */
static wf_status_t ebc4(wf_runtime_t *rt,wf_actor_raw_t *a,const wf_r17_human_raw23_ctx_t *ctx,
                        wf_r17_human_raw23_trace_t *tr,int *consumed){
    wf_actor_raw_t *t,*link;uint8_t st,sel,lst,lsel;
    *consumed=0;
    if(wf_be16(a,O_MASH)!=0x4000u)return WF_OK;
    st=state_lo(a);sel=wf_u8(a,O_SELECTOR+1u);
    if(st==5u&&(sel==0x4Au||sel==0x51u)){
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x004Bu);
        wf_set_be16(a,O_MASH,0u);*consumed=1;
        if(tr){tr->raw_action=0x4Bu;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EBC4u;}
        return WF_OK;
    }
    t=actor_from_ptr(rt,wf_be32(a,O_TARGET),0);
    if(!t)return block(tr,0x0000EBF6u);
    if(!bit(t,O_FLAGS34,4)){
        if(st==4u&&wf_u8(a,O_STATE4_LO)==8u){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0038u);
            wf_set_be16(a,O_MASH,0u);*consumed=1;
            if(tr){tr->raw_action=0x38u;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EC14u;}
            return WF_OK;
        }
        if(st==4u&&wf_u8(a,O_STATE4_LO)==9u){
            uint32_t physical=wf_be32(a,O_LINK)&0x00FFFFFFu;
            link=actor_from_ptr(rt,physical,0);
            if(!link&&physical!=0u){
                /* DE86 has saved the caller D3, then CLR.L D3 at DEB2.
                 * Preserve that live prefix when this dependency blocks. */
                if(ctx->m68k_live&&ctx->m68k_live->valid)ctx->m68k_live->d[3]=0u;
                return block(tr,0x0000EC34u);
            }
            /* EC38 reads BYTE $21(A2), even when A2 is physical zero.
             * Intact canonical ROM[$21]=$FF, so that case branches EC6A.
             * This is a ROM-byte specialization, not a default actor. Other
             * nonactor addresses remain guarded; no generic bus ABI added. */
            if(link){
                lst=state_lo(link);
                if(lst==5u){
                    lsel=wf_u8(link,O_SELECTOR+1u);
                    if((lsel==0x35u||lsel==0x0Eu||lsel==0x0Fu||lsel==0x23u)&&
                       !bit(link,O_SELECTOR,4)){
                        return WF_OK; /* exact $EC60->$EDB0 carry-clear guard */
                    }
                }
            }
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0078u);
            wf_set_be16(a,O_MASH,0u);*consumed=1;
            if(tr){tr->raw_action=0x78u;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EC6Au;}
            return WF_OK;
        }
    }
    if(st==5u){
        uint16_t out=0u;
        switch(sel){
            case 0x5D:out=0x67u;break;case 0x5E:out=0x56u;break;
            case 0x5F:out=0x5Cu;break;case 0x60:out=0x5Bu;break;
            case 0x61:out=0x59u;break;case 0x62:out=0x5Au;break;
            case 0x63:out=0x55u;break;case 0x52:out=0x58u;break;
            case 0x53:out=0x57u;break;case 0x64:out=0x4Bu;break;
            default:break;
        }
        if(out!=0u){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,out);wf_set_be16(a,O_MASH,0u);
            *consumed=1;
            if(tr){tr->raw_action=(uint8_t)out;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EBC4u;}
        }
    }
    return WF_OK;
}

/* $F12E. Exact fixed-object link predicate.  The Y gate is deliberately
 * byte-sized after the WORD subtraction/normalization; this reproduces the
 * ROM's CMPI.B #$20,D0 rather than replacing it with a conventional WORD
 * absolute-distance test. */
static int f12e_link(wf_actor_raw_t *a,unsigned actor_slot,wf_actor_raw_t *obj,uint32_t obj_rom_ptr){
    uint16_t d;
    if(!a||!obj)return 0;
    if(wf_be32(obj,O_OBJLINK)!=0u)return 0;
    if(wf_u8(obj,O_OBJ_STATE+1u)!=3u)return 0;
    d=(uint16_t)(wf_be16(obj,O_Y)-wf_be16(a,O_Y));
    if(wf_be16(obj,O_Y)<wf_be16(a,O_Y))d=(uint16_t)(0u-d);
    if((uint8_t)d>=0x20u)return 0;
    d=(uint16_t)(wf_be16(obj,O_X)-wf_be16(a,O_X));
    if(wf_be16(obj,O_X)<wf_be16(a,O_X))d=(uint16_t)(0u-d);
    if(d>=0x20u)return 0;
    wf_set_be32(obj,O_OBJLINK,actor_ptr(actor_slot));
    wf_set_be32(a,O_OBJLINK,obj_rom_ptr);
    return 1;
}

/* $F106. Object0 is tested first; the first carry-set $F12E result ends the
 * scan.  Missing WP31 fixed-object ownership fails closed. */
static wf_status_t f106_scan(wf_actor_raw_t *a,unsigned actor_slot,
                             const wf_r17_human_raw23_ctx_t *ctx,
                             wf_r17_human_raw23_trace_t *tr,int *linked){
    *linked=0;
    if(!ctx->fixed_object0||!ctx->fixed_object1)return block(tr,0x0000F106u);
    if(f12e_link(a,actor_slot,ctx->fixed_object0,0x001C0F1Cu)){*linked=1;return WF_OK;}
    if(f12e_link(a,actor_slot,ctx->fixed_object1,0x001C1028u)){*linked=1;return WF_OK;}
    return WF_OK;
}

/* $F0BA->$F106.  On a successful object link the ROM writes state5/raw70 and
 * returns carry-set to consume the normal fresh-button route. */
static wf_status_t pickup_f0ba(wf_actor_raw_t *a,unsigned actor_slot,
                               const wf_r17_human_raw23_ctx_t *ctx,
                               wf_r17_human_raw23_trace_t *tr,int *consumed){
    uint8_t st;int linked=0;wf_status_t s;
    *consumed=0;
    if((ctx->mode_0161&2u)==0u)return WF_OK;
    if(bit(a,O_REL_FLAGS,7))return WF_OK;
    if(!bit(a,O_FLAGS33,2))return WF_OK;
    st=state_lo(a);
    if(st!=0u&&st!=1u)return WF_OK;
    s=f106_scan(a,actor_slot,ctx,tr,&linked);
    if(s!=WF_OK)return s;
    if(!linked)return WF_OK;
    wf_set_be16(a,O_STATE,5u);
    wf_set_be16(a,O_SELECTOR,0x0070u);
    *consumed=1;
    if(tr){tr->raw_action=0x70u;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000F0EAu;}
    return WF_OK;
}

/* $F178.  The caller keeps D0 as the raw-action result; this helper only
 * establishes actor->target and, when empty, reciprocal target->actor +$26. */
static void f178_link(wf_actor_raw_t *a,unsigned actor_slot,wf_actor_raw_t *t,unsigned target_slot){
    wf_set_be32(a,O_LINK,actor_ptr(target_slot));
    if(wf_be32(t,O_LINK)==0u)wf_set_be32(t,O_LINK,actor_ptr(actor_slot));
}

/* $EA42 direct pre-table override.  Carry-set in ROM bypasses all numeric
 * classifiers and reaches the common $DFE8 state5/raw-action writer.  The two
 * $1129E-dependent families remain explicit fail-closed blockers. */
static void rng_context_with_mode(wf_r17_rng21b4_context_t *dst,
                                  const wf_r17_rng21b4_context_t *src,
                                  uint16_t mode){
    dst->state_1c005c=src->state_1c005c;
    dst->global_1c0080=src->global_1c0080;
    dst->d[0]=src->d[0];dst->d[1]=(src->d[1]&0xFFFF0000u)|(uint32_t)mode;
    /* $DEB2 CLR.L D3 is local to DE86, whose MOVEM restores D0-D3. */
    dst->d[2]=src->d[2];dst->d[3]=0u;dst->d[4]=src->d[4];
    dst->d[5]=src->d[5];dst->d[6]=src->d[6];dst->d[7]=src->d[7];
    dst->valid=src->valid;
}

static wf_status_t ea42_direct(wf_runtime_t *rt,wf_actor_raw_t *a,unsigned actor_slot,
                               const wf_r17_human_raw23_ctx_t *ctx,
                               wf_r17_human_raw23_trace_t *tr,int *consumed){
    wf_actor_raw_t *t,*a2;unsigned ts=0,a2s=0;uint8_t st;uint16_t sel,raw=0u;unsigned code=0u;
    *consumed=0;
    if(bit(a,O_REL_FLAGS,7))return WF_OK;
    st=state_lo(a);
    if(st==0u||st==1u){
        t=actor_from_ptr(rt,wf_be32(a,O_TARGET),&ts);
        if(!t)return block(tr,0x0000EA42u);
        if(state_lo(t)!=5u)return WF_OK;
        sel=(uint16_t)(wf_be16(t,O_SELECTOR)&0x80FFu);
        if(sel==0x8009u||sel==0x801Au){raw=0x000Au;code=3u;}
        else if(sel==0x800Fu||sel==0x800Eu){raw=0x000Au;code=0u;}
        else if(sel==0x8022u||sel==0x803Bu){
            raw=0x0072u;f178_link(a,actor_slot,t,ts);
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,raw);
            *consumed=1;if(tr){tr->raw_action=(uint8_t)raw;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EAC2u;}
            return WF_OK;
        }else if(sel==0x8048u){
            uint16_t d2=0x000Au;
            a2=actor_from_ptr(rt,wf_be32(t,O_LINK),&a2s);
            if(!a2)return block(tr,0x0000EAD4u);
            if(wf_be32(a2,O_AUX9C)!=0u||bit(a2,O_SELECTOR,5))return WF_OK;
            if((ctx->mode_0161&1u)!=0u&&wf_u8(a,O_INPUT_FRESH_LO)!=1u)d2=0x0049u;
            code=0x0Eu;
            if(d2==0x000Au)code=wf_u8(a2,O_STATE4_LO)==8u?0x0Fu:0x10u;
            if(!e958_carry(a,t,code))return WF_OK;
            raw=d2;
            if(raw==0x000Au)wf_set_be32(a,O_LINK,actor_ptr(ts));
            else{wf_set_be32(a,O_LINK,actor_ptr(a2s));wf_set_be32(a2,O_AUX9C,actor_ptr(actor_slot));}
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,raw);
            *consumed=1;if(tr){tr->raw_action=(uint8_t)raw;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EB26u;}
            return WF_OK;
        }else if(sel==0x8049u){
            if(!e958_carry(a,t,0x0Du))return WF_OK;
            raw=0x000Au;wf_set_be32(a,O_LINK,actor_ptr(ts));wf_set_be32(t,O_AUX9C,actor_ptr(actor_slot));
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,raw);
            *consumed=1;if(tr){tr->raw_action=(uint8_t)raw;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EB48u;}
            return WF_OK;
        }else return WF_OK;
        if(!e958_carry(a,t,code))return WF_OK;
        f178_link(a,actor_slot,t,ts);
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,raw);
        *consumed=1;if(tr){tr->raw_action=(uint8_t)raw;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EA90u;}
        return WF_OK;
    }
    /* $EB68/$EB8E.  These two branches call the shared probability helper
     * $1129E.  No synthetic RNG is allowed: without the exact live D-register
     * snapshot and shared $21B4 state, the branch remains fail-closed. */
    if(wf_be16(a,O_STATE)==0x8006u&&wf_be16(a,O_TIMER44)!=0u&&wf_u8(a,0x25u)==0u){
        wf_r17_rng21b4_context_t rc;wf_r17_1129e_trace_t rr;int carry=0;wf_status_t rs;
        if(ctx->m68k_live&&ctx->rng_state_1c005c){
            /* $DE86 saves D0-D3; $DEB2 clears D3 before EA42. The
             * $EB7C mode write and $1129E temporaries must not escape
             * the matching $DFFC MOVEM restore in the caller. */
            wf_r17_m68k_shadow_t call_regs=*ctx->m68k_live;
            wf_r17_m68k_clr_l(&call_regs,3u);
            wf_r17_m68k_move_w(&call_regs,1u,2u);
            rs=wf_r17_1129e_live(rt,actor_slot,ctx->mode_0161,ctx->global_0162,ctx->rng_state_1c005c,ctx->global_1c0080,&call_regs,&rr,&carry);
        }else{
            if(!ctx->rng_1129e)return block(tr,0x0001129Eu);
            rng_context_with_mode(&rc,ctx->rng_1129e,2u);
            rs=wf_r17_1129e(rt,actor_slot,ctx->mode_0161,ctx->global_0162,&rc,&rr,&carry);
        }
        if(rs!=WF_OK)return block(tr,rr.blocked_dependency_rom?rr.blocked_dependency_rom:0x0001129Eu);
        if(carry){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x003Fu);
            *consumed=1;if(tr){tr->raw_action=0x3Fu;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EB88u;}
            return WF_OK;
        }
    }
    if(st==5u&&wf_u8(a,O_SELECTOR+1u)==0x7Bu){
        wf_r17_rng21b4_context_t rc;wf_r17_1129e_trace_t rr;int carry=0;wf_status_t rs;
        if(bit(a,O_SELECTOR,7))return WF_OK;
        wf_set_u8(a,O_SELECTOR,(uint8_t)(wf_u8(a,O_SELECTOR)|0x80u)); /* $EB9E BSET #7,+$60 */
        if(ctx->m68k_live&&ctx->rng_state_1c005c){
            /* $DE86 saves D0-D3; $DEB2 clears D3 before EA42. The
             * $EBA6 mode write and $1129E temporaries must not escape
             * the matching $DFFC MOVEM restore in the caller. */
            wf_r17_m68k_shadow_t call_regs=*ctx->m68k_live;
            wf_r17_m68k_clr_l(&call_regs,3u);
            wf_r17_m68k_move_w(&call_regs,1u,0u);
            rs=wf_r17_1129e_live(rt,actor_slot,ctx->mode_0161,ctx->global_0162,ctx->rng_state_1c005c,ctx->global_1c0080,&call_regs,&rr,&carry);
        }else{
            if(!ctx->rng_1129e)return block(tr,0x0001129Eu);
            rng_context_with_mode(&rc,ctx->rng_1129e,0u);
            rs=wf_r17_1129e(rt,actor_slot,ctx->mode_0161,ctx->global_0162,&rc,&rr,&carry);
        }
        if(rs!=WF_OK)return block(tr,rr.blocked_dependency_rom?rr.blocked_dependency_rom:0x0001129Eu);
        if(carry){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x007Cu);
            *consumed=1;if(tr){tr->raw_action=0x7Cu;tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EBB2u;}
            return WF_OK;
        }
    }
    return WF_OK;
}

static wf_status_t maybe_swap_target(wf_runtime_t *rt,wf_actor_raw_t *a,
                                     wf_r17_human_raw23_trace_t *tr){
    wf_actor_raw_t *primary,*cand;unsigned cs=0,ps=0;uint8_t st;uint32_t old_primary;
    old_primary=wf_be32(a,O_TARGET);
    primary=actor_from_ptr(rt,old_primary,&ps);
    if(!primary)return block(tr,0x0000DEE2u);
    if((wf_be16(primary,O_FLAGS34)&7u)!=0u)return WF_OK;
    st=state_lo(a);cand=primary;cs=ps;
    if((st==1u&&wf_u8(a,O_AF)==0u)||st==0u){
        uint32_t candidate=wf_be32(a,O_TARGET2);
        /* $DF06 loads the literal LONG, then $DF0A reads BYTE $21(A1).
         * Physical zero addresses canonical ROM[$21]=$FF on the 24-bit
         * 68000 bus, so CMP.B #4 fails and execution continues at $DF3A
         * without swapping targets. Other nonactor addresses stay guarded. */
        if((candidate&0x00FFFFFFu)==0u)return WF_OK;
        cand=actor_from_ptr(rt,candidate,&cs);
        if(!cand)return block(tr,0x0000DF06u);
    }
    if(cand!=primary){
        if(state_lo(cand)!=4u)return WF_OK;
        if(wf_u8(cand,O_STATE4_LO)!=9u&&wf_u8(cand,O_STATE4_LO)!=8u)return WF_OK;
    }
    if(!e958_carry(a,cand,0u))return WF_OK;
    wf_set_be32(a,O_TARGET2,old_primary);
    wf_set_be32(a,O_TARGET,actor_ptr(cs));
    return WF_OK;
}

/* Ordered $E002..$E4BA action-category classifier chain.  This models the
 * numeric ROM fields and side effects only; semantic move names are outside
 * this owner.  `alt_e496` represents the $DE86 branch taken when +$32.bit0
 * is set, which skips $EA42 and classifiers $E002..$E442. */
static wf_status_t classify_action(wf_runtime_t *rt,wf_actor_raw_t *a,unsigned actor_slot,
                                   const wf_r17_human_raw23_ctx_t *ctx,
                                   wf_r17_human_raw23_trace_t *tr,int alt_e496,
                                   uint8_t *category,int *d3_nonzero,wf_actor_raw_t **target_out){
    wf_actor_raw_t *t,*link;unsigned ts=0,ls=0;uint8_t st,sub,sel;uint16_t tier,ch;
    *d3_nonzero=0;*target_out=0;
    st=state_lo(a);
    if(alt_e496){
        /* $E496: alternate branch only. */
        if(st==1u&&wf_u8(a,O_AF)==1u){
            t=actor_from_ptr(rt,wf_be32(a,O_TARGET),&ts);
            if(!t)return block(tr,0x0000E496u);
            f178_link(a,actor_slot,t,ts);*category=0x13u;*target_out=t;
            if(tr)tr->last_stage_rom=0x0000E496u;
            return WF_OK;
        }
        goto e4ba;
    }

    /* $E002: category $11. */
    if(bit(a,O_REL_FLAGS,7)&&(st==0u||st==1u)){
        *category=0x11u;if(tr)tr->last_stage_rom=0x0000E002u;return WF_OK;
    }

    t=actor_from_ptr(rt,wf_be32(a,O_TARGET),&ts);
    if(!t)return block(tr,0x0000E02Eu);
    *target_out=t;

    /* $E02E: state2 table at $E070; +$62.bit7 selects the second $24-byte half. */
    if(!bit(a,O_FLAGS34,1)&&st==2u&&wf_be16(a,O_TIMER44)==0u){
        tier=wf_be16(t,O_TIER);ch=wf_be16(a,O_CHAR);
        if(tier>2u||ch>=12u)return block(tr,0x0000E02Eu);
        *category=wf_r17_e070[(bit(a,0x62u,7)?36u:0u)+(unsigned)tier*12u+(unsigned)ch];
        /* $E04E BCHG chooses the table half from the old bit, then toggles it. */
        wf_set_u8(a,0x62u,(uint8_t)(wf_u8(a,0x62u)^0x80u));
        if(tr)tr->last_stage_rom=0x0000E02Eu;
        return WF_OK;
    }

    /* $E0B8: category $05 and reciprocal $F178 link. */
    if(!bit(a,O_FLAGS34,1)&&(st==0u||st==1u)&&wf_be32(t,O_LINK)==0u&&state_lo(t)==4u){
        sub=wf_u8(t,O_STATE4_LO);
        if(sub==8u||sub==6u){
            /* $E104 writes WORD D4, which is not in $DE86's D0-D3
             * save set. D3 stays zero; no $E82E remap is requested. */
            wf_r17_m68k_move_w(ctx->m68k_live,4u,1u);
            f178_link(a,actor_slot,t,ts);*category=5u;
            if(tr)tr->last_stage_rom=0x0000E0B8u;
            return WF_OK;
        }
        /* $E110: categories $06/$07, same $F178 side effect. */
        if(sub==9u||sub==7u){
            /* Both $E164 and $E174 write WORD D4, preserving its high word. */
            wf_r17_m68k_move_w(ctx->m68k_live,4u,1u);
            f178_link(a,actor_slot,t,ts);*category=(wf_be16(t,O_TIER)==0u)?6u:7u;
            if(tr)tr->last_stage_rom=0x0000E110u;
            return WF_OK;
        }
    }

    /* $E180: linked pair category $08; sets linked +$44.bit6. */
    link=actor_from_ptr(rt,wf_be32(a,O_LINK),&ls);
    (void)ls;
    if(link&&st==0x0Bu&&!bit(a,O_TIMER44,5)&&!bit(a,O_TIMER44,6)){
        wf_set_u8(link,O_TIMER44,(uint8_t)(wf_u8(link,O_TIMER44)|0x40u));
        *category=8u;*target_out=link;if(tr)tr->last_stage_rom=0x0000E180u;return WF_OK;
    }

    /* $E1B0: linked pair category $09; clears actor +$44. */
    if(link&&st==0x0Cu&&wf_u8(a,O_TIMER44+1u)==1u){
        wf_set_be16(a,O_TIMER44,0u);*category=9u;*target_out=link;
        if(tr)tr->last_stage_rom=0x0000E1B0u;
        return WF_OK;
    }

    /* $E1D4: state5 raw15/raw16 dynamic table.  BCHG D1,+$62(A0)
     * uses linked target +$70 as the memory bit number and its old value as
     * the table-half selector. */
    sel=wf_u8(a,O_SELECTOR+1u);
    if(link&&st==5u&&(sel==0x15u||sel==0x16u)){
        uint8_t v,mask;unsigned old;
        tier=wf_be16(link,O_TIER);ch=wf_be16(a,O_CHAR);
        if(tier>2u||ch>=12u)return block(tr,0x0000E1D4u);
        wf_set_be16(a,O_TIMER44,0u);wf_set_be16(a,0x46u,0u);*d3_nonzero=1;
        mask=(tier==0u)?0x01u:((tier==1u)?0x02u:0x04u);v=wf_u8(a,0x62u);old=(v&mask)!=0u;
        wf_set_u8(a,0x62u,(uint8_t)(v^mask));
        *category=wf_r17_e232[(unsigned)tier*24u+(old?12u:0u)+(unsigned)ch];
        *target_out=link;if(tr)tr->last_stage_rom=0x0000E1D4u;return WF_OK;
    }

    /* $E27A: actor state9.  Global mode bit1 sets actor +$33.bit2. */
    if(st==9u){
        if((ctx->mode_0161&2u)!=0u)wf_set_u8(a,O_FLAGS33,(uint8_t)(wf_u8(a,O_FLAGS33)|0x04u));
        if(state_lo(t)==4u&&(wf_u8(t,O_STATE4_LO)==8u||wf_u8(t,O_STATE4_LO)==9u)) *category=0x0Fu;
        else if(state_lo(t)==5u&&wf_u8(t,O_SELECTOR+1u)==0x61u) *category=0x0Fu;
        else *category=0x0Eu;
        f178_link(a,actor_slot,t,ts);if(tr)tr->last_stage_rom=0x0000E27Au;return WF_OK;
    }

    /* $E2DC: target +$33.bit4 dynamic table at $E33A. */
    if(!bit(a,O_FLAGS34,5)&&bit(t,O_FLAGS33,4)&&(st==0u||st==1u)){
        tier=wf_be16(t,O_TIER);ch=wf_be16(a,O_CHAR);
        if(tier>2u||ch>=12u)return block(tr,0x0000E2DCu);
        *d3_nonzero=1;
        *category=wf_r17_e33a[(bit(a,0x62u,7)?36u:0u)+(unsigned)tier*12u+(unsigned)ch];
        /* $E312 BCHG chooses the table half from the old bit, then toggles it. */
        wf_set_u8(a,0x62u,(uint8_t)(wf_u8(a,0x62u)^0x80u));
        f178_link(a,actor_slot,t,ts);if(tr)tr->last_stage_rom=0x0000E2DCu;return WF_OK;
    }

    /* $E382: category $10 behind-context predicate; WORD differences are
     * reduced exactly as SUB.W/BCC/NEG.W before unsigned CMP.W limits. */
    if(!bit(a,O_FLAGS34,5)&&(st==0u||st==1u)&&!(st==1u&&wf_u8(a,O_AF)!=0u)&&
       wf_be32(t,O_LINK)==0u&&state_lo(t)==4u&&wf_u8(t,O_STATE4_LO)==1u){
        uint16_t dy=(uint16_t)(wf_be16(a,O_Y)-wf_be16(t,O_Y));
        uint16_t dx=(uint16_t)(wf_be16(a,O_X)-wf_be16(t,O_X));
        if(wf_be16(a,O_Y)<wf_be16(t,O_Y))dy=(uint16_t)(0u-dy);
        if(wf_be16(a,O_X)<wf_be16(t,O_X))dx=(uint16_t)(0u-dx);
        if(dy<0x10u&&dx<0x50u&&wf_u8(a,O_FACING)==wf_u8(t,O_FACING)){
            *category=0x10u;f178_link(a,actor_slot,t,ts);wf_set_be16(t,0x52u,0u);
            if(tr)tr->last_stage_rom=0x0000E382u;
            return WF_OK;
        }
    }

    /* $E40C: category $12 against target state5/raw61. */
    if((st==0u||st==1u)&&state_lo(t)==5u&&wf_u8(t,O_SELECTOR+1u)==0x61u){
        *category=0x12u;f178_link(a,actor_slot,t,ts);if(tr)tr->last_stage_rom=0x0000E40Cu;return WF_OK;
    }

    /* $E442: category $14 against exact target state5 raw set. */
    if((st==0u||st==1u)&&state_lo(t)==5u){
        sel=wf_u8(t,O_SELECTOR+1u);
        if(sel==0x25u||sel==0x36u||sel==0x53u||sel==0x52u){
            *category=0x14u;f178_link(a,actor_slot,t,ts);if(tr)tr->last_stage_rom=0x0000E442u;return WF_OK;
        }
    }

e4ba:
    /* $E4BA fallback category $00. */
    if(bit(a,O_FLAGS34,1)||bit(a,O_FLAGS32,0))return WF_BLOCKED;
    if((ctx->mode_0161&2u)!=0u&&bit(a,O_FLAGS32,0))return WF_BLOCKED;
    st=state_lo(a);
    if(st==0u||st==1u){*category=0u;if(tr)tr->last_stage_rom=0x0000E4BAu;return WF_OK;}
    return WF_BLOCKED;
}

/* $E926 raw-$48 conditional rewrite. */
static uint8_t e926_rewrite(const wf_actor_raw_t *a,const wf_actor_raw_t *t,uint8_t raw){
    if(raw!=0x48u)return raw;
    if(bit(t,O_FLAGS33,0)&&bit(a,O_FLAGS33,0)&&!bit(a,O_FLAGS33,2))return raw;
    return 0x0Au;
}

/* $E830 optional post-selection remap, called only when D3!=0. */
static wf_status_t e830_remap(wf_actor_raw_t *a,uint8_t *raw,wf_r17_human_raw23_trace_t *tr){
    uint16_t idx=wf_be16(a,0x30u),ch=wf_be16(a,O_CHAR);uint8_t mode,need;unsigned p;
    if(idx>=15u||ch>=12u)return block(tr,0x0000E830u);
    mode=wf_r17_e917[idx];if(mode==0xFFu)return WF_OK;
    p=wf_r17_e830_offsets[ch];
    for(;;){
        uint8_t side=wf_r17_e830_records[p++];uint8_t match,repl;
        if(side==0xFFu)return WF_OK;
        match=wf_r17_e830_records[p++];repl=wf_r17_e830_records[p++];
        if(match!=*raw)continue;
        need=(uint8_t)((bit(a,O_FACING,7)?1u:0u)^((mode!=0u)?1u:0u));
        if(side==need)*raw=repl;
        return WF_OK;
    }
}

/* $EF9A action-specific geometry/fallback filter. `consumed` means the helper
 * itself wrote the actor main state/selector and the caller must not perform
 * the generic state5/raw write. */
static wf_status_t ef9a_filter(wf_actor_raw_t *a,wf_actor_raw_t *t,uint8_t fresh,uint8_t ch,
                               uint8_t *raw,wf_r17_human_raw23_trace_t *tr,int *consumed){
    uint8_t meta;unsigned s1,s2;
    *consumed=0;
    if(*raw>=0x4Au)return WF_OK;
    meta=wf_r17_f070[*raw];if(meta==0u)return WF_OK;
    if((meta&0x80u)==0u){
        if(e958_carry(a,t,(unsigned)(meta&0x7Fu)))return WF_OK;
        goto fallback;
    }
    s1=(*raw==8u)?1u:12u;
    if(e958_carry(a,t,s1)){
        if(*raw!=8u){if(ch>=12u)return block(tr,0x0000EFFEu);*raw=wf_r17_effe[ch];}
        return WF_OK;
    }
    s2=(*raw==8u)?3u:2u;
    if(e958_carry(a,t,s2)){
        if(*raw==8u)*raw=0x0Au;
        return WF_OK;
    }
fallback:
    if(fresh==3u){wf_set_be16(a,O_STATE,2u);}
    else if(fresh==1u){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0u);}
    else {wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x72u);}
    *consumed=1;if(tr){tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000EF9Au;}
    return WF_OK;
}

wf_status_t wf_r17_human_dcde_raw23(wf_runtime_t *rt,unsigned slot,const wf_r17_human_raw23_ctx_t *ctx,
                                    wf_r17_human_raw23_trace_t *tr){
    wf_actor_raw_t *a,*primary,*target=0;wf_status_t s;int consumed=0,d3=0,alt=0;uint8_t fresh,st,cat=0,raw,ch;
    clear_trace(tr);
    if(!rt||!ctx||slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
    a=&rt->actors[slot];
    if(tr)tr->last_stage_rom=0x0000DCDEu;
    if(!bit(a,O_STATE,7)||wf_be16(a,O_FE)!=0u)return WF_OK;
    s=dd02(rt,a,ctx,tr,&consumed);if(s!=WF_OK||consumed)return s;
    if(tr)tr->last_stage_rom=0x0000DE86u;
    if((ctx->mode_0161&2u)!=0u&&!bit(a,O_FLAGS32,1)&&!bit(a,O_FLAGS33,2))goto direction_block;
    fresh=(uint8_t)(wf_u8(a,O_INPUT_FRESH_LO)&3u);if(tr)tr->fresh_mask=fresh;
    if(fresh==0u)goto direction_block;
    s=ebc4(rt,a,ctx,tr,&consumed);if(s!=WF_OK||consumed)return s;
    s=pickup_f0ba(a,slot,ctx,tr,&consumed);if(s!=WF_OK||consumed)return s;
    st=state_lo(a);if(st==4u)goto direction_block;
    alt=bit(a,O_FLAGS32,0);
    if(!alt){
        s=ea42_direct(rt,a,slot,ctx,tr,&consumed);if(s!=WF_OK||consumed)return s;
        primary=actor_from_ptr(rt,wf_be32(a,O_TARGET),0);if(!primary)return block(tr,0x0000EA42u);
        s=maybe_swap_target(rt,a,tr);if(s!=WF_OK)return s;
    }
    s=classify_action(rt,a,slot,ctx,tr,alt,&cat,&d3,&target);
    if(s==WF_BLOCKED){
        /* A no-match classifier chain is ROM carry-clear and falls through to
         * the held-direction owner.  Only explicit dependency blockers retain
         * a nonzero blocked address. */
        if(tr&&tr->blocked_dependency_rom!=0u)return s;
        goto direction_block;
    }
    if(s!=WF_OK)return s;
    if(tr)tr->category=cat;
    ch=(uint8_t)wf_be16(a,O_CHAR);
    if(ch>=12u||cat>=21u)return block(tr,0x0000E4FEu);
    raw=wf_r17_action_matrix[ch][cat][fresh-1u];if(tr)tr->raw_action=raw;
    if(raw==0xFFu){
        wf_set_be16(a,O_STATE,2u);wf_set_be32(a,O_LINK,0u);
        if(tr){tr->consumed_before_raw23=1u;tr->last_stage_rom=0x0000DFC2u;}
        return WF_OK;
    }
    if(!target)target=actor_from_ptr(rt,wf_be32(a,O_TARGET),0);
    if(!target)return block(tr,0x0000E926u);
    raw=e926_rewrite(a,target,raw);
    s=ef9a_filter(a,target,fresh,ch,&raw,tr,&consumed);if(s!=WF_OK||consumed)return s;
    if(d3){s=e830_remap(a,&raw,tr);if(s!=WF_OK)return s;}
    wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,raw);
    if(tr){tr->raw_action=raw;tr->selected_raw23=(raw==0x23u);tr->last_stage_rom=0x0000DFE8u;}
    return WF_OK;
direction_block:
    {
        wf_r17_human_ddc8_ctx_t dc={ctx->mode_0161,ctx->global_1670};
        wf_r17_human_ddc8_trace_t dt;wf_status_t ds=wf_r17_human_ddc8(rt,slot,&dc,&dt);
        if(tr){tr->last_stage_rom=dt.last_stage_rom?dt.last_stage_rom:0x0000DDC8u;tr->blocked_dependency_rom=dt.blocked_dependency_rom;}
        return ds;
    }
}
