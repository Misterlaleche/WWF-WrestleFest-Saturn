#include "wf_r17_rng1129e.h"
#include "wf_r17_m68k_shadow.h"

#define BIT8(n) ((uint8_t)(1u << (n)))
#define ACTOR_BASE 0x001C05B0u
#define ACTOR_STRIDE 0x10Cu

enum { O_TARGET=0x7A, O_LINK=0x26, O_TIER=0x70, O_FLAGS56=0x56 };

static uint16_t lo16(uint32_t v){ return (uint16_t)v; }
static uint32_t put_lo16(uint32_t v,uint16_t w){ return (v&0xFFFF0000u)|(uint32_t)w; }
static uint32_t put_lo8(uint32_t v,uint8_t b){ return (v&0xFFFFFF00u)|(uint32_t)b; }
static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){ return (wf_u8(a,o)&BIT8(b))!=0u; }

static wf_actor_raw_t *actor_from_ptr(wf_runtime_t *rt,uint32_t p){
    uint32_t d;unsigned s;
    p&=0x00ffffffu; /* 68000 dereference bus; the stored LONG remains intact. */
    if(!rt||p<ACTOR_BASE)return 0;
    d=p-ACTOR_BASE;
    if((d%ACTOR_STRIDE)!=0u)return 0;
    s=(unsigned)(d/ACTOR_STRIDE);
    return s<WF_ACTOR_SLOTS?&rt->actors[s]:0;
}

static void clear_trace(wf_r17_1129e_trace_t *tr){
    if(!tr)return;
    tr->blocked_dependency_rom=0u;tr->rng_before=0u;tr->rng_after=0u;
    tr->d1_at_rng=0u;tr->d2_at_rng=0u;tr->mode_entry=0u;tr->threshold=0u;
    tr->rng_called=0u;tr->carry=0u;
}
static wf_status_t blocked(wf_r17_1129e_trace_t *tr,uint32_t pc){
    if(tr)tr->blocked_dependency_rom=pc;
    return WF_BLOCKED;
}

