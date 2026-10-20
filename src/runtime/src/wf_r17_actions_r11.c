#include <stdint.h>
#include <string.h>
#include "wf_r17_actions_r11.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_r17_tag_6e34.h"

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
    uint16_t duration[5];
    uint16_t pose[5];
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
 * temporary sprite immediately. IDs 19/20 are ROM rows $10E4C/$10E52. */
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
    wf_set_be16(v,O_VISUAL,(uint16_t)((id==19u?0x64u:0x65u)|wf_be16(a,O_FACING)));
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
