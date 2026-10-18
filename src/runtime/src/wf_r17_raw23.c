#include <stdint.h>
#include "wf_r17_raw23.h"

#define BIT8(n) ((uint8_t)(1u << (n)))
#define ACTOR_BASE 0x001C05B0u
#define ACTOR_STRIDE 0x10Cu

/* Raw actor offsets used by the ROM routines in this unit.  These remain
 * numeric where the global runtime header does not yet expose a semantic name. */
enum {
    O_MOTION=0x01, O_CHAR=0x02, O_CHAR_LO=0x03, O_VISUAL=0x04,
    O_X=0x06, O_Y=0x0A, O_Z=0x0E, O_CLEAR18=0x18, O_B1B=0x1B,
    O_LATCH=0x1C, O_STATE=0x20, O_TIMER=0x22, O_FRAME=0x24,
    O_LINK=0x26, O_MAG=0x2A, O_ANGLE=0x2C, O_ANGLE_LO=0x2D,
    O_FACING=0x2E, O_FLAGS32=0x32, O_FLAGS33=0x33, O_PINFLAGS=0x35,
    O_CONTACT=0x36, O_CONTACT37=0x37, O_WORD42=0x42, O_SUBSTATE=0x44,
    O_PROFILE=0x4C, O_ROLE=0x56, O_VX=0x58, O_VY=0x5A, O_VZ=0x5C,
    O_VZDEC=0x5E, O_SELECTOR=0x60, O_STATE4=0x64, O_STAMINA=0x66,
    O_DAMAGE=0x68, O_TIER_LO=0x71, O_ANGLE_TARGET=0x9A, O_MASH=0xAA,
    O_TARGET_X=0xBE, O_DC=0xDC, O_DE=0xDE, O_FE=0xFE
};

typedef struct desc {
    uint32_t rom;
    uint16_t kind;
    uint16_t count;
    uint16_t duration[6];
    uint16_t pose[6];
} desc_t;

static const desc_t D_15CE0={0x00015CE0u,1u,2u,{0xFF00u,0x0010u},{0x01D6u,0x01D7u}};
static const desc_t D_15CF0={0x00015CF0u,1u,4u,{0x0004u,0x0018u,0x000Cu,0xFF00u},{0x0084u,0x0085u,0x00F3u,0x00F4u}};
static const desc_t D_1A24A={0x0001A24Au,1u,3u,{0x0028u,0x0018u,0x0004u},{0x0059u,0x005Au,0x0000u}};
static const desc_t D_11C62={0x00011C62u,2u,6u,{6u,6u,6u,6u,6u,6u},{9u,10u,11u,12u,13u,14u}};
static const desc_t D_11DD0={0x00011DD0u,1u,2u,{8u,12u},{0x0069u,0x0069u}};
static const int16_t TRAJ[13][2]={
    {0,0},{48,-32},{56,-8},{88,-8},{24,-8},{64,-1},{16,16},
    {40,24},{64,16},{88,0},{64,-12},{56,-1},{24,-1}
};
static const uint8_t SPEED_11D4A[12]={0x2Fu,0x35u,0x31u,0x2Du,0x38u,0x30u,0x2Cu,0x38u,0x28u,0x2Fu,0x30u,0x2Fu};
static const uint16_t TAB_176BA[6]={0x0036u,0x001Eu,0x0016u,0x0010u,0x000Au,0x0828u};

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){return (wf_u8(a,o)&BIT8(b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|BIT8(b)));}
static void clrbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~BIT8(b)));}
static void chgbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)^BIT8(b)));}
static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}

