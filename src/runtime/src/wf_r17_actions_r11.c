#include <stdint.h>
#include "wf_r17_actions_r11.h"

#define BIT8(n) ((uint8_t)(1u << (n)))
#define ACTOR_BASE 0x001C05B0u
#define ACTOR_STRIDE 0x10Cu

enum {
    O_MOTION=0x01, O_VISUAL=0x04, O_X=0x06, O_Y=0x0A, O_Z=0x0E,
    O_FLAGS18=0x18, O_B1B=0x1B, O_LATCH=0x1C, O_STATE=0x20,
    O_TIMER=0x22, O_FRAME=0x24, O_LINK=0x26, O_FACING=0x2E,
    O_PROFILE=0x4C, O_SELECTOR=0x60, O_OBJ74=0x74, O_OBJ75=0x75,
    O_OBJLINK=0x76
};

typedef struct desc {
    uint32_t rom;
    uint16_t kind;
    uint16_t count;
    uint16_t duration[3];
    uint16_t pose[3];
} desc_t;

static const desc_t D_19E80={0x00019E80u,1u,2u,{0x0008u,0x0008u,0u},{0x0072u,0x0073u,0u}};
static const desc_t D_19E90={0x00019E90u,1u,2u,{0x0008u,0x0008u,0u},{0x0077u,0x0078u,0u}};
static const desc_t D_19FC6={0x00019FC6u,1u,3u,{0x000Au,0x000Au,0x000Au},{0x0000u,0x01CDu,0x01CEu}};
static const desc_t D_1A594={0x0001A594u,1u,1u,{0x0008u,0u,0u},{0x0221u,0u,0u}};

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){return (wf_u8(a,o)&BIT8(b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|BIT8(b)));}
static void chgbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)^BIT8(b)));}
static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}
static wf_status_t block(wf_r17_actions_r11_trace_t *tr,uint32_t pc){if(tr)tr->blocked_dependency_rom=pc;return WF_BLOCKED;}

static wf_actor_raw_t *actor_from_ptr(wf_runtime_t *rt,uint32_t p){
    uint32_t d;unsigned s;
    if(!rt||p<ACTOR_BASE)return 0;
    d=p-ACTOR_BASE;if((d%ACTOR_STRIDE)!=0u)return 0;s=(unsigned)(d/ACTOR_STRIDE);if(s>=WF_ACTOR_SLOTS)return 0;
    return &rt->actors[s];
}
static wf_actor_raw_t *object_from_ptr(wf_r17_actions_r11_bridge_t *b,uint32_t p){
    if(!b)return 0;
    if(p==0x001C0F1Cu)return b->fixed_object0;
    if(p==0x001C1028u)return b->fixed_object1;
    return 0;
}
static void helper_10bd0(wf_actor_raw_t *a,int16_t dx,int16_t dy,int16_t dz){
    if(bit(a,O_FACING,7))dx=(int16_t)-dx;
    wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(uint16_t)dx));
    wf_set_be16(a,O_Y,(uint16_t)(wf_be16(a,O_Y)+(uint16_t)dy));
    wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+(uint16_t)dz));
}
static void desc_load_frame(wf_actor_raw_t *a,const desc_t *d){
    uint16_t i=wf_be16(a,O_FRAME),p;if(i>=d->count)return;
    wf_set_be16(a,O_TIMER,d->duration[i]);p=(uint16_t)(d->pose[i]^wf_be16(a,O_FACING));wf_set_be16(a,O_VISUAL,p);
}
static wf_status_t common_tick_1c0c8(wf_actor_raw_t *a,const desc_t *d){
    uint16_t v,i;if(!a||!d)return WF_BAD_STATE;if(d->kind==0u)return WF_OK;
    if(!bit(a,O_LATCH,7)){wf_set_be16(a,O_FRAME,0u);desc_load_frame(a,d);setbit(a,O_LATCH,7);return WF_OK;}
    if(wf_be16(a,O_TIMER)==0xFF00u)return WF_OK;
    v=wf_be16(a,O_TIMER);wf_set_be16(a,O_TIMER,(uint16_t)(v-1u));if(v!=0u)return WF_OK;
    i=(uint16_t)(wf_be16(a,O_FRAME)+1u);wf_set_be16(a,O_FRAME,i);
    if(i<d->count){desc_load_frame(a,d);return WF_OK;}
    wf_set_be16(a,O_FRAME,0x00FEu);wf_set_be16(a,O_TIMER,0u);return WF_OK;
}

