#include <stdint.h>
#include <string.h>
#include "wf_r17_actions_r11.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_r17_tag_6e34.h"
#include "wf_r17_rng1129e.h"

#define BIT8(n) ((uint8_t)(1u << (n)))
#define ACTOR_BASE 0x001C05B0u
#define ACTOR_STRIDE 0x10Cu

enum {
    O_MOTION=0x01, O_VISUAL=0x04, O_X=0x06, O_Y=0x0A, O_Z=0x0E,
    O_RENDER_LIST=0x12, O_FLAGS18=0x18, O_B1B=0x1B, O_LATCH=0x1C, O_STATE=0x20,
    O_TIMER=0x22, O_FRAME=0x24, O_LINK=0x26, O_FACING=0x2E,
    O_PROFILE=0x4C, O_SELECTOR=0x60, O_OBJ74=0x74, O_OBJ75=0x75,
    O_OBJLINK=0x76
};

typedef struct desc {
    uint32_t rom;
    uint16_t kind;
    uint16_t count;
    uint16_t duration[7];
    uint16_t pose[7];
} desc_t;

/* $12614[0] -> $12850; $12888 selects $1286C for role7/flags5. */
static const desc_t D_12850={0x00012850u,1u,5u,
    {0x000au,0x0006u,0x0004u,0x000au,0x0002u},
    {0x0000u,0x0092u,0x0093u,0x0094u,0x0093u}};
static const desc_t D_1286C={0x0001286cu,1u,5u,
    {0x0005u,0x0005u,0x0005u,0x0009u,0x0002u},
    {0x0000u,0x0092u,0x0093u,0x0094u,0x0093u}};
static const desc_t D_19E80={0x00019E80u,1u,2u,{0x0008u,0x0008u,0u},{0x0072u,0x0073u,0u}};
static const desc_t D_19E90={0x00019E90u,1u,2u,{0x0008u,0x0008u,0u},{0x0077u,0x0078u,0u}};
static const desc_t D_19FC6={0x00019FC6u,1u,3u,{0x000Au,0x000Au,0x000Au},{0x0000u,0x01CDu,0x01CEu}};
static const desc_t D_1A594={0x0001A594u,1u,1u,{0x0008u,0u,0u},{0x0221u,0u,0u}};
/* $12614[$4F] -> $18AFE: callback $18B0E, two 16-tick poses. */
static const desc_t D_18AFE={0x00018afeu,1u,2u,{0x0010u,0x0010u},{0x0066u,0x0067u}};
/* $12614[$15] -> $14CE0, callback $14CF4. */
static const desc_t D_14CE0={0x00014ce0u,1u,3u,
    {0x000au,0x000eu,0x0010u},{0x00f8u,0x00f9u,0x00fau}};
/* $12614[$16] -> $14D8A: looping linked walking animation. */
static const desc_t D_14D8A={0x00014d8au,2u,4u,{10u,10u,10u,10u},{0x43u,0x44u,0x45u,0x46u}};
static const desc_t D_153A0={0x000153a0u,1u,5u,{8u,16u,24u,0xff00u,16u},{0x84u,0x85u,0x15du,0x15eu,0x15eu}};
static const desc_t D_13020={0x00013020u,1u,3u,{32u,24u,32u},{0xafu,0xb0u,0u}};
static const desc_t D_1969E={0x0001969eu,1u,2u,{16u,16u},{0x4bu,0x4cu}};
static const desc_t D_1892C={0x0001892cu,1u,5u,{12u,12u,0xff00u,12u,12u},{0x62u,0x63u,0u,0x66u,0x67u}};
static const desc_t D_1A532={0x0001a532u,0u,0u,{0u},{0u}};
static const desc_t D_14E2C={0x00014e2cu,1u,3u,
    {0x0008u,0x000au,0x0010u},{0x00feu,0x00ffu,0x0100u}};
/* $12614[$29] -> $1656A, callback $16582. */
static const desc_t D_1656A={0x0001656au,1u,4u,
    {0x0014u,0x0008u,0x000eu,0x000eu},
    {0x00f5u,0x00f6u,0x00f7u,0x01d2u}};

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){return (wf_u8(a,o)&BIT8(b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|BIT8(b)));}
static void chgbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)^BIT8(b)));}
static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}
static wf_status_t block(wf_r17_actions_r11_trace_t *tr,uint32_t pc){if(tr)tr->blocked_dependency_rom=pc;return WF_BLOCKED;}

static wf_actor_raw_t *actor_from_ptr(wf_runtime_t *rt,uint32_t p){
    return wf_actor_from_rom_ptr(rt,p,0);
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
    if(d->kind==2u){wf_set_be16(a,O_FRAME,0u);desc_load_frame(a,d);return WF_OK;}
    wf_set_be16(a,O_FRAME,0x00FEu);wf_set_be16(a,O_TIMER,0u);return WF_OK;
}

/* $12888 idle/movement callback. $10B62 temporarily supplies +$3E=-24
 * to the already-owned $280DC resolver, then restores +$36 from its saved
 * D0 and applies only +$38 to X. */
static wf_status_t raw00(wf_runtime_t *rt,wf_actor_raw_t *a,
                          const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    if(!bit(a,O_LATCH,7)){
        wf_set_u8(a,O_MOTION,0u);
        uint16_t contact=wf_be16(a,WF_A_CONTACT);
        wf_set_be16(a,WF_A_FIELD3E,0xffe8u);
        wf_status_t s=wf_r17_resolver_280dc(rt,a);
        if(s!=WF_OK)return block(tr,0x00010b62u);
        if(wf_u8(a,WF_A_CONTACT+1u)&3u)wf_set_be16(a,WF_A_VX,0u);
        wf_set_be16(a,WF_A_CONTACT,contact);
        wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+wf_be16(a,WF_A_CORR_X)));
        wf_set_be16(a,WF_A_FIELD3E,0u);
    }else{
        wf_set_be16(a,O_PROFILE,0x0021u);
        if(wf_be16(a,O_FRAME)==3u)wf_set_be16(a,O_PROFILE,0x0007u);
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,0u);
            setbit(a,O_LINK,7u);
        }
    }
    *dp=bit(a,WF_A_ROLE,7u)&&bit(a,WF_A_FLAGS33,5u)?
        &D_1286C:&D_12850;
    return WF_OK;
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
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x0017u);wf_set_be16(t,O_STATE,0x00FFu);chgbit(a,O_FACING,7);
        /* $1A5E8/$1A5EA CLR.W; dispatcher saves D0 only, so these survive. */
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,0x0020,0,0);
    }
    *dp=&D_1A594;return WF_OK;
}

/* $18B0E..$18B8C: stand-up action selected at $11B5A. */
static wf_status_t raw4f(wf_runtime_t *rt,wf_actor_raw_t *a,const desc_t **dp){
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(a,O_RENDER_LIST,2u);
        wf_set_be16(a,O_FACING,(uint16_t)(wf_be16(a,WF_A_FLAGS32)&0x0080u)<<8);
    }else if(bit(a,WF_A_FLAGS34,0u)){
        wf_set_be16(a,O_STATE,0u);
        rt->g.global_1682=0x00fau;
        wf_set_be16(a,O_RENDER_LIST,0u);
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,1u);
        wf_set_be16(a,WF_A_AE,1u);
        chgbit(a,O_FACING,7u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~2u));
        wf_set_be16(a,O_RENDER_LIST,4u);
        wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+
            (bit(a,WF_A_FLAGS33,7u)?0x0038u:0xffc8u)));
    }
    *dp=&D_18AFE;return WF_OK;
}
/* $14CF4: linked hold action. $1C03E has loaded A2 from +$26. */
static wf_status_t raw15(wf_runtime_t *rt,wf_actor_raw_t *a,
                          const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!linked)return block(tr,0x0001c094u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        if(wf_be16(a,WF_A_TIMER44)==1u){
            setbit(a,O_LATCH,7u);
            wf_set_be16(a,O_FRAME,0u);
            wf_set_be16(a,O_TIMER,0u);
        }else if(low(a,WF_A_PRIOR)==0x0cu){
            helper_10bd0(a,0x0020,0,0);
        }
        if(wf_be16(a,WF_A_TIMER44)!=3u){
            wf_set_be16(linked,O_STATE,5u);
            wf_set_be16(linked,O_SELECTOR,0x7bu);
        }
    }else{
        wf_set_be16(linked,O_X,wf_be16(a,O_X));
        wf_set_be16(linked,O_Y,wf_be16(a,O_Y));
        wf_set_u8(linked,O_FACING,wf_u8(a,O_FACING));
        chgbit(linked,O_FACING,7u);
        if(bit(a,WF_A_LOCKOUT,7u)){
            wf_set_be16(a,O_STATE,5u);
            wf_set_be16(a,O_SELECTOR,0x50u);
        }else if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_FRAME,0u);
            wf_set_be16(a,O_TIMER,0u);
            wf_set_be16(linked,WF_A_PENDING_DAMAGE,1u);
        }
    }
    *dp=&D_14CE0;return WF_OK;
}
/* $14DA2: walk while keeping the linked opponent at the same world position. */
static wf_status_t raw16(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 static const uint8_t speed[12]={27u,29u,28u,25u,30u,27u,24u,30u,22u,27u,28u,27u}; /* $116AE */
 wf_actor_raw_t*linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(!linked)return block(tr,0x1c094u);
 if(!bit(a,O_LATCH,7u)){
  unsigned id=wf_be16(a,WF_A_CHARACTER);if(id>=12u)return block(tr,0x11762u);
  wf_set_u8(a,O_MOTION,1u);
  uint16_t magnitude=bit(a,WF_A_FLAGS32,3u)?12u:speed[id];
  if(bit(a,WF_A_ROLE,7u)&&bit(a,WF_A_FLAGS33,5u))magnitude=(uint8_t)(magnitude+6u);
  if(bit(a,WF_A_REL_FLAGS,7u))magnitude=(uint16_t)(magnitude-8u);
  wf_set_be16(a,WF_A_MAGNITUDE,(uint16_t)((magnitude&0xff00u)|((magnitude&255u)>>1)));
 }else if(wf_be16(a,WF_A_LOCKOUT)){
  wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x50u);
 }else{
  uint16_t angle=wf_be16(a,WF_A_ANGLE_WORD);
  if(angle!=0u&&angle!=0x80u){wf_set_u8(a,O_FACING,(uint8_t)(angle&128u));wf_set_u8(linked,O_FACING,(uint8_t)((angle&128u)^128u));}
  uint16_t contact=wf_be16(a,WF_A_CONTACT);
  wf_set_be16(a,WF_A_FIELD3E,0xffd8u);
  wf_status_t status=wf_r17_resolver_280dc(rt,a);if(status!=WF_OK)return block(tr,0x280dcu);
  if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
  wf_set_be16(a,WF_A_CONTACT,contact);
  wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+wf_be16(a,WF_A_CORR_X)));
  wf_set_be16(a,WF_A_FIELD3E,0u);
  wf_set_be16(linked,O_X,wf_be16(a,O_X));wf_set_be16(linked,O_Y,wf_be16(a,O_Y));wf_set_be16(linked,O_Z,wf_be16(a,O_Z));
  wf_set_u8(a,O_VISUAL,(uint8_t)((wf_u8(a,O_VISUAL)&127u)|(wf_u8(a,O_FACING)&128u)));
 }
 *dp=&D_14D8A;return WF_OK;
}
/* $1A538, the paired participant's invisible hold while $15 animates. */
static wf_status_t raw7b(wf_runtime_t *rt,wf_actor_raw_t *a,
                          const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!linked)return block(tr,0x0001c094u);
    uint8_t old=wf_u8(a,O_LATCH);
    setbit(a,O_LATCH,7u);
    if(!(old&0x80u)){
        wf_set_be16(a,O_VISUAL,0xffffu);
        if(bit(linked,WF_A_FLAGS33,5u))setbit(a,O_SELECTOR,7u);
        if(wf_be16(a,WF_A_TIMER44)==1u||low(a,WF_A_PRIOR)!=5u)
            wf_set_be16(a,WF_A_TIMER44,
                bit(linked,WF_A_ROLE,7u)?0xbbu:0x80u);
    }else{
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_TIMER44)-1u);
        wf_set_be16(a,WF_A_TIMER44,timer);
        if(!timer){
            wf_set_be16(a,O_STATE,5u);
            wf_set_be16(a,O_SELECTOR,0x7cu);
            wf_set_be16(linked,O_STATE,0xffu);
        }
    }
    *dp=&D_1A532;return WF_OK;
}
/* $21424: restore the two team partners after a linked jumping attack. */
static wf_status_t jump_handover_21424(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_actions_r11_trace_t *tr){
    uint32_t first_ptr=wf_be32(a,0x86u);wf_actor_raw_t *first=actor_from_ptr(rt,first_ptr);
    if(!first)return block(tr,0x21428u);
    wf_actor_raw_t *link=actor_from_ptr(rt,wf_be32(first,O_LINK));
    wf_actor_raw_t *second=link?actor_from_ptr(rt,wf_be32(link,0x86u)):NULL;
    if(!second||wf_be16(second,WF_A_TIER)>2u)return block(tr,0x21448u);
    uint32_t second_ptr=wf_be32(link,0x86u);
    if(bit(first,WF_A_FLAGS33,7u))rt->g.global_1688=first_ptr;else rt->g.global_1684=first_ptr;
    setbit(first,WF_A_FLAGS34,0u);
    if(bit(second,WF_A_FLAGS33,7u))rt->g.global_1688=second_ptr;else rt->g.global_1684=second_ptr;
    setbit(second,WF_A_FLAGS34,0u);
    wf_set_be16(second,0xe6u,31u); /* $214BA: three WORDs, all $001F. */
    if(!bit(first,WF_A_FLAGS33,1u)){setbit(first,WF_A_ROLE,6u);wf_set_be32(first,0x7au,second_ptr);}
    wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)&~64u));
    setbit(second,WF_A_FLAGS32,2u);setbit(second,WF_A_ROLE,6u);wf_set_be32(second,0x7au,first_ptr);
    return WF_OK;
}
/* $153BC/$153A0, raw $0D: jumping attack. $26AE is the shared ROM
 * trajectory owner. Its D0-D5 are preserved; the caller's D1 survives. */