static wf_status_t block(wf_r17_raw23_trace_t *tr,uint32_t pc){if(tr)tr->blocked_dependency_rom=pc;return WF_BLOCKED;}
static uint32_t actor_rom_ptr(unsigned slot){return slot<WF_ACTOR_SLOTS?ACTOR_BASE+(uint32_t)slot*ACTOR_STRIDE:0u;}
static wf_actor_raw_t *actor_from_ptr(wf_runtime_t *rt,uint32_t p,unsigned *slot){
    uint32_t d;unsigned s;
    if(!rt||p<ACTOR_BASE)return 0;
    d=p-ACTOR_BASE;if((d%ACTOR_STRIDE)!=0u)return 0;s=(unsigned)(d/ACTOR_STRIDE);if(s>=WF_ACTOR_SLOTS)return 0;
    if(slot) *slot=s;
    return &rt->actors[s];
}
static const wf_actor_raw_t *actor_from_ptr_c(const wf_runtime_t *rt,uint32_t p,unsigned *slot){return actor_from_ptr((wf_runtime_t*)rt,p,slot);}

static wf_status_t emit(wf_r17_raw23_ctx_t *ctx,uint16_t cmd,wf_r17_raw23_trace_t *tr,uint32_t pc){
    if(!ctx||!ctx->audio_emit)return block(tr,pc);
    return ctx->audio_emit(cmd,ctx->user);
}

/* Avoid compiler runtime division helpers on SH-2 while retaining 68000 DIVU/DIVS
 * truncation semantics. */
static uint32_t udiv32_16(uint32_t n,uint16_t d){
    uint32_t q=0u,r=0u;unsigned i;
    for(i=0u;i<32u;i++){
        r=(r<<1)|((n&0x80000000u)!=0u?1u:0u);
        n<<=1;
        q<<=1;
        if(r>=(uint32_t)d){r-=(uint32_t)d;q|=1u;}
    }
    return q;
}
static int divs32_16(int32_t n,uint16_t d,int16_t *out){
    uint32_t mag,q;int neg=n<0;int32_t v;
    if(d==0u||!out)return 0;
    mag=neg?((uint32_t)(-(n+1))+1u):(uint32_t)n;q=udiv32_16(mag,d);
    if((!neg&&q>32767u)||(neg&&q>32768u))return 0;
    v=neg?-(int32_t)q:(int32_t)q;*out=(int16_t)v;return 1;
}

wf_status_t wf_r17_trajectory_26ae(wf_actor_raw_t *a,const wf_actor_raw_t *t,uint16_t idx){
    int32_t ox,oy,nx,ny;uint16_t vz,dec,d;uint32_t q;int16_t vx,vy;
    if(!a||!t||idx>=13u)return WF_BAD_STATE;
    ox=TRAJ[idx][0];oy=TRAJ[idx][1];if(bit(t,O_FACING,7))ox=-ox;
    vz=wf_be16(a,O_VZ);dec=wf_be16(a,O_VZDEC);if(dec==0u)return WF_BAD_STATE;
    q=udiv32_16((uint32_t)vz,dec)&0xffffu;d=(uint16_t)(q<<1);if(d==0u)return WF_OK;
    nx=(((int32_t)(uint16_t)wf_be16(t,O_X)+ox)-(int32_t)(uint16_t)wf_be16(a,O_X))*256;
    ny=(((int32_t)(uint16_t)wf_be16(t,O_Y)+oy)-(int32_t)(uint16_t)wf_be16(a,O_Y))*256;
    if(!divs32_16(nx,d,&vx)||!divs32_16(ny,d,&vy))return WF_BAD_STATE;
    if(vx>0x0200) vx=0x0200;
    if(vx<(int16_t)0xFE00) vx=(int16_t)0xFE00;
    if(vy>0x00C0) vy=0x00C0;
    if(vy<(int16_t)0xFF40) vy=(int16_t)0xFF40;
    wf_set_be16(a,O_VX,(uint16_t)vx);wf_set_be16(a,O_VY,(uint16_t)vy);return WF_OK;
}