/* $19EA0, raw $70. */
static wf_status_t raw70(wf_actor_raw_t *a,wf_r17_actions_r11_bridge_t *b,const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *o;
    if(!bit(a,O_LATCH,7)){
        wf_set_u8(a,O_MOTION,0u);
        o=object_from_ptr(b,wf_be32(a,O_OBJLINK));if(!o)return block(tr,0x00019EA0u);
        wf_set_be16(a,O_OBJ74,wf_be16(o,O_OBJ74));setbit(a,O_OBJ74,7);wf_set_be16(o,O_LATCH,1u);
        wf_set_be16(a,O_X,wf_be16(o,O_X));wf_set_be16(a,O_Y,wf_be16(o,O_Y));wf_set_be16(a,O_Z,wf_be16(o,O_Z));
        wf_set_u8(a,O_FACING,wf_u8(o,O_FACING));wf_set_u8(a,O_B1B,1u);
    }else if(low(a,O_FRAME)==0xFEu){
        wf_set_be16(a,O_STATE,0u);setbit(a,O_FLAGS18,7);wf_set_be16(a,O_Y,(uint16_t)(wf_be16(a,O_Y)-0x0010u));
    }
    *dp=wf_u8(a,O_OBJ75)==0u?&D_19E80:&D_19E90;return WF_OK;
}

/* $19FDA, raw $72. */
static wf_status_t raw72(wf_actor_raw_t *a,const desc_t **dp){
    if(!bit(a,O_LATCH,7))wf_set_u8(a,O_MOTION,0u);
    else{
        wf_set_be16(a,O_PROFILE,1u);
        if(wf_be16(a,O_FRAME)==2u)wf_set_be16(a,O_PROFILE,0x0011u);
        else if(low(a,O_FRAME)==0xFEu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7);}
    }
    *dp=&D_19FC6;return WF_OK;
}

/* $1A5A0, raw $7C. */
static wf_status_t raw7c(wf_runtime_t *rt,wf_actor_raw_t *a,const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x0001A5A0u);
    if(!bit(a,O_LATCH,7)){
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(t,O_MOTION,0u);chgbit(a,O_FACING,7);wf_set_be16(t,O_STATE,0x00FFu);wf_set_be16(t,O_VISUAL,0xFFFFu);
    }else if(low(a,O_FRAME)==0xFEu){
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0017u);wf_set_be16(t,O_STATE,0x00FFu);chgbit(a,O_FACING,7);helper_10bd0(a,0x0020,0,0);
    }
    *dp=&D_1A594;return WF_OK;
}

wf_status_t wf_r17_dispatch_1c03e_r11(wf_runtime_t *rt,unsigned slot,wf_r17_actions_r11_bridge_t *b){
    wf_actor_raw_t *a;wf_r17_actions_r11_trace_t *tr;const desc_t *d=0;uint16_t sel;wf_status_t s;
    if(!rt||!b||slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
    a=&rt->actors[slot];tr=&b->trace[slot];
    tr->callback_rom=0u;tr->descriptor_rom=0u;tr->tick_descriptor_rom=0u;tr->blocked_dependency_rom=0u;tr->selector=(uint8_t)(wf_be16(a,O_SELECTOR)&0xFFu);
    if(low(a,O_LATCH)!=5u)return block(tr,0x0001C03Eu);
    if(low(a,O_STATE)==0xFFu)return WF_OK;
    sel=(uint16_t)(wf_be16(a,O_SELECTOR)&0x00FFu);
    if(sel==0x23u){
        if(!b->raw23_ctx)return block(tr,0x00015D08u);
        s=wf_r17_dispatch_1c03e_raw23(rt,slot,b->raw23_ctx,&b->raw23_trace[slot]);
        tr->callback_rom=b->raw23_trace[slot].callback_rom;tr->descriptor_rom=b->raw23_trace[slot].descriptor_rom;tr->tick_descriptor_rom=b->raw23_trace[slot].tick_descriptor_rom;tr->blocked_dependency_rom=b->raw23_trace[slot].blocked_dependency_rom;return s;
    }
    switch(sel){
        case 0x70u:tr->callback_rom=0x00019EA0u;tr->descriptor_rom=0x00019E80u;s=raw70(a,b,&d,tr);break;
        case 0x72u:tr->callback_rom=0x00019FDAu;tr->descriptor_rom=0x00019FC6u;s=raw72(a,&d);break;
        case 0x7Cu:tr->callback_rom=0x0001A5A0u;tr->descriptor_rom=0x0001A594u;s=raw7c(rt,a,&d,tr);break;
        default:return block(tr,0x00012614u);
    }
    if(s!=WF_OK)return s;
    tr->tick_descriptor_rom=d->rom;s=common_tick_1c0c8(a,d);if(s!=WF_OK)return s;
    return WF_OK;
}

wf_status_t wf_r17_dispatch_1c03e_r11_hook(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user){
    wf_r17_actions_r11_bridge_t *b=(wf_r17_actions_r11_bridge_t*)user;unsigned slot;
    if(!rt||!actor||!b||actor<rt->actors||actor>=rt->actors+WF_ACTOR_SLOTS)return WF_BAD_STATE;
    slot=(unsigned)(actor-rt->actors);return wf_r17_dispatch_1c03e_r11(rt,slot,b);
}