static wf_status_t raw0d(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_actions_r11_bridge_t *b,const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1c094u);
    *dp=&D_153A0;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);rt->g.global_1670=0u;
        wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~2u));
        wf_set_be16(a,WF_A_VZ,0x680u);wf_set_be16(a,WF_A_VZ_DEC,0x48u);
        uint16_t route=bit(a,WF_A_FLAGS34,3u)?0u:(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x53u)?1u:2u;
        wf_set_be16(a,WF_A_TIMER44,route);
        if(route==2u)wf_set_be16(a,O_FACING,wf_be16(a,O_X)<wf_be16(t,O_X)?0x8000u:0u);
        uint16_t dx=route?16u:0xfff0u;if(bit(a,O_FACING,7u))dx=(uint16_t)(0u-dx);
        wf_set_be16(t,O_X,(uint16_t)(wf_be16(t,O_X)+dx));
        wf_status_t status=wf_r17_trajectory_26ae(a,t,0u);
        dx=(uint16_t)(0u-dx);wf_r17_m68k_move_w(&rt->m68k_regs,1u,dx);
        wf_set_be16(t,O_X,(uint16_t)(wf_be16(t,O_X)+dx));
        if(status!=WF_OK)return block(tr,0x26aeu);return WF_OK;
    }
    if(wf_be16(a,O_FRAME)==0u&&wf_be16(a,O_TIMER)==0u)wf_set_u8(a,O_MOTION,2u);
    if(low(a,O_FRAME)==3u){
        wf_set_be16(a,WF_A_FIELD42,0xfff0u);if(!(wf_u8(a,WF_A_CONTACT+1u)&16u))return WF_OK;
        wf_set_be16(a,O_TIMER,0u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,WF_A_FIELD42,0u);
        unsigned route=wf_be16(a,WF_A_TIMER44);
        if(route==0u){
            wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~8u));
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,WF_A_STATE4,0x1cu);wf_set_be16(t,WF_A_PENDING_DAMAGE,18u);
            rt->g.global_15d2=15u;rt->g.global_15d3=25u;
            wf_status_t status=jump_handover_21424(rt,a,tr);if(status!=WF_OK)return status;
        }else if(route==1u){
            uint16_t x=wf_be16(a,O_X),tx=wf_be16(t,O_X),y=wf_be16(a,O_Y),ty=wf_be16(t,O_Y);
            if(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x53u&&(uint8_t)(x>=tx?x-tx:tx-x)<24u&&(uint8_t)(y>=ty?y-ty:ty-y)<20u){
                wf_actor_raw_t *other=actor_from_ptr(rt,wf_be32(t,O_LINK));if(!other)return block(tr,0x1553cu);
                wf_set_be16(t,O_STATE,4u);wf_set_be16(t,WF_A_STATE4,3u);
                wf_set_be16(other,O_STATE,5u);wf_set_be16(other,O_SELECTOR,0x6fu);
                wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~64u));
                wf_set_u8(t,WF_A_FLAGS33,(uint8_t)(wf_u8(t,WF_A_FLAGS33)&~64u));wf_set_be16(t,WF_A_PENDING_DAMAGE,18u);
            }
        }else if(route==2u){
            uint16_t x=wf_be16(a,O_X),tx=wf_be16(t,O_X),y=wf_be16(a,O_Y),ty=wf_be16(t,O_Y);
            if(!bit(t,WF_A_FLAGS32,0u)&&(!(rt->g.mode_0161&2u)||bit(t,WF_A_FLAGS33,2u))&&
               (low(t,O_STATE)<=1u||(low(t,O_STATE)==4u&&low(t,WF_A_STATE4)==1u))&&
               (uint16_t)(x>=tx?x-tx:tx-x)<48u&&(uint16_t)(y>=ty?y-ty:ty-y)<48u){
                wf_actor_raw_t *object=NULL;
                if(bit(t,O_OBJ74,7u)){object=object_from_ptr(b,wf_be32(t,O_OBJLINK));if(!object)return block(tr,0x15610u);}
                wf_set_be16(t,O_STATE,4u);wf_set_be16(t,WF_A_STATE4,0x20u);
                wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(t,O_FACING,7u);
                wf_set_be16(t,WF_A_PENDING_DAMAGE,18u);wf_set_u8(t,WF_A_C7,(uint8_t)(wf_u8(t,WF_A_C7)+1u));
                if(object){wf_set_be16(t,O_OBJ74,0u);wf_set_be16(object,O_LATCH,2u);wf_set_be32(t,O_OBJLINK,0u);setbit(t,O_FLAGS18,7u);}
                rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=low(a,O_SELECTOR)==0x1bu?15u:14u;return WF_OK;
            }
        }else return block(tr,0x154a4u);
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,0x4au,0u);setbit(a,O_LINK,7u);}
    return WF_OK;
}
/* $14E40: release the held opponent into the three-pose throw. */
static wf_status_t raw17(wf_runtime_t *rt,wf_actor_raw_t *a,
                          const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *victim=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!victim)return block(tr,0x0001c094u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(victim,O_STATE,0xffu);
        wf_set_be16(a,WF_A_FIELD3E,0xffe8u);
        wf_status_t s=wf_r17_resolver_280dc(rt,a);
        if(s!=WF_OK)return block(tr,0x00010b62u);
        if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
        wf_set_be16(a,WF_A_CONTACT,0xffe8u);
        wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+wf_be16(a,WF_A_CORR_X)));
        wf_set_be16(a,WF_A_FIELD3E,0u);
        wf_set_be16(victim,O_X,wf_be16(a,O_X));
        wf_set_be16(victim,O_Y,wf_be16(a,O_Y));
    }else if(wf_be16(a,O_FRAME)==1u&&wf_be16(a,O_TIMER)==0u){
        wf_set_be16(victim,O_STATE,2u);
        setbit(victim,O_LINK,7u);
        setbit(a,WF_A_FLAGS34,7u);
        setbit(victim,WF_A_FLAGS34,6u);
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x40u));
        wf_set_u8(victim,WF_A_FLAGS33,(uint8_t)(wf_u8(victim,WF_A_FLAGS33)&~0x40u));
        wf_set_be16(victim,WF_A_TIMER44,2u);
        wf_set_be16(victim,O_FACING,wf_be16(a,O_FACING));
        chgbit(victim,O_FACING,7u);
        wf_set_u8(victim,WF_A_ANGLE,
            bit(victim,O_FACING,7u)?0x40u:0xc0u);
        /* $10B9A copies the thrower's coordinates, then applies
         * D0=$50,D1=-2,D2=0 using its facing direction. */
        wf_set_be16(victim,O_X,(uint16_t)(wf_be16(a,O_X)+
            (bit(a,O_FACING,7u)?0xffb0u:0x0050u)));
        wf_set_be16(victim,O_Y,(uint16_t)(wf_be16(a,O_Y)-2u));
        wf_set_be16(victim,O_Z,wf_be16(a,O_Z));
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);
        chgbit(a,O_FACING,7u);
        setbit(a,O_LINK,7u);
    }
    *dp=&D_14E2C;return WF_OK;
}
/* $16582 linked CPU grapple. $2052 is solely the sound command on pose one;
 * Runtime18's requested silent output has no corresponding state change. */
static wf_status_t raw29(wf_runtime_t *rt,wf_actor_raw_t *a,
                          const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!linked)return block(tr,0x0001c094u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_set_u8(a,WF_A_TIMER44,(uint8_t)(wf_u8(a,WF_A_TIMER44)&~0x80u));
        setbit(linked,WF_A_TIMER44,7u);
        wf_set_be16(linked,O_STATE,0x00ffu);
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(linked,WF_A_PENDING_DAMAGE,1u);
        wf_set_u8(linked,WF_A_C7,(uint8_t)(wf_u8(linked,WF_A_C7)+1u));
        wf_set_be16(linked,0x54u,0x0100u);
        uint16_t repeat=(uint16_t)(wf_be16(linked,WF_A_TIMER52)+1u);
        wf_set_be16(linked,WF_A_TIMER52,repeat);
        if(repeat<2u){
            wf_set_be16(a,O_STATE,11u);
            wf_set_be16(linked,O_STATE,11u);
            wf_set_be16(a,WF_A_TIMER44,0u);
            wf_set_be16(linked,WF_A_TIMER44,0x8000u);
        }else{
            wf_set_be16(a,O_STATE,12u);
            wf_set_be16(a,WF_A_TIMER44,0x2000u);
            wf_set_be16(linked,O_STATE,0x00ffu);
            wf_set_be16(linked,WF_A_TIMER44,0u);
        }
    }
    *dp=&D_1656A;return WF_OK;
}

/* $13034: standing attack/reaction, including the +$92 collision latch. */
static wf_status_t raw06(wf_actor_raw_t *a,const desc_t **dp){
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        int previous=bit(a,WF_A_FLAGS34,3u);
        wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~8u));
        if(previous){
            setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);
        }else if(bit(a,O_SELECTOR,7u)){
            setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);
            wf_set_be16(a,O_VISUAL,0x60u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
            wf_set_be16(a,O_TIMER,12u);
        }else wf_set_be32(a,0x92u,0u);
    }else{
        if(!bit(a,O_SELECTOR,7u)){
            wf_set_be16(a,O_PROFILE,1u);
            if(low(a,O_FRAME)==0u){
                wf_set_be16(a,O_PROFILE,4u);
                if(wf_be32(a,0x92u)){
                    wf_set_be32(a,O_PROFILE,0u);wf_set_be16(a,O_TIMER,12u);
                    setbit(a,O_SELECTOR,7u);*dp=&D_13020;return WF_OK;
                }
                if(low(a,O_TIMER)<2u)goto finish;
            }
        }
        if(low(a,O_FRAME)==0xfeu){
finish:
            wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);wf_set_be16(a,O_PROFILE,0u);
        }
    }
    *dp=&D_13020;return WF_OK;
}

/* $196AE -> $188BC/$24EC2/$C1EA and $21978, tag with a linked opponent. */
static wf_status_t raw66(wf_runtime_t *rt,wf_actor_raw_t *a,
                         wf_r17_actions_r11_bridge_t *b,const desc_t **dp,
                         wf_r17_actions_r11_trace_t *tr){
    *dp=&D_1969E;
    if(!bit(a,O_LATCH,7u)){
        wf_actor_raw_t *victim=actor_from_ptr(rt,wf_be32(a,O_LINK));
        wf_actor_raw_t *partner=actor_from_ptr(rt,wf_be32(a,WF_A_TEAM_LINK));
        if(!victim||!partner)return block(tr,0x196d8u);
        wf_set_u8(a,O_MOTION,0u);
        uint8_t flags=wf_u8(a,WF_A_FLAGS33);
        wf_r17_m68k_move_b(&rt->m68k_regs,0u,flags);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((rt->m68k_regs.d[0]&65535u)>>5));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(rt->m68k_regs.d[0]&4u));
        wf_set_be16(a,O_X,(flags&128u)?0x2f6u:0x218u);
        wf_set_be16(a,O_Y,(flags&128u)?0x121u:0x193u);
        wf_set_be16(victim,O_X,wf_be16(a,O_X));wf_set_be16(victim,O_Y,wf_be16(a,O_Y));
        wf_r17_m68k_move_b(&rt->m68k_regs,0u,(uint8_t)((flags&128u)^128u));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(rt->m68k_regs.d[0]<<8));
        wf_set_be16(a,O_FACING,(uint16_t)rt->m68k_regs.d[0]);wf_set_be16(victim,O_FACING,wf_be16(a,O_FACING));
        wf_set_be16(victim,O_STATE,0xffu);setbit(partner,WF_A_FLAGS34,3u);
        /* $24EC2 preserves D0-D2 and derives the new active wrestler's tier. */
        uint16_t maximum=wf_be16(partner,0x72u);
        if(maximum){
            uint16_t third=(uint16_t)(maximum/3u),threshold=24u,tier=2u;
            while(threshold<wf_be16(partner,WF_A_STAMINA)){
                third=(uint16_t)(third<<1);threshold=third;
                if(--tier==0u)break;
            }
            wf_set_be16(partner,WF_A_TIER,tier);
        }
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~1u));
        setbit(partner,WF_A_FLAGS33,0u);setbit(a,WF_A_FLAGS32,0u);
        wf_set_u8(partner,WF_A_FLAGS32,(uint8_t)(wf_u8(partner,WF_A_FLAGS32)&~1u));
        if(bit(a,WF_A_ROLE,7u)){
            wf_set_u8(partner,WF_A_ROLE,(uint8_t)(wf_u8(partner,WF_A_ROLE)&~64u));setbit(a,WF_A_ROLE,6u);
        }else if(!bit(partner,WF_A_FLAGS33,1u)){
            uint32_t input=wf_be32(a,0x8au),other=wf_be32(partner,0x8au);
            wf_r17_m68k_move_l(&rt->m68k_regs,0u,input);wf_r17_m68k_move_l(&rt->m68k_regs,1u,other);
            wf_set_be32(partner,0x8au,input);wf_set_be32(a,0x8au,other);
            wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~2u));setbit(partner,WF_A_FLAGS33,1u);
            setbit(a,WF_A_ROLE,6u);wf_set_u8(partner,WF_A_ROLE,(uint8_t)(wf_u8(partner,WF_A_ROLE)&~64u));
        }
        wf_status_t st=wf_r17_tag_palettes_c1ea(rt,b->video_batch,b->video_user);
        if(st!=WF_OK)return block(tr,0xc1eau);
        wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)|(wf_u8(partner,WF_A_FLAGS34)&7u)));
        wf_set_be16(a,0xe6u,wf_be16(partner,0xe6u));
        wf_set_u8(partner,WF_A_FLAGS34,(uint8_t)(wf_u8(partner,WF_A_FLAGS34)&0xf8u));wf_set_be16(partner,0xe6u,0u);
        if(bit(a,WF_A_FLAGS33,7u))rt->g.global_1688=wf_actor_rom_base((unsigned)(a-rt->actors));
        else rt->g.global_1684=wf_actor_rom_base((unsigned)(a-rt->actors));
        wf_set_be16(partner,O_STATE,5u);wf_set_be16(partner,O_SELECTOR,0x4du);setbit(partner,WF_A_ROLE,6u);
        wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~1u));
        wf_set_be32(partner,WF_A_TARGET,wf_be32(a,O_LINK));
    }else{
        if(!bit(a,O_SELECTOR,7u)){
            if(low(a,O_FRAME)!=0xfeu)return WF_OK;
            setbit(a,O_SELECTOR,7u);setbit(a,O_LATCH,7u);
        }
        *dp=&D_14CE0;
        if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);}
    }
    return WF_OK;
}

/* $10D3A uses eleven of the twelve compact records, then queues the
 * temporary sprite immediately. IDs 13/14 and 19/20 use ROM rows $10E28/$10E2E/$10E4C/$10E52. */