int wf_r17_helper_11152_carry(const wf_actor_raw_t *a,const wf_actor_raw_t *t){
    uint16_t x,y,l,r,top,bot;
    if(!a||!t)return 0;
    if(low(t,O_STATE)!=4u)return 0;
    if(low(t,O_STATE4)!=8u&&low(t,O_STATE4)!=9u)return 0;
    x=wf_be16(t,O_X);y=wf_be16(t,O_Y);
    if(!bit(t,O_FACING,7)){l=(uint16_t)(x-8u);r=(uint16_t)(x+104u);}else{l=(uint16_t)(x-104u);r=(uint16_t)(x+8u);}
    top=(uint16_t)(y-72u);bot=(uint16_t)(y+24u);
    return (l<wf_be16(a,O_X) && r>=wf_be16(a,O_X) && top<wf_be16(a,O_Y) && bot>=wf_be16(a,O_Y))?1:0;
}

int wf_r17_helper_11412_carry(const wf_runtime_t *rt,const wf_actor_raw_t *a){
    const wf_actor_raw_t *t,*back;uint32_t ap;unsigned s=0;
    if(!rt||!a)return 1;
    t=actor_from_ptr_c(rt,wf_be32(a,O_LINK),&s);if(!t)return 1;
    back=actor_from_ptr_c(rt,wf_be32(t,O_LINK),0);if(!back)return 1;
    ap=actor_rom_ptr((unsigned)(a-rt->actors));
    return wf_be32(t,O_LINK)==ap&&back==a?0:1;
}

static wf_status_t helper_1110e(wf_actor_raw_t *a,wf_r17_raw23_ctx_t *ctx,wf_r17_raw23_trace_t *tr){return emit(ctx,bit(a,O_FLAGS33,2)?0x002Du:0x0029u,tr,0x00002052u);}
static wf_status_t helper_1112e(wf_actor_raw_t *a,wf_r17_raw23_ctx_t *ctx,wf_r17_raw23_trace_t *tr){
    uint8_t f=low(a,O_FRAME);if(wf_be16(a,O_TIMER)==0u&&(f==1u||f==4u))return emit(ctx,0x0033u,tr,0x00002052u);return WF_OK;
}
static void helper_10f56(wf_actor_raw_t *a){
    if(wf_be16(a,O_ANGLE_TARGET)!=0u)return;
    if(bit(a,O_ROLE,7)&&(bit(a,O_FLAGS33,5)||bit(a,O_FLAGS33,2))){wf_set_be16(a,O_ANGLE_TARGET,0x0030u);return;}
    if(wf_u8(a,O_TIER_LO)==2u)wf_set_be16(a,O_ANGLE_TARGET,0x00E0u);
    else if(wf_u8(a,O_TIER_LO)==1u)wf_set_be16(a,O_ANGLE_TARGET,0x0080u);
    else wf_set_be16(a,O_ANGLE_TARGET,0x0030u);
}
static void helper_10b9a(wf_runtime_t *rt,wf_actor_raw_t *a,int16_t dx,int16_t dy,int16_t dz){
    wf_actor_raw_t *l=actor_from_ptr(rt,wf_be32(a,O_LINK),0);if(!l)return;
    wf_set_be16(l,O_X,wf_be16(a,O_X));wf_set_be16(l,O_Y,wf_be16(a,O_Y));wf_set_be16(l,O_Z,wf_be16(a,O_Z));
    if(bit(a,O_FACING,7))dx=(int16_t)-dx;
    wf_set_be16(l,O_X,(uint16_t)(wf_be16(l,O_X)+(uint16_t)dx));wf_set_be16(l,O_Y,(uint16_t)(wf_be16(l,O_Y)+(uint16_t)dy));wf_set_be16(l,O_Z,(uint16_t)(wf_be16(l,O_Z)+(uint16_t)dz));
}
static void helper_10bd0(wf_actor_raw_t *a,int16_t dx,int16_t dy,int16_t dz){
    if(bit(a,O_FACING,7))dx=(int16_t)-dx;
    wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(uint16_t)dx));wf_set_be16(a,O_Y,(uint16_t)(wf_be16(a,O_Y)+(uint16_t)dy));wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+(uint16_t)dz));
}
static wf_status_t helper_10fc6(wf_actor_raw_t *a,wf_r17_raw23_ctx_t *ctx,wf_r17_raw23_trace_t *tr){
    uint8_t c=wf_u8(a,O_CONTACT);
    if((ctx->mode_0161&0x02u)==0u&&(c==1u||c==5u)){
        if(bit(a,O_CONTACT37,1)){if(ctx->referee_1151!=0u)return WF_OK;ctx->referee_1150=1u;ctx->referee_1180=1u;return WF_OK;}
        if(bit(a,O_CONTACT37,0)){if(ctx->referee_11b1!=0u)return WF_OK;ctx->referee_11b0=1u;ctx->referee_11e0=1u;return WF_OK;}
        return WF_OK;
    }
    if(c==3u){wf_set_be16(a,O_STATE,4u);wf_set_be16(a,O_STATE4,0x0017u);if(emit(ctx,0x0028u,tr,0x00002052u)!=WF_OK)return WF_BLOCKED;return emit(ctx,0x0032u,tr,0x00002052u);}
    return WF_OK;
}