wf_status_t wf_r17_1129e(wf_runtime_t *rt,
                         unsigned actor_slot,
                         uint8_t global_0161,
                         uint16_t global_0162,
                         const wf_r17_rng21b4_context_t *rng,
                         wf_r17_1129e_trace_t *tr,
                         int *carry_out){
    static const uint8_t t0_main[30]={
        3,2,1,2,2,1,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    };
    static const uint8_t t0_alt[3]={3,2,3};
    static const uint8_t t1_main[50]={
        0x10,0x0F,0x0C,0x09,0x07,0x10,0x0E,0x0B,0x08,0x06,
        0x10,0x0D,0x0A,0x08,0x06,0x10,0x0C,0x09,0x07,0x05,
        0x10,0x0C,0x08,0x07,0x05,0x10,0x0C,0x08,0x07,0x06,
        0x10,0x0C,0x08,0x07,0x06,0x10,0x0C,0x08,0x07,0x06,
        0x10,0x0C,0x08,0x07,0x06,0x10,0x0B,0x07,0x06,0x05
    };
    static const uint8_t t1_alt[5]={0x10,0x0E,0x0A,0x08,0x07};
    static const uint8_t t1_mode[5]={0x10,0x0F,0x0D,0x0B,0x08};
    static const uint8_t t2_target7[3]={2,1,1};
    static const uint8_t t2_target0[3]={3,2,3};
    static const uint8_t t3[3]={3,2,1};
    wf_actor_raw_t *a,*p;uint32_t d[8],out;uint16_t mode,tier,idx;uint8_t threshold=0u;unsigned i;

    clear_trace(tr);
    if(carry_out)*carry_out=0;
    if(!rt||actor_slot>=WF_ACTOR_SLOTS||!rng||!rng->valid||!rng->state_1c005c)return blocked(tr,0x000021B4u);
    a=&rt->actors[actor_slot];
    for(i=0u;i<8u;i++)d[i]=rng->d[i];
    mode=lo16(d[1]);
    if(tr)tr->mode_entry=mode;
    if(mode>3u)return blocked(tr,0x000112AAu);
    d[1]=put_lo16(d[1],(uint16_t)(mode<<2)); /* $112A8 LSL.W #2,D1 */

    if(mode==0u){
        d[1]=0u; /* $112C0 CLR.L D1 */
        p=actor_from_ptr(rt,wf_be32(a,O_TARGET));
        if(!p)return blocked(tr,0x000112C8u);
        tier=wf_be16(a,O_TIER);
        if(bit(p,O_FLAGS56,7)){
            uint32_t prod;
            if(global_0162>=10u||tier>=3u)return blocked(tr,0x000113ACu);
            prod=(uint32_t)global_0162*3u; /* $112E2 MULU #3,D1 after MOVE.W */
            d[1]=prod;
            d[1]=put_lo16(d[1],(uint16_t)(lo16(d[1])+tier)); /* $112E6 ADD.W +$70,D1 */
            idx=lo16(d[1]);
            threshold=t0_main[idx];
        }else{
            if(tier>=3u)return blocked(tr,0x000113CAu);
            d[1]=put_lo16(d[1],tier); /* CLR.L D1 then $112E6 ADD.W +$70,D1 */
            idx=lo16(d[1]);
            threshold=t0_alt[idx];
        }
        d[1]=put_lo8(d[1],threshold); /* $112EA MOVE.B 0(A1,D1.W),D1 */
        if(tr){tr->rng_before=*rng->state_1c005c;tr->d1_at_rng=d[1];tr->d2_at_rng=d[2];tr->threshold=threshold;tr->rng_called=1u;}
        out=wf_r17_rng21b4_step(rng->state_1c005c,rng->global_1c0080,d);
        if(tr)tr->rng_after=out;
        if((uint16_t)(out&0x000Fu)<(uint16_t)threshold){ if(carry_out)*carry_out=1;if(tr)tr->carry=1u; }
        return WF_OK;
    }

    if(mode==1u){
        if(bit(a,O_FLAGS56,7)){ if(carry_out)*carry_out=1;if(tr)tr->carry=1u;return WF_OK; }
        p=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!p)return blocked(tr,0x0001130Cu);
        d[2]=put_lo16(d[2],(uint16_t)(lo16(d[2])&0x00FFu));
        idx=lo16(d[2]);if(idx>=4u)idx=4u;
        if(!bit(p,O_FLAGS56,7))threshold=t1_alt[idx];
        else if((global_0161&1u)!=0u)threshold=t1_mode[idx];
        else{
            if(global_0162>=10u)return blocked(tr,0x000113CDu);
            threshold=t1_main[(unsigned)global_0162*5u+idx];
        }
        d[2]=put_lo16(d[2],idx);
        d[2]=put_lo8(d[2],threshold); /* MOVE.B table,D2: high16 is inherited */
        if(tr){tr->rng_before=*rng->state_1c005c;tr->d1_at_rng=d[1];tr->d2_at_rng=d[2];tr->threshold=threshold;tr->rng_called=1u;}
        out=wf_r17_rng21b4_step(rng->state_1c005c,rng->global_1c0080,d);
        if(tr)tr->rng_after=out;
        if((uint16_t)(out&0x000Fu)<=(uint16_t)threshold){ if(carry_out)*carry_out=1;if(tr)tr->carry=1u; }
        return WF_OK;
    }

    tier=wf_be16(a,O_TIER);
    if(tier>=3u)return blocked(tr,mode==2u?0x00011409u:0x0001140Fu);
    d[2]=put_lo16(d[2],tier); /* $11388 MOVE.W +$70(A0),D2 */
    if(mode==2u){
        p=actor_from_ptr(rt,wf_be32(a,O_TARGET));
        if(!p)return blocked(tr,0x00011376u);
        threshold=bit(p,O_FLAGS56,7)?t2_target7[tier]:t2_target0[tier];
    }else threshold=t3[tier];
    if(tr){tr->rng_before=*rng->state_1c005c;tr->d1_at_rng=d[1];tr->d2_at_rng=d[2];tr->threshold=threshold;tr->rng_called=1u;}
    out=wf_r17_rng21b4_step(rng->state_1c005c,rng->global_1c0080,d);
    if(tr)tr->rng_after=out;
    if((uint8_t)(out&0x0Fu)<=threshold){ if(carry_out)*carry_out=1;if(tr)tr->carry=1u; }
    return WF_OK;
}

wf_status_t wf_r17_1129e_live(wf_runtime_t *rt,
                              unsigned actor_slot,
                              uint8_t global_0161,
                              uint16_t global_0162,
                              uint32_t *state_1c005c,
                              uint32_t global_1c0080,
                              const wf_r17_m68k_shadow_t *regs,
                              wf_r17_1129e_trace_t *trace,
                              int *carry_out){
    wf_r17_rng21b4_context_t c;unsigned i;
    if(!regs||!regs->valid||!state_1c005c)return WF_BLOCKED;
    c.state_1c005c=state_1c005c;c.global_1c0080=global_1c0080;c.valid=1u;
    for(i=0u;i<8u;i++)c.d[i]=regs->d[i];
    /* $1129E saves/restores D0,D1,D2; D3..D7 are not written. The persistent
       shadow therefore remains unchanged; only shared RNG state advances. */
    return wf_r17_1129e(rt,actor_slot,global_0161,global_0162,&c,trace,carry_out);
}