static wf_status_t entry_effect(wf_runtime_t *rt,wf_actor_raw_t *a,unsigned id,
                               wf_r17_actions_r11_bridge_t *b,wf_r17_actions_r11_trace_t *tr){
    unsigned i;for(i=0;i<11u;i++)if(!(rt->transient_1258[i][0]&128u))break;
    if(i==11u)return WF_OK;
    if(!b->render)return block(tr,0x10d3au);
    wf_actor_raw_t *v=&b->transient_views[i];
    memset(v,0,sizeof *v);
    /* The ROM's +$2E facing read crosses into the next compact record.
     * Preserve those six bytes too, without aliasing a short C object. */
    memcpy(v->b,(const uint8_t*)rt->transient_1258+i*0x2au,0x30u);
    wf_set_be16(v,0,0x8000u);wf_set_be16(v,O_RENDER_LIST,0u);
    wf_set_be16(v,WF_A_CHARACTER,wf_be16(a,WF_A_CHARACTER));
    uint16_t pose;switch(id){case 13u:pose=0x57u;break;case 14u:pose=0x58u;break;case 19u:pose=0x64u;break;case 20u:pose=0x65u;break;default:return block(tr,0x10d64u);}
    wf_set_be16(v,O_VISUAL,(uint16_t)(pose|wf_be16(a,O_FACING)));
    int16_t offset=(int8_t)wf_u8(a,0x19u);
    if(bit(a,O_FACING,7u))offset=(int16_t)-offset;
    wf_set_be16(v,O_X,(uint16_t)(wf_be16(a,O_X)+offset));
    wf_set_be16(v,O_Y,(uint16_t)(wf_be16(a,O_Y)-1u));
    wf_set_be16(v,O_Z,(uint16_t)(wf_be16(a,O_Z)+1u));
    wf_r17_m68k_move_l(&rt->m68k_regs,0u,id*6u);
    wf_status_t st=wf_r17_world_to_screen_247c(v,b->render->scroll_a_x,b->render->scroll_a_y);
    if(st==WF_OK)st=wf_r17_queue_27b8(b->render,v);
    if(st!=WF_OK)return block(tr,0x27b8u);
    memcpy(rt->transient_1258[i],v->b,0x2au);
    return WF_OK;
}
static wf_status_t raw4d(wf_runtime_t *rt,wf_actor_raw_t *a,
                        wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                        wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1892C;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(a,O_X,bit(a,WF_A_FLAGS33,7u)?0x34cu:0x1c8u);
        wf_set_be16(a,O_Y,bit(a,WF_A_FLAGS33,7u)?0x120u:0x192u);
        wf_set_be16(a,O_Z,0x140u);wf_set_be16(a,O_FRAME,0u);
    }else{
        /* $2052 at pose 0 is the explicitly silent sound output. */
        if(bit(a,WF_A_FLAGS34,3u)){
            if(low(a,O_FRAME)==1u&&wf_be16(a,O_TIMER)==0u){
                wf_set_be16(a,O_TIMER,2u);wf_set_be16(a,O_RENDER_LIST,4u);
                wf_set_be16(a,WF_A_AE,0u);chgbit(a,O_FACING,7u);
                wf_set_be16(a,O_STATE,8u);
                wf_set_be16(a,WF_A_TIMER44,bit(a,WF_A_FLAGS33,7u)?7u:4u);
            }
            return WF_OK;
        }
        if(low(a,O_FRAME)==2u&&wf_be16(a,O_TIMER)==0u)wf_set_be16(a,O_RENDER_LIST,2u);
        if(low(a,O_FRAME)==0xfeu){
            wf_set_u8(a,0xc6u,(uint8_t)(wf_u8(a,0xc6u)&~64u));
            wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,WF_A_AE,0u);
            wf_set_be16(a,O_X,bit(a,WF_A_FLAGS33,7u)?0x2f6u:0x218u);
            wf_set_be16(a,O_Y,bit(a,WF_A_FLAGS33,7u)?0x121u:0x193u);
            wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~8u));
            wf_set_be16(a,O_STATE,0u);
            wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~8u));
            wf_set_be16(a,O_RENDER_LIST,0u);rt->g.global_1697=1u;return WF_OK;
        }
    }
    uint16_t frame=wf_be16(a,O_FRAME);
    return frame<2u?entry_effect(rt,a,19u+frame,b,tr):WF_OK;
}

/* Pin callbacks and their tag/royal continuations, ROM $132F4/$180C4. */
static const desc_t D_132D8={0x132d8u,1u,5u,{6u,6u,6u,6u,6u},{0x84u,0x85u,0x1au,0x1bu,0x1cu}};
static const desc_t D_125C0={0x125c0u,1u,1u,{0xff00u},{0xffffu}};
static const desc_t D_180AC={0x180acu,1u,1u,{0xff00u},{0x1du}};
static const desc_t D_180B8={0x180b8u,1u,1u,{0xff00u},{0x1eu}};
static const desc_t D_1AD10={0x1ad10u,1u,3u,{8u,8u,8u},{0x216u,0x217u,0x216u}};
static const desc_t D_1AD76={0x1ad76u,0u,0u,{0u},{0u}};
static void pin_clear(wf_actor_raw_t*a,size_t off,uint8_t mask){wf_set_u8(a,off,(uint8_t)(wf_u8(a,off)&~mask));}
static wf_actor_raw_t *pin_candidate(wf_runtime_t *rt,wf_actor_raw_t *a){
    wf_actor_raw_t *t=actor_from_ptr(rt,wf_be32(a,O_LINK)),*chosen=0;
    rt->g.global_1692=0x400u;
    for(unsigned i=4u;i<9u;i++){
        wf_actor_raw_t*c=&rt->actors[i];
        if(c==a||c==t||!bit(c,0u,7u)||bit(c,0xb5u,3u)||bit(c,0xb5u,7u)||bit(c,0x32u,4u))continue;
        wf_actor_raw_t *base=low(a,O_SELECTOR)==0x48u?t:a;if(!base)continue;
        if((bit(base,0x56u,5u)==bit(c,0x56u,5u))==(low(a,O_SELECTOR)==0x48u))continue;
        uint16_t ax=wf_be16(base,O_X),cx=wf_be16(c,O_X),ay=wf_be16(base,O_Y),cy=wf_be16(c,O_Y);
        uint16_t distance=(uint16_t)((ax>=cx?ax-cx:cx-ax)+(ay>=cy?ay-cy:cy-ay));
        if(distance>=rt->g.global_1692)continue;
        rt->g.global_1692=distance;rt->g.global_168c=ACTOR_BASE+i*ACTOR_STRIDE;chosen=c;
    }
    if(chosen){setbit(chosen,0xb5u,7u);pin_clear(chosen,0xb6u,0x80u);}return chosen;
}
static int pin_random(wf_runtime_t *rt,unsigned first){
    uint32_t saved[3]={rt->m68k_regs.d[0],rt->m68k_regs.d[1],rt->m68k_regs.d[2]};unsigned sample=50u;
    for(int n=3;n>=0;n--){wf_r17_m68k_moveq(&rt->m68k_regs,1u,(int8_t)n);sample=(wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs)&255u)>>1;if(sample<100u)break;if(!n)sample=50u;}
    memcpy(rt->m68k_regs.d,saved,sizeof saved);if(!sample)sample=1u;return sample>first;
}
static wf_status_t pin_helpers(wf_runtime_t *rt,wf_actor_raw_t*a,int defender,wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u){
        if(!defender&&!bit(a,0x56u,7u))return WF_OK;
        wf_actor_raw_t *candidate=pin_candidate(rt,a);if(!candidate)return WF_OK;
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x24ccu);
        /* $21838 starts with $28,$3C; $2172A with $46,$1E. */
        if(pin_random(rt,defender?40u:70u)){pin_clear(candidate,0xb5u,0x80u);setbit(candidate,0xb6u,7u);}
        else{wf_set_be32(candidate,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,ACTOR_BASE+(uint32_t)(candidate-rt->actors)*ACTOR_STRIDE);}return WF_OK;
    }
    if(!bit(a,0x33u,0u)||(!defender&&bit(a,0x32u,0u)))return WF_OK;
    wf_actor_raw_t *partner=actor_from_ptr(rt,wf_be32(a,0x86u));if(!partner)return block(tr,defender?0x21756u:0x215e2u);
    setbit(partner,0x34u,0u);pin_clear(partner,0x34u,2u);
    int cpu=bit(partner,0x32u,0u);if(cpu)wf_set_be16(partner,0xe6u,1u);else rt->g.global_1682=0xfau;
    if(defender){
        if(bit(partner,0x33u,1u)){if(cpu){wf_set_be32(partner,0x7au,wf_be32(a,O_LINK));setbit(partner,0x34u,2u);}return WF_OK;}
        if(!cpu&&bit(partner,0x74u,7u))return WF_OK;
        if(cpu)setbit(partner,0x34u,2u);
        setbit(partner,0xb5u,7u);wf_set_be32(partner,0x7eu,wf_be32(a,O_LINK));
        if(!cpu)wf_set_be32(a,0x7eu,wf_be32(a,0x86u));return WF_OK;
    }
    wf_actor_raw_t *t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x21638u);
    if(bit(partner,0x33u,1u)){if(cpu){wf_set_be32(partner,0x7au,wf_be32(t,0x86u));setbit(partner,0x34u,2u);}return WF_OK;}
    if(bit(a,0x33u,1u)){
        if(cpu){wf_set_be32(partner,0x7au,wf_be32(t,0x86u));setbit(partner,0x34u,2u);}else pin_clear(partner,0x56u,0x40u);
        pin_clear(a,0x33u,2u);setbit(partner,0x33u,1u);setbit(partner,0x35u,3u);return WF_OK;
    }
    if(!cpu&&bit(partner,0x74u,7u))return WF_OK;
    if(cpu)setbit(partner,0x34u,2u);
    setbit(partner,0xb5u,7u);wf_set_be32(partner,0x7eu,wf_be32(t,0x86u));if(!cpu)wf_set_be32(a,0x7eu,wf_be32(a,0x86u));return WF_OK;
}
static wf_status_t raw48(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_132D8;if(bit(a,O_SELECTOR,7u)){*dp=&D_125C0;return WF_OK;}
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x132f4u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x5cu,0x480u);wf_set_be16(a,0x5eu,0x48u);
        wf_status_t s=wf_r17_trajectory_26ae(a,t,1u);if(s!=WF_OK)return s;
        wf_set_u8(a,O_FACING,(uint8_t)((wf_u8(a,0x58u)&0x80u)^0x80u));
        if(!bit(t,O_SELECTOR,7u)){wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x4au);}return WF_OK;
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_MOTION,2u);
    if(low(a,O_FRAME)<4u)return WF_OK;
    wf_set_be16(a,0x42u,0xffe0u);if(!bit(a,0x37u,4u))return WF_OK;wf_set_be16(a,0x42u,0u);
    if(low(t,O_STATE)!=5u||low(t,O_SELECTOR)!=0x4au||wf_r17_helper_11412_carry(rt,a)){
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_set_be16(a,0x68u,3u);setbit(a,O_LINK,7u);wf_set_be32(t,0x9cu,0u);return WF_OK;
    }
    wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_STATE,5u);setbit(a,O_SELECTOR,7u);setbit(a,0x35u,0u);setbit(a,0x33u,6u);setbit(t,0x33u,6u);
    wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));for(unsigned off=6;off<=14;off+=4)wf_set_be16(a,off,wf_be16(t,off));
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xfff8u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,64,-8,0);wf_set_be16(a,O_VISUAL,0xffffu);
    wf_status_t s=pin_helpers(rt,a,0,tr);if(s!=WF_OK)return s;s=pin_helpers(rt,t,1,tr);if(s!=WF_OK)return s;
    wf_set_be16(t,O_STATE,5u);setbit(t,O_SELECTOR,7u);*dp=&D_125C0;return WF_OK;
}
static wf_status_t raw4a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_180AC;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);uint16_t hp=wf_be16(a,0x66u),delay=(uint16_t)(42u-hp);
        if((int16_t)delay<0)delay=1u;wf_set_be16(a,0xaau,hp?delay:0x100u);
    }else{
        if(!(rt->g.mode_0161&1u)&&!bit(a,0x56u,7u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x11448u);
            uint8_t flags=wf_u8(p,0x34u)&0xc0u;rt->m68k_regs.d[0]=(rt->m68k_regs.d[0]&0xffffff00u)|flags;
            if(flags){pin_clear(p,0x34u,0xc0u);p=actor_from_ptr(rt,wf_be32(p,0x7au));if(!p)return block(tr,0x11462u);pin_clear(p,0x34u,0xc0u);}
        }
        if(!t)return block(tr,0x180ecu);
        if(wf_be16(a,0x66u)&&bit(t,O_SELECTOR,7u)&&!(wf_u8(a,0x56u)&0xc0u)){
            rt->g.global_167a|=0x80u;if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){uint16_t d=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,d?d:0x4000u);}
        }
        if(low(t,O_STATE)!=5u||(low(t,O_SELECTOR)!=0x48u&&low(t,O_SELECTOR)!=0xeu&&low(t,O_SELECTOR)!=0xfu))goto escape;
        if(bit(a,0x32u,4u)){
            rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=0x29u;wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x9cu));
            if(p&&wf_be32(p,O_LINK)==ACTOR_BASE+(uint32_t)(a-rt->actors)*ACTOR_STRIDE&&low(p,O_STATE)==5u&&low(p,O_SELECTOR)==0x49u){wf_set_be16(p,O_STATE,7u);pin_clear(p,0x35u,1u);wf_set_be16(p,O_Z,wf_be16(a,O_Z));}
        }else if(!bit(a,0xfeu,7u))goto descriptor;
        wf_set_be16(t,O_STATE,7u);pin_clear(t,0x35u,1u);setbit(t,O_LINK,7u);
        escape:wf_set_be16(a,0x9au,32u);wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,wf_be16(a,0x64u)&255u);return WF_OK;
    }
    descriptor:
    if(low(a,O_STATE)!=14u&&bit(a,O_SELECTOR,7u))*dp=low(a,0x64u)==8u?&D_180AC:&D_180B8;
    else{*dp=&D_125C0;wf_set_u8(a,O_VISUAL+1u,low(a,0x64u)==8u?0x12u:0x13u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));}return WF_OK;
}
/* $12614[$4B] -> $181EC, failed-pin escape and $21282 cleanup. */
static const desc_t D_181EC={0x181ecu,1u,2u,{12u,12u},{0x6eu,0x6fu}};
static const desc_t D_181FC={0x181fcu,1u,2u,{12u,12u},{0x70u,0x71u}};
static wf_status_t escape_tag_212d4(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
    if(!bit(a,0x33u,0u))return WF_OK;
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x212deu);
    if(bit(a,0x33u,1u)&&bit(p,0x33u,1u))pin_clear(p,0x56u,0x40u);
    else{
        if(bit(a,0x33u,1u)||bit(a,0x56u,7u))pin_clear(p,0xb5u,0x80u);
        else{setbit(a,0x33u,1u);pin_clear(p,0x33u,2u);setbit(p,0x35u,5u);setbit(p,0x56u,6u);}
        rt->g.global_1697|=1u;
    }
    int clear=bit(p,0x32u,0u);
    if(!clear){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        wf_actor_raw_t*q=t?actor_from_ptr(rt,wf_be32(t,0x86u)):0;
        if(!q)return block(tr,0x21344u);clear=bit(q,0x32u,0u);
    }
    /* The branch at $21354 still clears A1 (our partner), not A2. */
    if(clear){pin_clear(p,0x34u,5u);wf_set_be16(p,0xe6u,0u);}return WF_OK;
}
static wf_status_t escape_cleanup_21282(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
    if(wf_u8(a,0x109u)==4u||wf_u8(a,0x109u)==5u){rt->g.global_15d2=15u;rt->g.global_15d3=30u;}
    wf_set_be16(a,0x108u,0u);if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u){
        setbit(a,0xb6u,7u);uint32_t ptr=wf_be32(a,0x7eu);
        if(ptr){wf_actor_raw_t*p=actor_from_ptr(rt,ptr);if(!p)return block(tr,0x21368u);pin_clear(p,0xb5u,0x80u);setbit(p,0xb6u,7u);}
        return WF_OK; /* This owner is raw $4B, so the $53-only branch is unreachable. */
    }
    wf_status_t s=escape_tag_212d4(rt,a,tr);if(s!=WF_OK)return s;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x212bcu);
    return escape_tag_212d4(rt,t,tr);
}
static void escape_position_10b9a(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*t){
    uint16_t dx=bit(a,O_FACING,7u)?0xffb0u:0x50u;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,dx);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,16u);
    wf_set_be16(t,O_X,(uint16_t)(wf_be16(a,O_X)+dx));wf_set_be16(t,O_Y,(uint16_t)(wf_be16(a,O_Y)-1u));wf_set_be16(t,O_Z,(uint16_t)(wf_be16(a,O_Z)+16u));
}
static wf_status_t raw4b(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_181EC;
    if(!bit(a,O_LATCH,7u)){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x18216u);
        pin_clear(t,0x35u,1u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,15u);
        wf_set_be16(t,O_VISUAL,0xf0u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);wf_set_be16(t,0x9au,80u);escape_position_10b9a(rt,a,t);
        wf_status_t s=escape_cleanup_21282(rt,a,tr);if(s!=WF_OK)return s;
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x9cu));
        if(p&&wf_be32(p,O_LINK)==ACTOR_BASE+(uint32_t)(a-rt->actors)*ACTOR_STRIDE&&low(p,O_STATE)==5u&&low(p,O_SELECTOR)==0x49u){
            wf_set_be16(p,O_STATE,4u);wf_set_be16(p,0x64u,15u);setbit(p,O_FLAGS18,7u);wf_set_u8(p,O_FACING,wf_u8(a,O_FACING)^0x80u);
            wf_set_be16(p,0x9au,80u);pin_clear(p,0x35u,1u);wf_set_be16(p,O_VISUAL,0xf0u);wf_set_u8(p,O_VISUAL,wf_u8(p,O_FACING));pin_clear(p,0x33u,0x40u);
            /* $10B9A reloads A1 from A0+$26 despite caller EXG A2,A3. */
            escape_position_10b9a(rt,a,t);
        }
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);wf_set_be16(a,0x64u,0u);setbit(a,O_LINK,7u);wf_set_be32(a,0x9cu,0u);return WF_OK;
    }
    *dp=low(a,0x64u)==8u?&D_181EC:&D_181FC;return WF_OK;
}
static wf_status_t pin_outcome(wf_runtime_t*rt,wf_actor_raw_t*a,unsigned own,unsigned rival,wf_r17_actions_r11_trace_t*tr){
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u)),*t=actor_from_ptr(rt,wf_be32(a,0x7au));
    wf_actor_raw_t*q=t?actor_from_ptr(rt,wf_be32(t,0x86u)):0;if(!p||!t||!q)return block(tr,0x1ad56u);
    wf_set_u8(a,0xa0u,(uint8_t)own);wf_set_u8(p,0xa0u,(uint8_t)own);wf_set_u8(t,0xa0u,(uint8_t)rival);wf_set_u8(q,0xa0u,(uint8_t)rival);return WF_OK;
}
static wf_status_t raw8b(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1AD10;if(!bit(a,O_LATCH,7u))wf_set_u8(a,O_MOTION,0u);
    else if(low(a,O_FRAME)==0xfeu)return pin_outcome(rt,a,0x40u,0x80u,tr);return WF_OK;
}
static wf_status_t raw8c(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1AD76;if(!bit(a,O_LATCH,7u)){setbit(a,O_LATCH,7u);wf_set_be16(a,O_VISUAL,0x1d3u|wf_be16(a,O_FACING));wf_set_be16(a,O_TIMER,64u);}
    uint16_t previous=wf_be16(a,O_TIMER);wf_set_be16(a,O_TIMER,(uint16_t)(previous-1u));if(previous)return WF_OK;
    if(wf_u8(a,0xffu)==3u)return pin_outcome(rt,a,0x80u,0x40u,tr);
    if(wf_u8(a,0xffu)!=5u)return WF_OK;
    if(!(rt->g.mode_0161&1u))return pin_outcome(rt,a,0x80u,0x80u,tr);
    for(unsigned i=0;i<9;i++)if(bit(&rt->actors[i],0u,7u)&&!bit(&rt->actors[i],0x56u,7u))wf_set_u8(&rt->actors[i],0xa0u,0x80u);return WF_OK;
}