static void desc_load_frame(wf_actor_raw_t *a,const desc_t *d){
    uint16_t i=wf_be16(a,O_FRAME);uint16_t p;if(i>=d->count)return;
    wf_set_be16(a,O_TIMER,d->duration[i]);p=(uint16_t)(d->pose[i]^wf_be16(a,O_FACING));wf_set_be16(a,O_VISUAL,p);
}
static wf_status_t common_tick_1c0c8(wf_actor_raw_t *a,const desc_t *d){
    uint16_t v,i;if(!a||!d)return WF_BAD_STATE;if(d->kind==0u)return WF_OK;
    if(!bit(a,O_LATCH,7)){wf_set_be16(a,O_FRAME,0u);desc_load_frame(a,d);setbit(a,O_LATCH,7);return WF_OK;}
    if(wf_be16(a,O_TIMER)==0xFF00u)return WF_OK;
    v=wf_be16(a,O_TIMER);wf_set_be16(a,O_TIMER,(uint16_t)(v-1u));if(v!=0u)return WF_OK;
    i=(uint16_t)(wf_be16(a,O_FRAME)+1u);wf_set_be16(a,O_FRAME,i);
    if(i<d->count){desc_load_frame(a,d);return WF_OK;}
    wf_set_be16(a,O_FRAME,0x00FEu);
    if(d->kind==2u){wf_set_be16(a,O_FRAME,0u);desc_load_frame(a,d);}else wf_set_be16(a,O_TIMER,0u);
    return WF_OK;
}