/* $12614[$3D] aliases the full main-idle owner $114B2. */
static const desc_t D_114AC={0x114acu,0u,0u,{0u},{0u}};
wf_status_t wf_r17_action_idle_114b2(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
    if(!rt||!a)return WF_BAD_STATE;
    int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched){
        wf_set_u8(a,O_MOTION,0u);pin_clear(a,0x33u,0x58u);pin_clear(a,0x32u,4u);
        static const uint8_t clear_words[]={0x12u,0xaau,0x44u,0x46u,0x24u,0x22u,0x18u,0x1au,0x3eu,0x40u,0x42u};
        for(unsigned i=0;i<sizeof clear_words;i++)wf_set_be16(a,clear_words[i],0u);
        if(!(rt->g.credited_route&0x8000u))pin_clear(a,0x32u,8u);
        if(!wf_be16(a,0xd0u))wf_set_be16(a,0xd0u,0xd0u);
        if(bit(a,0x32u,4u)){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x7au);}
        int release=bit(a,0x34u,6u);pin_clear(a,0x34u,0x40u);
        if(release){wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,0x7au));if(!t)return block(tr,0x11546u);pin_clear(t,0x34u,0x80u);rt->g.global_1697=1u;}
    }else{
        wf_set_be16(a,O_PROFILE,1u);
        if(wf_actor_unlink_115d2(rt,a)!=WF_OK)return block(tr,0x115deu);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
    }
    if(bit(a,0x74u,7u)){wf_set_be16(a,O_PROFILE,14u);wf_set_u8(a,O_B1B,16u);wf_set_be16(a,O_VISUAL,(bit(a,0x75u,0u)?0x82u:0x7eu)|wf_be16(a,O_FACING));}
    if(bit(a,0xfeu,7u)){
        pin_clear(a,0x34u,0x10u);
        if(bit(a,0xffu,0u)){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x8cu);}
        else if(!bit(a,0xffu,1u)){wf_set_be16(a,O_STATE,1u);wf_set_be16(a,0xaeu,12u);}
    }
    return WF_OK;
}

/* $12614[$08] -> $1316C. Selector high bits are ignored by this callback. */
static const desc_t D_1316C={0x1316cu,1u,3u,{6u,12u,8u},{0xcbu,0xccu,0xcdu}};
static const desc_t D_18A90={0x18a90u,1u,2u,{16u,16u},{0x66u,0x67u}};
static wf_status_t action_offset_10b62(wf_runtime_t*rt,wf_actor_raw_t*a,uint16_t displacement,wf_r17_actions_r11_trace_t*tr){
    uint16_t contact=wf_be16(a,0x36u);wf_set_be16(a,0x3eu,displacement);wf_status_t status=wf_r17_resolver_280dc(rt,a);if(status!=WF_OK)return block(tr,0x280dcu);
    if(wf_be16(a,0x36u)&3u)wf_set_be16(a,0x58u,0u);
    wf_set_be16(a,0x36u,contact);wf_set_be16(a,6u,(uint16_t)(wf_be16(a,6u)+wf_be16(a,0x38u)));wf_set_be16(a,0x3eu,0u);return WF_OK;
}
static wf_status_t raw08(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1316C;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x13180u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);for(unsigned off=6;off<=14;off+=4)wf_set_be16(a,off,wf_be16(t,off));wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));wf_set_be16(a,0x18u,0x50u);wf_set_be16(a,0x4au,2u);return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        if(!wf_r17_helper_11412_carry(rt,a)&&low(t,O_STATE)==4u&&low(t,0x64u)==8u){wf_set_be16(t,O_STATE,0xffu);setbit(a,0x33u,6u);setbit(t,0x33u,6u);wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));}
        else{wf_set_be16(a,O_STATE,0u);setbit(a,0x18u,7u);wf_set_be16(a,O_TIMER,2u);wf_set_be16(a,0x4au,0u);setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(bit(a,O_FACING,7u)?0xff80u:0x80u)));}
        return WF_OK;
    }
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if((bit(a,0x56u,7u)||!bit(t,0x33u,5u))&&bit(t,0x56u,7u)&&bit(t,0x33u,5u)){
        wf_set_be16(t,O_STATE,12u);wf_set_be16(t,0x44u,0x2000u);wf_set_be16(a,O_STATE,0xffu);wf_set_be16(a,0x44u,0u);
    }else{wf_set_be16(a,O_STATE,12u);wf_set_be16(a,0x44u,0x2000u);wf_set_be16(t,O_STATE,0xffu);wf_set_be16(t,0x44u,0u);}
    setbit(a,0x18u,7u);wf_set_be16(a,0x4au,0u);wf_set_be16(t,0x9au,0u);wf_set_be16(t,0x54u,0u);wf_set_be16(t,0x52u,0u);wf_set_be16(t,0xaau,0u);
    wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));chgbit(t,O_FACING,7u);wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(bit(a,O_FACING,7u)?0xff80u:0x80u)));wf_set_be16(t,O_X,wf_be16(a,O_X));wf_set_be16(t,O_Y,wf_be16(a,O_Y));
    wf_status_t status=action_offset_10b62(rt,a,0u,tr);if(status!=WF_OK)return status;return action_offset_10b62(rt,t,0u,tr);
}
/* $18AA0: return from the apron, with exact tier update $24EC2. */
static wf_status_t raw4e(wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_18A90;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);uint16_t maximum=wf_be16(a,0x72u),hp=wf_be16(a,0x66u);
        if(maximum)wf_set_be16(a,0x70u,hp<=24u?2u:hp<=(uint16_t)((maximum/3u)*2u)?1u:0u);
        wf_set_be16(a,O_RENDER_LIST,2u);
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);pin_clear(a,0xc6u,0x40u);pin_clear(a,0x34u,0x10u);wf_set_be16(a,0xaeu,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,O_RENDER_LIST,0u);pin_clear(a,0x32u,1u);wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(bit(a,0x33u,7u)?0xffc8u:56u)));
    }return WF_OK;
}

static const desc_t D_1603A={0x1603au,1u,5u,{6u,10u,16u,6u,18u},{0x107u,0x108u,0x109u,0x10au,0x10bu}};
/* $16056: linked throw. $2052 is the runtime's silent sound output. */
static wf_status_t raw24(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1603A;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16056u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_status_t status=action_offset_10b62(rt,a,0xffe8u,tr);if(status!=WF_OK)return status;
        wf_set_be16(t,O_STATE,0xffu);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=wf_u8(a,3u)==8u?0x27u:1u;return WF_OK;
    }
    if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x16u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_be16(t,0x68u,14u);chgbit(t,O_FACING,7u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,64u);
        wf_set_be16(t,O_X,(uint16_t)(wf_be16(a,O_X)+(bit(a,O_FACING,7u)?16u:0xfff0u)));wf_set_be16(t,O_Y,wf_be16(a,O_Y));wf_set_be16(t,O_Z,(uint16_t)(wf_be16(a,O_Z)+64u));pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,24,0,0);setbit(a,O_LINK,7u);
    }return WF_OK;
}

static const desc_t D_1981E={0x1981eu,1u,3u,{0xff00u,64u,16u},{0x16u,0x13u,0x68u}};
static const desc_t D_115F2={0x115f2u,2u,4u,{10u,10u,10u,10u},{1u,2u,3u,4u}};
static const desc_t D_1160A={0x1160au,2u,4u,{12u,12u,12u,12u},{5u,6u,7u,8u}};
static const desc_t D_11622={0x11622u,2u,4u,{20u,16u,20u,16u},{0x7cu,0x7du,0x7eu,0x7fu}};
static const desc_t D_1163A={0x1163au,2u,4u,{20u,16u,20u,16u},{0x80u,0x81u,0x82u,0x83u}};
static const desc_t *ring_walk_descriptor(wf_actor_raw_t*a){
    if(bit(a,0x74u,7u)){wf_set_u8(a,0x1bu,16u);return bit(a,0x75u,0u)?&D_1163A:&D_11622;}
    return bit(a,0x32u,3u)?&D_1160A:&D_115F2;
}
/* Exact $1F15C/$20C8, including its shared RAM scratch bytes. */
static int ring_exit_arrived(wf_runtime_t*rt,wf_actor_raw_t*a){
    uint16_t x=wf_be16(a,6u),y=wf_be16(a,10u),tx=wf_be16(a,0xbeu),ty=wf_be16(a,0xc0u);
    if((unsigned)(x>=tx?x-tx:tx-x)<8u&&(unsigned)(y>=ty?y-ty:ty-y)<8u)return 1;
    static const uint8_t base[4]={0,192,64,128};unsigned quadrant=0;uint16_t dx=(uint16_t)(tx-x),dy=(uint16_t)(ty-y);
    if(dx&0x8000u){quadrant|=1u;dx=(uint16_t)(0u-dx);}if(dy&0x8000u){quadrant|=2u;dy=(uint16_t)(0u-dy);}
    if(dx>255u)dx=255u;if(dy>255u)dy=255u;
    unsigned sum=(unsigned)dx+dy,component=(quadrant==0u||quadrant==3u)?dx:dy;uint8_t angle=(uint8_t)(base[quadrant]+component*64u/(sum?sum:1u));uint8_t*s=rt->g.direction_scratch_15e4;
    s[0]=angle;s[1]=(uint8_t)(wf_actor_rom_base((unsigned)(a-rt->actors))>>16);s[2]=(uint8_t)(dx>>8);s[3]=(uint8_t)dx;s[4]=(uint8_t)(dy>>8);s[5]=(uint8_t)dy;s[6]=(uint8_t)(ty>>8);s[7]=(uint8_t)ty;wf_set_u8(a,0x2du,angle);return 0;
}
static wf_status_t raw68(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    unsigned phase=wf_be16(a,0x44u)&3u;*dp=&D_1981E;
    if(phase==0u){
        if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_u8(a,0x19u,0xd0u);return WF_OK;}
        if(!wf_be16(a,O_FRAME)){
            if(!bit(a,0x37u,4u))return WF_OK;
            if(!(rt->g.mode_0161&1u)&&bit(a,0x33u,0u)){rt->g.global_1678=0x80u;rt->g.global_1679=wf_u8(a,O_FACING);wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x6au);return WF_OK;}
            wf_set_u8(a,O_MOTION,0u);pin_clear(a,0x33u,8u);wf_set_be16(a,O_TIMER,0u);return WF_OK;
        }
        if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))setbit(a,0x18u,7u);
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,5u);
            if(!(rt->g.mode_0161&1u))wf_set_be16(a,0x44u,1u);
            else{wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_SELECTOR,0x7au);wf_set_be16(a,0x44u,4u);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=0x29u;}
        }return WF_OK;
    }
    if(phase==1u&&!bit(a,O_LATCH,7u)){
        static const uint8_t magnitude[12]={27,29,28,25,30,27,24,30,22,27,28,27};unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x1174cu);
        wf_set_u8(a,O_MOTION,1u);unsigned v=bit(a,0x32u,3u)?12u:magnitude[id];if(bit(a,0x56u,7u)&&bit(a,0x33u,5u))v=(v+6u)&255u;if(bit(a,0x74u,7u))v=(v-8u)&65535u;wf_set_be16(a,0x2au,(uint16_t)v);chgbit(a,O_FACING,7u);
        wf_set_be16(a,0xbeu,wf_be16(a,O_X)>=0x280u?0x3b0u:0x142u);wf_set_be16(a,0xc0u,0xe0u);
    }else if(phase==2u&&!bit(a,O_LATCH,7u)){wf_set_be16(a,0xbeu,0x279u);wf_set_be16(a,0xc0u,0xe0u);}
    else if(phase==3u&&!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,1u);wf_set_be16(a,0x2cu,0u);}
    *dp=ring_walk_descriptor(a);
    if(phase<3u){if(ring_exit_arrived(rt,a)){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,(uint16_t)(phase+1u));}}
    else if(wf_be16(a,O_Y)>=0x100u){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_Y,0x100u);wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x69u);}
    return WF_OK;
}