static wf_status_t phase0(wf_actor_raw_t *a,wf_actor_raw_t *t,wf_r17_raw23_ctx_t *ctx,const desc_t **dp,wf_r17_raw23_trace_t *tr){
    uint16_t x;
    if(!bit(a,O_LATCH,7)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_MASH,0x1000u);wf_set_be16(t,O_ANGLE_TARGET,0x1000u);
        {wf_status_t s=emit(ctx,0x0031u,tr,0x00002052u);if(s!=WF_OK)return s;}
        clrbit(a,O_FACING,7);x=(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7)?(uint16_t)-0x38:0x38u));if(x>=wf_be16(a,O_X))setbit(a,O_FACING,7);
    }else if(low(a,O_FRAME)==0xFEu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SUBSTATE,(uint16_t)(wf_be16(a,O_SUBSTATE)+1u));if((ctx->mode_0161&0x02u)!=0u)wf_set_be16(a,O_SUBSTATE,8u);}
    *dp=&D_1A24A;return WF_OK;
}
static wf_status_t phase1_4(wf_actor_raw_t *a,wf_actor_raw_t *t,wf_r17_raw23_ctx_t *ctx,const desc_t **dp,wf_r17_raw23_trace_t *tr){
    uint16_t idx;wf_status_t s;(void)dp;
    if(!bit(a,O_LATCH,7)){
        uint8_t x=(uint8_t)(wf_u8(a,O_FACING)^wf_u8(t,O_FACING));uint8_t sub=low(a,O_SUBSTATE);
        idx=(uint16_t)(x?5u+sub:10u-sub);wf_set_be16(a,O_VZ,0x0500u);wf_set_be16(a,O_VZDEC,0x0048u);s=wf_r17_trajectory_26ae(a,t,idx);if(s!=WF_OK)return s;wf_set_u8(a,O_MOTION,2u);return WF_OK;
    }
    if(bit(a,O_CONTACT37,4)){s=helper_1110e(a,ctx,tr);if(s!=WF_OK)return s;wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);ctx->shake_1800=0x000Eu;}
    if(low(a,O_FRAME)==0xFEu){wf_set_be16(a,O_STATE,5u);if(wf_u8(a,O_CONTACT)!=1u)wf_set_be16(a,O_SUBSTATE,(uint16_t)(wf_be16(a,O_SUBSTATE)+1u));else wf_set_be16(a,O_SUBSTATE,5u);}
    return WF_OK;
}
static wf_status_t phase5(wf_actor_raw_t *a,wf_actor_raw_t *t,wf_r17_raw23_ctx_t *ctx,const desc_t **dp,wf_r17_raw23_trace_t *tr){
    uint16_t d,tid;
    if(!bit(a,O_LATCH,7)){
        wf_set_u8(a,O_MOTION,1u);tid=wf_be16(a,O_CHAR);if(tid>=12u)return block(tr,0x00011D4Au);wf_set_be16(a,O_MAG,0u);wf_set_u8(a,O_MAG+1u,SPEED_11D4A[tid]);wf_set_be16(t,O_ANGLE_TARGET,0x0200u);
        d=wf_be16(t,O_DC);if(d>=5u)d=5u;d=TAB_176BA[d];if(d<wf_be16(t,O_STAMINA))d=1u;else d=(uint16_t)(d-wf_be16(t,O_STAMINA));if(low(a,O_SELECTOR)==0x23u)d=(uint16_t)(d+6u);wf_set_be16(t,O_MASH,d);
        clrbit(a,O_FACING,7);wf_set_be16(a,O_ANGLE,0x00C0u);if(wf_be16(a,O_X)<0x0270u){setbit(a,O_FACING,7);wf_set_be16(a,O_ANGLE,0x0040u);}*dp=&D_11C62;return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,0u);
    if((bit(a,O_FACING,7)&&bit(a,O_CONTACT37,0))||(!bit(a,O_FACING,7)&&bit(a,O_CONTACT37,1))){wf_status_t s=helper_10fc6(a,ctx,tr);if(s!=WF_OK)return s;wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SUBSTATE,(uint16_t)(wf_be16(a,O_SUBSTATE)+1u));return WF_OK;}
    *dp=&D_11C62;return helper_1112e(a,ctx,tr);
}
static wf_status_t phase6(wf_actor_raw_t *a,const desc_t **dp){
    if(!bit(a,O_LATCH,7)){chgbit(a,O_FACING,7);chgbit(a,O_ANGLE_LO,7);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_MAG,0x000Cu);}else{if(low(a,O_FRAME)==1u)wf_set_u8(a,O_MOTION,1u);if(low(a,O_FRAME)==0xFEu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SUBSTATE,(uint16_t)(wf_be16(a,O_SUBSTATE)+1u));}}
    *dp=&D_11DD0;return WF_OK;
}
static wf_status_t phase7(wf_actor_raw_t *a,wf_actor_raw_t *t,wf_r17_raw23_ctx_t *ctx,const desc_t **dp,wf_r17_raw23_trace_t *tr){
    uint16_t x;int transition=0;
    if(!bit(a,O_LATCH,7)){
        wf_set_u8(a,O_MOTION,1u);{uint16_t id=wf_be16(a,O_CHAR);if(id>=12u)return block(tr,0x00011D4Au);wf_set_be16(a,O_MAG,0u);wf_set_u8(a,O_MAG+1u,SPEED_11D4A[id]);}
        x=wf_be16(t,O_X);x=(uint16_t)(x+(bit(t,O_FACING,7)?(uint16_t)-0x38:0x38u));x=(uint16_t)(x+(bit(a,O_FACING,7)?(uint16_t)-0x60:0x60u));wf_set_be16(a,O_TARGET_X,x);
    }else{x=wf_be16(a,O_TARGET_X);if(!bit(a,O_FACING,7)){if(x>=wf_be16(a,O_X))transition=1;}else if(x<wf_be16(a,O_X))transition=1;if(transition){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SUBSTATE,(uint16_t)(wf_be16(a,O_SUBSTATE)+1u));}}
    *dp=&D_11C62;return helper_1112e(a,ctx,tr);
}
static wf_status_t phase8(wf_runtime_t *rt,wf_actor_raw_t *a,wf_actor_raw_t *t,wf_r17_raw23_ctx_t *ctx,const desc_t **dp,wf_r17_raw23_trace_t *tr){
    wf_status_t s;uint8_t idx;
    if(bit(a,O_SELECTOR,7)){
        if(bit(t,O_FLAGS32,4)){ctx->global_15d2=wf_u8(t,O_CHAR_LO);ctx->global_15d3=0x29u;}
        else if(!bit(a,O_FE,7))return WF_OK;
        wf_set_be16(a,O_STATE,7u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,O_STATE4,0x0009u);clrbit(a,O_PINFLAGS,0);helper_10bd0(a,(int16_t)0xFFD0,0,0);*dp=&D_15CF0;return WF_OK;
    }
    if(!bit(a,O_LATCH,7)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_VZ,0x0600u);wf_set_be16(a,O_VZDEC,0x0048u);s=wf_r17_trajectory_26ae(a,t,2u);if(s!=WF_OK)return s;*dp=&D_15CF0;return WF_OK;}
    if(wf_be16(a,O_FRAME)==0u&&wf_be16(a,O_TIMER)==0u){wf_set_u8(a,O_MOTION,2u);*dp=&D_15CF0;return WF_OK;}
    if((int16_t)wf_be16(a,O_VZ)>=0)setbit(a,O_SELECTOR,4);
    idx=low(a,O_FRAME);if(idx==2u&&wf_be16(a,O_TIMER)==0u){wf_set_u8(a,O_B1B,0x20u);wf_set_be16(a,O_WORD42,0x0020u);}
    if(!bit(a,O_CONTACT37,4)){
        if(idx==0xFEu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7);return WF_OK;}
        *dp=&D_15CF0;return WF_OK;
    }
    setbit(a,O_CLEAR18,7);wf_set_be16(a,O_WORD42,0u);s=helper_1110e(a,ctx,tr);if(s!=WF_OK)return s;
    if(wf_r17_helper_11412_carry(rt,a)!=0) goto impact_failure;
    if(!wf_r17_helper_11152_carry(a,t)) goto impact_failure;
    wf_set_u8(a,O_MOTION,0u);ctx->global_15d2=wf_u8(a,O_CHAR_LO);ctx->global_15d3=0x25u;ctx->shake_1800=0x000Eu;clrbit(a,O_SELECTOR,4);wf_set_be16(t,O_MASH,0u);wf_set_be16(t,O_ANGLE_TARGET,0u);wf_set_be16(t,O_DE,(uint16_t)(wf_be16(t,O_DE)+1u));
    if(bit(a,O_FLAGS33,0)&&bit(t,O_FLAGS33,0)&&!bit(a,O_FLAGS33,2)){
        wf_set_be16(a,O_PROFILE,0x801Du);setbit(a,O_SELECTOR,7);setbit(a,O_PINFLAGS,0);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x0064u);wf_set_be16(t,O_DAMAGE,0x0014u);
        helper_10b9a(rt,t,0x0030,(int16_t)-1,0);if(!ctx->special_215b6)return block(tr,0x000215B6u);s=ctx->special_215b6(rt,a,ctx->user);if(s!=WF_OK)return s;*dp=&D_15CF0;return WF_OK;
    }
    wf_set_be16(a,O_TIMER,0x0014u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,O_STATE4,0x0007u);
    /* ROM $15F76 EXG A0,A2: after the swap $15F7C writes +$68 on the
     * original attacker, not the victim.  This corrects the earlier prose audit. */
    wf_set_be16(t,O_ANGLE_TARGET,0u);wf_set_be16(a,O_DAMAGE,0x0014u);helper_10f56(t);*dp=&D_15CF0;return WF_OK;