/* $199FC/$19A20: climb down to the floor; mode bit 1 selects Royal. */
static const desc_t D_199FC={0x199fcu,1u,7u,{12u,12u,12u,12u,12u,12u,12u},{0x20u,0x21u,0x22u,0x23u,0x24u,0x25u,0x26u}};
static const desc_t D_19B3C={0x19b3cu,1u,3u,{64u,16u,8u},{0x13u,0x68u,0u}};
static const desc_t D_19BBA={0x19bbau,0u,0u,{0u},{0u}};
static wf_status_t ring_transfer(wf_runtime_t*rt,uint8_t command,wf_r17_actions_r11_trace_t*tr){
    uint16_t n=rt->g.transfer_count_1828;
    if(n>=sizeof rt->g.transfer_queue_1808)return block(tr,0x19a3cu);
    rt->g.transfer_queue_1808[n]=command;rt->g.transfer_count_1828=(uint16_t)(n+1u);return WF_OK;
}
static wf_status_t raw69(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_199FC;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(a,0x1bu,0x57u);
        if(!(rt->g.mode_0161&2u))return ring_transfer(rt,1u,tr);
        wf_set_be16(a,O_Y,0x150u);wf_set_be16(a,O_Z,0xf0u);
        if(bit(a,0x74u,7u)){wf_actor_raw_t*object=object_from_ptr(b,wf_be32(a,0x76u));if(!object)return block(tr,0x19a70u);wf_set_be16(object,0x1cu,2u);wf_set_be16(a,0x74u,0u);}
        return WF_OK;
    }
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER))pin_clear(a,0x33u,4u);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,0u);rt->g.global_1697=1u;wf_set_be16(a,0xaeu,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,O_Z,0x140u);
    if(!(rt->g.mode_0161&2u)){wf_set_be16(a,O_Y,0x118u);return ring_transfer(rt,2u,tr);}
    wf_set_be16(a,O_Y,0x160u);
    /* $250E enumerates all nine enabled actor records at $5122. */
    unsigned count=0;for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){wf_actor_raw_t*p=&rt->actors[i];if(!(wf_be16(p,0u)&0x8000u))continue;if(!bit(p,0x33u,0u))continue;if(!bit(p,0x32u,1u)&&bit(p,0x33u,2u))continue;count++;}
    if(count==2u){rt->g.global_1678=0xc0u;rt->g.global_1679=0u;}return WF_OK;
}
static wf_status_t raw6a(wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_19B3C;
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);wf_set_u8(a,0x19u,0xd0u);wf_set_be16(a,O_Z,0x100u);wf_set_be16(a,O_X,wf_be16(a,O_X)<0x280u?0x1f0u:0x438u);wf_set_be16(a,O_Y,0x168u);}
    else{if(!low(a,O_FRAME)&&!low(a,O_TIMER))setbit(a,0x18u,7u);if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x6bu);chgbit(a,O_FACING,7u);}}return WF_OK;
}
static wf_status_t raw6b(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19BBA;
    if(!bit(a,O_LATCH,7u)){
        static const uint8_t magnitude[12]={27,29,28,25,30,27,24,30,22,27,28,27};unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x1174cu);
        wf_set_u8(a,O_MOTION,1u);unsigned v=bit(a,0x32u,3u)?12u:magnitude[id];if(bit(a,0x56u,7u)&&bit(a,0x33u,5u))v=(v+6u)&255u;if(bit(a,0x74u,7u))v=(v-8u)&65535u;wf_set_be16(a,0x2au,(uint16_t)v);
        int right=bit(a,0x33u,0u)?wf_be16(a,O_X)>=0x280u:bit(a,0x33u,7u);wf_set_be16(a,0xbeu,right?0x42cu:0x210u);wf_set_be16(a,0xc0u,0x120u);
    }
    *dp=ring_walk_descriptor(a);if(ring_exit_arrived(rt,a)){wf_set_be16(a,O_STATE,0u);pin_clear(a,0x32u,0x20u);}return WF_OK;
}

static const desc_t D_1623C={0x1623cu,1u,5u,{20u,8u,16u,0xff00u,32u},{0x204u,0x205u,0x206u,0x207u,0x208u}};
static const desc_t D_1A9D4={0x1a9d4u,2u,2u,{24u,16u},{0x204u,0x205u}};
/* $16258 linked suplex and its original reversal $1A9E4. */
static wf_status_t raw26(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1623C;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16258u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_STATE,0xffu);wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xf8u));wf_set_be16(a,0xf8u,(uint16_t)(wf_be16(a,0xf8u)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x85u);return WF_OK;}
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,18u);wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0xff80u:0x80u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x500u);wf_set_be16(a,0x5eu,0x48u);wf_set_u8(a,O_MOTION,0u);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=20u;return WF_OK;
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){wf_set_u8(a,O_MOTION,2u);setbit(a,0x33u,3u);return WF_OK;}
    if(low(a,O_FRAME)==3u&&bit(a,0x37u,4u)){rt->g.global_1800=14u;wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);return WF_OK;}
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,5u);
        if(!(rt->g.mode_0161&1u)){wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);}
        wf_set_be16(t,0x68u,22u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,112u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,32u);
        wf_set_be16(t,O_X,(uint16_t)(wf_be16(a,O_X)+(bit(a,O_FACING,7u)?0xff90u:112u)));wf_set_be16(t,O_Y,(uint16_t)(wf_be16(a,O_Y)-1u));wf_set_be16(t,O_Z,(uint16_t)(wf_be16(a,O_Z)+32u));if(bit(a,O_FACING,7u))wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xff90u);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);
        if(rt->g.credited_route&&!bit(a,0x33u,1u)){static const uint16_t chance[12]={136u,128u,68u,64u,120u,120u,80u,128u,128u,133u,128u,64u};unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x110b6u);uint32_t d0=rt->m68k_regs.d[0],d1=rt->m68k_regs.d[1];uint16_t v=(uint16_t)(wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs)&255u);if(v<chance[id])setbit(a,0x34u,4u);rt->m68k_regs.d[0]=d0;rt->m68k_regs.d[1]=d1;}
        setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }return WF_OK;
}
static wf_status_t raw85(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1A9D4;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1a9e4u);
    if(!bit(a,O_LATCH,7u)){wf_r17_m68k_move_w(&rt->m68k_regs,0u,32u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,32,0,0);wf_set_be16(a,0x44u,80u);}
    uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,timer);if(timer)return WF_OK;
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,14u);wf_set_be16(a,0x68u,8u);wf_set_be16(a,O_VISUAL,0x2bu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0xc006u);wf_set_be16(t,O_VISUAL,0x60u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+72u));wf_set_be32(a,0x92u,wf_actor_rom_base((unsigned)(t-rt->actors)));wf_set_be32(t,0x92u,wf_actor_rom_base((unsigned)(a-rt->actors)));setbit(a,0x33u,6u);setbit(t,0x33u,6u);wf_set_be16(a,0x18u,0u);wf_set_be16(t,0x18u,0u);wf_set_be16(a,0x1au,0u);wf_set_be16(t,0x1au,0u);return WF_OK;
}

/* $19F0E/$19F1E: tagged-out encouragement animation and original effects. */
static const desc_t D_19F0E={0x19f0eu,1u,2u,{12u,12u},{0x55u,0x56u}};
static wf_status_t raw71(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19F0E;
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x44u,6u);wf_set_be16(a,O_FRAME,0u);}
    else{
        /* $1D526 can end this action immediately when the tag timer expires. */
        uint16_t timer=wf_be16(a,0xe6u);
        if(bit(a,0x34u,0u)&&timer){
            wf_set_be16(a,0xe6u,(uint16_t)(timer-1u));
            if(timer==1u){setbit(a,0x56u,6u);pin_clear(a,0x34u,3u);uint32_t ptr=wf_actor_rom_base((unsigned)(a-rt->actors));if(bit(a,0x33u,7u))rt->g.global_1688=ptr;else rt->g.global_1684=ptr;if(bit(a,0xc6u,6u)){pin_clear(a,0xc6u,0x40u);setbit(a,0x33u,5u);}rt->g.global_1682=0x177u;wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x4eu);return WF_OK;}
        }
        if(low(a,O_FRAME)==0xfeu){pin_clear(a,O_LATCH,0x80u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,8u);uint16_t count=wf_be16(a,0x44u);wf_set_be16(a,0x44u,(uint16_t)(count-1u));if(!count){wf_set_be16(a,O_STATE,1u);wf_set_be16(a,0xaeu,1u);pin_clear(a,0xc6u,0x40u);setbit(a,0x33u,5u);}}
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){if(bit(a,0x33u,7u))rt->g.global_11b0=4u;else rt->g.global_1150=4u;}
    uint8_t frame=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))frame=(uint8_t)(frame+1u);rt->m68k_regs.d[1]=(rt->m68k_regs.d[1]&0xffffff00u)|frame;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,frame?14u:13u);return entry_effect(rt,a,frame?14u:13u,b,tr);
}

static const desc_t D_18772={0x18772u,1u,4u,{12u,12u,12u,12u},{0x60u,0x61u,0x66u,0x67u}};
/* $188BC: exchange the active participant, preserving the original input links. */
static wf_status_t tag_exchange_188bc(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*p,wf_r17_actions_r11_bridge_t*b,wf_r17_actions_r11_trace_t*tr){
    uint16_t maximum=wf_be16(p,0x72u);if(maximum){uint16_t third=(uint16_t)(maximum/3u),threshold=24u,tier=2u;while(threshold<wf_be16(p,0x66u)){third=(uint16_t)(third<<1);threshold=third;if(!--tier)break;}wf_set_be16(p,0x70u,tier);}
    pin_clear(a,0x33u,1u);setbit(p,0x33u,0u);setbit(a,0x32u,0u);pin_clear(p,0x32u,1u);
    if(bit(a,0x56u,7u)){pin_clear(p,0x56u,0x40u);setbit(a,0x56u,6u);}
    else if(!bit(p,0x33u,1u)){uint32_t input=wf_be32(a,0x8au),other=wf_be32(p,0x8au);wf_r17_m68k_move_l(&rt->m68k_regs,0u,input);wf_r17_m68k_move_l(&rt->m68k_regs,1u,other);wf_set_be32(p,0x8au,input);wf_set_be32(a,0x8au,other);pin_clear(a,0x33u,2u);setbit(p,0x33u,1u);setbit(a,0x56u,6u);pin_clear(p,0x56u,0x40u);}
    wf_status_t st=wf_r17_tag_palettes_c1ea(rt,b->video_batch,b->video_user);return st==WF_OK?st:block(tr,0xc1eau);
}
static void tag_timer_21978(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*p){
    wf_set_u8(a,0x34u,(uint8_t)(wf_u8(a,0x34u)|(wf_u8(p,0x34u)&7u)));wf_set_be16(a,0xe6u,wf_be16(p,0xe6u));pin_clear(p,0x34u,7u);wf_set_be16(p,0xe6u,0u);uint32_t ptr=wf_actor_rom_base((unsigned)(a-rt->actors));if(bit(a,0x33u,7u))rt->g.global_1688=ptr;else rt->g.global_1684=ptr;
}
static int tag_special_1f760(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*t){
    if(!rt->g.credited_route||low(t,O_STATE)!=4u||low(t,0x64u)!=8u)return 0;
    uint16_t x=wf_be16(t,O_X),y=wf_be16(t,O_Y);
    if(bit(a,0x33u,7u)){if(y>=0x160u)return 0;return bit(t,O_FACING,7u)?x>=0x2d0u&&x<0x300u:x>=0x298u;}
    if(y<0x170u)return 0;return bit(t,O_FACING,7u)?x<0x26au:x>=0x1f8u&&x<0x248u;
}
static wf_status_t tag_rescue_214c0(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*p,int*nonzero,wf_r17_actions_r11_trace_t*tr){
    *nonzero=0;if(!rt->g.credited_route)return WF_OK;
    /* Royal: failed search returns MOVEQ #1; success ends on the nonzero
     * candidate pointer MOVE.L at $215AC. Both leave Z clear. */
    if(rt->g.mode_0161&1u){wf_actor_raw_t*c=pin_candidate(rt,a);*nonzero=1;if(c){if(!wf_be32(a,O_LINK))wf_set_be16(a,0u,1u); /* $2159C/$215A2 null-link diagnostic write. */
        wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_actor_rom_base((unsigned)(c-rt->actors)));}return WF_OK;}
    setbit(a,0x56u,6u);pin_clear(p,0x56u,0x40u);uint32_t ptr=wf_actor_rom_base((unsigned)(a-rt->actors));if(bit(a,0x33u,7u))rt->g.global_1688=ptr;else rt->g.global_1684=ptr;setbit(a,0x34u,0u);pin_clear(a,0x34u,2u);wf_set_u8(a,O_STATE,0u);pin_clear(a,0xb5u,0x10u);wf_set_be16(a,O_SELECTOR,0x37u);
    if(!bit(p,0x33u,1u)){
        static const uint8_t weights[12][3]={{15,30,40},{10,25,40},{5,15,30},{10,25,40},{20,35,45},{20,35,45},{20,35,45},{10,35,45},{10,15,20},{10,25,40},{15,35,45},{15,35,45}};
        uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);unsigned tier;
        if(t)tier=wf_be16(t,0x70u);
        else if(!(link&0x00ffffffu))tier=0u; /* 68000 24-bit bus: ROM WORD $000070 is $0000. */
        else return block(tr,0x21536u);
        unsigned id=wf_u8(p,0x57u);if(id>=12u||tier>=3u)return block(tr,0x2154cu);if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x24ccu);
        if(!pin_random(rt,weights[id][tier])){wf_set_be16(p,0x44u,bit(p,0x33u,7u)?7u:4u);wf_set_be32(p,0x7au,wf_be32(a,0x7au));wf_set_be16(p,O_STATE,8u);return WF_OK;}
        setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,0x7au));
    }
    *nonzero=1;return WF_OK;
}
static wf_status_t raw4c(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18772;wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x187c4u);
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);int right=bit(a,0x33u,7u);wf_set_be16(a,O_X,right?0x2f6u:0x218u);wf_set_be16(a,O_Y,right?0x121u:0x193u);wf_set_be16(a,O_Z,0x140u);wf_set_u8(a,O_FACING,right?128u:0u);wf_set_be16(p,O_STATE,5u);wf_set_be16(p,O_SELECTOR,0x4du);pin_clear(a,0x32u,8u);return WF_OK;}
    if(low(a,O_FRAME)==1u&&low(a,O_TIMER)<2u){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,0x7au));if(!t)return block(tr,0x1f76eu);
        if(tag_special_1f760(rt,a,t)){wf_status_t st=tag_exchange_188bc(rt,a,p,b,tr);if(st!=WF_OK)return st;tag_timer_21978(rt,a,p);pin_clear(a,0x32u,1u);wf_set_be16(t,0x9au,0x1000u);wf_set_be16(t,0xaau,0x1000u);int nz=0;st=tag_rescue_214c0(rt,a,p,&nz,tr);if(st!=WF_OK)return st;if(nz){wf_set_be16(p,O_STATE,1u);wf_set_be16(p,0xaeu,2u);setbit(p,0x32u,0u);}return WF_OK;}
        wf_set_be16(a,O_RENDER_LIST,2u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffe0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,-32,0,0);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=24u;
    }
    if(low(a,O_FRAME)==0xfeu){tag_timer_21978(rt,a,p);wf_status_t st=tag_exchange_188bc(rt,a,p,b,tr);if(st!=WF_OK)return st;wf_set_be16(a,O_STATE,1u);wf_set_be16(a,0xaeu,3u);setbit(a,O_LINK,7u);chgbit(a,O_FACING,7u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,O_RENDER_LIST,4u);wf_set_be16(p,O_TIMER,0u);wf_set_be16(a,0x2cu,0u);wf_set_be16(a,O_X,wf_be16(p,O_X));}
    return WF_OK;
}

static const desc_t D_17A40={0x17a40u,1u,4u,{8u,8u,8u,8u},{0x18bu,0x18cu,0x18du,0x1cfu}};
static const desc_t D_194CA={0x194cau,0u,0u,{0u},{0u}};
static const desc_t D_19264={0x19264u,1u,2u,{12u,12u},{0x18eu,0x18fu}};
/* $17A58: $1C0A0 substitutes ROM $7F000 for a literal null link.
 * A nonzero tagged-null pointer instead reaches the vectors via the 24-bit bus.
 * Both ranges are ROM: writes are ignored by the arcade bus. */
static wf_status_t raw37(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_17A40;uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);if(!t&&(link&0x00ffffffu))return block(tr,0x17a64u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);if(t){wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x61u);}
        if(!bit(a,O_SELECTOR,7u)){
            wf_set_be16(a,O_X,t?wf_be16(t,O_X):link?0x654u:0u);wf_set_be16(a,O_Y,t?wf_be16(t,O_Y):link?0xffffu:0u);wf_set_be16(a,O_Z,t?wf_be16(t,O_Z):link?0xffffu:0u);wf_set_u8(a,O_FACING,t?wf_u8(t,O_FACING):link?0xffu:0u);setbit(a,0x33u,6u);if(t)setbit(t,0x33u,6u);
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x17a9cu);if(!bit(p,0x32u,1u)){wf_set_be16(p,O_STATE,5u);wf_set_be16(p,O_SELECTOR,0x4eu);if(bit(a,0xc6u,6u)){pin_clear(a,0xc6u,0x40u);setbit(a,0x33u,5u);}}return WF_OK;
        }
        setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)&&t)setbit(t,O_SELECTOR,7u);
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);}return WF_OK;
}
static wf_status_t tag_prepare_211ec(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u){wf_actor_raw_t*c=pin_candidate(rt,a);if(c){if(!wf_be32(a,O_LINK))wf_set_be16(a,0u,1u);wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_actor_rom_base((unsigned)(c-rt->actors)));}return WF_OK;}
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x21206u);unsigned tier=wf_be16(p,0x70u);if(tier>=3u)return block(tr,0x21218u);static const uint16_t timer[3]={125u,187u,187u};wf_set_be16(p,0xe6u,timer[tier]);setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);setbit(p,0x34u,2u);
    if(bit(a,0x33u,1u))wf_set_be32(p,0x7au,wf_be32(a,O_LINK));else{setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));}return WF_OK;
}
static wf_status_t raw61(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_194CA;int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched&&!bit(a,O_SELECTOR,7u)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,5u);uint16_t recovery=wf_be16(a,0xaau);if(!recovery||(recovery&0x8000u)){unsigned tier=wf_be16(a,0x70u);if(tier>=3u)return block(tr,0x10ca8u);static const uint8_t delay[3]={24u,28u,32u};wf_set_be16(a,0xaau,(uint16_t)(delay[tier]-1u));}
        wf_status_t st=tag_prepare_211ec(rt,a,tr);if(st!=WF_OK)return st;wf_set_be16(a,0x44u,170u);return WF_OK;
    }
    if(!(wf_u8(a,0x56u)&0xc0u)){rt->g.global_167a|=128u;if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,timer?timer:0x4000u);}}
    if(!bit(a,O_SELECTOR,7u)){wf_set_be16(a,O_VISUAL,0x12u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));}
    else{wf_set_be16(a,O_VISUAL,0xffffu);uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,timer);if(!timer){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x59u);}}return WF_OK;
}
static wf_status_t tag_cleanup_213a6(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*t,wf_r17_actions_r11_trace_t*tr){
    if(rt->g.mode_0161&1u){uint32_t ptr=wf_be32(a,0x7eu);if(ptr){wf_actor_raw_t*c=actor_from_ptr(rt,ptr);if(!c)return block(tr,0x21414u);pin_clear(c,0xb5u,0x80u);}return WF_OK;}
    if(t){
        pin_clear(t,0x34u,1u);if(bit(t,0x33u,1u))pin_clear(t,0x56u,0x40u);wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x213ceu);if(!bit(p,0x33u,1u)){pin_clear(p,0xb5u,0x80u);rt->g.global_1697|=1u;}
    }else{
        if(wf_be32(a,O_LINK)&0x00ffffffu)return block(tr,0x213b6u);
        /* $213B6 re-reads the tagged link, not dispatcher A2=$7F000.
         * Physical ROM $33/$86 are FF/FFFFFFFF; the next +$33 wraps
         * to ROM $32=FF. All target writes hit ROM and bit1 stays set. */
    }
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x213e8u);pin_clear(p,0x34u,4u);if(bit(p,0x33u,1u))pin_clear(p,0x56u,0x40u);else pin_clear(p,0xb5u,0x80u);return WF_OK;
}
static wf_status_t raw59(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19264;uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    /* $19274 only dereferences A2 during init or frame0/timer0. After
     * release the renderer can clear +$26 while the final pose still runs.
     * Null A2=$7F000 and tagged physical zero both address read-only ROM. */
    if(!bit(a,O_LATCH,7u)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x1927cu);
        if(t){wf_set_be16(t,O_STATE,0xffu);pin_clear(t,0x35u,2u);}pin_clear(a,0x35u,4u);return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x1929cu);
        if(t){wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,2u);wf_set_be16(t,0x68u,3u);chgbit(t,O_FACING,7u);pin_clear(t,0x33u,0x40u);}
        setbit(a,O_LINK,7u);pin_clear(a,0x33u,0x40u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,41u);wf_status_t st=tag_cleanup_213a6(rt,a,t,tr);if(st!=WF_OK)return st;
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));}return WF_OK;
}

static const desc_t D_13734={0x13734u,1u,2u,{20u,16u},{0xe4u,0xe5u}};
static const desc_t D_1A148={0x1a148u,1u,1u,{24u},{0x1dfu}};
static wf_status_t interrupt_cleanup_212a0(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
 if(!rt->g.credited_route)return WF_OK;if(rt->g.mode_0161&1u)return block(tr,0x21358u);
 wf_status_t st=escape_tag_212d4(rt,a,tr);if(st!=WF_OK)return st;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x212bcu);return escape_tag_212d4(rt,t,tr);
}
/* At each $24900 call in $13744, A1 is still descriptor $13734.
 * Its ROM +$86 LONG is $00E433FC, distinct from every actor address. */
static void interrupt_audio_24900(wf_runtime_t*rt){if(!(rt->g.mode_0161&1u)){rt->g.global_15d2=15u;rt->g.global_15d3=25u;}}
static void interrupt_position(wf_runtime_t*rt,wf_actor_raw_t*owner,wf_actor_raw_t*target,int16_t x,int16_t y,int16_t z){
 if(bit(owner,O_FACING,7u))x=(int16_t)-x;wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)x);wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)y);wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)z);
 wf_set_be16(target,O_X,(uint16_t)(wf_be16(owner,O_X)+(uint16_t)x));wf_set_be16(target,O_Y,(uint16_t)(wf_be16(owner,O_Y)+(uint16_t)y));wf_set_be16(target,O_Z,(uint16_t)(wf_be16(owner,O_Z)+(uint16_t)z));
}
static wf_status_t raw0a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_13734;if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);return WF_OK;}wf_set_be16(a,O_PROFILE,1u);
 if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
  wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1376au);if(wf_be16(t,0xfeu))return WF_OK;
  wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,O_LINK));unsigned state=low(t,O_STATE),sub=low(t,0x64u);uint16_t sel=(uint16_t)(wf_be16(t,O_SELECTOR)&0x80ffu);
  if(state==5u)wf_r17_m68k_move_w(&rt->m68k_regs,1u,sel);
  if(state==4u&&(sub==8u||sub==9u)){wf_set_be16(t,O_STATE,4u);wf_set_u8(t,0x65u,(uint8_t)(sub-2u));wf_set_be16(t,0x68u,2u);setbit(a,O_LINK,7u);wf_set_be16(t,0xe4u,(uint16_t)(wf_be16(t,0xe4u)+1u));rt->g.global_15d2=15u;rt->g.global_15d3=11u;return WF_OK;}
  if(state==5u&&(sel==0x8048u||sel==0x800eu)){
   if(sel==0x8048u){wf_actor_raw_t*q=actor_from_ptr(rt,wf_be32(t,0x9cu));if(q&&low(q,O_STATE)==5u&&low(q,O_SELECTOR)==0x49u){t=q;p=actor_from_ptr(rt,wf_be32(t,O_LINK));wf_set_be32(a,O_LINK,wf_actor_rom_base((unsigned)(t-rt->actors)));goto release49;}wf_set_be16(t,0x64u,15u);chgbit(t,O_FACING,7u);}
   else wf_set_be16(t,0x64u,0x10fu);
   if(!p)return block(tr,0x13826u);interrupt_position(rt,p,t,sel==0x8048u?80:20,-1,32);interrupt_audio_24900(rt);wf_set_be16(t,O_STATE,4u);pin_clear(t,0x35u,1u);pin_clear(t,0x33u,0x40u);pin_clear(p,0x33u,0x40u);wf_status_t st=escape_cleanup_21282(rt,p,tr);if(st!=WF_OK)return st;wf_set_be16(p,O_STATE,7u);setbit(a,O_LINK,7u);wf_set_be32(t,0x9cu,0u);setbit(t,O_LINK,7u);return WF_OK;
  }
  if(state==5u&&sel==0x801au){
   if(!p)return block(tr,0x138c4u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);pin_clear(t,0x35u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffa0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(p,-96,-1,0);wf_status_t st=escape_cleanup_21282(rt,p,tr);if(st!=WF_OK)return st;setbit(t,0x18u,7u);wf_set_be16(p,O_STATE,7u);interrupt_audio_24900(rt);pin_clear(t,0x33u,0x40u);pin_clear(p,0x33u,0x40u);setbit(a,O_LINK,7u);setbit(t,O_LINK,7u);return WF_OK;
  }
  if(state==5u&&sel==0x8009u){
   if(!p)return block(tr,0x1394eu);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);pin_clear(t,0x35u,2u);chgbit(t,O_FACING,7u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(t,-48,-1,0);wf_set_be16(p,O_STATE,7u);chgbit(p,O_FACING,7u);pin_clear(p,0x35u,4u);wf_status_t st=interrupt_cleanup_212a0(rt,p,tr);if(st!=WF_OK)return st;interrupt_audio_24900(rt);pin_clear(t,0x33u,0x40u);pin_clear(p,0x33u,0x40u);setbit(a,O_LINK,7u);setbit(t,O_LINK,7u);return WF_OK;
  }
  if(state==5u&&sel==0x8061u){interrupt_audio_24900(rt);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);return WF_OK;}
  if(state==5u&&sel==0x8049u){
release49:
   if(!p)return block(tr,0x139e0u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,15u);pin_clear(t,0x35u,1u);pin_clear(p,O_SELECTOR,0x20u);pin_clear(t,0x33u,0x40u);setbit(a,O_LINK,7u);wf_set_be32(p,0x9cu,0u);return WF_OK;
  }
  return WF_OK;
 }
 if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}return WF_OK;
}
static wf_status_t raw77(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_1A148;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1a154u);
 if(!bit(a,O_LATCH,7u))wf_set_be16(t,O_STATE,0xffu);
 else if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x8061u);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x8037u);}return WF_OK;
}

static const desc_t D_15B6C={0x15b6cu,1u,6u,
    {16u,16u,10u,10u,8u,32u},{0x18bu,0xceu,0xcfu,0xd0u,0xd1u,0xd2u}};
/* $2503C raw $26 -> $259A0, type-2 text "GIVE UP" at FG $0E30.
 * $2513A..$25188 update only the odd tile/attribute bytes for glyphs,
 * while the space clears four complete LONGs. Preserve that write order. */
static wf_status_t action22_message_2503c(wf_runtime_t*rt,
                                          wf_r17_actions_r11_bridge_t*b,
                                          wf_r17_actions_r11_trace_t*tr){
    static const char text[]="GIVE UP";
    wf_r17_fg0_write_t writes[52];size_t count=0u;
    for(unsigned i=0u;i<7u;i++){
        uint32_t at=0xc0e30u+8u*i;
        if(text[i]==' '){
            writes[count++]=(wf_r17_fg0_write_t){at,0u,4u};
            writes[count++]=(wf_r17_fg0_write_t){at+4u,0u,4u};
            writes[count++]=(wf_r17_fg0_write_t){at+0x100u,0u,4u};
            writes[count++]=(wf_r17_fg0_write_t){at+0x104u,0u,4u};
        }else{
            uint16_t tile=(uint16_t)(((unsigned)text[i]-0x10u)*4u+0x94u);
            for(unsigned column=0u;column<2u;column++)for(unsigned row=0u;row<2u;row++){
                uint32_t cell=at+4u*column+0x100u*row;
                uint16_t value=(uint16_t)(tile+2u*column+row);
                writes[count++]=(wf_r17_fg0_write_t){cell+1u,value&255u,1u};
                writes[count++]=(wf_r17_fg0_write_t){cell+3u,value>>8,1u};
            }
        }
    }
    if(!b->video_batch)return block(tr,0x2503cu);
    wf_status_t status=b->video_batch(rt,writes,count,b->video_user);
    return status==WF_OK?status:block(tr,0x2503cu);
}
/* Only the $22 path of $21358: its low selector cannot be $53. */
static wf_status_t action22_cleanup_21358(wf_runtime_t*rt,wf_actor_raw_t*a,
                                         wf_r17_actions_r11_trace_t*tr){
    setbit(a,0xb6u,7u);
    if(wf_be32(a,0x7eu)){
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x7eu));
        if(!p)return block(tr,0x21368u);
        pin_clear(p,0xb5u,0x80u);setbit(p,0xb6u,7u);
    }
    return WF_OK;
}
static wf_status_t action22_result_111c8(wf_runtime_t*rt,wf_actor_raw_t*a,
                                        wf_actor_raw_t*t,
                                        wf_r17_actions_r11_bridge_t*b,
                                        int*carry,wf_r17_actions_r11_trace_t*tr){
    static const uint16_t bonus[4]={24u,32u,12u,0u};
    *carry=0;
    if(!rt->g.credited_route||bit(a,0x33u,2u)||!bit(a,0x33u,0u)||
       !bit(t,0x33u,0u)||wf_be16(t,0x66u))return WF_OK;
    int previous=bit(a,O_SELECTOR,3u);setbit(a,O_SELECTOR,3u);
    if(!previous)return WF_OK;
    if(!(rt->g.mode_0161&1u)){
        wf_set_be16(&rt->referee,O_STATE,0x8009u);rt->g.global_169e=0u;
        wf_set_be16(a,0xfeu,0x8000u);
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));
        if(!p)return block(tr,0x1123cu);
        wf_set_be16(p,0xfeu,0x8000u);wf_set_be16(t,0xfeu,0x8001u);
        p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x1124cu);
        wf_set_be16(p,0xfeu,0x8001u);
        /* $90D6 preserves all registers. ADD.W wraps before the clamp. */
        uint16_t amount=bonus[(rt->g.config_0066>>13)&3u];
        for(unsigned i=0u;i<4u;i++){
            wf_actor_raw_t*c=&rt->actors[i];if(bit(c,0xffu,0u))continue;
            uint16_t health=(uint16_t)(wf_be16(c,0x66u)+amount);
            wf_set_be16(c,0x66u,health);
            wf_set_be16(c,0x6au,(uint16_t)(wf_be16(c,0x6au)+amount));
            if(health>=wf_be16(c,0x72u))wf_set_be16(c,0x66u,wf_be16(c,0x72u));
        }
        rt->g.global_169e=0u;wf_set_be16(a,0x4au,4u);
        if(!bit(a,0x56u,7u)){
            rt->g.global_15d2=15u;rt->g.global_15d3=42u;
            wf_status_t status=action22_message_2503c(rt,b,tr);
            if(status!=WF_OK)return status;
            *carry=1;return WF_OK;
        }
    }
    wf_set_be16(a,0xc4u,(uint16_t)(wf_be16(a,0xc4u)+1u));
    wf_status_t status=action22_cleanup_21358(rt,a,tr);
    if(status==WF_OK)*carry=1;
    return status;
}
/* $215B6 as reached by raw $22. Royal immediately returns because its
 * selector test requires $48; TAG uses the $2172C partner delay table. */