impact_failure:
    wf_set_be16(a,O_STATE,4u);wf_set_be16(a,O_STATE4,0x0005u);chgbit(a,O_FACING,7);
    setbit(a,O_LINK,7);wf_set_be16(t,O_DAMAGE,0x0003u);
    s=emit(ctx,0x0032u,tr,0x00002052u);if(s!=WF_OK)return s;
    wf_set_be16(t,O_ANGLE_TARGET,0u);wf_set_be16(t,O_MASH,0u);
    wf_set_be16(a,O_VISUAL,0x001Fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    *dp=&D_15CF0;return WF_OK;
}

static wf_status_t raw23_callback(wf_runtime_t *rt,wf_actor_raw_t *a,wf_actor_raw_t *t,wf_r17_raw23_ctx_t *ctx,const desc_t **dp,wf_r17_raw23_trace_t *tr){
    uint8_t sub=(uint8_t)(wf_be16(a,O_SUBSTATE)&0x0Fu);if(tr)tr->substate=sub;
    switch(sub){case 0:return phase0(a,t,ctx,dp,tr);case 1:case 2:case 3:case 4:return phase1_4(a,t,ctx,dp,tr);case 5:return phase5(a,t,ctx,dp,tr);case 6:return phase6(a,dp);case 7:return phase7(a,t,ctx,dp,tr);case 8:return phase8(rt,a,t,ctx,dp,tr);default:return block(tr,0x00015D08u);}
}