static wf_status_t action22_partner_215b6(wf_runtime_t*rt,wf_actor_raw_t*a,
                                         wf_r17_actions_r11_trace_t*tr){
    static const uint16_t delay[3]={93u,125u,187u};
    if(!rt->g.credited_route||(rt->g.mode_0161&1u)||
       bit(a,0x32u,0u)||!bit(a,0x33u,0u))return WF_OK;
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));
    if(!p)return block(tr,0x215e2u);
    int cpu=bit(p,0x32u,0u);
    if(cpu){
        wf_set_be16(p,0xe6u,1u);
        unsigned tier=wf_be16(p,0x70u);
        if(tier>=3u)return block(tr,0x2172cu);
        wf_set_be16(p,0xe6u,delay[tier]);
    }
    setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);
    if(!cpu)rt->g.global_1682=0xfau;
    if(bit(p,0x33u,1u)){
        if(cpu){
            wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
            if(!t)return block(tr,0x216b8u);
            wf_set_be32(p,0x7au,wf_be32(t,0x86u));setbit(p,0x34u,2u);
        }
        return WF_OK;
    }
    if(bit(a,0x33u,1u)){
        if(cpu){
            wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
            if(!t)return block(tr,0x216a4u);
            wf_set_be32(p,0x7au,wf_be32(t,0x86u));setbit(p,0x34u,2u);
        }else pin_clear(p,0x56u,0x40u);
        pin_clear(a,0x33u,2u);setbit(p,0x33u,1u);setbit(p,0x35u,3u);
        return WF_OK;
    }
    if(!cpu&&bit(p,0x74u,7u))return WF_OK;
    if(cpu)setbit(p,0x34u,2u);
    setbit(p,0xb5u,7u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x216d8u);
    wf_set_be32(p,0x7eu,wf_be32(t,0x86u));
    if(!cpu)wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
    return WF_OK;
}
/* $15B8C: linked submission, with $111C8 qualification at completion. */
static wf_status_t raw22(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_15B6C;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x15b98u);
        wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(a,O_X,wf_be16(t,O_X));wf_set_be16(a,O_Y,wf_be16(t,O_Y));
        wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x15bbau);
        if(low(t,O_STATE)==4u&&low(t,0x64u)==8u&&
           !wf_r17_helper_11412_carry(rt,a)){
            setbit(a,0x33u,6u);setbit(t,0x33u,6u);
            setbit(a,0x35u,1u);setbit(t,0x35u,2u);
            wf_set_be16(t,O_STATE,0xffu);
            rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=10u;
        }else{
            wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
            setbit(a,O_LINK,7u);wf_set_be16(a,O_TIMER,1u);
        }
        return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,bit(a,O_SELECTOR,7u)?0x8014u:0u);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if(!t)return block(tr,0x15c4eu);
    if(bit(a,0xfeu,7u))goto release;
    wf_set_be16(a,O_FRAME,3u);wf_set_be16(a,O_TIMER,0u);
    wf_set_be16(t,0x68u,3u);
    int carry=0;wf_status_t status=action22_result_111c8(rt,a,t,b,&carry,tr);
    if(status!=WF_OK)return status;
    if(carry){
        if(rt->g.mode_0161&1u){
            rt->g.global_15d2=wf_u8(t,3u);rt->g.global_15d3=41u;
            setbit(t,0x32u,4u);setbit(a,O_LINK,7u);
            wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        }
release:
        wf_set_be16(a,O_STATE,7u);wf_set_be16(t,O_STATE,4u);
        wf_set_be16(t,0x64u,wf_be16(t,0x64u)&0xffu);
        pin_clear(a,0x35u,2u);pin_clear(t,0x35u,4u);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        return WF_OK;
    }
    int already=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
    if(already)return WF_OK;
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x5fu);
    return action22_partner_215b6(rt,a,tr);
}

static const desc_t D_141D8={0x141d8u,1u,3u,{20u,0xff00u,8u},{0xefu,0xf0u,0xf0u}};
/* $11192 uses the same $E958 selector-A box as $11152, against raw61. */
static int jump_hit_11192(const wf_actor_raw_t*a,const wf_actor_raw_t*t){
 if(low(t,O_STATE)!=5u||low(t,O_SELECTOR)!=0x61u)return 0;
 uint16_t x=wf_be16(t,O_X),y=wf_be16(t,O_Y),left=(uint16_t)(x-(bit(t,O_FACING,7u)?104u:8u)),right=(uint16_t)(x+(bit(t,O_FACING,7u)?8u:104u));
 return left<wf_be16(a,O_X)&&right>=wf_be16(a,O_X)&&(uint16_t)(y-72u)<wf_be16(a,O_Y)&&(uint16_t)(y+24u)>=wf_be16(a,O_Y);
}
static wf_status_t raw10(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_141D8;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(!bit(a,O_SELECTOR,7u)){
  if(!bit(a,O_LATCH,7u)){
   if(!t)return block(tr,0x14200u);wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x48u);unsigned route;
   if(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x61u)route=5u;
   else{
    uint16_t x=(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7u)?-56:56));wf_set_u8(a,O_FACING,(uint8_t)((wf_u8(a,O_FACING)&127u)|(x>=wf_be16(a,O_X)?128u:0u)));wf_set_be16(a,O_X,x);wf_set_be16(a,O_Y,wf_be16(t,O_Y));wf_set_be16(a,O_Z,wf_be16(t,O_Z));
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,16u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,16u);helper_10bd0(a,16,0,16);route=low(t,0x64u)==8u?10u:11u;
   }
   wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)route);wf_status_t st=wf_r17_trajectory_26ae(a,t,(uint16_t)route);if(st!=WF_OK)return block(tr,0x26aeu);
   if(route==5u)wf_set_be16(a,O_FACING,(wf_be16(a,0x58u)&0x8000u)^0x8000u);
   wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+64u));wf_set_u8(a,O_B1B,16u);return WF_OK;
  }
  if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER))setbit(a,0x18u,7u);
  if(low(a,O_FRAME)==1u){
   wf_set_be16(a,0x42u,0xffe0u);if(!bit(a,0x37u,4u))return WF_OK;wf_set_be16(a,0x42u,0u);wf_set_u8(a,O_MOTION,0u);if(!t)return block(tr,0x11156u);
   if(wf_r17_helper_11152_carry(a,t)){
    wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,(uint16_t)((uint16_t)(wf_be16(t,0x64u)&255u)-2u));wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0x68u,10u);wf_set_be16(t,0xd2u,(uint16_t)(wf_be16(t,0xd2u)+1u));return WF_OK;
   }
   if(jump_hit_11192(a,t)){
    wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);wf_set_be16(t,0x68u,10u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));return WF_OK;
   }
   wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,80u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,80,-1,0);chgbit(a,O_FACING,7u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_set_be16(a,0x68u,3u);rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);return WF_OK;
  }
 }
 if(low(a,O_FRAME)==0xfeu){setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_r17_m68k_move_w(&rt->m68k_regs,0u,24u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,24,-1,0);setbit(a,O_LINK,7u);}return WF_OK;
}

static const desc_t D_13D26={0x13d26u,1u,6u,{8u,16u,16u,0xff00u,0xff00u,0xff00u},{0x84u,0x85u,0xeau,0xebu,0xebu,0xecu}};
static const desc_t D_11C62_R11={0x11c62u,2u,6u,{6u,6u,6u,6u,6u,6u},{9u,10u,11u,12u,13u,14u}};
static const desc_t D_11DD0_R11={0x11dd0u,1u,2u,{8u,12u},{0x69u,0x69u}};
/* $13D46 shares its first three phase addresses with raw23. The selected
 * descriptor is A1 on return, including the original descriptor at an edge. */
static wf_status_t raw0e(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_13D26;if(rt->g.mode_0161&2u)wf_set_be16(a,0x44u,3u);
 unsigned phase=wf_be16(a,0x44u)&3u;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(phase==0u){
  if(!bit(a,O_LATCH,7u)){
   static const uint8_t speed[12]={47u,53u,49u,45u,56u,48u,44u,56u,40u,47u,48u,47u};
   /* The sixth $176BA entry is literally the following $0828 opcode. */
   static const uint16_t mash[6]={54u,30u,22u,16u,10u,0x828u};unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x11d4au);if(!t)return block(tr,0x1761eu);
   wf_set_u8(a,O_MOTION,1u);wf_set_be16(a,0x2au,speed[id]);wf_set_be16(t,0x9au,0x200u);unsigned n=wf_be16(t,0xdcu);if(n>5u)n=5u;
   uint16_t hp=wf_be16(t,0x66u),v=mash[n];wf_r17_m68k_move_w(&rt->m68k_regs,1u,hp);v=v<hp?1u:(uint16_t)(v-hp);wf_set_be16(t,0xaau,v);
   pin_clear(a,O_FACING,0x80u);wf_set_be16(a,0x2cu,0xc0u);if(wf_be16(a,O_X)<0x270u){setbit(a,O_FACING,7u);wf_set_be16(a,0x2cu,0x40u);}
  }else{
   wf_set_be16(a,O_PROFILE,0u);
   if((bit(a,O_FACING,7u)&&bit(a,0x37u,0u))||(!bit(a,O_FACING,7u)&&bit(a,0x37u,1u))){
    uint8_t c=wf_u8(a,0x36u);if(!(rt->g.mode_0161&2u)&&(c==1u||c==5u)){
     if(bit(a,0x37u,1u)){if(!(rt->g.global_1150&255u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}}
     else if(bit(a,0x37u,0u)&&!(rt->g.global_11b0&255u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
    }else if(c==3u){wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,0x17u);}
    wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,(uint16_t)(wf_be16(a,0x44u)+1u));return WF_OK;
   }
  }
  *dp=&D_11C62_R11;return WF_OK; /* $1112E emits sound only. */
 }
 if(phase==1u){
  if(!bit(a,O_LATCH,7u)){chgbit(a,O_FACING,7u);chgbit(a,0x2du,7u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x2au,12u);}
  else{if(low(a,O_FRAME)==1u)wf_set_u8(a,O_MOTION,1u);if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,(uint16_t)(wf_be16(a,0x44u)+1u));}}
  *dp=&D_11DD0_R11;return WF_OK;
 }
 if(phase==2u){
  if(!bit(a,O_LATCH,7u)){
   static const uint8_t speed[12]={47u,53u,49u,45u,56u,48u,44u,56u,40u,47u,48u,47u};unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x11d4au);if(!t)return block(tr,0x1772eu);
   wf_set_u8(a,O_MOTION,1u);wf_set_be16(a,0x2au,speed[id]);wf_set_be16(a,0xbeu,(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7u)?-56:56)+(bit(a,O_FACING,7u)?-96:96)));
  }else{uint16_t x=wf_be16(a,0xbeu);if((!bit(a,O_FACING,7u)&&x>=wf_be16(a,O_X))||(bit(a,O_FACING,7u)&&x<wf_be16(a,O_X))){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,(uint16_t)(wf_be16(a,0x44u)+1u));}}
  *dp=&D_11C62_R11;return WF_OK;
 }
 if(!bit(a,O_LATCH,7u)){
  if(!t)return block(tr,0x26aeu);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x5cu,0x600u);wf_set_be16(a,0x5eu,0x48u);wf_status_t st=wf_r17_trajectory_26ae(a,t,2u);if(st!=WF_OK)return block(tr,0x26aeu);wf_set_be16(a,O_FACING,(wf_be16(a,0x58u)&0x8000u)^0x8000u);return WF_OK;
 }
 if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,32u);
 if(!bit(a,O_SELECTOR,7u)){
  if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){wf_set_u8(a,O_MOTION,2u);return WF_OK;}
  if(low(a,O_FRAME)==3u){
   setbit(a,O_SELECTOR,4u);if(bit(a,0x37u,4u)){
    if(!t)return block(tr,0x11152u);setbit(a,0x18u,7u);wf_set_u8(a,O_MOTION,0u);
    if(!wf_r17_helper_11152_carry(a,t)||wf_r17_helper_11412_carry(rt,a)){
     wf_set_be16(a,O_STATE,4u);if(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x54u){wf_set_be16(a,0x64u,30u);return WF_OK;}
     wf_set_be16(a,0x64u,5u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));chgbit(a,O_FACING,7u);wf_set_be16(a,0x68u,3u);setbit(a,O_LINK,7u);wf_set_be16(t,0x9au,0u);wf_set_be16(t,0xaau,0u);return WF_OK;
    }
    wf_set_be16(a,O_TIMER,0u);setbit(a,0x33u,6u);setbit(t,0x33u,6u);rt->g.global_1800=14u;wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x51u);wf_set_be16(t,0x68u,17u);pin_clear(a,O_SELECTOR,0x10u);wf_set_be16(t,0xdau,(uint16_t)(wf_be16(t,0xdau)+1u));wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=32u;
    /* $258E[3] preserves all live registers and applies own facing to VX. */
    wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x38u);wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,O_X,(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7u)?-48:48)));wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
    if(rt->g.credited_route&&!bit(a,0x33u,2u)&&bit(a,0x33u,0u)&&bit(t,0x33u,0u)){setbit(a,0x35u,0u);setbit(a,O_SELECTOR,7u);/* $216E8 does nothing for selector0E outside TAG. */if(!(rt->g.mode_0161&1u))return pin_helpers(rt,a,0,tr);}
    return WF_OK;
   }
  }
 }
 if(low(a,O_FRAME)!=0xfeu&&bit(a,0x37u,4u)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);rt->g.global_1800=14u;if(!bit(a,O_SELECTOR,7u))wf_set_be16(a,O_STATE,7u);}return WF_OK;
}

static const desc_t D_18BF4={0x18bf4u,1u,2u,{0xff00u,2u},{0x15u,0x1du}};
static const desc_t D_18C04={0x18c04u,1u,2u,{0xff00u,2u},{0x17u,0x1eu}};
static const desc_t D_18C14={0x18c14u,1u,2u,{0xff00u,2u},{0x16u,0x13u}};
/* $18C24, splash victim. Its terminal TAG handoff uses the existing exact
 * $21732/$215B6 partner owners; A1 remains the original descriptor at FE. */
static wf_status_t raw51(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_18BF4;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(!bit(a,O_LATCH,7u)){
  uint16_t hp=wf_be16(a,0x66u),delay=(uint16_t)(42u-hp);if((int16_t)delay<0)delay=1u;wf_set_be16(a,0xaau,hp?delay:0x100u);
  wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x38u);wf_set_u8(a,O_MOTION,2u);
 }else{
  if(!low(a,O_FRAME)&&bit(a,0x37u,4u)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);}
  if(low(a,O_FRAME)==0xfeu){
   if(!t)return block(tr,0x18c78u);
   if(rt->g.credited_route&&bit(a,0x33u,0u)&&bit(t,0x33u,0u)&&!bit(a,0x33u,2u)){
    wf_set_be16(a,O_STATE,0x8005u);wf_set_be16(a,O_LATCH,0x8005u);wf_set_be16(a,O_SELECTOR,0xc04au);wf_status_t st=pin_helpers(rt,a,1,tr);if(st!=WF_OK)return st;
    if(low(t,O_SELECTOR)!=0x0eu){wf_set_be16(t,O_STATE,0x8005u);wf_set_be16(t,O_LATCH,0x8005u);wf_set_be16(t,O_SELECTOR,0xc048u);return pin_helpers(rt,t,0,tr);}return WF_OK;
   }
   wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,wf_be16(a,0x64u)&255u);wf_set_be16(a,0x9au,0u);
   wf_set_be16(a,0x9au,bit(a,0x56u,7u)&&(bit(a,0x33u,5u)||bit(a,0x33u,2u))?48u:low(a,0x70u)==2u?224u:low(a,0x70u)==1u?128u:48u);
   wf_set_be16(t,O_STATE,7u);setbit(a,O_LINK,7u);return WF_OK;
  }
 }
 if(!t)return block(tr,0x18ceau);*dp=low(t,O_SELECTOR)==0x0eu?&D_18C14:low(a,0x64u)==8u?&D_18BF4:&D_18C04;return WF_OK;
}

static const desc_t D_17266={0x17266u,1u,5u,{20u,12u,8u,0xff00u,24u},{0x131u,0x132u,0x133u,0x134u,0x135u}};
static const desc_t D_1A940={0x1a940u,2u,2u,{24u,16u},{0x131u,0x132u}};
/* $1108C preserves D0/D1 around its live $21B4 call. */
static wf_status_t landing_1108c(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
 if(!rt->g.credited_route||bit(a,0x33u,1u))return WF_OK;
 static const uint16_t chance[12]={136u,128u,68u,64u,120u,120u,80u,128u,128u,133u,128u,64u};unsigned id=wf_be16(a,2u);
 if(id>=12u)return block(tr,0x110b6u);if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x21b4u);
 uint32_t d0=rt->m68k_regs.d[0],d1=rt->m68k_regs.d[1];uint16_t sample=(uint16_t)(wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs)&255u);
 if(sample<chance[id])setbit(a,0x34u,4u);rt->m68k_regs.d[0]=d0;rt->m68k_regs.d[1]=d1;return WF_OK;
}
/* $17282, linked slam. $1110E/$2052 are the explicit silent sound device. */
static wf_status_t raw33(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_17266;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(!bit(a,O_LATCH,7u)){
  if(!t)return block(tr,0x1728eu);wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_STATE,0xffu);wf_set_u8(a,0x19u,64u);wf_set_u8(a,0x1bu,64u);
  wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xf6u));wf_set_be16(a,0xf6u,(uint16_t)(wf_be16(a,0xf6u)+1u));
  if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
  wf_r17_1129e_trace_t rr;int carry=0;wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
  if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x84u);return WF_OK;}
  interrupt_position(rt,a,t,256,0,16);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=7u;return WF_OK;
 }
 if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER))wf_set_u8(a,0x19u,0x4cu);
 if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
  /* $258E[22] =0100,0300,0048, with facing applied to VX. */
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,22u);wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0xff00u:0x100u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x48u);wf_set_u8(a,O_MOTION,2u);
 }
 if(low(a,O_FRAME)==3u&&!(rt->g.mode_0161&2u)){
  wf_set_be16(a,0x3eu,128u);wf_status_t st=wf_r17_resolver_280dc(rt,a);if(st!=WF_OK)return block(tr,0x280dcu);wf_set_be16(a,0x3eu,0u);
  uint8_t contact=(uint8_t)(wf_u8(a,0x37u)&3u);wf_r17_m68k_move_b(&rt->m68k_regs,0u,contact);
  if(contact){int tagged=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);if(!tagged){
   if(!t)return block(tr,0x17364u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(0x135u|wf_be16(a,O_FACING)));wf_set_be16(a,O_VISUAL,(uint16_t)(0x135u|wf_be16(a,O_FACING)));wf_set_be16(a,O_FRAME,3u);wf_set_be16(a,O_TIMER,0xff00u);
   wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2025u);setbit(t,0x32u,3u);wf_set_be16(t,0x44u,0u);wf_set_be16(t,0x68u,18u);chgbit(t,O_FACING,7u);interrupt_position(rt,a,t,128,1,64);
   if(bit(a,0x37u,1u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}else if(bit(a,0x37u,0u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
   pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
  }}
 }
 if(low(a,O_FRAME)==3u&&bit(a,0x37u,4u)){
  wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
  if(!(rt->g.mode_0161&1u)){if(!t)return block(tr,0x110eeu);wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);}
  wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);rt->g.global_1800=14u;
  if(!bit(a,O_SELECTOR,7u)){
   if(!t)return block(tr,0x173f2u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2005u);setbit(t,0x32u,3u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_be16(t,0x68u,18u);interrupt_position(rt,a,t,176,0,16);pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
  }
 }
 if(low(a,O_FRAME)==0xfeu){
  setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x22u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_r17_m68k_move_w(&rt->m68k_regs,0u,32u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,32,0,0);setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
 }return WF_OK;
}
/* $1A950: the reversal chosen by $172B4 after $1129E. */
static wf_status_t raw84(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_1A940;if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,80u);uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,timer);if(timer)return WF_OK;
 wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1a984u);rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,14u);wf_set_be16(a,0x68u,8u);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0xc006u);wf_set_be16(a,O_VISUAL,0x2bu);wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+72u));wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_set_be16(t,O_VISUAL,0x60u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
 /* $1A9AE stores A2 itself, preserving its tagged high byte. */
 wf_set_be32(a,0x92u,wf_be32(a,O_LINK));wf_set_be32(t,0x92u,wf_actor_rom_base((unsigned)(a-rt->actors)));setbit(a,0x33u,6u);setbit(t,0x33u,6u);wf_set_be16(a,0x18u,0u);wf_set_be16(t,0x18u,0u);wf_set_be16(a,0x1au,0u);wf_set_be16(t,0x1au,0u);return WF_OK;
}

wf_status_t wf_r17_dispatch_1c03e_r11(wf_runtime_t *rt,unsigned slot,wf_r17_actions_r11_bridge_t *b){
    wf_actor_raw_t *a;wf_r17_actions_r11_trace_t *tr;const desc_t *d=0;uint16_t sel;wf_status_t s;
    if(!rt||!b||slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
    uint32_t saved_d0=rt->m68k_regs.d[0]; /* $1C03E/$1C0C2 MOVEM */
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
        case 0x22u:tr->callback_rom=0x15b8cu;tr->descriptor_rom=0x15b6cu;s=raw22(rt,a,b,&d,tr);break;
        case 0x0eu:tr->callback_rom=0x13d46u;tr->descriptor_rom=0x13d26u;s=raw0e(rt,a,&d,tr);break;
        case 0x51u:tr->callback_rom=0x18c24u;tr->descriptor_rom=0x18bf4u;s=raw51(rt,a,&d,tr);break;
        case 0x33u:tr->callback_rom=0x17282u;tr->descriptor_rom=0x17266u;s=raw33(rt,a,&d,tr);break;
        case 0x84u:tr->callback_rom=0x1a950u;tr->descriptor_rom=0x1a940u;s=raw84(rt,a,&d,tr);break;
        case 0x26u:tr->callback_rom=0x16258u;tr->descriptor_rom=0x1623cu;s=raw26(rt,a,&d,tr);break;
        case 0x85u:tr->callback_rom=0x1a9e4u;tr->descriptor_rom=0x1a9d4u;s=raw85(rt,a,&d,tr);break;
        case 0x69u:tr->callback_rom=0x19a20u;tr->descriptor_rom=0x199fcu;s=raw69(rt,a,b,&d,tr);break;
        case 0x6au:tr->callback_rom=0x19b50u;tr->descriptor_rom=0x19b3cu;s=raw6a(a,&d);break;
        case 0x6bu:tr->callback_rom=0x19bc0u;tr->descriptor_rom=0x19bbau;s=raw6b(rt,a,&d,tr);break;
        case 0x68u:tr->callback_rom=0x19832u;tr->descriptor_rom=0x1981eu;s=raw68(rt,a,&d,tr);break;
        case 0x24u:tr->callback_rom=0x16056u;tr->descriptor_rom=0x1603au;s=raw24(rt,a,&d,tr);break;
        case 0x08u:tr->callback_rom=0x13180u;tr->descriptor_rom=0x1316cu;s=raw08(rt,a,&d,tr);break;
        case 0x4eu:tr->callback_rom=0x18aa0u;tr->descriptor_rom=0x18a90u;s=raw4e(a,&d);break;
        case 0x3du:tr->callback_rom=0x114b2u;tr->descriptor_rom=0x114acu;d=&D_114AC;s=wf_r17_action_idle_114b2(rt,a,tr);break;
        case 0x48u:tr->callback_rom=0x132f4u;tr->descriptor_rom=0x132d8u;s=raw48(rt,a,&d,tr);break;
        case 0x4au:tr->callback_rom=0x180c4u;tr->descriptor_rom=0x180acu;s=raw4a(rt,a,&d,tr);break;
        case 0x4bu:tr->callback_rom=0x1820cu;tr->descriptor_rom=0x181ecu;s=raw4b(rt,a,&d,tr);break;
        case 0x8bu:tr->callback_rom=0x1ad24u;tr->descriptor_rom=0x1ad10u;s=raw8b(rt,a,&d,tr);break;
        case 0x8cu:tr->callback_rom=0x1ad7cu;tr->descriptor_rom=0x1ad76u;s=raw8c(rt,a,&d,tr);break;
        case 0x00u:tr->callback_rom=0x00012888u;s=raw00(rt,a,&d,tr);
                   if(s==WF_OK)tr->descriptor_rom=d->rom;break;
        case 0x0du:tr->callback_rom=0x153bcu;tr->descriptor_rom=0x153a0u;s=raw0d(rt,a,b,&d,tr);break;
        case 0x06u:tr->callback_rom=0x00013034u;tr->descriptor_rom=0x00013020u;s=raw06(a,&d);break;
        case 0x15u:tr->callback_rom=0x00014cf4u;tr->descriptor_rom=0x00014ce0u;s=raw15(rt,a,&d,tr);break;
        case 0x16u:tr->callback_rom=0x00014da2u;tr->descriptor_rom=0x00014d8au;s=raw16(rt,a,&d,tr);break;
        case 0x17u:tr->callback_rom=0x00014e40u;tr->descriptor_rom=0x00014e2cu;s=raw17(rt,a,&d,tr);break;
        case 0x29u:tr->callback_rom=0x00016582u;tr->descriptor_rom=0x0001656au;s=raw29(rt,a,&d,tr);break;
        case 0x7bu:tr->callback_rom=0x0001a538u;tr->descriptor_rom=0x0001a532u;s=raw7b(rt,a,&d,tr);break;
        case 0x4fu:tr->callback_rom=0x00018b0eu;tr->descriptor_rom=0x00018afeu;s=raw4f(rt,a,&d);break;
        case 0x4du:tr->callback_rom=0x00018948u;tr->descriptor_rom=0x0001892cu;s=raw4d(rt,a,b,&d,tr);break;
        case 0x66u:tr->callback_rom=0x000196aeu;tr->descriptor_rom=0x0001969eu;s=raw66(rt,a,b,&d,tr);break;
        case 0x70u:tr->callback_rom=0x00019EA0u;tr->descriptor_rom=0x00019E80u;s=raw70(a,b,&d,tr);break;
        case 0x10u:tr->callback_rom=0x141ecu;tr->descriptor_rom=0x141d8u;s=raw10(rt,a,&d,tr);break;
        case 0x0au:tr->callback_rom=0x13744u;tr->descriptor_rom=0x13734u;s=raw0a(rt,a,&d,tr);break;
        case 0x77u:tr->callback_rom=0x1a154u;tr->descriptor_rom=0x1a148u;s=raw77(rt,a,&d,tr);break;
        case 0x37u:tr->callback_rom=0x17a58u;tr->descriptor_rom=0x17a40u;s=raw37(rt,a,&d,tr);break;
        case 0x61u:tr->callback_rom=0x194d0u;tr->descriptor_rom=0x194cau;s=raw61(rt,a,&d,tr);break;
        case 0x59u:tr->callback_rom=0x19274u;tr->descriptor_rom=0x19264u;s=raw59(rt,a,&d,tr);break;
        case 0x4cu:tr->callback_rom=0x1878au;tr->descriptor_rom=0x18772u;s=raw4c(rt,a,b,&d,tr);break;
        case 0x71u:tr->callback_rom=0x19f1eu;tr->descriptor_rom=0x19f0eu;s=raw71(rt,a,b,&d,tr);break;
        case 0x72u:tr->callback_rom=0x00019FDAu;tr->descriptor_rom=0x00019FC6u;s=raw72(a,&d);break;
        case 0x7Cu:tr->callback_rom=0x0001A5A0u;tr->descriptor_rom=0x0001A594u;s=raw7c(rt,a,&d,tr);break;
        default:return block(tr,0x00012614u);
    }
    if(s!=WF_OK)return s;
    tr->tick_descriptor_rom=d->rom;s=common_tick_1c0c8(a,d);if(s!=WF_OK)return s;
    rt->m68k_regs.d[0]=saved_d0;
    return WF_OK;
}

wf_status_t wf_r17_dispatch_1c03e_r11_hook(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user){
    wf_r17_actions_r11_bridge_t *b=(wf_r17_actions_r11_bridge_t*)user;unsigned slot;
    if(!rt||!actor||!b||actor<rt->actors||actor>=rt->actors+WF_ACTOR_SLOTS)return WF_BAD_STATE;
    slot=(unsigned)(actor-rt->actors);return wf_r17_dispatch_1c03e_r11(rt,slot,b);
}