wf_status_t wf_r17_dispatch_1c03e_raw23(wf_runtime_t *rt,unsigned slot,wf_r17_raw23_ctx_t *ctx,wf_r17_raw23_trace_t *tr){
    wf_actor_raw_t *a,*t;const desc_t *d=&D_15CE0;wf_status_t s;uint16_t selector;
    if(tr){tr->callback_rom=0u;tr->descriptor_rom=0u;tr->tick_descriptor_rom=0u;tr->blocked_dependency_rom=0u;tr->substate=0u;}
    if(!rt||!ctx||slot>=WF_ACTOR_SLOTS) return WF_BAD_STATE;
    a=&rt->actors[slot];
    if(low(a,O_LATCH)!=5u) return block(tr,0x0001C03Eu);
    if(low(a,O_STATE)==0xFFu) return WF_OK;
    selector=(uint16_t)(wf_be16(a,O_SELECTOR)&0x00FFu);if(selector!=0x23u)return block(tr,0x00012614u);
    t=actor_from_ptr(rt,wf_be32(a,O_LINK),0);if(!t)return block(tr,0x0001C094u);
    if(tr){tr->callback_rom=0x00015D08u;tr->descriptor_rom=0x00015CE0u;}
    s=raw23_callback(rt,a,t,ctx,&d,tr);if(s!=WF_OK)return s;if(tr)tr->tick_descriptor_rom=d->rom;
    s=common_tick_1c0c8(a,d);if(s!=WF_OK)return s;
    if(bit(a,O_CLEAR18,7)){wf_set_be16(a,O_CLEAR18,0u);wf_set_be16(a,O_CLEAR18+2u,0u);}
    return WF_OK;
}

wf_status_t wf_r17_dispatch_1c03e_raw23_hook(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user){
    wf_r17_raw23_bridge_t *b=(wf_r17_raw23_bridge_t*)user;
    ptrdiff_t d;
    if(!rt||!actor||!b||!b->ctx)return WF_BAD_STATE;
    d=actor-rt->actors;
    if(d<0||d>=(ptrdiff_t)WF_ACTOR_SLOTS)return WF_BAD_STATE;
    return wf_r17_dispatch_1c03e_raw23(rt,(unsigned)d,b->ctx,&b->trace[(unsigned)d]);
}
