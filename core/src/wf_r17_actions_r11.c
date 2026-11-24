#if defined(WF_PLATFORM_SATURN) && defined(__GNUC__)
/* Keep the large arcade action translation inside Saturn HWRAM. Compact
 * code also reduces instruction-cache pressure during action dispatch. */
#pragma GCC optimize ("Os")
#endif
#include <stdint.h>
#include <string.h>
#include "wf_r17_actions_r11.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_r17_tag_6e34.h"
#include "wf_r17_rng1129e.h"
#include "wf_cpu_runtime.h"
#include "wf_r17_candidate_2187e.h"

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
    uint16_t duration[9]; /* $1747E has nine animation records. */
    uint16_t pose[9];
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
/* $12614[$64] -> $19608: two-pose linked movement, second pose holds. */
static const desc_t D_19608={0x00019608u,1u,2u,{8u,0xff00u},{0x16u,0x13u}};
static const desc_t D_1892C={0x0001892cu,1u,5u,{12u,12u,0xff00u,12u,12u},{0x62u,0x63u,0u,0x66u,0x67u}};
static const desc_t D_1A532={0x0001a532u,0u,0u,{0u},{0u}};
static const desc_t D_14E2C={0x00014e2cu,1u,3u,
    {0x0008u,0x000au,0x0010u},{0x00feu,0x00ffu,0x0100u}};
/* $12614[$29] -> $1656A, callback $16582. */
static const desc_t D_1656A={0x0001656au,1u,4u,
    {0x0014u,0x0008u,0x000eu,0x000eu},
    {0x00f5u,0x00f6u,0x00f7u,0x01d2u}};
/* $12614[$31] -> $16FD8, callback $17000. */
static const desc_t D_16FD8={0x00016fd8u,1u,8u,
    {32u,4u,4u,4u,4u,0xff00u,8u,28u},
    {0xc2u,0xc3u,0xc4u,0xc5u,0xc6u,0xc7u,0xc8u,0xc9u}};
/* $12614[$32] -> $1716C, shared callback $17194. */
static const desc_t D_1716C={0x0001716cu,1u,3u,{24u,8u,20u},
    {0x159u,0x15au,0x4du}};
/* $12614[$42] uses the same callback as $32 with a shorter victim animation. */
static const desc_t D_17180={0x00017180u,1u,3u,{8u,12u,4u},
    {0x157u,0x158u,0x158u}};
/* $12614[$27] -> $16388, callback $1639C. */
static const desc_t D_16388={0x00016388u,1u,3u,{12u,12u,32u},
    {0xbdu,0xbeu,0u}};

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){return (wf_u8(a,o)&BIT8(b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|BIT8(b)));}
static void chgbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)^BIT8(b)));}
static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}
/* Original output-only helpers. World state and the live D registers stay
 * under their existing owners; $2052 supplies the cabinet sound strobe. */
static void sound_1110e(wf_runtime_t *rt,const wf_actor_raw_t *a){
    wf_runtime_emit_audio(rt,bit(a,WF_A_FLAGS33,2u)?0x2du:0x29u);
}
static void sound_1112e(wf_runtime_t *rt,const wf_actor_raw_t *a){
    if(!wf_be16(a,O_TIMER)&&(low(a,O_FRAME)==1u||low(a,O_FRAME)==4u))
        wf_runtime_emit_audio(rt,0x33u);
}
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
        /* $19EFC jumps directly to the caller, leaving the default A1
         * descriptor in place instead of testing actor byte $75. */
        *dp=&D_19E80;return WF_OK;
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
            /* $14D10..$14D18 sets the helper arguments in live D0-D2.
             * D0 is restored by the dispatcher, but D1/D2 survive. */
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0020u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
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
        if(status!=WF_OK)return block(tr,0x26aeu);
        return WF_OK;
    }
    if(wf_be16(a,O_FRAME)==0u&&wf_be16(a,O_TIMER)==0u)wf_set_u8(a,O_MOTION,2u);
    if(low(a,O_FRAME)==3u){
        wf_set_be16(a,WF_A_FIELD42,0xfff0u);if(!(wf_u8(a,WF_A_CONTACT+1u)&16u))return WF_OK;
        sound_1110e(rt,a); /* $15486 */
        wf_set_be16(a,O_TIMER,0u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,WF_A_FIELD42,0u);
        unsigned route=wf_be16(a,WF_A_TIMER44);
        if(route==0u){
            wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~8u));
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,WF_A_STATE4,0x1cu);wf_set_be16(t,WF_A_PENDING_DAMAGE,18u);
            rt->g.global_15d2=15u;rt->g.global_15d3=25u;wf_runtime_emit_audio(rt,0x2bu); /* $154DA */wf_runtime_emit_audio(rt,0x32u); /* $154E4 */
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
               (uint16_t)(x>=tx?x-tx:tx-x)<48u&&(uint16_t)(y>=ty?y-ty:ty-y)<48u){wf_runtime_emit_audio(rt,0x2bu); /* $155D0 */wf_runtime_emit_audio(rt,0x32u); /* $155DA */
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
        uint16_t contact=wf_be16(a,WF_A_CONTACT);
        wf_set_be16(a,WF_A_FIELD3E,0xffe8u);
        wf_status_t s=wf_r17_resolver_280dc(rt,a);
        if(s!=WF_OK)return block(tr,0x00010b62u);
        if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
        wf_set_be16(a,WF_A_CONTACT,contact);
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
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x0050u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xfffeu);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
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
/* $16582 linked CPU grapple. $2052 supplies the pose-one sound command. */
static wf_status_t raw29(wf_runtime_t *rt,wf_actor_raw_t *a,
                          const desc_t **dp,wf_r17_actions_r11_trace_t *tr){
    wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!linked)return block(tr,0x0001c094u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_set_u8(a,WF_A_TIMER44,(uint8_t)(wf_u8(a,WF_A_TIMER44)&~0x80u));
        setbit(linked,WF_A_TIMER44,7u);
        wf_set_be16(linked,O_STATE,0x00ffu);
    }else if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_runtime_emit_audio(rt,0x2au); /* $165B6 */
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
 * temporary sprite immediately. IDs 0/1/3/4/6/7 use $10DDA/$10DE0/$10DEC/
 * $10DF2/$10DFE/$10E04; IDs 13/14/18/19/20 use $10E28/$10E2E/$10E46/$10E4C/$10E52. */
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
    uint16_t pose;int16_t vertical=-1;
    switch(id){
        case 0u:pose=0x164u;vertical=-2;break;
        case 1u:pose=0x165u;vertical=-2;break;
        case 3u:pose=0x166u;vertical=-4;break;
        case 4u:pose=0x167u;vertical=-2;break;
        case 6u:pose=0x184u;vertical=-2;break;
        case 7u:pose=0x185u;vertical=-2;break;
        case 8u:pose=0x16eu;vertical=-2;break;
        case 9u:pose=0x16fu;vertical=-2;break;
        case 10u:pose=0x53u;vertical=-2;break;
        case 11u:pose=0x54u;vertical=-2;break;
        case 12u:pose=0x21au;vertical=-2;break;
        case 13u:pose=0x57u;break;case 14u:pose=0x58u;break;
        case 15u:pose=0xe9u;vertical=-2;break;
        case 16u:pose=0x1d4u;vertical=-2;break;
        case 17u:pose=0x1d5u;vertical=-2;break;
        case 18u:pose=0x6du;break;case 19u:pose=0x64u;break;
        case 20u:pose=0x65u;break;
        case 21u:pose=0x1e5u;break;case 22u:pose=0x1e6u;break;
        case 23u:pose=0x1e7u;break;default:return block(tr,0x10d64u);
    }
    wf_set_be16(v,O_VISUAL,(uint16_t)(pose|wf_be16(a,O_FACING)));
    int16_t offset=(int8_t)wf_u8(a,0x19u);
    if(bit(a,O_FACING,7u))offset=(int16_t)-offset;
    wf_set_be16(v,O_X,(uint16_t)(wf_be16(a,O_X)+offset));
    wf_set_be16(v,O_Y,(uint16_t)(wf_be16(a,O_Y)+(uint16_t)vertical));
    wf_set_be16(v,O_Z,(uint16_t)(wf_be16(a,O_Z)-(uint16_t)vertical));
    wf_r17_m68k_move_l(&rt->m68k_regs,0u,id*6u);
    wf_status_t st=wf_r17_world_to_screen_247c(v,b->render->scroll_a_x,b->render->scroll_a_y);
    if(st==WF_OK)st=wf_r17_queue_27b8(b->render,v);
    if(st!=WF_OK)return block(tr,0x27b8u);
    memcpy(rt->transient_1258[i],v->b,0x2au);
    return WF_OK;
}
wf_status_t wf_r17_action_effect_10d3a(wf_runtime_t *rt,wf_actor_raw_t *a,
                                       unsigned id,wf_r17_actions_r11_bridge_t *b,
                                       wf_r17_actions_r11_trace_t *tr){
    if(!rt||!a||!b)return WF_BAD_STATE;
    return entry_effect(rt,a,id,b,tr);
}
static const desc_t D_1835E={0x1835eu,1u,2u,{0xff00u,12u},{0x6cu,0x6cu}};
/* $1836E: rope rebound. The latched owner emits ROM effect row 18 before
 * advancing its signed movement countdown and the common animation tick. */
static wf_status_t raw3f(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1835E;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,1u);wf_set_be16(a,0x2au,18u);wf_set_be16(a,O_RENDER_LIST,2u);wf_set_u8(a,0x19u,24u);
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~BIT8(4)));
        if(wf_be16(a,O_X)<0x280u)rt->g.global_1150=3u;else rt->g.global_11b0=3u;
        return WF_OK;
    }
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,18u);
    wf_status_t status=entry_effect(rt,a,18u,b,tr);if(status!=WF_OK)return status;
    if(!low(a,O_FRAME)){
        uint16_t speed=(uint16_t)(wf_be16(a,0x2au)-1u);wf_set_be16(a,0x2au,speed);
        if(speed&0x8000u){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x2au,0u);wf_set_be16(a,O_TIMER,0u);}
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_RENDER_LIST,0u);setbit(a,O_FLAGS18,7u);}
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
        if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){wf_runtime_emit_audio(rt,0x30u); /* $1898E */}
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
static const desc_t D_13434={0x13434u,1u,2u,{16u,0xff00u},{0x1e9u,0x1eau}};
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
        if(!cpu)wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
        return WF_OK;
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
/* $12614[$49] -> $13434/$13444: Royal pin intervention. */
static wf_status_t raw49(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_13434;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x13444u);
    if(bit(a,O_SELECTOR,7u)){
        if(wf_be32(t,O_LINK))return WF_OK;
        pin_clear(a,0x35u,1u);
        goto release;
    }
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,2u);wf_set_u8(a,O_B1B,0x40u);
        wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
        wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x48u);
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,O_LINK));
        if(!p)return block(tr,0x1347cu);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
        wf_status_t s=wf_r17_trajectory_26ae(a,p,0u);if(s!=WF_OK)return block(tr,0x26aeu);
        wf_set_u8(a,O_FACING,(uint8_t)((wf_u8(a,0x58u)&0x80u)^0x80u));
        return WF_OK;
    }
    if(low(a,O_FRAME)!=1u||!bit(a,0x37u,4u))return WF_OK;
    setbit(a,O_FLAGS18,7u);wf_set_be16(a,0x42u,0u);
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x9cu));
    if(p!=a||low(t,O_STATE)!=5u||low(t,O_SELECTOR)!=0x4au||bit(t,O_SELECTOR,5u))goto release;
    p=actor_from_ptr(rt,wf_be32(t,O_LINK));
    if(!p||low(p,O_STATE)!=5u||low(p,O_SELECTOR)!=0x48u)goto release;
    wf_set_u8(a,O_MOTION,0u);setbit(a,O_SELECTOR,7u);setbit(a,0x35u,0u);
    setbit(t,O_SELECTOR,5u);setbit(a,0x33u,6u);
    wf_set_be16(t,0xaau,(uint16_t)(wf_be16(t,0xaau)+12u));
    wf_set_be16(t,0x68u,10u);
    for(unsigned off=6u;off<=14u;off+=4u)wf_set_be16(a,off,wf_be16(p,off));
    static const int8_t adjust[8]={16,48,8,48,-8,32,24,32};
    unsigned index=(low(t,0x64u)==8u?0u:4u)+(wf_u8(a,O_FACING)!=wf_u8(t,O_FACING)?2u:0u);
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(int16_t)adjust[index]);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)(int16_t)adjust[index+1u]);
    helper_10bd0(a,adjust[index],-1,adjust[index+1u]);
    return WF_OK;
release:
    setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);
    wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(a,0x68u,3u);
    p=actor_from_ptr(rt,wf_be32(t,O_LINK));
    /* $135B0/$135B4 also run after the pinned actor cleared its link.
     * Null (including a high-byte tag on the 24-bit bus) addresses ROM
     * $00009C: the original CLR.L has no writable target there. */
    if(p)wf_set_be32(p,0x9cu,0u);
    else if(wf_be32(t,O_LINK)&0x00ffffffu)return block(tr,0x135b0u);
    setbit(a,O_LINK,7u);
    return WF_OK;
}
static wf_status_t raw4a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_180AC;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);uint16_t hp=wf_be16(a,0x66u),delay=(uint16_t)(42u-hp);
        if((int16_t)delay<0)delay=1u;
        wf_set_be16(a,0xaau,hp?delay:0x100u);
    }else{
        if(!(rt->g.mode_0161&1u)&&!bit(a,0x56u,7u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x11448u);
            uint8_t flags=wf_u8(p,0x34u)&0xc0u;rt->m68k_regs.d[0]=(rt->m68k_regs.d[0]&0xffffff00u)|flags;
            if(flags){pin_clear(p,0x34u,0xc0u);p=actor_from_ptr(rt,wf_be32(p,0x7au));if(!p)return block(tr,0x11462u);pin_clear(p,0x34u,0xc0u);}
        }
        if(!t){
            /* A cleared link addresses ROM on the 24-bit bus: $60.bit7=0
             * skips the mash helper and $21=$FF takes $18184's escape. */
            if(!(wf_be32(a,O_LINK)&0x00ffffffu))goto escape;
            return block(tr,0x180ecu);
        }
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
        if(!q)return block(tr,0x21344u);
        clear=bit(q,0x32u,0u);
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
    else if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){wf_runtime_emit_audio(rt,0x30u); /* $1AD42 */}
    else if(low(a,O_FRAME)==0xfeu)return pin_outcome(rt,a,0x40u,0x80u,tr);
    return WF_OK;
}
static wf_status_t raw8c(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1AD76;if(!bit(a,O_LATCH,7u)){setbit(a,O_LATCH,7u);wf_set_be16(a,O_VISUAL,0x1d3u|wf_be16(a,O_FACING));wf_set_be16(a,O_TIMER,64u);}
    uint16_t previous=wf_be16(a,O_TIMER);wf_set_be16(a,O_TIMER,(uint16_t)(previous-1u));if(previous)return WF_OK;
    if(wf_u8(a,0xffu)==3u)return pin_outcome(rt,a,0x80u,0x40u,tr);
    if(wf_u8(a,0xffu)!=5u)return WF_OK;
    if(!(rt->g.mode_0161&1u))return pin_outcome(rt,a,0x80u,0x80u,tr);
    for(unsigned i=0;i<9;i++)if(bit(&rt->actors[i],0u,7u)&&!bit(&rt->actors[i],0x56u,7u))wf_set_u8(&rt->actors[i],0xa0u,0x80u);
    return WF_OK;
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
/* $16056: linked throw with the original $2052 impact command. */
static wf_status_t raw24(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1603A;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16056u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_status_t status=action_offset_10b62(rt,a,0xffe8u,tr);if(status!=WF_OK)return status;
        wf_set_be16(t,O_STATE,0xffu);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=wf_u8(a,3u)==8u?0x27u:1u;return WF_OK;
    }
    if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x16u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_be16(t,0x68u,14u);wf_runtime_emit_audio(rt,0x32u); /* $160C6 */chgbit(t,O_FACING,7u);
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
/* $12614[$3E] -> $18314/$1831A. Each quantum runs $1174C then $11710;
 * only the first entry supplies the walking motion and horizontal angle. */
static wf_status_t raw3e(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,1u);
        wf_set_be16(a,0x2cu,bit(a,O_FACING,7u)?0xc0u:0x40u);
    }
    unsigned id=wf_be16(a,2u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)id);
    wf_set_be16(a,0x2au,0u);
    if(id>=12u)return block(tr,0x1176au);
    static const uint8_t speed_3e[12]={27u,29u,28u,25u,30u,27u,24u,30u,22u,27u,28u,27u};
    uint16_t speed=bit(a,0x32u,3u)?12u:speed_3e[id];
    if(bit(a,0x56u,7u)&&bit(a,0x33u,5u))speed=(uint8_t)(speed+6u);
    if(bit(a,0x74u,7u))speed=(uint16_t)(speed-8u);
    wf_set_be16(a,0x2au,speed);*dp=ring_walk_descriptor(a);
    if(bit(a,0xfeu,7u)){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}
    return WF_OK;
}
/* $12614[$3C] -> $18022/$18028. Walking selects the shared $11710
 * descriptor; the selector-latched wait retains the kind-zero $18022. */
static const desc_t D_18022={0x18022u,0u,0u,{0u},{0u}};
static wf_status_t raw3c(wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18022;
    if(!bit(a,O_SELECTOR,7u)){
        if(!bit(a,O_LATCH,7u)){
            wf_set_u8(a,O_MOTION,1u);
            unsigned id=wf_be16(a,2u);wf_set_be16(a,0x2au,0u);
            if(id>=12u)return block(tr,0x1176au);
            static const uint8_t magnitude[12]={27u,29u,28u,25u,30u,27u,24u,30u,22u,27u,28u,27u};
            uint16_t speed=bit(a,0x32u,3u)?12u:magnitude[id];
            if(bit(a,0x56u,7u)&&bit(a,0x33u,5u))speed=(uint8_t)(speed+6u);
            if(bit(a,0x74u,7u))speed=(uint16_t)(speed-8u);
            wf_set_be16(a,0x2au,speed);wf_set_be16(a,0x2cu,0u);
            if(wf_be16(a,O_Y)>(bit(a,0x33u,2u)?0x128u:0x150u))chgbit(a,0x2du,7u);
        }else if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
            wf_set_u8(a,O_MOTION,0u);setbit(a,O_SELECTOR,7u);
            wf_set_be16(a,O_TIMER,0xff00u);wf_set_be16(a,O_FRAME,0u);setbit(a,O_LINK,7u);
            if(bit(a,0xfeu,7u)){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}return WF_OK;
        }
        *dp=ring_walk_descriptor(a);return WF_OK;
    }
    if(bit(a,0xfeu,7u)){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}return WF_OK;
}
/* Exact $1F15C/$20C8, including its shared RAM scratch bytes. */
static int ring_exit_arrived(wf_runtime_t*rt,wf_actor_raw_t*a){
    uint16_t x=wf_be16(a,6u),y=wf_be16(a,10u),tx=wf_be16(a,0xbeu),ty=wf_be16(a,0xc0u);
    if((unsigned)(x>=tx?x-tx:tx-x)<8u&&(unsigned)(y>=ty?y-ty:ty-y)<8u)return 1;
    static const uint8_t base[4]={0,192,64,128};unsigned quadrant=0;uint16_t dx=(uint16_t)(tx-x),dy=(uint16_t)(ty-y);
    if(dx&0x8000u){quadrant|=1u;dx=(uint16_t)(0u-dx);}if(dy&0x8000u){quadrant|=2u;dy=(uint16_t)(0u-dy);}
    if(dx>255u)dx=255u;
    if(dy>255u)dy=255u;
    unsigned sum=(unsigned)dx+dy,component=(quadrant==0u||quadrant==3u)?dx:dy;uint8_t angle=(uint8_t)(base[quadrant]+component*64u/(sum?sum:1u));uint8_t*s=rt->g.direction_scratch_15e4;
    s[0]=angle;s[1]=(uint8_t)(wf_actor_rom_base((unsigned)(a-rt->actors))>>16);s[2]=(uint8_t)(dx>>8);s[3]=(uint8_t)dx;s[4]=(uint8_t)(dy>>8);s[5]=(uint8_t)dy;s[6]=(uint8_t)(ty>>8);s[7]=(uint8_t)ty;wf_set_u8(a,0x2du,angle);return 0;
}
static wf_status_t raw68(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    unsigned phase=wf_be16(a,0x44u)&3u;*dp=&D_1981E;
    if(phase==0u){
        if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_u8(a,0x19u,0xd0u);return WF_OK;}
        if(!wf_be16(a,O_FRAME)){
            if(!bit(a,0x37u,4u))return WF_OK;
            sound_1112e(rt,a); /* $19888: frameWORD0, so the helper emits nothing. */
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
    if(low(a,O_FRAME)==3u&&bit(a,0x37u,4u)){sound_1110e(rt,a); /* $162DE */rt->g.global_1800=14u;wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);return WF_OK;}
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,5u);
        if(!(rt->g.mode_0161&1u)){wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);}
        wf_set_be16(t,0x68u,22u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $16320 */setbit(t,0x64u,5u);setbit(t,0x32u,3u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));
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
    if(y<0x170u)return 0;
    return bit(t,O_FACING,7u)?x<0x26au:x>=0x1f8u&&x<0x248u;
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
        setbit(a,O_LINK,7u);pin_clear(a,0x33u,0x40u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,41u);wf_status_t st=tag_cleanup_213a6(rt,a,t,tr);if(st!=WF_OK)return st;wf_runtime_emit_audio(rt,0x29u); /* $192D0 */
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));}return WF_OK;
}

static const desc_t D_13734={0x13734u,1u,2u,{20u,16u},{0xe4u,0xe5u}};
static const desc_t D_1A148={0x1a148u,1u,1u,{24u},{0x1dfu}};
static wf_status_t action22_cleanup_21358(wf_runtime_t*rt,wf_actor_raw_t*a,
                                         wf_r17_actions_r11_trace_t*tr);
static wf_status_t interrupt_cleanup_212a0(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
 if(!rt->g.credited_route)return WF_OK;
 if(rt->g.mode_0161&1u)return action22_cleanup_21358(rt,a,tr);
 wf_status_t st=escape_tag_212d4(rt,a,tr);if(st!=WF_OK)return st;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x212bcu);return escape_tag_212d4(rt,t,tr);
}
wf_status_t wf_r17_action_cleanup_212a0(wf_runtime_t*rt,wf_actor_raw_t*a,uint32_t*blocked_rom){
 wf_r17_actions_r11_trace_t trace={0};
 if(!rt||!a)return WF_BAD_STATE;
 wf_status_t status=interrupt_cleanup_212a0(rt,a,&trace);
 if(blocked_rom)*blocked_rom=trace.blocked_dependency_rom;
 return status;
}
/* At each $24900 call in $13744, A1 is still descriptor $13734.
 * Its ROM +$86 LONG is $00E433FC, distinct from every actor address. */
static void interrupt_audio_24900(wf_runtime_t*rt){if(!(rt->g.mode_0161&1u)){rt->g.global_15d2=15u;rt->g.global_15d3=25u;}}
static void interrupt_position(wf_runtime_t*rt,wf_actor_raw_t*owner,wf_actor_raw_t*target,int16_t x,int16_t y,int16_t z){
 if(bit(owner,O_FACING,7u))x=(int16_t)-x;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)x);wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)y);wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)z);
 wf_set_be16(target,O_X,(uint16_t)(wf_be16(owner,O_X)+(uint16_t)x));wf_set_be16(target,O_Y,(uint16_t)(wf_be16(owner,O_Y)+(uint16_t)y));wf_set_be16(target,O_Z,(uint16_t)(wf_be16(owner,O_Z)+(uint16_t)z));
}
static wf_status_t raw0a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_13734;if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);return WF_OK;}wf_set_be16(a,O_PROFILE,1u);
 if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
  wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1376au);if(wf_be16(t,0xfeu))return WF_OK;
  wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,O_LINK));unsigned state=low(t,O_STATE),sub=low(t,0x64u);uint16_t sel=(uint16_t)(wf_be16(t,O_SELECTOR)&0x80ffu);
  if(state==5u)wf_r17_m68k_move_w(&rt->m68k_regs,1u,sel);
  if(state==4u&&(sub==8u||sub==9u)){wf_set_be16(t,O_STATE,4u);wf_set_u8(t,0x65u,(uint8_t)(sub-2u));wf_set_be16(t,0x68u,2u);wf_runtime_emit_audio(rt,0x32u); /* $137AC */setbit(a,O_LINK,7u);wf_set_be16(t,0xe4u,(uint16_t)(wf_be16(t,0xe4u)+1u));rt->g.global_15d2=15u;rt->g.global_15d3=11u;goto audio0a;}
  if(state==5u&&(sel==0x8048u||sel==0x800eu)){
   if(sel==0x8048u){wf_actor_raw_t*q=actor_from_ptr(rt,wf_be32(t,0x9cu));if(q&&low(q,O_STATE)==5u&&low(q,O_SELECTOR)==0x49u){t=q;p=actor_from_ptr(rt,wf_be32(t,O_LINK));wf_set_be32(a,O_LINK,wf_actor_rom_base((unsigned)(t-rt->actors)));goto release49;}wf_set_be16(t,0x64u,15u);chgbit(t,O_FACING,7u);}
   else wf_set_be16(t,0x64u,0x10fu);
   if(!p)return block(tr,0x13826u);
   /* $10B9E positions the actor linked from A0 (p), not the interrupted A2 (t). */
   wf_actor_raw_t*positioned=actor_from_ptr(rt,wf_be32(p,O_LINK));if(!positioned)return block(tr,0x10b9eu);
   interrupt_position(rt,p,positioned,sel==0x8048u?80:20,-1,32);interrupt_audio_24900(rt);wf_set_be16(t,O_STATE,4u);pin_clear(t,0x35u,1u);pin_clear(t,0x33u,0x40u);pin_clear(p,0x33u,0x40u);wf_status_t st=escape_cleanup_21282(rt,p,tr);if(st!=WF_OK)return st;wf_set_be16(p,O_STATE,7u);setbit(a,O_LINK,7u);wf_set_be32(t,0x9cu,0u);setbit(t,O_LINK,7u);goto audio0a;
  }
  if(state==5u&&sel==0x801au){
   if(!p)return block(tr,0x138c4u);
   wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);pin_clear(t,0x35u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffa0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(p,-96,-1,0);wf_status_t st=escape_cleanup_21282(rt,p,tr);if(st!=WF_OK)return st;setbit(t,0x18u,7u);wf_set_be16(p,O_STATE,7u);interrupt_audio_24900(rt);pin_clear(t,0x33u,0x40u);pin_clear(p,0x33u,0x40u);setbit(a,O_LINK,7u);setbit(t,O_LINK,7u);goto audio0a;
  }
  if(state==5u&&sel==0x8009u){
   if(!p)return block(tr,0x1394eu);
   wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);pin_clear(t,0x35u,2u);chgbit(t,O_FACING,7u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(t,-48,-1,0);wf_set_be16(p,O_STATE,7u);chgbit(p,O_FACING,7u);pin_clear(p,0x35u,4u);wf_status_t st=interrupt_cleanup_212a0(rt,p,tr);if(st!=WF_OK)return st;interrupt_audio_24900(rt);pin_clear(t,0x33u,0x40u);pin_clear(p,0x33u,0x40u);setbit(a,O_LINK,7u);setbit(t,O_LINK,7u);goto audio0a;
  }
  if(state==5u&&sel==0x8061u){interrupt_audio_24900(rt);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);goto audio0a;}
  if(state==5u&&sel==0x8049u){
release49:
   if(!p)return block(tr,0x139e0u);
   wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,15u);pin_clear(t,0x35u,1u);pin_clear(p,O_SELECTOR,0x20u);pin_clear(t,0x33u,0x40u);setbit(a,O_LINK,7u);wf_set_be32(p,0x9cu,0u);goto audio0a;
  }
  return WF_OK;
 }
 if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}return WF_OK;
audio0a:
 wf_runtime_emit_audio(rt,0x2au); /* $13A00 */
 return WF_OK;
}
static wf_status_t raw77(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_1A148;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1a154u);
 if(!bit(a,O_LATCH,7u))wf_set_be16(t,O_STATE,0xffu);
 else if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x8061u);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x8037u);}
 return WF_OK;
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
/* $21358: Royal credited cleanup, including the selector-$53 linked pair. */
static wf_status_t action22_cleanup_21358(wf_runtime_t*rt,wf_actor_raw_t*a,
                                         wf_r17_actions_r11_trace_t*tr){
    setbit(a,0xb6u,7u);
    if(wf_be32(a,0x7eu)){
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x7eu));
        if(!p)return block(tr,0x21368u);
        pin_clear(p,0xb5u,0x80u);setbit(p,0xb6u,7u);
        if(low(a,O_SELECTOR)==0x53u){
            wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
            if(!t)return block(tr,0x21380u);
            setbit(t,0xb6u,7u);
            uint32_t link=wf_be32(t,0x7eu);
            if(!link)return block(tr,0x2138eu);
            p=actor_from_ptr(rt,link);
            if(!p)return block(tr,0x21390u);
            pin_clear(p,0xb5u,0x80u);setbit(p,0xb6u,7u);
        }
    }
    return WF_OK;
}
wf_status_t wf_r17_action_cleanup_21358(wf_runtime_t *rt,wf_actor_raw_t *a,
                                       uint32_t *blocked_rom){
    wf_r17_actions_r11_trace_t trace={0};
    wf_status_t status=action22_cleanup_21358(rt,a,&trace);
    if(blocked_rom)*blocked_rom=trace.blocked_dependency_rom;
    return status;
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
        wf_set_be16(&rt->referee,O_STATE,0x8009u);rt->g.global_169e=0u;wf_runtime_emit_audio(rt,0x32u); /* $11226 */wf_runtime_emit_audio(rt,0x32u); /* $11230 */
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
wf_status_t wf_r17_action_partner_215b6(wf_runtime_t *rt,wf_actor_raw_t *a,
                                        uint32_t *blocked_rom){
    wf_r17_actions_r11_trace_t trace={0};
    wf_status_t status=action22_partner_215b6(rt,a,&trace);
    if(blocked_rom)*blocked_rom=trace.blocked_dependency_rom;
    return status;
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
    status=action22_partner_215b6(rt,a,tr);
    if(status==WF_OK){wf_runtime_emit_audio(rt,0x32u); /* $15CD8 */}
    return status;
}

/* $12614[$09] -> $135C8, callback $135EC. Reciprocal grounded hold;
 * shared $111C8 qualifies its result and $215B6 routes the partner. */
static const desc_t D_135C8={0x135c8u,1u,7u,
    {16u,16u,12u,22u,10u,16u,16u},{0x18bu,0xd8u,0xd9u,0xdau,0xdbu,0xdcu,0xddu}};
static wf_status_t raw09(wf_runtime_t*rt,wf_actor_raw_t*a,
                          wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_135C8;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x135f8u);
        wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(a,O_X,wf_be16(t,O_X));wf_set_be16(a,O_Y,wf_be16(t,O_Y));
        wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x1361au);
        if(low(t,O_STATE)==4u&&low(t,0x64u)==8u&&
           !wf_r17_helper_11412_carry(rt,a)){
            setbit(a,0x33u,6u);setbit(t,0x33u,6u);
            setbit(a,0x35u,1u);setbit(t,0x35u,2u);
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
            rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=9u;
            wf_set_be16(t,O_STATE,0xffu);
        }else{
            wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
            setbit(a,O_LINK,7u);wf_set_be16(a,O_TIMER,1u);
        }
        return WF_OK;
    }
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if(!t)return block(tr,0x136a2u);
    if(bit(a,0xfeu,7u))goto release09;
    wf_set_be16(a,O_FRAME,4u);wf_set_be16(a,O_TIMER,0u);wf_set_be16(t,0x68u,3u);
    int carry=0;wf_status_t status=action22_result_111c8(rt,a,t,b,&carry,tr);
    if(status!=WF_OK)return status;
    if(carry){
        if(rt->g.mode_0161&1u){
            rt->g.global_15d2=wf_u8(t,3u);rt->g.global_15d3=41u;
            setbit(t,0x32u,4u);setbit(a,O_LINK,7u);
            wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        }
release09:
        wf_set_be16(a,O_STATE,7u);wf_set_be16(t,O_STATE,4u);
        wf_set_be16(t,0x64u,wf_be16(t,0x64u)&255u);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        pin_clear(a,0x35u,2u);pin_clear(t,0x35u,4u);
        return WF_OK;
    }
    int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
    if(previous)return WF_OK;
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x5du);
    /* The original sound command follows the $215B6 transaction. */
    status=action22_partner_215b6(rt,a,tr);
    if(status==WF_OK){wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $1372C */}
    return status;
}

static const desc_t D_141D8={0x141d8u,1u,3u,{20u,0xff00u,8u},{0xefu,0xf0u,0xf0u}};
/* $11192 uses the same $E958 selector-A box as $11152, against raw61. */
static int jump_hit_11192(const wf_actor_raw_t*a,const wf_actor_raw_t*t){
 if(low(t,O_STATE)!=5u||low(t,O_SELECTOR)!=0x61u)return 0;
 uint16_t x=wf_be16(t,O_X),y=wf_be16(t,O_Y),left=(uint16_t)(x-(bit(t,O_FACING,7u)?104u:8u)),right=(uint16_t)(x+(bit(t,O_FACING,7u)?8u:104u));
 return left<wf_be16(a,O_X)&&right>=wf_be16(a,O_X)&&(uint16_t)(y-72u)<wf_be16(a,O_Y)&&(uint16_t)(y+24u)>=wf_be16(a,O_Y);
}
/* $12614[$0B] -> $13A1E/$13A2E. A2 is consumed only by initial
 * positioning or a landing contact, never by the no-contact FE exit. */
static const desc_t D_13A1E={0x13a1eu,1u,2u,{0xff00u,16u},{0xf1u,0xf2u}};
static wf_status_t raw0b(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_13A1E;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x2cu);
    if(!bit(a,O_LATCH,7u)){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!t)return block(tr,0x13a38u);
        if(low(t,0x64u)!=9u){
            wf_set_be16(a,O_X,wf_be16(t,O_X));wf_set_be16(a,O_Y,wf_be16(t,O_Y));
            wf_set_be16(a,O_Y,(uint16_t)(wf_be16(a,O_Y)-1u));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,120u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,120,-1,0);
        }else{
            wf_set_be16(a,O_FACING,0u);
            uint16_t x=(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7u)?-56:56));
            if(x>=wf_be16(a,O_X))setbit(a,O_FACING,7u);
            wf_set_be16(a,O_X,x);wf_r17_m68k_move_w(&rt->m68k_regs,0u,56u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xfff8u);
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,56,-8,0);
        }
        /* $258E[35] preserves D0/D1: VX=-512,VZ=$400,VZDEC=$58. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,35u);
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x200u:0xfe00u);
        wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x400u);
        wf_set_be16(a,0x5eu,0x58u);wf_set_u8(a,O_MOTION,2u);return WF_OK;
    }
    if(bit(a,0x37u,4u)){
        wf_set_be16(a,O_TIMER,0u);wf_set_u8(a,O_MOTION,0u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!t)return block(tr,0x13abau);
        if(low(t,O_STATE)==4u&&(low(t,0x64u)==8u||low(t,0x64u)==9u)){
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,wf_be16(t,0x64u)&255u);
            wf_set_u8(t,0x65u,(uint8_t)(wf_u8(t,0x65u)-8u));
            wf_set_u8(t,0x65u,(uint8_t)(wf_u8(t,0x65u)+6u));
            wf_set_be16(t,0xd4u,(uint16_t)(wf_be16(t,0xd4u)+1u));wf_set_be16(t,0x68u,11u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x29u); /* $13AF8 */wf_runtime_emit_audio(rt,0x32u); /* $13B02 */return WF_OK;
        }
        if(jump_hit_11192(a,t)){
            wf_set_be16(a,O_TIMER,0u);wf_set_be16(t,O_STATE,5u);
            wf_set_be16(t,O_SELECTOR,0x77u);wf_set_be16(t,0x68u,11u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,41u);wf_runtime_emit_audio(rt,0x32u); /* $13B30 */wf_runtime_emit_audio(rt,0x29u); /* $13B3A */
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));return WF_OK; /* $14422 RTS */
        }
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_set_be16(a,0x68u,3u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $13B6C */
        rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);}
    return WF_OK;
}

/* $12614[$0C] -> $13B98/$13BB0, callback $13BC8. The two descriptors
 * differ only in the last pose, selected by the post-trajectory facing. */
static const desc_t D_13B98={0x13b98u,1u,4u,{8u,16u,24u,0xff00u},{0x84u,0x85u,0x1c1u,0x1c2u}};
static const desc_t D_13BB0={0x13bb0u,1u,4u,{8u,16u,24u,0xff00u},{0x84u,0x85u,0x1c1u,0x1c3u}};
static wf_status_t raw0c(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    *dp=&D_13B98;
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x26aeu);
        wf_set_u8(a,O_MOTION,0u);rt->g.global_1670=0u;
        pin_clear(a,WF_A_FLAGS32,2u);
        wf_set_be16(a,WF_A_VZ,0x700u);wf_set_be16(a,WF_A_VZ_DEC,0x48u);
        uint16_t route=low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x61u?5u:2u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
        if(wf_r17_trajectory_26ae(a,t,route)!=WF_OK)return block(tr,0x26aeu);
        wf_set_be16(a,O_FACING,(uint16_t)((wf_be16(a,WF_A_VX)&0x8000u)^0x8000u));
        goto choose_pose;
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_MOTION,2u);setbit(a,WF_A_FLAGS33,3u);
        return WF_OK;
    }
    if(bit(a,O_SELECTOR,7u))goto after_contact;
    if(!bit(a,WF_A_CONTACT+1u,4u))goto choose_pose;
    rt->g.global_1800=14u;wf_set_be16(a,0x4au,0u);setbit(a,O_SELECTOR,7u);
    if(!t)return block(tr,0x11152u);
    if(wf_r17_helper_11152_carry(a,t)){
        wf_set_be16(t,O_STATE,4u);
        wf_set_be16(t,0x64u,(uint16_t)((wf_be16(t,0x64u)&0xffu)-2u));
    }else if(jump_hit_11192(a,t)){
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);
    }else{
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_set_be16(a,0x68u,3u);wf_runtime_emit_audio(rt,0x32u); /* $13D00 */rt->g.global_15d2=15u;rt->g.global_15d3=26u;
        setbit(a,O_LINK,7u);return WF_OK;
    }
    wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,16u);
    wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
    wf_runtime_emit_audio(rt,0x2bu); /* $13CAC */wf_runtime_emit_audio(rt,0x32u); /* $13CB6 */wf_set_be16(t,0x68u,15u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
after_contact:
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);return WF_OK;}
choose_pose:
    if(bit(a,O_FACING,7u))*dp=&D_13BB0;
    return WF_OK;
}

static wf_status_t raw10(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_141D8;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(!bit(a,O_SELECTOR,7u)){
  if(!bit(a,O_LATCH,7u)){
   if(!t)return block(tr,0x14200u);
   wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x48u);unsigned route;
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
   wf_set_be16(a,0x42u,0xffe0u);if(!bit(a,0x37u,4u))return WF_OK;wf_set_be16(a,0x42u,0u);sound_1110e(rt,a); /* $142E6 */wf_set_u8(a,O_MOTION,0u);if(!t)return block(tr,0x11156u);
   if(wf_r17_helper_11152_carry(a,t)){
    wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,(uint16_t)((uint16_t)(wf_be16(t,0x64u)&255u)-2u));wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0x68u,10u);wf_set_be16(t,0xd2u,(uint16_t)(wf_be16(t,0xd2u)+1u));wf_runtime_emit_audio(rt,0x2bu); /* $14340 */wf_runtime_emit_audio(rt,0x32u); /* $1434A */return WF_OK;
   }
   if(jump_hit_11192(a,t)){
    wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);wf_set_be16(t,0x68u,10u);wf_runtime_emit_audio(rt,0x32u); /* $1438E */wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));return WF_OK;
   }
   wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,80u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,80,-1,0);chgbit(a,O_FACING,7u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_set_be16(a,0x68u,3u);wf_runtime_emit_audio(rt,0x32u); /* $1440E */rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);return WF_OK;
  }
 }
 if(low(a,O_FRAME)==0xfeu){setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_r17_m68k_move_w(&rt->m68k_regs,0u,24u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,24,-1,0);setbit(a,O_LINK,7u);}return WF_OK;
}

/* $12614[$0F] -> $13F78, callback $13F90. Landing either acquires
 * the grounded target, interrupts raw61, or keeps the original miss pose. */
static const desc_t D_13F78={0x13f78u,1u,4u,{12u,12u,20u,0xff00u},{0x84u,0x85u,0x1a6u,0x1a7u}};
static wf_status_t raw0f(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_13F78;
    if(bit(a,O_SELECTOR,7u)){*dp=&D_125C0;return WF_OK;}
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x26aeu);
        wf_set_u8(a,O_MOTION,0u);rt->g.global_1670=0u;pin_clear(a,0x32u,2u);
        wf_set_be16(a,0x5cu,0x700u);wf_set_be16(a,0x5eu,0x48u);
        wf_status_t st=wf_r17_trajectory_26ae(a,t,1u);
        return st==WF_OK?WF_OK:block(tr,0x26aeu);
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_MOTION,2u);return WF_OK;
    }
    if(low(a,O_FRAME)<2u)return WF_OK;
    wf_set_be16(a,0x42u,0xffe8u);if(!bit(a,0x37u,4u))return WF_OK;
    sound_1110e(rt,a); /* $13FFC */
    wf_set_be16(a,0x42u,0u);wf_set_u8(a,O_MOTION,0u);
    rt->g.global_1800=14u;wf_set_be16(a,0x4au,0u);
    if(!t)return block(tr,0x11152u);
    if(wf_r17_helper_11152_carry(a,t)){
        if(wf_r17_helper_11412_carry(rt,a))goto miss0f;
        wf_set_be16(a,O_STATE,5u);setbit(a,O_SELECTOR,7u);
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x51u);
        setbit(a,0x33u,6u);setbit(t,0x33u,6u);
        wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));wf_set_be16(t,0x68u,15u);wf_runtime_emit_audio(rt,0x32u); /* $1405C */wf_runtime_emit_audio(rt,0x2bu); /* $14066 */
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=21u;
        if(rt->g.credited_route&&!bit(a,0x33u,2u)&&bit(a,0x33u,0u)&&bit(t,0x33u,0u)){
            setbit(a,0x35u,0u);goto position0f;
        }
        *dp=&D_125C0;return WF_OK;
    }
    if(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x54u){
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,30u);chgbit(a,O_FACING,7u);return WF_OK;
    }
    if(!jump_hit_11192(a,t))goto miss0f;
    wf_set_be16(a,O_STATE,5u);
    wf_status_t st=tag_cleanup_213a6(rt,t,actor_from_ptr(rt,wf_be32(t,O_LINK)),tr);
    if(st!=WF_OK)return st;
    setbit(a,O_SELECTOR,7u);wf_set_be16(t,O_STATE,5u);
    wf_set_be16(t,O_SELECTOR,0x51u);wf_set_be16(t,0x68u,15u);wf_runtime_emit_audio(rt,0x32u); /* $1412C */
    setbit(a,0x33u,6u);setbit(t,0x33u,6u);setbit(a,0x35u,0u);
    wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));
    wf_actor_raw_t*previous=actor_from_ptr(rt,wf_be32(t,O_LINK));
    if(!previous)return block(tr,0x1414eu);
    wf_set_be16(previous,O_STATE,0u);wf_runtime_emit_audio(rt,0x2bu); /* $14158 */
    wf_set_be32(t,O_LINK,wf_actor_rom_base((unsigned)(a-rt->actors)));
position0f:
    for(unsigned off=6u;off<=14u;off+=4u)wf_set_be16(a,off,wf_be16(t,off));
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xfff8u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
    helper_10bd0(a,64,-8,0);*dp=&D_125C0;return WF_OK;
miss0f:
    wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);chgbit(a,O_FACING,7u);
    wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(a,0x68u,3u);wf_runtime_emit_audio(rt,0x32u); /* $141AE */rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
    helper_10bd0(a,-48,0,0);return WF_OK;
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
  sound_1112e(rt,a); /* $176B2 */
  *dp=&D_11C62_R11;return WF_OK;
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
  sound_1112e(rt,a); /* $17786 */
  *dp=&D_11C62_R11;return WF_OK;
 }
 if(!bit(a,O_LATCH,7u)){
  if(!t)return block(tr,0x26aeu);
  wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x5cu,0x600u);wf_set_be16(a,0x5eu,0x48u);wf_status_t st=wf_r17_trajectory_26ae(a,t,2u);if(st!=WF_OK)return block(tr,0x26aeu);wf_set_be16(a,O_FACING,(wf_be16(a,0x58u)&0x8000u)^0x8000u);return WF_OK;
 }
 if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,32u);
 if(!bit(a,O_SELECTOR,7u)){
  if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){wf_set_u8(a,O_MOTION,2u);return WF_OK;}
  if(low(a,O_FRAME)==3u){
   setbit(a,O_SELECTOR,4u);if(bit(a,0x37u,4u)){
    if(!t)return block(tr,0x11152u);
    setbit(a,0x18u,7u);wf_set_u8(a,O_MOTION,0u);
    if(!wf_r17_helper_11152_carry(a,t)||wf_r17_helper_11412_carry(rt,a)){
     wf_set_be16(a,O_STATE,4u);if(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x54u){wf_set_be16(a,0x64u,30u);return WF_OK;}
     wf_set_be16(a,0x64u,5u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));chgbit(a,O_FACING,7u);wf_set_be16(a,0x68u,3u);wf_runtime_emit_audio(rt,0x32u); /* $13F62 */setbit(a,O_LINK,7u);wf_set_be16(t,0x9au,0u);wf_set_be16(t,0xaau,0u);return WF_OK;
    }
    wf_set_be16(a,O_TIMER,0u);setbit(a,0x33u,6u);setbit(t,0x33u,6u);rt->g.global_1800=14u;wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x51u);wf_set_be16(t,0x68u,17u);pin_clear(a,O_SELECTOR,0x10u);wf_set_be16(t,0xdau,(uint16_t)(wf_be16(t,0xdau)+1u));wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_runtime_emit_audio(rt,0x32u); /* $13E5E */wf_runtime_emit_audio(rt,0x2bu); /* $13E68 */rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=32u;
    /* $258E[3] preserves all live registers and applies own facing to VX. */
    wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x38u);wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,O_X,(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7u)?-48:48)));wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
    if(rt->g.credited_route&&!bit(a,0x33u,2u)&&bit(a,0x33u,0u)&&bit(t,0x33u,0u)){setbit(a,0x35u,0u);setbit(a,O_SELECTOR,7u);/* $216E8 does nothing for selector0E outside TAG. */if(!(rt->g.mode_0161&1u))return pin_helpers(rt,a,0,tr);}
    return WF_OK;
   }
  }
 }
 if(low(a,O_FRAME)!=0xfeu&&bit(a,0x37u,4u)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);rt->g.global_1800=14u;if(!bit(a,O_SELECTOR,7u))wf_set_be16(a,O_STATE,7u);}return WF_OK;
}

/* $175C4 reuses $0E's first three phase entry points. Its final phase
 * performs a distinct aerial grapple and victim recovery. */
static const desc_t D_175A8={0x175a8u,1u,5u,
    {8u,20u,16u,0xff00u,16u},{0x84u,0x85u,0xe6u,0xe7u,0xe8u}};
static wf_status_t raw35(wf_runtime_t*rt,wf_actor_raw_t*a,
                         wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_175A8;
    if(rt->g.mode_0161&2u)wf_set_be16(a,0x44u,3u);
    unsigned phase=wf_be16(a,0x44u)&3u;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(phase<3u){
        int bonus=phase==0u&&!bit(a,O_LATCH,7u)&&wf_u8(a,0x61u)==0x23u;
        wf_status_t st=raw0e(rt,a,dp,tr);
        if(*dp==&D_13D26)*dp=&D_175A8;
        if(st==WF_OK&&bonus&&t)wf_set_be16(t,0xaau,(uint16_t)(wf_be16(t,0xaau)+6u));
        return st;
    }
    if(!bit(a,O_SELECTOR,7u)){
        if(!bit(a,O_LATCH,7u)){
            if(!t)return block(tr,0x177b8u);
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x5cu,0x600u);
            wf_set_be16(a,0x5eu,0x48u);
            uint8_t af=wf_u8(a,O_FACING),tf=wf_u8(t,O_FACING);
            wf_r17_m68k_move_b(&rt->m68k_regs,1u,af);
            wf_r17_m68k_move_b(&rt->m68k_regs,2u,(uint8_t)(tf^af));
            uint16_t route=(tf^af)?4u:3u;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
            if(wf_r17_trajectory_26ae(a,t,route)!=WF_OK)return block(tr,0x26aeu);
            return WF_OK;
        }
        if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
            wf_set_u8(a,O_MOTION,2u);return WF_OK;
        }
        if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,40u);
        if((int16_t)wf_be16(a,0x5cu)>=0){setbit(a,O_SELECTOR,4u);wf_set_be16(a,0x42u,32u);}
        if(bit(a,0x37u,4u)){
            if(!t)return block(tr,0x11152u);
            wf_set_be16(a,0x42u,0u);setbit(a,O_FLAGS18,7u);
            if(wf_r17_helper_11152_carry(a,t)&&!wf_r17_helper_11412_carry(rt,a)){
                wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
                rt->g.global_1800=14u;setbit(a,O_SELECTOR,7u);
                wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
                wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,26u);
                pin_clear(a,O_SELECTOR,0x10u);wf_set_be16(t,0x68u,18u);
                wf_set_be16(t,0xdcu,(uint16_t)(wf_be16(t,0xdcu)+1u));
                wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x2bu); /* $17878 */wf_runtime_emit_audio(rt,0x32u); /* $17882 */
                rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=18u;
                wf_set_be16(t,0x9au,0u);
                if(!wf_be16(a,0x9au)){
                    if((!bit(a,0x56u,7u)||(!bit(a,0x33u,5u)&&!bit(a,0x33u,2u)))&&low(a,0x70u)==2u)
                        wf_set_be16(a,0x9au,0xe0u);
                    else if((!bit(a,0x56u,7u)||(!bit(a,0x33u,5u)&&!bit(a,0x33u,2u)))&&low(a,0x70u)==1u)
                        wf_set_be16(a,0x9au,0x80u);
                    else wf_set_be16(a,0x9au,0x30u);
                }
                return WF_OK;
            }
            wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);
            wf_set_be16(a,0x68u,3u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $178E2 */
            rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);
            wf_set_be16(t,0x9au,0u);wf_set_be16(t,0xaau,0u);
            *dp=&D_125C0;wf_set_be16(a,O_VISUAL,0x1fu);
            wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));return WF_OK;
        }
    }
    if(low(a,O_FRAME)>=4u){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,15u);
        wf_status_t st=entry_effect(rt,a,15u,b,tr);if(st!=WF_OK)return st;
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);}
    return WF_OK;
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
 if(!t)return block(tr,0x18ceau);
 *dp=low(t,O_SELECTOR)==0x0eu?&D_18C14:low(a,0x64u)==8u?&D_18BF4:&D_18C04;return WF_OK;
}

static const desc_t D_17266={0x17266u,1u,5u,{20u,12u,8u,0xff00u,24u},{0x131u,0x132u,0x133u,0x134u,0x135u}};
static const desc_t D_1A940={0x1a940u,2u,2u,{24u,16u},{0x131u,0x132u}};
/* $1108C preserves D0/D1 around its live $21B4 call. */
static wf_status_t landing_1108c(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
 if(!rt->g.credited_route||bit(a,0x33u,1u))return WF_OK;
 static const uint16_t chance[12]={136u,128u,68u,64u,120u,120u,80u,128u,128u,133u,128u,64u};unsigned id=wf_be16(a,2u);
 if(id>=12u)return block(tr,0x110b6u);
 if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x21b4u);
 uint32_t d0=rt->m68k_regs.d[0],d1=rt->m68k_regs.d[1];uint16_t sample=(uint16_t)(wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs)&255u);
 if(sample<chance[id])setbit(a,0x34u,4u);
 rt->m68k_regs.d[0]=d0;rt->m68k_regs.d[1]=d1;return WF_OK;
}
/* $17282, linked slam with its original landing and impact commands. */
static wf_status_t raw33(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_17266;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
 if(!bit(a,O_LATCH,7u)){
  if(!t)return block(tr,0x1728eu);
  wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_STATE,0xffu);wf_set_u8(a,0x19u,64u);wf_set_u8(a,0x1bu,64u);
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
   if(!t)return block(tr,0x17364u);
   wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(0x135u|wf_be16(a,O_FACING)));wf_set_be16(a,O_VISUAL,(uint16_t)(0x135u|wf_be16(a,O_FACING)));wf_set_be16(a,O_FRAME,3u);wf_set_be16(a,O_TIMER,0xff00u);
   wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2025u);setbit(t,0x32u,3u);wf_set_be16(t,0x44u,0u);wf_set_be16(t,0x68u,18u);chgbit(t,O_FACING,7u);interrupt_position(rt,a,t,128,1,64);
   if(bit(a,0x37u,1u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}else if(bit(a,0x37u,0u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
   pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
  }}
 }
 if(low(a,O_FRAME)==3u&&bit(a,0x37u,4u)){sound_1110e(rt,a); /* $173C0 */
  wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
  if(!(rt->g.mode_0161&1u)){if(!t)return block(tr,0x110eeu);wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);}
  wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $173DC */rt->g.global_1800=14u;
  if(!bit(a,O_SELECTOR,7u)){
   if(!t)return block(tr,0x173f2u);
   wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2005u);setbit(t,0x32u,3u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_be16(t,0x68u,18u);interrupt_position(rt,a,t,176,0,16);pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
  }
 }
 if(low(a,O_FRAME)==0xfeu){
  setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x22u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_r17_m68k_move_w(&rt->m68k_regs,0u,32u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,32,0,0);setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
 }return WF_OK;
}
/* $12614[$83] -> $1A8A6/$1A8B6. Independent WORD44 countdown;
 * terminal stores retain the raw A2 alias and literal self-alias order. */
static const desc_t D_1A8A6={0x1a8a6u,2u,2u,{24u,16u},{0x1f7u,0x1f8u}};
static wf_status_t raw83(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1A8A6;
    if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,80u);
    uint16_t counter=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,counter);
    if(counter)return WF_OK;
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_FLAGS18,7u);
    wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,14u);wf_set_be16(a,0x68u,8u);
    uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!t)return block(tr,0x1a8eau);
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0xc006u);
    wf_set_be16(a,O_VISUAL,0x2bu);wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+72u));
    wf_set_be16(t,O_Y,(uint16_t)(wf_be16(t,O_Y)+1u));wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(t,O_VISUAL,0x60u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
    wf_set_be32(a,0x92u,link);wf_set_be32(t,0x92u,wf_actor_rom_base((unsigned)(a-rt->actors)));
    pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    wf_set_be16(a,0x18u,0u);wf_set_be16(t,0x18u,0u);wf_set_be16(a,0x1au,0u);wf_set_be16(t,0x1au,0u);
    return WF_OK;
}

/* $1A950: the reversal chosen by $172B4 after $1129E. */
static wf_status_t raw84(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
 *dp=&D_1A940;if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,80u);uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,timer);if(timer)return WF_OK;
 wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1a984u);rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,14u);wf_set_be16(a,0x68u,8u);wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0xc006u);wf_set_be16(a,O_VISUAL,0x2bu);wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+72u));wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_set_be16(t,O_VISUAL,0x60u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
 /* $1A9AE stores A2 itself, preserving its tagged high byte. */
 wf_set_be32(a,0x92u,wf_be32(a,O_LINK));wf_set_be32(t,0x92u,wf_actor_rom_base((unsigned)(a-rt->actors)));setbit(a,0x33u,6u);setbit(t,0x33u,6u);wf_set_be16(a,0x18u,0u);wf_set_be16(t,0x18u,0u);wf_set_be16(a,0x1au,0u);wf_set_be16(t,0x1au,0u);return WF_OK;
}

static const desc_t D_19530={0x19530u,0u,0u,{0u},{0u}};

/* $21732 as reached by selector $5F. Unlike pin_helpers(defender=1),
 * CPU partners use the literal $217F8 table {62,93,125}. Royal $5F
 * never reaches the $4A-only random decision at $21812. */
static wf_status_t raw5f_prepare_21732(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u){
        wf_actor_raw_t*c=pin_candidate(rt,a);
        if(c){wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_actor_rom_base((unsigned)(c-rt->actors)));}
        return WF_OK;
    }
    if(!bit(a,0x33u,0u))return WF_OK;
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x21756u);
    setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);
    if(!bit(p,0x32u,0u)){
        rt->g.global_1682=0xfau;
        if(!bit(p,0x33u,1u)&&!bit(p,0x74u,7u)){
            setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
        }
        return WF_OK;
    }
    /* $2179C initializes one tick. $5E takes the short recovery branch;
     * $5F reads the character tier table at $217F8. */
    wf_set_be16(p,0xe6u,1u);
    if(low(a,O_SELECTOR)!=0x5eu){
        unsigned tier=wf_be16(p,0x70u);if(tier>=3u)return block(tr,0x217c8u);
        static const uint16_t timer[3]={62u,93u,125u};wf_set_be16(p,0xe6u,timer[tier]);
    }
    setbit(p,0x34u,2u);
    if(bit(p,0x33u,1u))wf_set_be32(p,0x7au,wf_be32(a,O_LINK));
    else{setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));}
    return WF_OK;
}
/* $19618: Royal linked movement. The initial $258E preset23 gives vertical
 * velocity $0200 and decay $0040; $10C60[0] chooses the recovery timer. */
static wf_status_t raw64(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19608;
    if(!bit(a,O_LATCH,7u)){
        wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x0200u);wf_set_be16(a,0x5eu,0x0040u);
        wf_set_u8(a,O_MOTION,2u);
        wf_set_be16(a,0xaau,0u);
        uint16_t score=wf_be16(a,0x66u);
        int16_t remaining=(int16_t)(0x2au-score);
        wf_set_be16(a,0xaau,score?(remaining<0?1u:(uint16_t)remaining):0x0100u);
    }
    if(bit(a,0x37u,4u)){
        setbit(a,O_SELECTOR,7u);wf_set_u8(a,O_MOTION,0u);
        wf_status_t status=raw5f_prepare_21732(rt,a,tr);
        if(status!=WF_OK)return status;
    }
    if(bit(a,O_SELECTOR,7u)&&wf_be16(a,0x66u)&&!(wf_u8(a,0x56u)&0xc0u)){
        rt->g.global_167a|=0x80u;
        if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
            uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);
            wf_set_be16(a,0xaau,timer?timer:0x4000u);
        }
    }
    return WF_OK;
}
/* $19536: persistent invisible hold victim; its kind-zero descriptor
 * intentionally performs no animation tick or transition of its own.
 * $5E uses $10C60[0], while $5D/$5F use $10C60[4]. */
static wf_status_t raw5f(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19530;int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched){
        wf_set_be16(a,0x52u,0u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_TIMER,0xff00u);
        wf_status_t st=raw5f_prepare_21732(rt,a,tr);if(st!=WF_OK)return st;
        if(low(a,O_SELECTOR)==0x5eu){
            /* $1957A clears AA before helper $10C60[0]. */
            wf_set_be16(a,0xaau,0u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
            uint16_t health=wf_be16(a,0x66u);
            int16_t remaining=(int16_t)(0x2au-health);
            wf_set_be16(a,0xaau,health?(remaining<0?1u:(uint16_t)remaining):0x100u);
            return WF_OK;
        }
        unsigned tier=wf_be16(a,0x70u);if(tier>=3u)return block(tr,0x10ca8u);
        static const uint16_t recovery[3]={9u,12u,15u};wf_set_be16(a,0xaau,recovery[tier]);return WF_OK;
    }
    /* $11432: caller D0 is restored by $1C03E, all D1-D7 survive. */
    if(!(rt->g.mode_0161&1u)&&!bit(a,0x56u,7u)){
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x11448u);
        if(wf_u8(p,0x34u)&0xc0u){pin_clear(p,0x34u,0xc0u);p=actor_from_ptr(rt,wf_be32(p,0x7au));if(!p)return block(tr,0x11462u);pin_clear(p,0x34u,0xc0u);}
    }
    /* $195F2/$10D04: a zero-health $5E skips the mash timer. */
    if((low(a,O_SELECTOR)!=0x5eu||wf_be16(a,0x66u))&&!(wf_u8(a,0x56u)&0xc0u)){
        rt->g.global_167a|=0x80u;
        if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
            uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,timer?timer:0x4000u);
        }
    }
    return WF_OK;
}

/* $12614[$52] -> $18D12, callback $18D2E. +$44 parity chooses the
 * normal held victim ($18D4C) or short damage reaction ($18E0C). */
static const desc_t D_18D12={0x18d12u,1u,2u,{22u,22u},{0x168u,0x169u}};
static const desc_t D_18D22={0x18d22u,1u,1u,{8u},{0x21bu}};
/* $2103E: unlike $21732, a CPU partner always receives +$7A and also
 * returns its address to the owner at +$7E when partner bit1 is clear. */
static wf_status_t raw52_prepare_2103e(wf_runtime_t*rt,wf_actor_raw_t*a,
                                        wf_actor_raw_t*t,wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u){
        wf_actor_raw_t*c=pin_candidate(rt,a);
        if(c){
            if(!wf_be32(a,O_LINK))wf_set_be16(a,0u,1u);
            wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_actor_rom_base((unsigned)(c-rt->actors)));
        }
        return WF_OK;
    }
    if(bit(t,0x32u,0u)||!bit(a,0x33u,0u))return WF_OK;
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x2106eu);
    setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);
    if(!bit(p,0x32u,0u)){
        rt->g.global_1682=0xfau;
        if(!bit(p,0x33u,1u)&&!bit(p,0x74u,7u)){
            setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
        }
        return WF_OK;
    }
    unsigned tier=wf_be16(p,0x70u);if(tier>=3u)return block(tr,0x210c2u);
    static const uint16_t delay[3]={62u,93u,125u};wf_set_be16(p,0xe6u,delay[tier]);
    if(!bit(p,0x33u,1u)){
        setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
    }
    setbit(p,0x34u,2u);wf_set_be32(p,0x7au,wf_be32(a,O_LINK));return WF_OK;
}
static wf_status_t raw52(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18D12;
    if(wf_be16(a,0x44u)&1u){
        *dp=&D_18D22;
        if(!bit(a,O_LATCH,7u)){
            wf_set_be16(a,0x52u,(uint16_t)(wf_be16(a,0x52u)+1u));wf_set_be16(a,0x54u,64u);
        }else if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,0u);}
        return WF_OK;
    }
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        /* $10C60(D0=3) preserves all registers and leaves a positive
         * recovery timer alone, including the $4000 terminal sentinel. */
        uint16_t recovery=wf_be16(a,0xaau);
        if(!recovery||(recovery&0x8000u)){
            unsigned tier=wf_be16(a,0x70u);if(tier>=3u)return block(tr,0x10ca8u);
            static const uint8_t delay[3]={7u,9u,11u};wf_set_be16(a,0xaau,delay[tier]);
        }
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x18d62u);
        wf_set_be16(a,O_X,wf_be16(t,O_X));wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
        unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x18d7eu);
        static const int8_t offset[12]={-40,-40,-40,-44,-40,-40,-44,-32,-40,-40,-40,-40};
        int16_t x=offset[id];if(bit(a,O_FACING,7u))x=(int16_t)-x;
        wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(uint16_t)x));
        int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
        if(!previous){
            wf_status_t status=raw52_prepare_2103e(rt,a,t,tr);if(status!=WF_OK)return status;
            setbit(a,0x35u,2u);wf_set_be16(a,0x52u,0u);
        }
        return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,9u);
    /* $10D04 preserves D0/A1/A2; role6/7 suppress human mash handling. */
    if(!(wf_u8(a,0x56u)&0xc0u)){
        rt->g.global_167a|=0x80u;
        if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
            uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,timer?timer:0x4000u);
        }
    }
    if(low(a,O_FRAME)==0xfeu){
        pin_clear(a,O_LATCH,0x80u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,1u);
    }
    uint8_t phase=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))phase=(uint8_t)(phase+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,phase);
    /* $18DF6 is NOP: the final D0=2/5 has no sprite or queue side effect. */
    return WF_OK;
}

/* $12614[$53] -> $18E3C. The two short reactions retain their own
 * descriptors while returning +$44 to the normal held-victim phase. */
static const desc_t D_18E3C={0x18e3cu,1u,2u,{22u,22u},{0x170u,0x171u}};
static const desc_t D_18E4C={0x18e4cu,1u,1u,{12u},{0x4fu}};
static const desc_t D_18E58={0x18e58u,1u,1u,{12u},{0x51u}};
/* Shared exact Royal tail2125C/21018. The zero LONG diagnostic is a
 * write to this actor, not a dereference of its absent linked actor. */
static wf_status_t royal_partner_candidate(wf_runtime_t*rt,wf_actor_raw_t*a,
                                            wf_r17_actions_r11_trace_t*tr){
    uint32_t candidate,dependency=0;int found;
    wf_status_t st=wf_r17_candidate_2187e(rt,(unsigned)(a-rt->actors),&candidate,&found,&dependency);
    if(st!=WF_OK)return block(tr,dependency);
    if(found){
        wf_actor_raw_t*c=actor_from_ptr(rt,candidate);uint32_t link=wf_be32(a,O_LINK);
        if(!link)wf_set_be16(a,0u,1u);
        wf_set_be32(c,0x7eu,link);wf_set_be32(a,0x7eu,candidate);
    }
    return WF_OK;
}
static wf_status_t raw53_partner_21114(wf_runtime_t*rt,wf_actor_raw_t*a,
                                      wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u)return royal_partner_candidate(rt,a,tr);
    if(!bit(a,0x33u,0u))return WF_OK;
    uint32_t partner=wf_be32(a,0x86u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x21144u);
    wf_actor_raw_t*op=actor_from_ptr(rt,wf_be32(t,0x86u));if(!op)return block(tr,0x21148u);
    /* MOVEA +86 only loads A1. The near CPU return never consumes A1. */
    if(bit(op,0x32u,0u)){
        uint16_t x=wf_be16(a,O_X),other=wf_be16(op,O_X);
        uint16_t distance=x>=other?(uint16_t)(x-other):(uint16_t)(other-x);
        if(distance<80u)return WF_OK;
        wf_actor_raw_t*p=actor_from_ptr(rt,partner);if(!p)return block(tr,0x2113cu);
        unsigned tier=wf_be16(p,0x70u);if(tier>=3u)return block(tr,0x211acu);
        static const uint16_t delay[3]={125u,187u,187u};wf_set_be16(p,0xe6u,delay[tier]);
        setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);setbit(p,0x34u,2u);
        if(bit(p,0x33u,1u))wf_set_be32(p,0x7au,wf_be32(a,O_LINK));
        else{setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));}
    }else{
        wf_actor_raw_t*p=actor_from_ptr(rt,partner);if(!p)return block(tr,0x2113cu);
        setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);rt->g.global_1682=250u;
        if(!bit(p,0x33u,1u)&&!bit(p,0x74u,7u)){
            setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,partner);
        }
    }
    return WF_OK;
}
static wf_status_t raw53(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    unsigned phase=wf_be16(a,0x44u)&3u;*dp=&D_18E3C;
    if(phase==3u)return block(tr,0x18e7au);
    if(phase){
        *dp=phase==1u?&D_18E4C:&D_18E58;
        if(!bit(a,O_LATCH,7u)){
            wf_set_be16(a,0x52u,(uint16_t)(wf_be16(a,0x52u)+1u));wf_set_be16(a,0x54u,64u);
        }else if(low(a,O_FRAME)==0xfeu)wf_set_be16(a,0x44u,0u);
        return WF_OK;
    }
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        uint16_t recovery=wf_be16(a,0xaau);
        if(!recovery||(recovery&0x8000u)){
            unsigned tier=wf_be16(a,0x70u);if(tier>=3u)return block(tr,0x10ca8u);
            static const uint8_t delay[3]={23u,27u,31u};wf_set_be16(a,0xaau,delay[tier]);
        }
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x18e9cu);
        wf_set_be16(a,O_X,wf_be16(t,O_X));wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
        wf_set_be16(a,O_FACING,wf_be16(t,O_FACING));
        unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x18ebeu);
        static const uint8_t profile[12]={0u,0u,1u,3u,0u,0u,3u,2u,4u,1u,1u,1u};
        static const int8_t offset[6]={-40,-36,-40,-44,-40,-1};
        int16_t x=offset[profile[id]];if(bit(a,O_FACING,7u))x=(int16_t)-x;
        wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+(uint16_t)x));return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,12u);
    if(!(wf_u8(a,0x56u)&0xc0u)){
        rt->g.global_167a|=0x80u;
        if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
            uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,timer?timer:0x4000u);
        }
    }
    if(low(a,O_FRAME)==0xfeu){
        pin_clear(a,O_LATCH,0x80u);int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
        if(!previous)return raw53_partner_21114(rt,a,tr);
    }
    return WF_OK;
}

static const desc_t D_12AE6={0x12ae6u,1u,4u,{8u,8u,8u,8u},{0x1aau,0x1abu,0x8100u,0u}};
static const desc_t D_12AFE={0x12afeu,1u,4u,{8u,8u,8u,8u},{0x9du,0x9eu,0x8100u,0u}};
/* $12B16: counter the reciprocal $11 relationship, then release its
 * dispatcher-linked actor. $10B9A uses the owner's facing and coordinates. */
static wf_status_t raw03(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be32(a,0x92u,0u);
        *dp=&D_12AE6;return WF_OK; /* $12B26 skips alternate selection. */
    }else{
        if(!bit(a,O_SELECTOR,7u)){
            wf_set_be16(a,O_PROFILE,3u);
            if(wf_be16(a,O_FRAME)<2u){
                wf_set_be16(a,O_PROFILE,0x12u);
                uint32_t related=wf_be32(a,0x92u);wf_actor_raw_t*t=actor_from_ptr(rt,related);
                if(related&&!t&&(related&0x00ffffffu))return block(tr,0x12b52u);
                /* Tagged physical zero reads ROM $65=0, so cannot match $11. */
                if(t&&low(t,0x64u)==0x11u&&wf_be32(t,0x92u)==wf_actor_rom_base((unsigned)(a-rt->actors))){
                    setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_LATCH,5u);wf_set_be32(a,O_LINK,related);goto descriptor;
                }
            }
        }
        if(bit(a,O_SELECTOR,7u)&&low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
            uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
            if(!t&&(link&0x00ffffffu))return block(tr,0x12b8cu);
            if(t){
                wf_set_be16(t,O_STATE,4u);wf_set_u8(t,0x65u,0x19u);wf_set_be16(t,O_VISUAL,0xb3u);wf_set_u8(t,O_VISUAL,wf_u8(a,O_FACING));
                interrupt_position(rt,a,t,-16,-1,40);
            }else{
                /* Literal null and tagged physical zero address ROM only.
                 * $10B9A's D1/D2 writes still survive the dispatcher. */
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,40u);
            }
        }
        if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}
    }
descriptor:
    *dp=bit(a,O_SELECTOR,7u)?&D_12AFE:&D_12AE6;return WF_OK;
}

static const desc_t D_14EF0={0x14ef0u,1u,4u,{12u,8u,8u,12u},{0x101u,0x102u,0x103u,0x104u}};
/* $14F08: linked throw; all animation ticks retain descriptor $14EF0. */
static wf_status_t raw18(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_14EF0;uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!bit(a,O_LATCH,7u)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x14f14u);
        wf_set_u8(a,O_MOTION,0u);if(t)wf_set_be16(t,O_STATE,0xffu);
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=2u;wf_set_u8(a,0x19u,16u);
    }else if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x14f42u);
        if(t){
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x22u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0x68u,12u);
        }
        wf_runtime_emit_audio(rt,0x32u); /* $14F5E */
        if(t){wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(t,O_FACING,7u);}
        pin_clear(a,0x33u,0x40u);if(t)pin_clear(t,0x33u,0x40u);
        if(t)interrupt_position(rt,a,t,80,1,80);
        else{wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,80u);}
    }else if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);chgbit(a,O_FACING,7u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

static const desc_t D_19C28={0x19c28u,1u,6u,{12u,12u,12u,12u,0xff00u,16u},{0x27u,0x28u,0x29u,0x2au,0x2bu,0x2au}};
/* $19C48: climb back through the ropes, then release the RINGSIDE owner. */
static wf_status_t raw6c(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
 *dp=&D_19C28;
 if(!bit(a,O_LATCH,7u)){
  wf_set_u8(a,O_MOTION,0u);setbit(a,0x33u,2u);wf_set_be16(a,O_RENDER_LIST,0u);wf_set_be16(a,O_Y,0x152u);wf_set_u8(a,O_B1B,5u);return WF_OK;
 }
 if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){wf_set_be16(a,O_Y,0x158u);return WF_OK;}
 if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){
  wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,O_Y,0x140u);setbit(a,O_FLAGS18,7u);
  /* $258E[15] at $2624 preserves D0-D1 and initializes the full WORDs. */
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,15u);wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x100u);wf_set_be16(a,0x5eu,0x38u);
 }
 if(low(a,O_FRAME)==4u&&bit(a,0x37u,4u)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);wf_set_be16(a,O_Z,0x100u);return WF_OK;}
 if(low(a,O_FRAME)==0xfeu){setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);wf_set_be16(a,0xaeu,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));}
 return WF_OK;
}

static const desc_t D_17AFE={0x17afeu,1u,3u,{6u,6u,6u},{0x6au,0x6bu,0x6au}};
/* $17B12: movement direction is selected by raw camera scroll $17EE. */
static wf_status_t raw38(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_17AFE;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,1u);wf_set_be16(a,0x2cu,0x80u);wf_set_be16(a,0x2au,24u);
        if(rt->g.global_17ee<0x240u)chgbit(a,0x2du,7u);
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);wf_set_be16(a,0x64u,0u);setbit(a,O_LINK,7u);wf_set_be32(a,0x9cu,0u);
    }
    return WF_OK;
}

static const desc_t D_16C5E={0x16c5eu,1u,6u,{6u,10u,8u,24u,16u,56u},{0x107u,0x108u,0x110u,0x20eu,0x20fu,0u}};
static const desc_t D_16C7E={0x16c7eu,1u,6u,{6u,5u,24u,16u,6u,18u},{0x107u,0x108u,0x110u,0x20eu,0x10au,0x10bu}};
static const desc_t D_1A7A4={0x1a7a4u,2u,2u,{28u,12u},{0x107u,0x108u}};
static const desc_t D_1A5F4={0x1a5f4u,2u,2u,{28u,12u},{0x14eu,0x14fu}};
static const desc_t D_1A794={0x1a794u,2u,2u,{28u,12u},{0x1b2u,0x1b3u}};
static const desc_t D_1A822={0x1a822u,2u,2u,{28u,12u},{0x1bau,0x1bbu}};
static const desc_t D_1AB32={0x1ab32u,0u,0u,{0u},{0u}};
static const desc_t D_19664={0x19664u,1u,1u,{8u},{0x219u}};
static const desc_t D_1A014={0x1a014u,1u,1u,{12u},{0x50u}};
static const desc_t D_1A050={0x1a050u,1u,1u,{12u},{0x52u}};
static const desc_t D_18FEE={0x18feeu,1u,3u,{20u,12u,12u},
    {0x17eu,0x17fu,0x180u}};
static const desc_t D_19076={0x19076u,1u,2u,{8u,8u},{0x174u,0x175u}};
static const desc_t D_192F2={0x192f2u,1u,2u,{16u,12u},{0x1a8u,0x1a9u}};
static const desc_t D_1939A={0x1939au,1u,2u,{16u,16u},{0x176u,0x177u}};
static const desc_t D_1ABA4={0x1aba4u,1u,5u,{24u,16u,16u,12u,12u},
    {0x10du,0x1c4u,0x1c5u,0x1c6u,0x1c7u}};
/* $10B62 preserves caller registers and the original contact WORD. */
static wf_status_t throw2f_offset(wf_runtime_t*rt,wf_actor_raw_t*a,uint16_t clearance,wf_r17_actions_r11_trace_t*tr){
    uint16_t contact=wf_be16(a,WF_A_CONTACT);wf_set_be16(a,WF_A_FIELD3E,clearance);
    wf_status_t status=wf_r17_resolver_280dc(rt,a);if(status!=WF_OK)return block(tr,0x10b62u);
    if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
    wf_set_be16(a,WF_A_CONTACT,contact);wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+wf_be16(a,WF_A_CORR_X)));wf_set_be16(a,WF_A_FIELD3E,0u);return WF_OK;
}
/* $16C9E, linked throw and its original character-specific animation. */
static wf_status_t raw2f(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_16C5E;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x16caau);
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_STATE,0xffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,48u);wf_status_t status=throw2f_offset(rt,a,48u,tr);if(status!=WF_OK)return status;
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xf0u));wf_set_be16(a,0xf0u,(uint16_t)(wf_be16(a,0xf0u)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;status=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);if(status!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x81u);return WF_OK;}
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=38u;wf_set_be16(a,0x44u,2u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,64u);status=throw2f_offset(rt,a,64u,tr);if(status!=WF_OK)return status;goto descriptor;
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,48u);
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){
        uint16_t count=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,count);
        if(count){wf_set_be16(a,O_FRAME,1u);wf_set_be16(a,O_TIMER,0u);goto descriptor;}
        if(!t)return block(tr,0x16d54u);
        if(wf_u8(a,3u)!=5u){
            pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
            if(!(rt->g.mode_0161&1u)){wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);}
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x26u);chgbit(t,O_FACING,7u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);wf_set_be16(t,0x68u,22u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));interrupt_position(rt,a,t,32,1,80);
        }else{
            setbit(a,O_FLAGS18,7u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x16u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_be16(t,0x68u,22u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $16DC2 */wf_status_t status=landing_1108c(rt,a,tr);if(status!=WF_OK)return status;chgbit(t,O_FACING,7u);interrupt_position(rt,a,t,-16,0,64);
        }
    }
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        setbit(a,O_FLAGS18,7u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,32u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,32,0,0);
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);return WF_OK;}
descriptor:
    if(wf_u8(a,3u)!=1u)*dp=&D_16C7E;
    return WF_OK;
}
/* $12614[$7F] -> $1A704/$1A718. The WORD44 loop counter advances
 * only at low-BYTE frameFE; reaching zero releases the linked pair. */
static const desc_t D_1A704={0x1a704u,1u,3u,{20u,28u,12u},
    {0x11bu,0x11cu,0x11du}};
static const desc_t D_1A674={0x1a674u,1u,3u,{16u,28u,12u},
    {0x10du,0x10eu,0x10fu}};
/* $1A688: repeat the linked hold once, then release with partner timer3.
 * Unlike $7F, this path preserves the partner's facing direction. */
static wf_status_t raw7e(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1A674;
    if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,2u);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    uint16_t count=(uint16_t)(wf_be16(a,0x44u)-1u);
    wf_set_be16(a,0x44u,count);
    if(count){wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);return WF_OK;}
    uint32_t link=wf_be32(a,O_LINK);
    wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!t&&(link&0x00ffffffu))return block(tr,0x1a6c8u);
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;
    wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0xc07bu);
    wf_set_be16(a,0x44u,1u);
    if(t){
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x15u);
        wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));
        wf_set_be16(t,O_VISUAL,0x39u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
        wf_set_be16(t,0x44u,3u);
    }
    wf_set_be16(a,O_VISUAL,0xffffu);
    wf_set_be16(a,O_FLAGS18,0u);if(t)wf_set_be16(t,O_FLAGS18,0u);
    wf_set_be16(a,0x1au,0u);if(t)wf_set_be16(t,0x1au,0u);
    return WF_OK;
}
static wf_status_t raw7f(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1A704;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x1fcu);
    if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,2u);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    uint16_t count=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,count);
    if(count){wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);return WF_OK;}
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;
    wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0xc07bu);wf_set_be16(a,0x44u,1u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1a758u);
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x15u);
    wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));wf_set_be16(t,O_VISUAL,0x39u);
    wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));wf_set_be16(a,O_VISUAL,0xffffu);
    wf_set_be16(t,0x44u,3u);wf_set_be16(a,O_FLAGS18,0u);wf_set_be16(t,O_FLAGS18,0u);
    wf_set_be16(a,0x1au,0u);wf_set_be16(t,0x1au,0u);return WF_OK;
}

/* $1A7B4: raw81 reversal returns the original linked pair to raw7B/raw15. */
static wf_status_t raw81(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1A7A4;if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,80u);
    uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,timer);if(timer)return WF_OK;
    uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);if(!t&&(link&0x00ffffffu))return block(tr,0x1a7e2u);
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0xc07bu);wf_set_be16(a,0x44u,1u);
    if(t){wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x15u);wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));chgbit(t,O_FACING,7u);wf_set_be16(t,0x44u,0u);wf_set_be16(t,O_VISUAL,0x39u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));}
    wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_FLAGS18,0u);if(t)wf_set_be16(t,O_FLAGS18,0u);wf_set_be16(a,0x1au,0u);if(t)wf_set_be16(t,0x1au,0u);return WF_OK;
}

/* $1A604/$1A7B4/$1A832 share an 80-tick linked reversal, with the
 * descriptor selecting the actual animation frames. $1A604 alone leaves
 * WORD44=3 on the partner; the other two clear it. */
static wf_status_t raw_reversal(wf_runtime_t*rt,wf_actor_raw_t*a,
                                const desc_t**dp,wf_r17_actions_r11_trace_t*tr,
                                const desc_t*d,uint16_t partner_timer,uint32_t guard_rom){
    *dp=d;
    if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,80u);
    uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);
    wf_set_be16(a,0x44u,timer);
    if(timer)return WF_OK;
    uint32_t link=wf_be32(a,O_LINK);
    wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!t&&(link&0x00ffffffu))return block(tr,guard_rom);
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;
    wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0xc07bu);
    wf_set_be16(a,0x44u,1u);
    if(t){
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x15u);
        wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));chgbit(t,O_FACING,7u);
        wf_set_be16(t,0x44u,partner_timer);wf_set_be16(t,O_VISUAL,0x39u);
        wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
    }
    wf_set_be16(a,O_VISUAL,0xffffu);
    wf_set_be16(a,O_FLAGS18,0u);if(t)wf_set_be16(t,O_FLAGS18,0u);
    wf_set_be16(a,0x1au,0u);if(t)wf_set_be16(t,0x1au,0u);
    return WF_OK;
}

/* $1AB38: walk to the selected rope edge before entering the exit state.
 * Its callback replaces the kind-zero descriptor with idle $115F2. */
static wf_status_t raw88(wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_115F2;
    int far_edge=bit(a,0x33u,7u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,1u,1u);wf_set_be16(a,0x2au,16u);
        wf_set_be16(a,0x2cu,far_edge?0xf8u:0x88u);
        return WF_OK;
    }
    uint16_t y=wf_be16(a,O_Y);
    if((!far_edge&&y<0x120u)||(far_edge&&y>=0x190u)){
        wf_set_be16(a,O_STATE,8u);wf_set_be16(a,0x4au,1u);
        wf_set_be16(a,0x44u,far_edge?5u:6u);
        if(!far_edge)*dp=&D_1AB32;
    }
    return WF_OK;
}

/* $19670/$1A020/$1A05C: the persistent impact frames use the compact
 * transient sprite pool. A full pool silently drops the effect as in ROM. */
static wf_status_t raw_effect_hold(wf_runtime_t*rt,wf_actor_raw_t*a,
                                   wf_r17_actions_r11_bridge_t*b,
                                   const desc_t**dp,wf_r17_actions_r11_trace_t*tr,
                                   unsigned selector){
    unsigned effect;
    if(selector==0x65u){*dp=&D_19664;effect=12u;}
    else if(selector==0x73u){*dp=&D_1A014;effect=10u;}
    else{*dp=&D_1A050;effect=11u;}
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        if(selector==0x65u)return WF_OK;
    }else if(selector!=0x65u&&low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0xc01du);
        return WF_OK;
    }
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)effect);
    wf_status_t st=entry_effect(rt,a,effect,b,tr);
    if(st!=WF_OK)return st;
    if(selector==0x65u&&low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0xc01cu);
    }
    return WF_OK;
}

/* $19002: linked hold end. On first entry the victim is suspended; at FE
 * both sides are released to the original grounded state. */
static wf_status_t raw55(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18FEE;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x1900au);
    if(!bit(a,O_LATCH,7u)){
        wf_set_be16(t,O_STATE,0xffu);
        pin_clear(a,0x35u,4u);pin_clear(t,0x35u,2u);
        return interrupt_cleanup_212a0(rt,a,tr);
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,42u);
        wf_runtime_emit_audio(rt,0x2au); /* $19036 */
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,2u);
        wf_set_be16(t,0x68u,3u);wf_set_be16(a,O_STATE,0u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $19086: linked shoulder throw; the actor moves 80 pixels after the
 * $21282 cleanup, then returns to its normal visible pose. */
static wf_status_t raw56(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19076;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x1908eu);
    if(!bit(a,O_LATCH,7u)){
        wf_set_be16(t,O_STATE,0xffu);pin_clear(t,0x35u,1u);
        wf_set_u8(a,0x19u,0x28u);wf_set_u8(a,O_B1B,0x60u);
        wf_set_be16(a,O_FACING,wf_be16(t,O_FACING));return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,41u);
        wf_runtime_emit_audio(rt,0x29u); /* $190C0 */
    }
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    setbit(a,O_FLAGS18,7u);setbit(t,O_FLAGS18,7u);
    wf_set_be16(a,O_STATE,7u);wf_set_be16(t,O_STATE,4u);
    wf_set_be16(t,0x64u,5u);wf_set_be16(t,0x68u,3u);
    wf_set_be16(a,O_VISUAL,0x22u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(t,O_VISUAL,0x16u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
    wf_status_t st=escape_cleanup_21282(rt,a,tr);if(st!=WF_OK)return st;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,80u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
    helper_10bd0(a,80,0,0);setbit(a,O_LINK,7u);
    wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    return WF_OK;
}

/* $19302/$193AA: linked throw finishers with distinct fall and release
 * animations. The cleanup runs before the victim is returned to state4. */
static wf_status_t raw5a_5b(wf_runtime_t*rt,wf_actor_raw_t*a,
                             const desc_t**dp,wf_r17_actions_r11_trace_t*tr,
                             unsigned selector){
    int is5a=selector==0x5au;
    *dp=is5a?&D_192F2:&D_1939A;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,is5a?0x1930au:0x193b2u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_be16(t,O_STATE,0xffu);
        if(is5a)wf_set_u8(a,0x19u,0xd0u);
        else{
            wf_set_u8(a,O_B1B,0x20u);
            wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));
        }
        pin_clear(a,0x35u,4u);pin_clear(t,0x35u,2u);
        return WF_OK;
    }
    if(low(a,O_FRAME)==(is5a?0u:1u)&&!wf_be16(a,O_TIMER)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,is5a?43u:41u);
        wf_runtime_emit_audio(rt,is5a?0x2bu:0x29u); /* $19334/$193E4 */
    }
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    setbit(a,O_FLAGS18,7u);setbit(t,O_FLAGS18,7u);
    wf_set_be16(a,O_STATE,is5a?7u:0u);
    wf_status_t st=interrupt_cleanup_212a0(rt,a,tr);if(st!=WF_OK)return st;
    wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,2u);wf_set_be16(t,0x68u,3u);
    wf_set_be16(a,O_VISUAL,is5a?0x68u:wf_be16(a,O_FACING));
    if(is5a)wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    if(is5a)interrupt_position(rt,a,t,32,0,0);
    setbit(a,O_LINK,7u);
    return WF_OK;
}

/* $1ABC0: suspended partner and one-time 48-pixel push at the end of
 * the first animation cycle. Selector bit7 prevents repeating the push. */
static wf_status_t raw89(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1ABA4;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x1abc8u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_STATE,0xffu);
        wf_set_be16(t,O_X,wf_be16(a,O_X));
        wf_set_be16(t,O_Z,wf_be16(a,O_Z));
        wf_set_u8(a,O_B1B,0x20u);setbit(a,0x34u,3u);
        wf_set_u8(a,O_FACING,(uint8_t)(wf_u8(a,0x33u)&0x80u));
        wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));
        chgbit(a,O_FACING,7u);return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,0x401au);
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,0x40u);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    wf_set_be16(a,O_FRAME,2u);wf_set_be16(a,O_TIMER,0u);
    int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
    if(previous)return WF_OK;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
    helper_10bd0(a,-48,0,0);wf_set_u8(a,0x19u,0x40u);
    wf_set_be16(t,O_X,wf_be16(a,O_X));
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u); /* $2052 */wf_runtime_emit_audio(rt,0x32u); /* $1AC5A */
    return WF_OK;
}

/* $12614[$1C] -> $15668, callback $1567C. The first frame qualifies the
 * linked actor; FE repeats the hold or hands the pair to $6F/$6E after the
 * shared $111C8 result owner, including its original $2052 commands. */
static const desc_t D_15668={0x15668u,1u,3u,{12u,22u,22u},{0x161u,0x162u,0x163u}};
static wf_status_t raw1c(wf_runtime_t*rt,wf_actor_raw_t*a,
                          wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_15668;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        if(!bit(a,O_SELECTOR,7u))return WF_OK;
        if(!t)return block(tr,0x156a0u);
        setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);
        wf_set_u8(t,O_SELECTOR,0xc0u);goto capture;
    }
    if(wf_be16(a,O_FRAME)&&!bit(a,0x32u,0u))wf_set_be16(a,O_PROFILE,10u);
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x156d4u);
        uint16_t own=wf_be16(a,O_Y),other=wf_be16(t,O_Y);
        uint16_t distance=own>=other?(uint16_t)(own-other):(uint16_t)(other-own);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,distance);
        if(distance>=16u)goto miss;
        own=wf_be16(a,O_X);other=wf_be16(t,O_X);
        distance=own>=other?(uint16_t)(own-other):(uint16_t)(other-own);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,distance);
        if(distance>=80u)goto miss;
        wf_r17_m68k_move_b(&rt->m68k_regs,1u,wf_u8(a,O_FACING));
        uint8_t facing=(uint8_t)(wf_u8(a,O_FACING)^wf_u8(t,O_FACING));
        wf_r17_m68k_move_b(&rt->m68k_regs,2u,facing);
        if(facing||wf_r17_helper_11412_carry(rt,a))goto miss;
        if(!((low(t,O_STATE)==4u&&low(t,0x64u)==1u)||
             low(t,O_STATE)==1u||low(t,O_STATE)==0u))goto miss;
capture:
        wf_set_be16(t,O_STATE,5u);wf_set_u8(t,O_SELECTOR+1u,0x52u);
        wf_set_be16(t,0x44u,0u);wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));
        int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
        if(!previous){
            wf_set_be16(t,0x64u,wf_be16(t,0x64u)&255u);
            setbit(a,0x33u,6u);setbit(t,0x33u,6u);setbit(a,0x35u,1u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $1576C */
            wf_status_t status=action22_partner_215b6(rt,a,tr);if(status!=WF_OK)return status;
            rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=29u;
        }
    }else if(low(a,O_FRAME)==0xfeu){
        if(!t)return block(tr,0x157d2u);
        if(bit(a,0xfeu,7u)){
            wf_set_be16(a,O_STATE,0u);wf_set_be16(t,O_STATE,0u);return WF_OK;
        }
        wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);wf_set_be16(t,0x68u,4u);
        int carry=0;wf_status_t status=action22_result_111c8(rt,a,t,b,&carry,tr);
        if(status!=WF_OK)return status;
        if(carry){
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x6fu);
            wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x6eu);
            if(rt->g.credited_route&&(rt->g.mode_0161&1u))
                return action22_cleanup_21358(rt,a,tr);
            return interrupt_cleanup_212a0(rt,a,tr);
        }
    }
    uint8_t phase=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))phase=(uint8_t)(phase+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,phase);
    unsigned effect=phase==0u?0u:phase==1u?1u:4u;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)effect);
    wf_status_t status=entry_effect(rt,a,effect,b,tr);if(status!=WF_OK)return status;
    if(phase==1u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u);return entry_effect(rt,a,3u,b,tr);}
    return WF_OK;
miss:
    setbit(a,O_LINK,7u);
    if(bit(a,0x32u,0u)){
        wf_set_be16(a,O_STATE,1u);wf_set_be16(a,0xaeu,1u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
    }else wf_set_be16(a,O_STATE,0u);
    return WF_OK;
}

/* $20F04 TAG partner/reserve transaction and the original Royal CPU
 * immediate return or human2187E scan. */
static wf_status_t raw1d_partner_20f04(wf_runtime_t*rt,wf_actor_raw_t*a,
                                    wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u)return bit(a,WF_A_ROLE,7u)?WF_OK:royal_partner_candidate(rt,a,tr);
    if(!bit(a,0x33u,0u))return WF_OK;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,WF_A_TEAM_LINK));
    if(!t||!p)return block(tr,0x20f2cu);
    uint32_t partner=wf_be32(a,WF_A_TEAM_LINK),link=wf_be32(a,O_LINK);
    if(bit(p,0x32u,0u)){
        uint16_t x=wf_be16(t,O_X),px=wf_be16(p,O_X),distance=x>=px?(uint16_t)(x-px):(uint16_t)(px-x);
        if(distance<80u)return WF_OK;
        unsigned tier=wf_be16(p,0x70u);if(tier>=3u)return block(tr,0x20fbau);
        wf_set_be16(p,0xe6u,1u);setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);
        if(bit(p,0x33u,1u)||bit(a,0x33u,1u)){
            wf_set_be32(p,WF_A_TARGET,link);setbit(p,0x34u,2u);
            if(!bit(p,0x33u,1u)){pin_clear(a,0x33u,2u);setbit(p,0x33u,1u);setbit(p,0x35u,3u);}
        }else{setbit(p,0x34u,2u);setbit(p,WF_A_CPU_B5,7u);wf_set_be32(p,WF_A_RESERVE,link);}
        return WF_OK;
    }
    setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);rt->g.global_1682=250u;
    if(bit(p,0x33u,1u))return WF_OK;
    if(bit(a,0x33u,1u)){
        pin_clear(p,WF_A_ROLE,0x40u);pin_clear(a,0x33u,2u);setbit(p,0x33u,1u);setbit(p,0x35u,3u);return WF_OK;
    }
    if(!bit(p,0x74u,7u)){setbit(p,WF_A_CPU_B5,7u);wf_set_be32(p,WF_A_RESERVE,link);wf_set_be32(a,WF_A_RESERVE,partner);}
    return WF_OK;
}
static const desc_t D_1585C={0x1585cu,1u,3u,{12u,22u,22u},{0x161u,0x16cu,0x16du}};
static wf_status_t raw1d(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,
                        const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1585C;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        if(!t)return block(tr,0x15884u);
        if(!bit(a,O_SELECTOR,7u)){wf_set_be16(t,0x52u,0u);wf_set_be16(a,0x44u,0u);return WF_OK;}
        setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);wf_set_u8(t,O_SELECTOR,0x80u);goto capture1d;
    }
    if(wf_be16(a,O_FRAME))wf_set_be16(a,O_PROFILE,13u);
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x158c2u);
        /* Original11412 dereferences through the24-bit bus, then compares
         * the reciprocal LONG without masking it. The bounded target has
         * already been resolved here; keep that exact raw comparison. */
        if(wf_be32(t,O_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))||low(t,O_STATE)!=4u||low(t,0x64u)!=1u){
            setbit(a,O_LINK,7u);wf_set_be16(a,O_STATE,0u);return WF_OK;
        }
capture1d:
        setbit(a,O_SELECTOR,7u);setbit(a,0x33u,6u);setbit(t,0x33u,6u);
        wf_set_be16(t,O_STATE,5u);wf_set_u8(t,O_SELECTOR+1u,0x53u);wf_set_be16(t,0x44u,0u);
    }else{
        if(bit(a,0xfeu,7u)){
            if(!t)return block(tr,0x1591eu);
            wf_set_be16(a,O_STATE,0u);wf_set_be16(t,O_STATE,0u);return WF_OK;
        }
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);
            if(!(rt->g.mode_0161&1u)){
                unsigned previous=bit(a,0x45u,0u);setbit(a,0x45u,0u);if(previous)goto effects1d;
                wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,WF_A_TEAM_LINK));if(!p)return block(tr,0x1595au);
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(p,O_X));
                setbit(a,O_FACING,7u);if(wf_be16(p,O_X)<wf_be16(a,O_X))pin_clear(a,O_FACING,0x80u);
            }else{
                pin_clear(a,O_FACING,0x80u);if(wf_be16(a,O_X)<0x280u)setbit(a,O_FACING,7u);
            }
            if(!t)return block(tr,0x15980u);
            wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));wf_set_be16(t,O_X,wf_be16(a,O_X));wf_set_be16(t,O_Y,(uint16_t)(wf_be16(a,O_Y)-1u));
            static const uint8_t profiles[12]={0,0,1,3,0,0,3,2,4,1,1,1};
            static const int8_t offsets[6]={-40,-36,-40,-44,-40,-1};
            unsigned graphic=wf_be16(t,WF_A_CHARACTER);if(graphic>=12u)return block(tr,0x159a2u);
            int16_t x=offsets[profiles[graphic]];if(bit(t,O_FACING,7u))x=(int16_t)-x;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)x);wf_set_be16(t,O_X,(uint16_t)(wf_be16(t,O_X)+(uint16_t)x));
            wf_status_t st=raw1d_partner_20f04(rt,a,tr);if(st!=WF_OK)return st;
            goto final_effect1d;
        }
    }
effects1d:;
    uint8_t phase=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))phase=(uint8_t)(phase+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,phase);
    if(phase==1u){wf_r17_m68k_move_w(&rt->m68k_regs,0u,8u);return entry_effect(rt,a,8u,b,tr);}
final_effect1d:
    if((uint8_t)rt->m68k_regs.d[1]){wf_r17_m68k_move_w(&rt->m68k_regs,0u,9u);return entry_effect(rt,a,9u,b,tr);}
    return WF_OK;
}

/* $12614[$57] -> $1913E/$1914E: the linked throw places its owner one
 * pixel above the partner, starts the partner's $76 reaction and releases
 * both hold flags at the final pose. */
static const desc_t D_1913E={0x1913eu,1u,2u,{16u,16u},{0x186u,0x187u}};
static wf_status_t raw57(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1913E;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x19156u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x76u);
        if(rt->g.credited_route&&(rt->g.mode_0161&1u))
            return action22_cleanup_21358(rt,a,tr);
        return interrupt_cleanup_212a0(rt,a,tr);
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,42u);wf_runtime_emit_audio(rt,0x2au); /* $19186 */
        wf_set_be16(t,0x68u,3u);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);pin_clear(a,0x33u,0x40u);
        pin_clear(t,0x33u,0x40u);setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $12614[$76] -> $1A0FE/$1A10E: the linked wrestler's landing reaction.
 * The first tick stops walking; terminal FE enters state4/substate2. */
static const desc_t D_1A0FE={0x1a0feu,1u,2u,{16u,16u},{0x16cu,0x0fu}};
static wf_status_t raw76(wf_runtime_t*rt,wf_actor_raw_t*a,
                          wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1A0FE;
    if(!bit(a,O_LATCH,7u))wf_set_u8(a,O_MOTION,0u);
    else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,2u);
    }
    if(!wf_be16(a,O_FRAME)&&wf_be16(a,O_TIMER)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,8u);
        return entry_effect(rt,a,8u,b,tr);
    }
    return WF_OK;
}

/* $12614[$58] -> $191B4, callback $191C8: leave the linked hold, inflict
 * damage at phase1/timer0, then release after the final animation pose. */
static const desc_t D_191B4={0x191b4u,1u,3u,{24u,12u,12u},{0x181u,0x182u,0x183u}};
static wf_status_t raw58(wf_runtime_t*rt,wf_actor_raw_t*a,
                          wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_191B4;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x191d0u);
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x6du);
        pin_clear(a,0x35u,4u);pin_clear(t,0x35u,2u);
        wf_status_t status;
        if(rt->g.credited_route&&(rt->g.mode_0161&1u))status=action22_cleanup_21358(rt,a,tr);
        else status=interrupt_cleanup_212a0(rt,a,tr);
        if(status!=WF_OK)return status;
    }else{
        if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
            if(!t)return block(tr,0x19208u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,42u);wf_runtime_emit_audio(rt,0x2au); /* $19202 */wf_set_be16(t,0x68u,3u);
        }
        if(low(a,O_FRAME)==0xfeu){
            if(!t)return block(tr,0x19222u);
            wf_set_be16(a,O_STATE,0u);pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);setbit(a,O_LINK,7u);
            return WF_OK;
        }
    }
    uint8_t phase=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))phase=(uint8_t)(phase+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,phase);
    if(phase>1u)return WF_OK;
    unsigned effect=6u+phase;wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)effect);
    return entry_effect(rt,a,effect,b,tr);
}

/* $12614[$6D] -> $19CF2, callback $19D06. Both effective phases0/1
 * deliberately emit the same ROM effect row4; FE selects human/CPU ground. */
static const desc_t D_19CF2={0x19cf2u,1u,3u,{16u,16u,16u},{0x163u,0x163u,0x10u}};
static wf_status_t raw6d(wf_runtime_t*rt,wf_actor_raw_t*a,
                          wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19CF2;
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);return WF_OK;}
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,bit(a,0x32u,0u)?10u:2u);return WF_OK;
    }
    uint8_t phase=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))phase=(uint8_t)(phase+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,phase);
    if(phase>1u)return WF_OK;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,4u);return entry_effect(rt,a,4u,b,tr);
}

/* $12614[$67] -> $19776, callback $1978A: the linked hold reversal
 * preserves motion, releases its target at phase2/timer0, then exits to7. */
static const desc_t D_19776={0x19776u,1u,3u,{16u,16u,8u},{0x1d9u,0x1dau,0x1dbu}};
static wf_status_t raw67(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19776;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x19792u);
        wf_set_be16(t,O_STATE,0xffu);pin_clear(a,0x35u,4u);pin_clear(t,0x35u,2u);return WF_OK;
    }
    if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x197b4u);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);chgbit(t,O_FACING,7u);wf_set_be16(t,0x68u,2u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $197D0 */interrupt_position(rt,a,t,56,1,0);
        wf_status_t status;
        if(rt->g.credited_route&&(rt->g.mode_0161&1u))status=action22_cleanup_21358(rt,a,tr);
        else status=interrupt_cleanup_212a0(rt,a,tr);
        if(status!=WF_OK)return status;
        wf_set_be16(t,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }
    return WF_OK;
}

/* $10FC6 as consumed by raw75: its carry selects state6. Rope activity
 * guards read the LOW BYTE of each WORD, while writes set the full WORD. */
static int raw75_edge_10fc6(wf_runtime_t*rt,wf_actor_raw_t*a){
    uint8_t contact=wf_u8(a,0x36u),bits=wf_u8(a,0x37u);
    if(!(rt->g.mode_0161&2u)&&(contact==1u||contact==5u)){
        if(bits&2u){
            if(!(rt->g.global_1150&255u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}
            return 1;
        }
        if(bits&1u){
            if(!(rt->g.global_11b0&255u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
            return 1;
        }
    }else if(contact==3u){
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,0x17u);
        wf_runtime_emit_audio(rt,0x28u); /* $1103C */wf_runtime_emit_audio(rt,0x32u); /* $11046 */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);
    }
    return 0;
}

/* $12614[$8D] -> $17F9A, callback $17FB2. $10FC6's carry is deliberately
 * ignored here: the original proceeds to the two landing contact tests. */
static const desc_t D_17F9A={0x17f9au,1u,4u,
    {6u,0xff00u,0xff00u,0xff00u},{0xb2u,0xb3u,0x1fu,0x12u}};
static wf_status_t raw8d(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_17F9A;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x234u);
    if(!bit(a,O_LATCH,7u)){
        /* $258E[12] preserves D0/D1; only BYTE$2E.bit7 negates VX. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,12u);
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x100u:0xff00u);
        wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0u);
        wf_set_be16(a,0x5eu,0x28u);wf_set_u8(a,O_MOTION,2u);return WF_OK;
    }
    (void)raw75_edge_10fc6(rt,a);
    if(low(a,O_FRAME)==1u&&bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $17FDE */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,80u);
        if(!bit(a,0x32u,5u)&&(rt->g.global_007e&7u)==7u){
            /* Retain the unsupported resolver boundary before entering
             * $280DC, including $10B66/$10B6A's displacement and D0. */
            wf_set_be16(a,0x3eu,80u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,0x36u));
            return block(tr,0x280dcu);
        }
        wf_status_t st=action_offset_10b62(rt,a,80u,tr);if(st!=WF_OK)return st;
        wf_set_be16(a,O_TIMER,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u);
        /* $258E[3]: zero horizontal speeds, VZ=$300, VZDEC=$38. */
        wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x38u);
        wf_set_u8(a,O_MOTION,2u);
    }
    if(low(a,O_FRAME)==2u&&bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $1800C */
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
        setbit(a,O_SELECTOR,7u);
    }
    return WF_OK;
}

/* $12614[$40] -> $183EC, callback $18404. The original descriptor
 * remains selected even when the landing/rope callback changes state. */
static const desc_t D_183EC={0x183ecu,1u,4u,{4u,4u,0xff00u,8u},{0x84u,0x85u,0x1afu,0x84u}};
static wf_status_t raw40(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_183EC;
    if(!bit(a,O_LATCH,7u)){
        /* $258E[11] preserves D0/D1 and applies facing to VX. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,11u);
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x2c0u:0xfd40u);
        wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x600u);wf_set_be16(a,0x5eu,0x68u);
        wf_set_u8(a,O_MOTION,0u);setbit(a,0x33u,3u);return WF_OK;
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_MOTION,2u);setbit(a,0x33u,3u);return WF_OK;
    }
    if(low(a,O_FRAME)==2u){
        if(raw75_edge_10fc6(rt,a)){
            wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,0x17u);return WF_OK;
        }
        wf_set_be16(a,O_PROFILE,0x16u);
        if(!bit(a,0x37u,4u))return WF_OK;
        sound_1110e(rt,a); /* $18468 */
        pin_clear(a,0x33u,8u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
    }
    if(low(a,O_FRAME)==0xfeu)wf_set_be16(a,O_STATE,0u);
    return WF_OK;
}

/* $12614[$41] -> $1848C, callback $18498. Its descriptor remains selected
 * through rope/linked/terminal state changes. $92 retains its full raw alias. */
static const desc_t D_1848C={0x1848cu,1u,1u,{14u},{0x1a4u}};
static wf_status_t raw41(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1848C;
    if(!bit(a,O_LATCH,7u)){wf_set_be32(a,0x92u,0u);return WF_OK;}
    if(raw75_edge_10fc6(rt,a)){wf_set_be16(a,O_STATE,6u);return WF_OK;}
    if(!bit(a,O_SELECTOR,7u)){
        wf_set_be16(a,O_PROFILE,0x10u);uint32_t related=wf_be32(a,0x92u);
        if(related){
            wf_actor_raw_t*t=actor_from_ptr(rt,related);
            /* A tagged null reads ROM$21=$FF, so its state comparison fails. */
            if(!t&&(related&0x00ffffffu))return block(tr,0x184ceu);
            if(t&&low(t,O_STATE)==4u&&low(t,0x64u)==0x1du){
                setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_TIMER,12u);wf_set_u8(a,O_MOTION,0u);
                wf_set_be32(t,O_LINK,wf_actor_rom_base((unsigned)(a-rt->actors)));wf_set_be32(a,O_LINK,related);return WF_OK;
            }
        }
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}
    return WF_OK;
}

/* $12614[$8A] -> $1AC62/$1AC72. Result release consumes A2 only at
 * latched BYTE frame zero with a zero WORD timer; terminal FE is local. */
static const desc_t D_1AC62={0x1ac62u,1u,2u,{32u,48u},{0x127u,0u}};
static wf_status_t raw8a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1AC62;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x228u);
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_B1B,32u);return WF_OK;}
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_actor_raw_t*t=NULL;
        if(rt->g.mode_0161&1u){
            t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1aca0u);
            rt->g.global_15d2=wf_u8(t,3u);rt->g.global_15d3=41u;setbit(t,0x32u,4u);
            pin_clear(a,0x35u,2u);pin_clear(t,0x35u,4u);
        }
        setbit(a,O_FLAGS18,7u);
        if(!t){t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1acc8u);}
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,2u);
        wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));chgbit(t,O_FACING,7u);
        interrupt_position(rt,a,t,48,-1,48);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $12614[$60] aliases $19530/$19536. Its latched continuation is the
 * same $11432/$10D04 owner as raw5F; init selects $10C60 argument6. */
static wf_status_t raw60(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19530;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x180u);
    if(bit(a,O_LATCH,7u))return raw5f(rt,a,dp,tr);
    setbit(a,O_LATCH,7u);wf_set_be16(a,0x52u,0u);wf_set_u8(a,O_MOTION,0u);
    wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_TIMER,0xff00u);
    if(rt->g.credited_route){
        if(rt->g.mode_0161&1u){
            uint32_t candidate,dependency=0u;int found;
            wf_status_t st=wf_r17_candidate_2187e(rt,(unsigned)(a-rt->actors),&candidate,&found,&dependency);
            if(st!=WF_OK)return block(tr,dependency);
            if(found){
                wf_actor_raw_t*c=actor_from_ptr(rt,candidate);if(!c)return block(tr,0x21828u);
                wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,candidate);
            }
        }else if(bit(a,0x33u,0u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));if(!p)return block(tr,0x21756u);
            setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);
            if(!bit(p,0x32u,0u)){
                rt->g.global_1682=0xfau;
                if(!bit(p,0x33u,1u)&&!bit(p,0x74u,7u)){
                    setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
                }
            }else{
                wf_set_be16(p,0xe6u,1u);unsigned tier=wf_be16(p,0x70u);
                if(tier>=3u)return block(tr,0x217c8u);
                static const uint16_t delay[3]={62u,93u,125u};wf_set_be16(p,0xe6u,delay[tier]);
                setbit(p,0x34u,2u);
                if(bit(p,0x33u,1u))wf_set_be32(p,0x7au,wf_be32(a,O_LINK));
                else{setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));}
            }
        }
    }
    /* $195AA/$195AE precede the bounded $10CA8 table read. */
    wf_set_be16(a,0xaau,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,6u);
    unsigned tier=wf_be16(a,0x70u);if(tier>=3u)return block(tr,0x10ca8u);
    static const uint16_t recovery[3]={3u,11u,23u};wf_set_be16(a,0xaau,recovery[tier]);return WF_OK;
}

/* $12614[$25] -> $16116/$1612E. The final BYTE frame either releases
 * the linked actor or repeats frame one while $111C8 qualifies the result. */
static const desc_t D_16116={0x16116u,1u,4u,
    {12u,12u,16u,24u},{0xf8u,0x124u,0x125u,0x126u}};
static wf_status_t raw25(wf_runtime_t*rt,wf_actor_raw_t*a,
                         wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_16116;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x94u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffe8u);
        wf_status_t st=throw2f_offset(rt,a,0xffe8u,tr);if(st!=WF_OK)return st;
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16142u);
        wf_set_be16(t,O_STATE,0xffu);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=31u;return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,32u);
    if(bit(a,O_SELECTOR,7u))wf_set_be16(a,O_PROFILE,0x801eu);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if(bit(a,0xfeu,7u)){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x161a0u);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,2u);
        wf_set_be16(t,O_FACING,wf_be16(a,O_FACING));chgbit(t,O_FACING,7u);
        interrupt_position(rt,a,t,48,-1,48);setbit(a,O_LINK,7u);return WF_OK;
    }
    wf_set_be16(a,O_FRAME,1u);wf_set_be16(a,O_TIMER,0u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x161deu);
    wf_set_be16(t,0x68u,5u);int carry=0;
    wf_status_t st=action22_result_111c8(rt,a,t,b,&carry,tr);if(st!=WF_OK)return st;
    if(carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x8au);setbit(a,O_FLAGS18,7u);}
    int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);if(previous)return WF_OK;
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x60u);
    setbit(a,0x35u,1u);setbit(t,0x35u,2u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);
    st=action22_partner_215b6(rt,a,tr);if(st==WF_OK){wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $16234 */}
    return st;
}

/* $12614[$28] -> $1641E/$16436. Contact takes priority after the
 * phase-one launch; only the phase-three impact consumes the linked actor. */
static const desc_t D_1641E={0x1641eu,1u,4u,
    {16u,16u,0xff00u,32u},{0x14eu,0x14fu,0x150u,0x151u}};
static wf_status_t raw28(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1641E;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xa0u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);
        wf_status_t st=throw2f_offset(rt,a,0xffd0u,tr);if(st!=WF_OK)return st;
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x1644au);
        wf_set_be16(t,O_STATE,0xffu);wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xe8u));
        wf_set_be16(a,0xe8u,(uint16_t)(wf_be16(a,0xe8u)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,
            rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x7du);}
        return WF_OK;
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=17u;
        wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+28u));
        /* $258E[29]: VX=0,VY=0,VZ=0,VZDEC=72; original saves D0/D1. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,29u);
        wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0u);
        wf_set_be16(a,0x5eu,72u);wf_set_u8(a,O_MOTION,2u);return WF_OK;
    }
    if(bit(a,0x37u,4u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
        wf_runtime_emit_audio(rt,0x2bu); /* $164BC */wf_runtime_emit_audio(rt,0x32u); /* $164C6 */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);rt->g.global_1800=14u;return WF_OK;
    }
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x164e6u);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,5u);wf_set_be16(t,0x68u,18u);
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));setbit(t,0x64u,5u);
        setbit(t,0x32u,3u);chgbit(t,O_FACING,7u);interrupt_position(rt,a,t,-96,0,0);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);
        wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        if(wf_u8(a,3u)==11u)return landing_1108c(rt,a,tr);
    }
    return WF_OK;
}

/* $12614[$43] -> $18518/$1853C. The terminal path calls $1108C twice:
 * first at $1862E, then through the shared $16BE2 release tail. */
static const desc_t D_18518={0x18518u,1u,7u,
    {14u,8u,16u,14u,6u,0xff00u,20u},
    {0x1b2u,0x1b3u,0x1b4u,0x1b5u,0x1b6u,0x1b7u,0x1b8u}};
static wf_status_t raw43(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18518;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x10cu);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x18548u);
        wf_set_be16(t,O_STATE,0xffu);wf_set_u8(a,0x19u,48u);wf_set_u8(a,O_B1B,80u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xeeu));
        wf_set_be16(a,0xeeu,(uint16_t)(wf_be16(a,0xeeu)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,
            rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x80u);return WF_OK;}
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=34u;
        interrupt_position(rt,a,t,192,0,0);return WF_OK;
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,0x19u,64u);
    if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0xff80u:128u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x400u);wf_set_be16(a,0x5eu,64u);wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,0x3eu,80u);
    }
    if(low(a,O_FRAME)==5u&&bit(a,0x37u,4u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);wf_set_be16(a,0x3eu,0u);sound_1110e(rt,a); /* $185EE */rt->g.global_1800=14u;
    }
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x18604u);
    wf_set_be16(t,0x68u,24u);chgbit(t,O_FACING,7u);interrupt_position(rt,a,t,128,-1,0);
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0xffa0u:96u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,96,0,0);
    wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
    setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,7u);
    st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
    wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);wf_set_be16(t,0x68u,16u);
    if(!(rt->g.mode_0161&1u)){
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
        if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
    }
    wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_u8(t,O_VISUAL,wf_u8(a,O_FACING));
    chgbit(t,O_FACING,7u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $16C38 */ /* $18634 shares raw2E release. */setbit(a,O_LINK,7u);
    wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);return WF_OK;
}

/* $12614[$75] -> $1A08C, callback $1A0AC. Moving actors select $1A098
 * only on the ordinary path; rope/terminal returns keep $1A08C for ticking. */
static const desc_t D_1A08C={0x1a08cu,1u,1u,{40u},{0x68u}};
static const desc_t D_1A098={0x1a098u,1u,3u,{6u,6u,6u},{0x188u,0x189u,0x18au}};
static wf_status_t raw75(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_1A08C;
    if(!bit(a,O_LATCH,7u))wf_set_be16(a,0x44u,0u);
    else{
        if(raw75_edge_10fc6(rt,a)){wf_set_be16(a,O_STATE,6u);return WF_OK;}
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,wf_u8(a,O_MOTION)?2u:0u);return WF_OK;
        }
    }
    if(wf_u8(a,O_MOTION))*dp=&D_1A098;
    else if(!wf_be16(a,O_TIMER)&&(low(a,O_FRAME)==1u||low(a,O_FRAME)==4u)){
        sound_1112e(rt,a); /* $1A0F6 */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,51u);
    }
    return WF_OK;
}

/* $12614[$34] -> $1747E, callback $174AA. The airborne phase waits at
 * frame5 until scheduler contact releases its sentinel timer. */
static const desc_t D_1747E={0x1747eu,1u,9u,
    {8u,9u,10u,8u,8u,0xff00u,16u,16u,40u},
    {0x90u,0x91u,0x138u,0x139u,0x13au,0x13bu,0x13cu,0x13du,0x68u}};
static wf_status_t raw34(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1747E;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        if(!t)return block(tr,0x174b6u);
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(t,O_STATE,0xffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xfau));
        wf_set_be16(a,0xfau,(uint16_t)(wf_be16(a,0xfau)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        wf_status_t status=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,rt->g.progression_0162,
            &rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(status!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x86u);}
        else{rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=28u;}
        return WF_OK;
    }
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){
        /* $258E[25] preserves D0/D1/A0-A2; the exact row at $2660
         * contains VX=$0100, VZ=$0300, VZDEC=$0058. */
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0xff00u:0x100u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x58u);wf_set_u8(a,O_MOTION,2u);return WF_OK;
    }
    if(bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $17518 */
        rt->g.global_1800=14u;wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
    }
    if(low(a,O_FRAME)==6u&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x1753cu);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x23u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);wf_set_be16(t,0x68u,28u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $1755E */
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));interrupt_position(rt,a,t,64,-1,64);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));setbit(a,O_LINK,7u);}
    return WF_OK;
}

/* $18668: the character-specific linked throw. Failure of the original
 * $1129E chance check switches to action82; success keeps one of two poses. */
static const desc_t D_1863C={0x1863cu,1u,6u,
    {16u,16u,10u,10u,10u,24u},
    {0x1bau,0x1bbu,0x1bcu,0x1bdu,0x1beu,0x1bfu}};
static const desc_t D_1865C={0x1865cu,1u,1u,{16u},{0x123u}};
static wf_status_t raw44(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1863C;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x1867au);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(a,O_B1B,80u);
        wf_set_be16(t,O_STATE,0xffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xf2u));
        wf_set_be16(a,0xf2u,(uint16_t)(wf_be16(a,0xf2u)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),
            rt->g.mode_0161,rt->g.progression_0162,&rt->g.rng_state,
            rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x82u);return WF_OK;}
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=16u;
        if(wf_u8(t,3u)==8u)*dp=&D_1865C;
        return WF_OK;
    }
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        wf_runtime_emit_audio(rt,0x2bu); /* $186DA */
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,5u);
        setbit(t,0x64u,5u);
        wf_set_u8(t,O_FACING,(uint8_t)(wf_u8(a,O_FACING)^0x80u));
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));
        setbit(t,0x32u,3u);wf_set_be16(t,0x68u,24u);wf_runtime_emit_audio(rt,0x32u); /* $1871A */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,32u);
        wf_set_be16(t,O_X,wf_be16(a,O_X));
        wf_set_be16(t,O_Y,(uint16_t)(wf_be16(a,O_Y)-1u));
        wf_set_be16(t,O_Z,(uint16_t)(wf_be16(a,O_Z)+32u));
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        wf_set_be16(a,O_VISUAL,0x1fu);
        wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
        setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $12614[$78] -> $1A186, callback $1A192. The final pose releases
 * both composition links before handing the actor to main state7. */
static const desc_t D_1A186={0x1a186u,1u,1u,{16u},{0x1d8u}};
static wf_status_t raw78(wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_1A186;
    if(!bit(a,O_LATCH,7u))wf_set_u8(a,O_MOTION,0u);
    else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);wf_set_be16(a,0x64u,0u);setbit(a,O_LINK,7u);wf_set_be32(a,0x9cu,0u);
    }
    return WF_OK;
}

/* $12614[$7A] -> $1A348/$1A364, Royal exit submachine at WORD44. */
static const desc_t D_1A348={0x1a348u,1u,1u,{0xff00u},{0x2bu}};
static const desc_t D_1A354={0x1a354u,1u,2u,{12u,12u},{0x66u,0x67u}};
static const desc_t D_11DCA={0x11dcau,0u,0u,{0u},{0u}};
static wf_status_t raw7a_magnitude(wf_actor_raw_t*a,wf_r17_actions_r11_trace_t*tr){
    static const uint8_t magnitude[12]={27,29,28,25,30,27,24,30,22,27,28,27};
    unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x1174cu);
    uint16_t v=bit(a,0x32u,3u)?12u:magnitude[id];
    if(bit(a,0x56u,7u)&&bit(a,0x33u,5u))v=(uint8_t)(v+6u);
    if(bit(a,0x74u,7u))v=(uint16_t)(v-8u);
    wf_set_be16(a,0x2au,v);return WF_OK;
}
static wf_status_t raw7a(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,
                         const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    unsigned sub=wf_be16(a,0x44u);*dp=&D_1A348;
    if(sub>=6u)return block(tr,0x1a370u);
    if(sub==0u||sub==4u){
        *dp=&D_11DCA;int previous=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
        if(!previous){
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x4au,3u);chgbit(a,O_FACING,7u);
            wf_set_be16(a,O_VISUAL,(uint16_t)(0x1d3u|wf_be16(a,O_FACING)));wf_set_be16(a,O_TIMER,40u);
            if(sub){
                int linked=bit(a,0x34u,6u);pin_clear(a,0x34u,0x40u);if(!linked)return WF_OK;
                pin_clear(a,0x34u,0x80u);wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,0x7au));if(!t)return block(tr,0x1a4a6u);
                pin_clear(t,0x34u,0x80u);
            }
        }
        uint16_t timer=(uint16_t)(wf_be16(a,O_TIMER)-1u);wf_set_be16(a,O_TIMER,timer);
        if(!timer){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,(uint16_t)(sub+1u));chgbit(a,O_FACING,7u);}return WF_OK;
    }
    if(sub==1u){
        *dp=&D_115F2;
        if(!bit(a,O_LATCH,7u)){
            wf_set_u8(a,O_MOTION,1u);wf_set_be16(a,0x32u,0x1000u);wf_status_t st=raw7a_magnitude(a,tr);if(st!=WF_OK)return st;
            uint16_t angle=wf_be16(a,O_X)>=0x280u?0x40u:0xc0u;wf_set_be16(a,0x2cu,angle);
            wf_set_be16(a,O_FACING,(uint16_t)((uint16_t)(angle<<8)&0x8000u));chgbit(a,O_FACING,7u);
        }else if(wf_u8(a,0x37u)){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,2u);}return WF_OK;
    }
    if(sub==2u){
        *dp=&D_1A354;
        if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);setbit(a,0x33u,2u);wf_set_be16(a,O_RENDER_LIST,2u);}
        else if(low(a,O_FRAME)==0xfeu){
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,-32,0,0);
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,3u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        }return WF_OK;
    }
    if(sub==3u){
        if(!bit(a,O_LATCH,7u)){
            wf_set_be16(a,O_RENDER_LIST,4u);wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x2c0u:0xfd40u);
            wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x180u);wf_set_be16(a,0x5eu,0x58u);wf_set_u8(a,O_MOTION,2u);
        }else if(bit(a,0x37u,4u)){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,0x44u,4u);}return WF_OK;
    }
    *dp=&D_115F2;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,1u);wf_status_t st=raw7a_magnitude(a,tr);if(st!=WF_OK)return st;
        wf_set_be16(a,0xc0u,0xb8u);wf_set_be16(a,0xbeu,bit(a,O_FACING,7u)?0x3c0u:0x140u);return WF_OK;
    }
    if(!wf_cpu_arrived_1f15c(rt,a))return WF_OK;
    unsigned slot=(unsigned)(a-rt->actors);
    if(slot<4u){
        static const uint16_t at[4]={0x104u,0x128u,0x154u,0x178u};wf_r17_fg0_write_t writes[36];size_t n=0;
        for(unsigned row=0;row<4u;row++)for(unsigned col=0;col<9u;col++)writes[n++]=(wf_r17_fg0_write_t){0xc0000u+at[slot]+row*0x100u+col*4u,0u,4u};
        if(!b->video_batch)return block(tr,0x7744u);
        wf_status_t st=b->video_batch(rt,writes,n,b->video_user);if(st!=WF_OK)return block(tr,0x7744u);
    }
    if(!b->resource_2b58)return block(tr,0x2b58u);
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,2u));
    wf_status_t st=b->resource_2b58(rt,wf_be16(a,2u),b->video_user);if(st!=WF_OK)return block(tr,0x2b58u);
    rt->g.global_16a7=(uint8_t)(rt->g.global_16a7+1u);memset(a,0,sizeof(*a));return WF_OK;
}
/* $12614[$79] -> $1A1BE, callback $1A28E. Character WORDs select
 * these literal $1A30C descriptors; $1A33C supplies the repeat BYTE. */
static const desc_t D_1A1BE={0x1a1beu,1u,8u,{6u,6u,6u,6u,6u,6u,16u,48u},{0x59u,0x5au,0x5bu,0x59u,0x5au,0x5bu,0x5cu,0x5du}};
static const desc_t D_1A1E6={0x1a1e6u,1u,2u,{16u,16u},{0x5au,0x59u}};
static const desc_t D_1A1F6={0x1a1f6u,1u,4u,{6u,6u,6u,6u},{0x59u,0x5au,0x5bu,0x5cu}};
static const desc_t D_1A20E={0x1a20eu,1u,2u,{16u,24u},{0x59u,0x5au}};
static const desc_t D_1A21E={0x1a21eu,1u,2u,{48u,16u},{0x59u,0x5au}};
static const desc_t D_1A22E={0x1a22eu,1u,5u,{16u,16u,16u,16u,16u},{0x59u,0x5au,0x5bu,0x5au,0x59u}};
static const desc_t D_1A24A={0x1a24au,1u,3u,{40u,24u,4u},{0x59u,0x5au,0u}};
static const desc_t D_1A25E={0x1a25eu,1u,2u,{12u,12u},{0x59u,0x5au}};
static const desc_t D_1A26E={0x1a26eu,1u,2u,{8u,8u},{0x59u,0x5au}};
static const desc_t D_1A27E={0x1a27eu,1u,2u,{16u,48u},{0x59u,0x5au}};
static wf_status_t raw79(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    static const desc_t*const characters[12]={&D_1A1BE,&D_1A1E6,&D_1A1F6,&D_1A20E,&D_1A20E,&D_1A20E,&D_1A21E,&D_1A22E,&D_1A24A,&D_1A25E,&D_1A26E,&D_1A27E};
    static const uint8_t repeats[12]={1u,2u,2u,1u,1u,1u,1u,1u,1u,4u,2u,1u};
    *dp=&D_1A1BE;
    if(!bit(a,O_LATCH,7u)){
        int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
        if(!previous){
            wf_set_u8(a,O_MOTION,0u); wf_runtime_emit_audio(rt,wf_u8(a,3u)<6u?0x32u:0x31u); /* $1A2B2 */
            unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x1a2c2u);
            wf_set_be16(a,0x44u,(uint16_t)(repeats[id]&15u));
        }
    }else if(low(a,O_FRAME)==0xfeu){
        uint16_t count=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,count);
        if(count)wf_set_be16(a,O_STATE,5u);
        else{
            wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));pin_clear(a,0x34u,0x10u);
            return WF_OK; /* $1A2F8 retains the original $1A1BE descriptor. */
        }
    }
    unsigned id=wf_be16(a,2u);if(id>=12u)return block(tr,0x1a306u);
    *dp=characters[id];return WF_OK;
}

/* $12614[$01/$45] -> $128DE/$128FA, identical ROM table entries.
 * The directed init consumes A2 only
 * when the prior FLAGS32.bit1 was set; all later phases leave it unread. */
static const desc_t D_128DE={0x128deu,1u,5u,{4u,8u,0xff00u,0xff00u,16u},
    {0x85u,0xabu,0xacu,0x17au,0x17au}};
static wf_status_t raw01_45(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_128DE;wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low(a,O_SELECTOR)*4u));
    if(!bit(a,O_LATCH,7u)){
        int directed=bit(a,0x32u,1u);pin_clear(a,0x32u,2u);
        if(!directed){
            int alternate=low(a,0x1eu)==2u;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,alternate?28u:27u);
            wf_set_be16(a,0x58u,alternate?(bit(a,O_FACING,7u)?0x300u:0xfd00u):0u);
            wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,alternate?0x700u:0x800u);
            wf_set_be16(a,0x5eu,0x60u);wf_set_u8(a,O_MOTION,2u);setbit(a,0x33u,3u);return WF_OK;
        }
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
        wf_set_be16(a,0x5cu,0x700u);wf_set_be16(a,0x5eu,0x60u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x26aeu);
        wf_status_t status=wf_r17_trajectory_26ae(a,t,0u);if(status!=WF_OK)return block(tr,0x26aeu);
        rt->g.global_1670=0u;wf_set_u8(a,O_MOTION,2u);wf_set_be16(a,0x4au,0u);return WF_OK;
    }
    if(raw75_edge_10fc6(rt,a)){
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,23u);wf_set_be16(a,0x68u,3u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $12970 */return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,low(a,O_FRAME)==2u?24u:0u);
    if(low(a,O_FRAME)==2u)wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,0x33u,2u)?0x110u:0x150u);
    if(low(a,O_FRAME)==2u&&wf_be16(a,O_Z)<=(bit(a,0x33u,2u)?0x110u:0x150u)){
        wf_set_be16(a,O_TIMER,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd8u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-40,0,0);
    }
    if(low(a,O_FRAME)==3u){
        if(!bit(a,0x37u,4u))return WF_OK;
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u); sound_1110e(rt,a); /* $129D4 */
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);}
    return WF_OK;
}
/* $12614[$02] -> $129F0, callback $12A08. Linked jumping throw;
 * the impact frame precedes the landing-contact/final-frame tests. */
static const desc_t D_129F0={0x129f0u,1u,4u,{12u,12u,0xff00u,8u},{0x142u,0x143u,0xacu,0x17au}};
static wf_status_t raw02(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_129F0;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,8u); /* $1C08A/$1C08E raw02 index. */
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x12a14u);
        wf_set_be16(t,O_STATE,0xffu);wf_set_u8(a,O_B1B,16u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffe8u);
        return action_offset_10b62(rt,a,0xffe8u,tr);
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        setbit(a,O_FLAGS18,7u);
        /* $258E preset7: VX=VY=VZ=0, VZDEC=$38; D registers preserved. */
        wf_set_be16(a,0x58u,0u);wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0u);
        wf_set_be16(a,0x5eu,0x38u);wf_set_u8(a,O_MOTION,2u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x10ba2u);
        interrupt_position(rt,a,t,-32,0,0);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,32u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,80u);
        helper_10bd0(a,32,0,80);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x29u);wf_set_be16(t,0x68u,15u);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x2bu); /* $12A86 */wf_runtime_emit_audio(rt,0x32u); /* $12A90 */
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        return WF_OK;
    }
    if(bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $12AA6 */
        wf_set_be16(a,O_TIMER,0u);wf_set_u8(a,O_MOTION,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd8u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-40,0,0);return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);
        if(wf_u8(a,3u)==7u)return landing_1108c(rt,a,tr);
    }
    return WF_OK;
}

/* $12614[$07] -> $130E6, callback $13116. Character BYTE7 keeps the
 * original eight-tick second pose; other characters select $130FE. */
static const desc_t D_130E6={0x130e6u,1u,4u,{4u,8u,20u,4u},{0xbfu,0xc0u,0xc1u,0xc0u}};
static const desc_t D_130FE={0x130feu,1u,4u,{4u,4u,20u,4u},{0xbfu,0xc0u,0xc1u,0xc0u}};
static wf_status_t raw07(wf_actor_raw_t*a,const desc_t**dp){
    if(!bit(a,O_LATCH,7u))wf_set_u8(a,O_MOTION,0u);
    else{
        wf_set_be16(a,O_PROFILE,1u);
        if(wf_be16(a,O_FRAME)==1u)wf_set_be16(a,O_PROFILE,28u);
        if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);}
    }
    *dp=wf_u8(a,3u)==7u?&D_130E6:&D_130FE;return WF_OK;
}

/* $12614[$2D] -> $169FA, callback $16A16. The original latch test is
 * followed by NOP: first entry also executes the rope/speed continuation. */
static const desc_t D_169FA={0x169fau,1u,1u,{10u},{0x95u}};
static wf_status_t raw2d(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_169FA;
    if(raw75_edge_10fc6(rt,a)){wf_set_be16(a,O_STATE,6u);return WF_OK;}
    wf_set_be16(a,O_PROFILE,11u);
    uint16_t speed=(uint16_t)(wf_be16(a,0x2au)-2u);wf_set_be16(a,0x2au,speed);
    if(!(speed&0x8000u))return WF_OK;
    wf_set_be16(a,O_STATE,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,8u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
    helper_10bd0(a,8,0,0);setbit(a,O_LINK,7u);return WF_OK;
}

/* $12614[$19] -> $14FB6, callback $15006. The wall lock selects the
 * local $14FDE pose sequence; init and terminal exits keep $14FB6. */
static const desc_t D_14FB6={0x14fb6u,1u,8u,{12u,8u,4u,10u,0xff00u,12u,16u,16u},
    {0x1f7u,0x1f8u,0x1f9u,0x1fau,0x1fbu,0x1fcu,0x1fdu,0x6au}};
static const desc_t D_14FDE={0x14fdeu,1u,8u,{12u,8u,4u,10u,0xff00u,12u,16u,16u},
    {0x1f7u,0x1f8u,0x1f9u,0x1fau,0x1fdu,0x1fdu,0x1fdu,0x6au}};
static wf_status_t raw19(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_14FB6;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x64u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);if(!t)return block(tr,0x15012u);
        wf_set_be16(t,O_STATE,0xffu);wf_set_u8(a,0x19u,46u);wf_set_u8(a,O_B1B,78u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xf4u));
        wf_set_be16(a,0xf4u,(uint16_t)(wf_be16(a,0xf4u)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t trace;int carry=0;
        wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&trace,&carry);
        if(st!=WF_OK)return block(tr,trace.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x83u);return WF_OK;}
        interrupt_position(rt,a,t,240,0,0);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=7u;return WF_OK;
    }
    if(wf_be16(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER))wf_set_u8(a,0x19u,127u);
    if(wf_be16(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        /* $258E preset22=$0100,$0300,$0048, all D registers preserved. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,22u);
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0xff00u:0x100u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x48u);wf_set_u8(a,O_MOTION,2u);
    }
    if(wf_be16(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)&&!(rt->g.mode_0161&2u)){
        wf_set_be16(a,0x3eu,128u);wf_status_t st=wf_r17_resolver_280dc(rt,a);
        if(st!=WF_OK)return block(tr,0x280dcu);
        wf_set_be16(a,0x3eu,0u);uint8_t contact=(uint8_t)(wf_u8(a,0x36u)&3u);wf_r17_m68k_move_b(&rt->m68k_regs,0u,contact);
        if(contact){
            setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_VISUAL,(uint16_t)(0x1fdu|wf_be16(a,O_FACING)));
            if(!t)return block(tr,0x150dcu);
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x25u);setbit(t,0x64u,5u);wf_set_be16(t,0x44u,0u);chgbit(t,O_FACING,7u);
            interrupt_position(rt,a,t,128,1,57);
            if(bit(a,0x37u,1u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}
            else if(bit(a,0x37u,0u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
            pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        }
    }
    if(bit(a,0x37u,4u)){sound_1110e(rt,a); /* $15122 */wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);rt->g.global_1800=14u;}
    if(wf_be16(a,O_FRAME)==5u&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x1514eu);
        if(!bit(a,O_SELECTOR,7u)){
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x21u);setbit(t,0x64u,5u);
            wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(t,O_FACING,7u);interrupt_position(rt,a,t,240,1,0);
            pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        }
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));setbit(t,0x32u,3u);
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
        wf_set_be16(t,0x68u,20u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $151A4 */
    }else{
        if(low(a,O_FRAME)==6u&&!wf_be16(a,O_TIMER)){
            setbit(a,O_FLAGS18,7u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,48u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,48,0,0);
        }
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,7u);wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
            wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));setbit(a,O_LINK,7u);return WF_OK;
        }
    }
    if(bit(a,O_SELECTOR,7u))*dp=&D_14FDE;
    return WF_OK;
}

/* $12614[$21] -> $15B24, callback $15B34. WORD frame1 supplies the
 * collision profile; the terminal BYTE FE releases the composition link. */
/* $12614[$1F] and [$1E] -> $159F8, callback $15A20..$15B22. +$76 is
 * consumed only by either release branch; keep the ROM write order for
 * self aliases and for the guards before the first object write. */
static const desc_t D_159F8={0x159f8u,1u,3u,{8u,8u,8u},{0x74u,0x75u,0x76u}};
static const desc_t D_15A0C={0x15a0cu,1u,3u,{8u,8u,8u},{0x79u,0x7au,0x7bu}};
static wf_status_t raw1f(wf_runtime_t*rt,wf_actor_raw_t*a,
                        wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                        wf_r17_actions_r11_trace_t*tr){
    wf_actor_raw_t*o;uint32_t object;
    *dp=&D_159F8;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(a,O_B1B,0x20u);
        wf_set_be32(a,0x92u,0u);
    }else{
        wf_set_be16(a,O_PROFILE,0u);
        if(low(a,O_FRAME)==1u){
            wf_set_be16(a,O_PROFILE,15u);
            if(wf_be16(a,O_TIMER)==0u){
                if(wf_be32(a,0x92u)==0u)goto descriptor;
                wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_TIMER,2u);
                wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
                setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_OBJ74,0u);
                object=wf_be32(a,O_OBJLINK);wf_set_be32(a,O_OBJLINK,0u);
                o=actor_from_ptr(rt,object);
                if(!o)o=object_from_ptr(b,object&0x00ffffffu);
                if(!o)return block(tr,0x15a7eu);
                wf_set_be16(o,O_LATCH,2u);
                wf_set_be16(o,O_X,wf_be16(a,O_X));
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0x60u:0xffa0u);
                wf_set_be16(o,O_X,(uint16_t)(wf_be16(o,O_X)+(uint16_t)rt->m68k_regs.d[0]));
                wf_set_be16(o,O_Y,wf_be16(a,O_Y));
                wf_set_be16(o,O_Z,wf_be16(a,O_Z));
                wf_set_be16(o,O_Z,(uint16_t)(wf_be16(o,O_Z)+0x40u));
                goto descriptor;
            }
        }
        if(low(a,O_FRAME)==0xfeu&&wf_be16(a,O_OBJ74)!=0u){
            wf_set_be16(a,O_OBJ74,0u);object=wf_be32(a,O_OBJLINK);
            o=actor_from_ptr(rt,object);
            if(!o)o=object_from_ptr(b,object&0x00ffffffu);
            if(!o)return block(tr,0x15ac6u);
            wf_set_be16(o,O_LATCH,2u);wf_set_be32(a,O_OBJLINK,0u);
            wf_set_be16(o,O_X,wf_be16(a,O_X));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0x60u:0xffa0u);
            wf_set_be16(o,O_X,(uint16_t)(wf_be16(o,O_X)+(uint16_t)rt->m68k_regs.d[0]));
            wf_set_be16(o,O_Y,wf_be16(a,O_Y));wf_set_be16(o,O_Z,wf_be16(a,O_Z));
            wf_set_be16(o,O_Z,(uint16_t)(wf_be16(o,O_Z)+0x10u));
            wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
            setbit(a,O_FLAGS18,7u);setbit(a,O_LINK,7u);return WF_OK;
        }
    }
descriptor:
    if(wf_u8(a,O_OBJ75)!=0u)*dp=&D_15A0C;
    return WF_OK;
}

static const desc_t D_15B24={0x15b24u,1u,2u,{14u,24u},{0xb4u,0xb5u}};
static wf_status_t raw21(wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_15B24;
    if(!bit(a,O_LATCH,7u))wf_set_u8(a,O_MOTION,0u);
    else{
        if(wf_be16(a,O_FRAME)==1u)wf_set_be16(a,O_PROFILE,8u);
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);
            wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        }
    }
    return WF_OK;
}

/* $12614[$04] -> $12BDA/$12C22. Capture tests the raw reciprocal LONG,
 * then selects one of three original descriptors. A2/A3 writes preserve
 * their original order, including self/target/third-actor aliases. */
static const desc_t D_12BDA={0x12bdau,1u,4u,{8u,16u,0xff00u,32u},{0xa4u,0xa5u,0xa6u,0x17au}};
static const desc_t D_12BF2={0x12bf2u,1u,4u,{2u,4u,0xff00u,32u},{0xa2u,0xa3u,0xa6u,0x17au}};
static const desc_t D_12C0A={0x12c0au,1u,4u,{4u,8u,0xff00u,32u},{0x1cbu,0x1ccu,0xa6u,0x17au}};
static wf_status_t raw04(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_12BDA;wf_r17_m68k_move_w(&rt->m68k_regs,0u,16u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_be32(a,0x92u,0u);wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+48u));
        setbit(a,0x33u,3u);pin_clear(a,0x33u,0x10u);
        int directed=bit(a,0x32u,1u);pin_clear(a,0x32u,2u);
        if(!directed){
            /* $258E[10] = FD80,0500,0060; D0/D1 are preserved. */
            wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x280u:0xfd80u);
            wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x500u);wf_set_be16(a,0x5eu,0x60u);
            wf_set_u8(a,O_MOTION,2u);return WF_OK;
        }
        rt->g.global_1670=0u;wf_set_u8(a,O_MOTION,2u);setbit(a,0x34u,3u);pin_clear(a,0x32u,1u);
        wf_set_be16(a,0x58u,bit(a,0x33u,7u)?0xfc00u:0x400u);
        wf_set_be16(a,0x5cu,0x200u);wf_set_be16(a,0x5eu,0x38u);
        wf_set_be16(a,0x5au,bit(a,0x33u,7u)?0xff00u:0x100u);return WF_OK;
    }
    if(!bit(a,O_SELECTOR,7u)&&low(a,O_FRAME)<2u){
        wf_set_be16(a,O_PROFILE,0u);
        uint16_t height=bit(a,0x33u,2u)?0x120u:0x160u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,height);
        if(wf_be16(a,O_Z)<=height)goto descriptor04;
        wf_set_be16(a,O_PROFILE,23u);uint32_t candidate=wf_be32(a,0x92u);
        if(candidate){
            wf_actor_raw_t*c=actor_from_ptr(rt,candidate);if(!c)return block(tr,0x12cdcu);
            if(low(c,0x64u)==0x12u&&wf_be32(c,0x92u)==wf_actor_rom_base((unsigned)(a-rt->actors))){
                wf_set_u8(a,O_MOTION,0u);setbit(a,O_SELECTOR,7u);
                if(!(rt->g.mode_0161&1u)){
                    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(c,0x86u));if(!p)return block(tr,0x110eeu);
                    if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
                }
                wf_set_be16(a,O_Z,wf_be16(c,O_Z));wf_set_be16(a,O_LATCH,5u);
                if(!bit(a,0x34u,3u)){
                    wf_status_t status=landing_1108c(rt,a,tr);if(status!=WF_OK)return status;
                    wf_set_be32(a,O_LINK,wf_be32(a,0x92u));
                }else{
                    /* Reload +92 as the original does after the helper. */
                    c=actor_from_ptr(rt,wf_be32(a,0x92u));if(!c)return block(tr,0x12d26u);
                    wf_set_be16(a,O_X,wf_be16(c,O_X));wf_set_be16(a,O_Y,wf_be16(c,O_Y));
                    wf_set_u8(a,0x19u,16u);wf_set_u8(a,O_B1B,64u);wf_set_be32(a,O_LINK,wf_be32(c,O_LINK));
                }
                goto descriptor04;
            }
        }
    }
    if(bit(a,O_SELECTOR,7u)&&low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        int third=bit(a,0x34u,3u);pin_clear(a,0x34u,8u);
        if(!third){
            wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+64u));setbit(a,O_FLAGS18,7u);
            if(!t)return block(tr,0x12d7au);
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,25u);setbit(t,0x34u,3u);
            wf_set_be16(t,O_VISUAL,0xb3u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
            /* $10B9A reloads the owner's raw LINK after these writes. */
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!p)return block(tr,0x10b9eu);
            interrupt_position(rt,a,p,16,-1,16);
        }else{
            wf_set_be16(a,0x4au,0u);if(!t)return block(tr,0x12db2u);
            wf_actor_raw_t*q=actor_from_ptr(rt,wf_be32(t,O_LINK));if(!q)return block(tr,0x12db6u);
            setbit(q,O_FLAGS18,7u);setbit(a,O_FLAGS18,7u);wf_set_be16(q,O_STATE,5u);wf_set_be16(q,O_SELECTOR,0x8006u);
            chgbit(q,O_FACING,7u);setbit(q,0x34u,3u);setbit(q,O_LINK,7u);
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,25u);wf_set_be16(t,0x68u,32u);
            pin_clear(t,0x33u,0x40u);pin_clear(q,0x33u,0x40u);
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(q,O_LINK));if(!p)return block(tr,0x10b9eu);
            interrupt_position(rt,q,p,24,-1,112);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xfff0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(q,-16,-1,0);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xfff0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,112u);
            setbit(t,O_LINK,7u);helper_10bd0(a,-16,0,112);
        }
    }
    if(low(a,O_FRAME)==2u){
        wf_set_u8(a,O_MOTION,2u);
        if(bit(a,0x37u,4u)){
            sound_1110e(rt,a); /* $12E50 */
            wf_set_be16(a,O_TIMER,0u);wf_set_u8(a,O_MOTION,0u);pin_clear(a,0x33u,0x18u);
            wf_set_be16(a,O_TIMER,0u);chgbit(a,O_FACING,7u);wf_set_u8(a,0x19u,0xd0u);
        }
    }
    if(low(a,O_FRAME)<3u){
        wf_set_be16(a,0x3eu,0xfff0u);
        if(raw75_edge_10fc6(rt,a)){
            wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,23u);wf_set_be16(a,0x68u,3u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $12EA4 */wf_set_be16(a,0x3eu,0u);return WF_OK;
        }
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));wf_set_be16(a,0x3eu,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-48,0,0);setbit(a,O_LINK,7u);return WF_OK;
    }
descriptor04:
    if(bit(a,O_SELECTOR,7u))*dp=bit(a,0x34u,3u)?&D_12C0A:&D_12BF2;
    return WF_OK;
}

/* $12614[$05] -> $12F08, callback $12F40. A reciprocal +$92 relation
 * changes to the linked attack; rope and terminal returns keep $12F08. */
static const desc_t D_12F08={0x12f08u,1u,6u,{6u,6u,6u,6u,4u,12u},{0x96u,0x97u,0x98u,0x99u,0x9au,0x9bu}};
static const desc_t D_12F28={0x12f28u,1u,4u,{4u,4u,6u,6u},{0x9fu,0xa0u,0xa1u,0x9bu}};
static wf_status_t raw05(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_12F08;wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,1u);wf_set_be32(a,0x92u,0u);goto descriptor;
    }
    sound_1112e(rt,a); /* $12F56 */
    if(!bit(a,O_SELECTOR,7u)){
        wf_set_be16(a,O_PROFILE,wf_be16(a,O_FRAME)<4u?6u:3u);
        uint32_t relation=wf_be32(a,0x92u);
        if(relation){
            wf_actor_raw_t*c=actor_from_ptr(rt,relation);if(!c)return block(tr,0x12f82u);
            uint32_t self=wf_actor_rom_base((unsigned)(a-rt->actors));
            if(low(c,0x64u)==0x10u&&wf_be32(c,0x92u)==self){
                wf_set_be32(a,O_LINK,relation);wf_set_be16(a,O_LATCH,5u);setbit(a,O_SELECTOR,7u);
                /* $1C08E leaves D0.W=selector5*4 before this RNG call. */
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,20u);
                wf_status_t status=landing_1108c(rt,a,tr);if(status!=WF_OK)return status;
                wf_set_be16(a,O_PROFILE,0u);goto descriptor;
            }
        }
        if(raw75_edge_10fc6(rt,a)){wf_set_be16(a,O_STATE,6u);return WF_OK;}
    }
    if(bit(a,O_SELECTOR,7u)&&low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x12fd4u);
        wf_set_be16(t,O_STATE,4u);wf_set_u8(t,0x65u,0x19u);interrupt_position(rt,a,t,16,-1,56);
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        goto descriptor;
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,3u);setbit(a,O_LINK,7u);return WF_OK;}
descriptor:
    if(bit(a,O_SELECTOR,7u))*dp=&D_12F28;
    return WF_OK;
}

/* $12614[$50] -> $18B8E, callback $18B94: immediate linked release.
 * Kind zero leaves all animation/latch fields unchanged. Target writes to
 * literal null ($7F000) or tagged physical zero hit read-only ROM. */
static const desc_t D_18B8E={0x18b8eu,0u,0u,{0u},{0u}};
static wf_status_t raw50(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                          wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18B8E;uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!t&&(link&0x00ffffffu))return block(tr,0x18ba0u);
    wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
    if(t){
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,1u);wf_set_be16(t,O_VISUAL,0x16u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
    }
    uint16_t dx=bit(a,O_FACING,7u)?0xffe0u:32u;
    wf_set_be16(a,O_X,(uint16_t)(wf_be16(a,O_X)+dx));
    if(t)wf_set_be16(t,O_X,(uint16_t)(wf_be16(t,O_X)-dx));
    setbit(a,O_LINK,7u);if(t)setbit(t,O_LINK,7u);
    pin_clear(a,0x33u,0x40u);if(t)pin_clear(t,0x33u,0x40u);
    return WF_OK;
}

/* $12614[$2E] -> $16A56, callback $16A7A. */
static const desc_t D_16A56={0x16a56u,1u,7u,
    {32u,2u,8u,9u,6u,0xff00u,16u},{0xb6u,0xb7u,0xb8u,0xb9u,0xbau,0xbbu,0xbcu}};
static wf_status_t raw2e(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_16A56;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be32(a,0x92u,0u);
        wf_set_u8(a,0x19u,0x40u);wf_set_u8(a,0x1bu,0x50u);return WF_OK;
    }
    if(!bit(a,O_SELECTOR,7u)){
        if(!wf_be16(a,O_FRAME)){
            wf_set_be16(a,O_PROFILE,5u);
            uint32_t candidate=wf_be32(a,0x92u);
            if(candidate){
                wf_actor_raw_t*t=actor_from_ptr(rt,candidate);
                if(!t)return block(tr,0x16abau);
                if(low(t,O_STATE)==4u&&low(t,0x64u)==0x1fu){
                    /* $16ACA copies the complete tagged candidate pointer. */
                    wf_set_be32(a,O_LINK,candidate);setbit(a,0x33u,6u);setbit(t,0x33u,6u);
                    wf_set_be16(t,O_STATE,0xffu);wf_set_be32(t,O_LINK,wf_actor_rom_base((unsigned)(a-rt->actors)));
                    setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_TIMER,0u);
                    wf_set_u8(a,0x19u,0x30u);wf_set_u8(a,0x1bu,0x50u);return WF_OK;
                }
            }
        }
        if(!wf_be16(a,O_TIMER)){
            wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);setbit(a,O_FLAGS18,7u);
            wf_set_be16(a,O_TIMER,2u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));return WF_OK;
        }
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,0x19u,0x40u);wf_set_u8(a,0x1bu,0x50u);
    }
    if(wf_be16(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
        /* $258E[9] =0080,0400,0040; only VX depends on facing. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,9u);
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0xff80u:0x80u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x400u);wf_set_be16(a,0x5eu,0x40u);wf_set_u8(a,O_MOTION,2u);
        setbit(a,0x33u,3u);wf_set_be16(a,0x3eu,0x50u);
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=0x13u;return WF_OK;
    }
    if(wf_be16(a,O_FRAME)==5u&&bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $16B96 */
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);wf_set_be16(a,0x3eu,0u);
        rt->g.global_1800=14u;return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16bbeu);
        wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));interrupt_position(rt,a,t,128,-1,0);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0xffe0u:32u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,32,0,0);
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,7u);
        wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
        wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);wf_set_be16(t,0x68u,16u);
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_u8(t,O_VISUAL,wf_u8(a,O_FACING));
        chgbit(t,O_FACING,7u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $16C38 */setbit(a,O_LINK,7u);
        wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    }
    return WF_OK;
}

/* $1639C..$1641C: capture qualification through +$92. This callback consumes
 * the relation only while selector bit7 is clear; +$26 is never dereferenced. */
static wf_status_t raw27(wf_runtime_t *rt,wf_actor_raw_t *a,const desc_t **dp,
                         wf_r17_actions_r11_trace_t *tr){
    *dp=&D_16388;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be32(a,0x92u,0u);return WF_OK;
    }
    if(!bit(a,O_SELECTOR,7u)){
        wf_set_be16(a,O_PROFILE,0x10u);
        uint32_t relation=wf_be32(a,0x92u);
        if(relation){
            wf_actor_raw_t*t=actor_from_ptr(rt,relation);
            if(!t)return block(tr,0x163c8u);
            if(low(t,O_STATE)==4u&&low(t,0x64u)==0x14u){
                setbit(a,O_SELECTOR,7u);wf_set_u8(a,O_FRAME+1u,1u);
                wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_TIMER,12u);
                return WF_OK;
            }
        }
    }
    if((low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)&&!bit(a,O_SELECTOR,7u))||
       low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $12614[$13] -> $149E2, callback $149F2. Keep full-WORD impact
 * phase, lazy target reads and the original failed-prefix mutations. */
static const desc_t D_149E2={0x149e2u,1u,2u,{32u,0xff00u},{0xedu,0xeeu}};
static wf_status_t raw13(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_149E2;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x4cu);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,2u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,56u);
        if(!t)return block(tr,0x14a06u);
        uint16_t dx=bit(t,O_FACING,7u)?0xffc8u:56u;
        pin_clear(a,O_FACING,0x80u);
        uint16_t x=(uint16_t)(wf_be16(t,O_X)+dx);
        if(x>=wf_be16(a,O_X))setbit(a,O_FACING,7u);
        wf_set_be16(a,O_X,x);wf_set_be16(a,O_Y,wf_be16(t,O_Y));wf_set_be16(a,O_Z,wf_be16(t,O_Z));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,16u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,32u);helper_10bd0(a,16,0,32);
        wf_set_be16(a,0x5cu,0x300u);wf_set_be16(a,0x5eu,0x30u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,2u);
        wf_status_t st=wf_r17_trajectory_26ae(a,t,2u);
        return st==WF_OK?WF_OK:block(tr,0x26aeu);
    }
    if(!bit(a,O_SELECTOR,7u)&&wf_be16(a,O_FRAME)==1u&&bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $14A7A */
        if(!t)return block(tr,0x11156u);
        if(wf_r17_helper_11152_carry(a,t)){
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,7u);wf_set_be16(t,0x68u,10u);
            setbit(a,O_SELECTOR,7u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,24u);
            wf_set_be16(t,0xd6u,(uint16_t)(wf_be16(t,0xd6u)+1u));
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x32u);wf_runtime_emit_audio(rt,0x2au); /* $14ABA */wf_runtime_emit_audio(rt,0x32u); /* $14AC4 */return WF_OK;
        }
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);chgbit(a,O_FACING,7u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffc0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,-64,0,0);
        wf_set_be16(a,0x68u,3u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x32u);wf_runtime_emit_audio(rt,0x32u); /* $14B32 */
        rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,64u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,64,0,0);setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $12614[$1B] -> $15384. The callback is shared with legacy raw0D,
 * but its descriptor, lazy reads and failed-prefix contracts are distinct. */
static const desc_t D_15384={0x15384u,1u,5u,{8u,24u,24u,0xff00u,16u},
    {0x84u,0x85u,0x15bu,0x15cu,0x15cu}};
static wf_status_t raw1b_handover_21424(wf_runtime_t*rt,wf_actor_raw_t*a,
                                      wf_r17_actions_r11_trace_t*tr){
    uint32_t first_raw=wf_be32(a,0x86u);wf_actor_raw_t*first=actor_from_ptr(rt,first_raw);
    if(!first)return block(tr,0x2142cu);
    if(bit(first,0x33u,7u))rt->g.global_1688=first_raw;else rt->g.global_1684=first_raw;
    setbit(first,0x34u,0u);
    wf_actor_raw_t*linked=actor_from_ptr(rt,wf_be32(first,O_LINK));
    if(!linked)return block(tr,0x2144cu);
    uint32_t second_raw=wf_be32(linked,0x86u);wf_actor_raw_t*second=actor_from_ptr(rt,second_raw);
    if(!second)return block(tr,0x2145cu);
    if(bit(second,0x33u,7u))rt->g.global_1688=second_raw;else rt->g.global_1684=second_raw;
    setbit(second,0x34u,0u);
    uint32_t saved_d0=rt->m68k_regs.d[0];uint16_t tier=wf_be16(second,0x70u);
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(tier<<1));
    if(tier>2u)return block(tr,0x21486u);
    wf_set_be16(second,0xe6u,31u);
    if(!bit(first,0x33u,1u)){setbit(first,0x56u,6u);wf_set_be32(first,0x7au,second_raw);}
    pin_clear(a,0x56u,0x40u);setbit(second,0x32u,2u);setbit(second,0x56u,6u);wf_set_be32(second,0x7au,first_raw);rt->m68k_regs.d[0]=saved_d0;
    return WF_OK;
}
static wf_status_t raw1b(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,
                         const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_15384;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x6cu);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);rt->g.global_1670=0;pin_clear(a,0x32u,2u);
        wf_set_be16(a,0x5cu,0x680u);wf_set_be16(a,0x5eu,0x48u);
        if(bit(a,0x34u,3u))wf_set_be16(a,0x44u,0u);
        else{
            if(!t)return block(tr,0x153f2u);
            wf_set_be16(a,0x44u,low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x53u?1u:2u);
        }
        if(low(a,0x44u)==2u){
            wf_set_be16(a,O_FACING,0u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,O_X));
            if(!t)return block(tr,0x15420u);
            if(wf_be16(a,O_X)<wf_be16(t,O_X))setbit(a,O_FACING,7u);
        }
        uint16_t dx=low(a,0x44u)?16u:0xfff0u;if(bit(a,O_FACING,7u))dx=(uint16_t)(0u-dx);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,dx);
        if(!t)return block(tr,0x15444u);
        wf_set_be16(t,O_X,(uint16_t)(wf_be16(t,O_X)+dx));wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
        wf_status_t st=wf_r17_trajectory_26ae(a,t,0u);
        dx=(uint16_t)(0u-dx);wf_r17_m68k_move_w(&rt->m68k_regs,1u,dx);wf_set_be16(t,O_X,(uint16_t)(wf_be16(t,O_X)+dx));
        return st==WF_OK?WF_OK:block(tr,0x26aeu);
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_MOTION,2u);
    if(low(a,O_FRAME)==3u){
        wf_set_be16(a,0x42u,0xfff0u);if(!bit(a,0x37u,4u))return WF_OK;
        sound_1110e(rt,a); /* $15486 */
        wf_set_be16(a,O_TIMER,0u);wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x42u,0u);
        unsigned route=wf_be16(a,0x44u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(route<<2));
        if(route==0u){
            pin_clear(a,0x34u,8u);if(!t)return block(tr,0x154bcu);
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x1cu);wf_set_be16(t,0x68u,0x12u);
            rt->g.global_15d2=0xfu;rt->g.global_15d3=0x19u;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x32u);wf_runtime_emit_audio(rt,0x2bu); /* $154DA */wf_runtime_emit_audio(rt,0x32u); /* $154E4 */
            wf_status_t st=raw1b_handover_21424(rt,a,tr);if(st!=WF_OK)return st;
        }else if(route==1u){
            if(!t)return block(tr,0x154f4u);
            if(low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x53u){
                uint16_t ox=wf_be16(a,O_X),tx=wf_be16(t,O_X),oy=wf_be16(a,O_Y),ty=wf_be16(t,O_Y);
                uint16_t dx=ox>=tx?(uint16_t)(ox-tx):(uint16_t)(tx-ox);
                if((uint8_t)dx<0x18u){
                    uint16_t dy=oy>=ty?(uint16_t)(oy-ty):(uint16_t)(ty-oy);
                    if((uint8_t)dy<0x14u){
                        wf_r17_m68k_move_w(&rt->m68k_regs,0u,dy);
                        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,3u);
                        wf_actor_raw_t*other=actor_from_ptr(rt,wf_be32(t,O_LINK));if(!other)return block(tr,0x15540u);
                        wf_set_be16(other,O_STATE,5u);wf_set_be16(other,O_SELECTOR,0x6fu);
                        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);wf_set_be16(t,0x68u,0x12u);
                    }
                }
            }
        }else if(route==2u){
            if(!t)return block(tr,0x15562u);
            if(!bit(t,0x32u,0u)&&(!(rt->g.mode_0161&2u)||bit(t,0x33u,2u))&&
               (low(t,O_STATE)<=1u||(low(t,O_STATE)==4u&&low(t,0x64u)==1u))){
                uint16_t ox=wf_be16(a,O_X),tx=wf_be16(t,O_X),oy=wf_be16(a,O_Y),ty=wf_be16(t,O_Y);
                uint16_t dx=ox>=tx?(uint16_t)(ox-tx):(uint16_t)(tx-ox),dy=oy>=ty?(uint16_t)(oy-ty):(uint16_t)(ty-oy);
                if(dx<0x30u&&dy<0x30u){wf_runtime_emit_audio(rt,0x2bu); /* $155D0 */wf_runtime_emit_audio(rt,0x32u); /* $155DA */
                    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x32u);
                    wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x20u);wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(t,O_FACING,7u);
                    wf_set_be16(t,0x68u,0x12u);wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
                    if(bit(t,0x74u,7u)){
                        wf_set_be16(t,0x74u,0u);uint32_t object=wf_be32(t,0x76u);wf_actor_raw_t*o=actor_from_ptr(rt,object);
			if(!o)o=object_from_ptr(b,object&0x00ffffffu);
			if(!o)return block(tr,0x15614u);
                        wf_set_be16(o,O_LATCH,2u);wf_set_be32(t,0x76u,0u);setbit(t,O_FLAGS18,7u);
                    }
                    rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=0xfu;return WF_OK;
                }
            }
        }else return block(tr,0x154a4u);
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_set_be16(a,0x4au,0u);setbit(a,O_LINK,7u);}
    return WF_OK;
}

/* $12614[$1A] -> $1520C, callback $15228..$15382. Selector bit7 has
 * priority over the animation latch, retaining the original held-pin phase. */
static const desc_t D_1520C={0x1520cu,1u,5u,{12u,8u,10u,10u,12u},
    {0x144u,0x145u,0x146u,0x147u,0x148u}};
static wf_status_t raw1a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1520C;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x68u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(bit(a,O_SELECTOR,7u)){
        if(!bit(a,0xfeu,7u)){
            if(!t)return block(tr,0x15318u);
            if(!bit(t,0x32u,4u))return WF_OK;
        }
        if(!t)return block(tr,0x15320u);
        rt->g.global_15d2=wf_u8(t,3u);rt->g.global_15d3=41u;
        pin_clear(a,0x35u,1u);goto release;
    }
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(a,0x19u,32u);wf_set_u8(a,O_B1B,96u);
        if(!t)return block(tr,0x1524au);
        wf_set_be16(t,O_STATE,0xffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xfcu));
        wf_set_be16(a,0xfcu,(uint16_t)(wf_be16(a,0xfcu)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,
            rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x87u);return WF_OK;}
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=35u;return WF_OK;
    }
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        sound_1110e(rt,a); /* $15296 */
        rt->g.global_1800=14u;wf_set_u8(a,0x19u,40u);
    }
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if(!t)return block(tr,0x152b4u);
    wf_set_be16(t,0x68u,22u);
    /* $2052 sound32 precedes the original215B6 transaction. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $152BE */
    if(!bit(a,0x33u,2u)&&bit(a,0x33u,0u)&&bit(t,0x33u,0u)){
        setbit(a,O_SELECTOR,7u);wf_set_be16(t,O_STATE,5u);
        wf_set_be16(t,O_SELECTOR,0x5eu);wf_set_be16(t,0x68u,22u);
        setbit(a,0x35u,0u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);
        /* Original215B6 Royal branch216E8 returns unless LOWselector48;
         * raw1A never enters that separate pin-candidate/RNG owner. */
        if(rt->g.mode_0161&1u)return WF_OK;
        return pin_helpers(rt,a,0,tr);
    }
release:
    setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,7u);
    wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,6u);
    chgbit(a,O_FACING,7u);interrupt_position(rt,a,t,-96,1,0);
    setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);
    wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));setbit(t,0x64u,5u);setbit(t,0x32u,3u);
    return WF_OK;
}

/* $12614[$30] -> $16E28, callback $16E48..$16FD6. */
static const desc_t D_16E28={0x16e28u,1u,6u,{16u,8u,6u,18u,22u,14u},
    {0x10du,0x10eu,0x10fu,0x1f0u,0x1f1u,0x1f2u}};
static wf_status_t raw30(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_16E28;
    uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(a,O_B1B,32u);
        if(!t&&(link&0x00ffffffu))return block(tr,0x1141au);
        /* $11412 reads the target through the 24-bit bus, then compares
         * its full raw back-link to A0; it never dereferences that back-link. */
        if(!t||wf_be32(t,O_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))){
            setbit(a,O_LINK,7u);wf_set_be16(a,O_STATE,0u);*dp=&D_125C0;return WF_OK;
        }
        if(!t)return block(tr,0x16e62u);
        wf_set_be16(t,O_STATE,0xffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xeau));
        wf_set_be16(a,0xeau,(uint16_t)(wf_be16(a,0xeau)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,
            rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x7eu);return WF_OK;}
        if(wf_be16(a,0x46u)){
            setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);
            wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(a,O_FACING,7u);
            wf_set_be16(a,O_X,wf_be16(t,O_X));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffe8u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            helper_10bd0(a,-24,0,0);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffc8u);
            st=throw2f_offset(rt,a,0xffc8u,tr);if(st!=WF_OK)return st;
        }
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=4u;return WF_OK;
    }
    if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER))wf_set_u8(a,0x19u,0xe0u);
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){wf_runtime_emit_audio(rt,0x2bu); /* $16F1C */}
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,0x19u,0u);if(!t)return block(tr,0x16f38u);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x13u);
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        wf_runtime_emit_audio(rt,0x32u); /* $16F58 */
        wf_set_be16(t,0x68u,(wf_u8(a,3u)==0u||wf_u8(a,3u)==5u)?19u:17u);
        setbit(t,0x64u,5u);setbit(t,0x32u,3u);
        interrupt_position(rt,a,t,80,0,16);pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?56u:0xffc8u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-56,0,0);
        wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
        chgbit(a,O_FACING,7u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));
        setbit(a,O_FLAGS18,7u);setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* $12614[$3B] -> $17CF6/$17D12. BYTE45 selects setup, pursuit,
 * then the original submission/result owner. Descriptors change per phase. */
static const desc_t D_17CF6={0x17cf6u,1u,5u,{8u,12u,32u,24u,24u},
    {0x19bu,0x19cu,0x19du,0x19eu,0x19fu}};
static wf_status_t raw3b_speed_1174c(wf_runtime_t*rt,wf_actor_raw_t*a,
                                    wf_r17_actions_r11_trace_t*tr){
    static const uint8_t speed[12]={27u,29u,28u,25u,30u,27u,24u,30u,22u,27u,28u,27u};
    unsigned id=wf_be16(a,2u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)id);
    wf_set_be16(a,0x2au,0u);if(id>=12u)return block(tr,0x1176au);
    uint16_t value=bit(a,0x32u,3u)?12u:speed[id];
    if(bit(a,0x56u,7u)&&bit(a,0x33u,5u))value=(uint8_t)(value+6u);
    if(bit(a,0x74u,7u))value=(uint16_t)(value-8u);
    wf_set_be16(a,0x2au,value);return WF_OK;
}
static wf_status_t raw3b(wf_runtime_t*rt,wf_actor_raw_t*a,
                         wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_17CF6;
    int prior=bit(a,O_SELECTOR,5u);setbit(a,O_SELECTOR,5u);
    if(!prior)wf_set_be16(a,0x44u,0u);
    unsigned sub=low(a,0x44u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(sub<<2));
    if(sub>=3u)return block(tr,0x17d2cu);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(sub==0u){
        *dp=&D_1603A;
        if(!bit(a,O_LATCH,7u)){
            wf_set_u8(a,O_MOTION,0u);if(!t)return block(tr,0x17d4au);
            wf_set_be16(t,O_STATE,0xffu);wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffe0u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            helper_10bd0(a,-32,0,0);return WF_OK;
        }
        if(low(a,O_FRAME)==2u&&!wf_be16(a,O_TIMER)){
            if(!t)return block(tr,0x17d70u);
            wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x8du);
            wf_set_be16(t,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_runtime_emit_audio(rt,0x32u); /* $17D84 */chgbit(t,O_FACING,7u);
            interrupt_position(rt,a,t,-16,0,64);
            pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        }
        if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,5u);wf_set_u8(a,0x45u,(uint8_t)(wf_u8(a,0x45u)+1u));
            wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));wf_r17_m68k_move_w(&rt->m68k_regs,0u,32u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            helper_10bd0(a,32,0,0);
        }
        return WF_OK;
    }
    if(sub==1u){
        if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));}
        if(!t)return block(tr,0x17deeu);
        if(!bit(t,O_SELECTOR,7u)){*dp=&D_11DCA;return WF_OK;}
        prior=bit(a,0x44u,7u);setbit(a,0x44u,7u);
        if(!prior){
            wf_set_u8(a,O_MOTION,1u);wf_status_t st=raw3b_speed_1174c(rt,a,tr);if(st!=WF_OK)return st;
            uint16_t x=(uint16_t)(wf_be16(t,O_X)+(bit(t,O_FACING,7u)?-80:80));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,x);wf_set_be16(a,0xbeu,x);wf_set_be16(a,0xc0u,wf_be16(t,O_Y));
        }
        *dp=ring_walk_descriptor(a);
        if(wf_cpu_arrived_1f15c(rt,a)){
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_STATE,5u);
            wf_set_u8(a,0x45u,(uint8_t)(wf_u8(a,0x45u)+1u));
        }
        return WF_OK;
    }
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);if(!t)return block(tr,0x17e5cu);
        wf_set_be16(t,O_STATE,0xffu);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=0x24u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,49u);wf_runtime_emit_audio(rt,0x31u); /* $17E76 */wf_set_be16(a,O_X,wf_be16(t,O_X));wf_set_be16(a,O_Y,wf_be16(t,O_Y));
        wf_set_u8(a,0x19u,32u);wf_set_u8(a,O_FACING,wf_u8(t,O_FACING));
        setbit(a,0x33u,6u);setbit(t,0x33u,6u);return WF_OK;
    }
    if(bit(a,O_SELECTOR,7u))wf_set_be16(a,O_PROFILE,0x801fu);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if(!bit(a,0xfeu,7u)){
        wf_set_be16(a,O_FRAME,2u);wf_set_be16(a,O_TIMER,0u);if(!t)return block(tr,0x17eceu);
        wf_set_be16(t,0x68u,5u);int carry=0;
        wf_status_t st=action22_result_111c8(rt,a,t,b,&carry,tr);
        if(st!=WF_OK){
            /* This caller exposes the in-helper D0 prefix on failure,
             * before $1C03E has restored the caller's saved register. */
            uint32_t dep=tr->blocked_dependency_rom;
            if(dep==0x1123cu||dep==0x1124cu||(dep==0x21368u&&!(rt->g.mode_0161&1u)))wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);
            if(dep==0x2503cu)wf_r17_m68k_move_w(&rt->m68k_regs,0u,38u);
            return st;
        }
        if(!carry){
            prior=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);if(prior)return WF_OK;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $17F4E */setbit(a,0x35u,1u);setbit(t,0x35u,2u);
            wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x62u);setbit(t,0x64u,5u);setbit(t,0x32u,3u);
            st=action22_partner_215b6(rt,a,tr);
            if(st!=WF_OK){
                if(tr->blocked_dependency_rom==0x2172cu){
                    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));
                    wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
                    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(wf_be16(p,0x70u)<<1));
                }
                return st;
            }
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,64u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
            wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,64,0,0);
            wf_set_be16(t,O_X,wf_be16(a,O_X));wf_set_u8(a,0x19u,0xd0u);return WF_OK;
        }
        setbit(a,0x18u,7u);
        if(rt->g.mode_0161&1u){rt->g.global_15d2=wf_u8(t,3u);rt->g.global_15d3=0x29u;setbit(t,0x32u,4u);}
    }
    wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x6fu);wf_set_be16(a,O_VISUAL,0x4du);
    wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));if(!t)return block(tr,0x17f1cu);
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x6eu);
    pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);pin_clear(a,0x35u,2u);pin_clear(t,0x35u,4u);
    return WF_OK;
}

/* $12614[$3A] -> $17BF8, callback $17C18..$17CF4. Landing RNG observes
 * the original dispatcher selector*4 in D0 before any phase4 sound/position. */
static const desc_t D_17BF8={0x17bf8u,1u,6u,{8u,8u,8u,16u,12u,16u},
    {0x107u,0x108u,0x197u,0x198u,0x199u,0x222u}};
static wf_status_t raw3a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_17BF8;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xe8u);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);if(!t)return block(tr,0x17c24u);
        wf_set_be16(t,O_STATE,0xffu);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=33u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,64u);
        return throw2f_offset(rt,a,64u,tr);
    }
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){wf_runtime_emit_audio(rt,0x2bu); /* $17C5A */}
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        if(!t)return block(tr,0x17c6eu);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x15u);
        setbit(t,0x64u,5u);setbit(t,0x32u,3u);
        wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        wf_runtime_emit_audio(rt,0x32u); /* $17C96 */wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0x68u,21u);
        wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(t,O_FACING,7u);
        interrupt_position(rt,a,t,80,-1,40);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);
        wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0xffe0u:32u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,32,0,0);
    }
    return WF_OK;
}

/* $17000..$1716A: original raw31 capture/release controller. The dispatcher
 * retains the original descriptor through all next-state changes. */
static wf_status_t raw31(wf_runtime_t *rt,wf_actor_raw_t *a,const desc_t **dp,
                         wf_r17_actions_r11_trace_t *tr){
    *dp=&D_16FD8;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xc4u); /* $1C08E selector*4. */
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be32(a,0x92u,0u);wf_set_u8(a,O_B1B,32u);
        return WF_OK;
    }
    if(!bit(a,O_SELECTOR,7u)&&!wf_be16(a,O_FRAME)){
        wf_set_be16(a,O_PROFILE,0x1bu);
        uint32_t relation=wf_be32(a,0x92u);
        if(relation){
            wf_actor_raw_t*t=actor_from_ptr(rt,relation);
            if(!t)return block(tr,0x17038u);
            if(low(t,O_STATE)==4u&&low(t,0x64u)==0x28u){
                wf_set_be32(a,O_LINK,relation);
                wf_set_be32(t,O_LINK,wf_actor_rom_base((unsigned)(a-rt->actors)));
                setbit(a,0x33u,6u);setbit(t,0x33u,6u);wf_set_be16(t,O_STATE,0xffu);
                setbit(a,O_SELECTOR,7u);wf_set_be16(a,O_TIMER,0u);
                wf_set_u8(a,O_FRAME+1u,(uint8_t)(low(a,O_FRAME)+1u));
                return WF_OK;
            }
        }
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));setbit(a,O_LINK,7u);
        wf_set_be16(a,O_TIMER,2u);return WF_OK;
    }
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        /* $258E preset$22 at$2696 preserves D; negate VX by own facing. */
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x180u:0xfe80u);
        wf_set_be16(a,0x5au,0u);wf_set_be16(a,0x5cu,0x280u);
        wf_set_be16(a,0x5eu,0x50u);wf_set_u8(a,O_MOTION,2u);
    }
    if(bit(a,0x37u,4u)){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);rt->g.global_1800=14u;
    }
    if(low(a,O_FRAME)==6u&&!wf_be16(a,O_TIMER)){
        sound_1110e(rt,a); /* $170E6 */
        /* $1110E is a sound-device-only call. A2 still names +26 as loaded
         * by $1C094; an invalid actor bus target remains fail-closed. */
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!t)return block(tr,0x170fau);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2au);
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_be16(t,0x68u,22u);
        setbit(t,0x32u,3u);setbit(t,0x64u,5u);
        wf_status_t status=landing_1108c(rt,a,tr);if(status!=WF_OK)return status;
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));
            if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        interrupt_position(rt,a,t,16,1,0);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,7u);
        wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        setbit(a,O_LINK,7u);
    }
    return WF_OK;
}

/* Only raw32's reachable branches of $17194..$17264. Its terminalFE
 * branch skips target release; the third phase1 hit owns that release. */
static wf_status_t raw32(wf_runtime_t *rt,wf_actor_raw_t *a,const desc_t **dp,
                         wf_r17_actions_r11_trace_t *tr){
    *dp=&D_1716C;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xc8u); /* $1C08E selector*4. */
    if(!bit(a,O_LATCH,7u)){
        int tagged=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
        if(tagged)return WF_OK;
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x44u,3u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!t)return block(tr,0x171b0u);
        wf_set_be16(t,O_STATE,0xffu);return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){wf_runtime_emit_audio(rt,0x2au); /* $171CA */}
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!t)return block(tr,0x171deu);
        wf_set_be16(t,0x68u,5u);
        uint16_t remaining=(uint16_t)(wf_be16(a,0x44u)-1u);
        wf_set_be16(a,0x44u,remaining);
        if(remaining){wf_set_be16(a,O_LATCH,5u);return WF_OK;}
        wf_set_u8(a,O_B1B,32u);wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,2u);
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));
            if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        interrupt_position(rt,a,t,-16,-1,0);
        pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));setbit(a,O_LINK,7u);
    }
    return WF_OK;
}
/* $17194, selector $42: the linked wrestler becomes inactive during this
 * reaction. At the terminal pose this selector releases and repositions the
 * linked wrestler; selector $32 skips that release. */
static wf_status_t raw42(wf_runtime_t *rt,wf_actor_raw_t *a,const desc_t **dp,
                         wf_r17_actions_r11_trace_t *tr){
    *dp=&D_17180;
    if(!bit(a,O_LATCH,7u)){
        int already=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
        if(already)return WF_OK;
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x44u,3u);
        wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!linked)return block(tr,0x171b0u);
        wf_set_be16(linked,O_STATE,0xffu);return WF_OK;
    }
    if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){wf_runtime_emit_audio(rt,0x2au); /* $171CA */}
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!linked)return block(tr,0x171deu);
        wf_set_be16(linked,0x68u,4u);
        uint16_t remaining=(uint16_t)(wf_be16(a,0x44u)-1u);
        wf_set_be16(a,0x44u,remaining);
        if(remaining){wf_set_be16(a,O_LATCH,5u);return WF_OK;}
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));setbit(a,O_LINK,7u);
        wf_actor_raw_t *linked=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!linked)return block(tr,0x17238u);
        wf_set_be16(linked,O_STATE,4u);wf_set_be16(linked,0x64u,2u);
        if(!(rt->g.mode_0161&1u)){
            wf_actor_raw_t *partner=actor_from_ptr(rt,wf_be32(linked,0x86u));
            if(!partner)return block(tr,0x110eeu);
            if(low(partner,O_STATE)==1u&&low(partner,0xaeu)==1u)
                setbit(partner,0x34u,5u);
        }
        interrupt_position(rt,a,linked,-16,-1,0);
        pin_clear(a,0x33u,0x40u);pin_clear(linked,0x33u,0x40u);
    }
    return WF_OK;
}

/* $1662C: preserve the entering run descriptor while rope/contact and the
 * signed decrement of +$2A select the next state. Init intentionally does
 * not clear motion: the preceding controller owns that velocity. */
static const desc_t D_16614={0x16614u,1u,4u,{4u,6u,4u,0xff00u},{0xa7u,0xa8u,0xa9u,0xaau}};
static wf_status_t raw2a(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_16614;if(!bit(a,O_LATCH,7u))return WF_OK;
    wf_set_be16(a,O_PROFILE,wf_be16(a,O_FRAME)?19u:3u);
    if(raw75_edge_10fc6(rt,a)){wf_set_be16(a,O_STATE,6u);return WF_OK;}
    if(low(a,O_FRAME)==3u){
        uint16_t remaining=(uint16_t)(wf_be16(a,0x2au)-3u);wf_set_be16(a,0x2au,remaining);
        if(remaining&0x8000u){
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x2au,0u);
            wf_set_be16(a,O_STATE,0u);setbit(a,O_LINK,7u);
        }
    }
    return WF_OK;
}

/* $17B70: partner attack. The next animation byte selects effect21..23;
 * D1.B is live across the output callback, including its original high word. */
static const desc_t D_17B5C={0x17b5cu,1u,3u,{6u,4u,10u},{0x1e2u,0x1e3u,0x1e4u}};
static wf_status_t raw39(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,
                         const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_17B5C;
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);return WF_OK;}
    wf_set_be16(a,O_PROFILE,wf_be16(a,O_FRAME)==2u?25u:0u);
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,1u);wf_set_be16(a,0xaeu,1u);setbit(a,O_LINK,7u);
        wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));return WF_OK;
    }
    uint8_t phase=low(a,O_FRAME);if(!wf_be16(a,O_TIMER))phase=(uint8_t)(phase+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,phase);
    unsigned effect=(phase==0u||phase==3u)?21u:phase==1u?22u:23u;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)effect);
    return entry_effect(rt,a,effect,b,tr);
}

/* $14670: four-pose jumping strike, landing against the linked opponent. */
static const desc_t D_14658={0x14658u,1u,4u,{8u,8u,40u,0xff00u},
    {0x84u,0x85u,0xefu,0xf0u}};
static wf_status_t raw11(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_14658;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x14694u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);rt->g.global_1670=0u;
        pin_clear(a,0x32u,2u);wf_set_be16(a,0x5cu,0x700u);
        wf_set_be16(a,0x5eu,0x48u);
        uint16_t route=low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x61u?5u:10u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
        if(wf_r17_trajectory_26ae(a,t,route)!=WF_OK)return block(tr,0x26aeu);
        return WF_OK;
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_MOTION,2u);setbit(a,0x33u,3u);return WF_OK;
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_B1B,0x20u);wf_set_be16(a,0x42u,0x20u);
    }
    if(low(a,O_FRAME)==3u){
        if(!bit(a,0x37u,4u))return WF_OK;
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,0x42u,0u);
        sound_1110e(rt,a); /* $1470A */
        rt->g.global_1800=14u;wf_set_be16(a,0x4au,0u);
        if(wf_r17_helper_11152_carry(a,t)){
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,24u);
            wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
            wf_set_be16(t,O_STATE,4u);
            wf_set_be16(t,0x64u,(uint16_t)((wf_be16(t,0x64u)&255u)-2u));
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
            wf_set_be16(t,0x68u,13u);return WF_OK;
        }
        if(jump_hit_11192(a,t)){
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,24u);
            wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
            wf_set_be16(a,O_B1B,12u);wf_set_be16(t,O_STATE,5u);
            wf_set_be16(t,O_SELECTOR,0x77u);wf_set_be16(t,0x68u,13u);
            return WF_OK;
        }
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_set_be16(a,0x68u,4u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $147D0 */
        rt->g.global_15d2=15u;rt->g.global_15d3=26u;
        setbit(a,O_LINK,7u);return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);
        wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }
    return WF_OK;
}

/* $14B64: five-pose aerial grapple. The first contact either transfers
 * the victim to state4/7 or rebounds the attacker; bit7 prevents a second
 * contact after the successful transfer. */
static const desc_t D_14B48={0x14b48u,1u,5u,
    {8u,16u,8u,0xff00u,8u},{0x84u,0x85u,0x191u,0x192u,0x192u}};
static wf_status_t raw14(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_14B48;
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!t)return block(tr,0x14b80u);
    if(bit(a,O_SELECTOR,7u))goto terminal;
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        uint16_t destination=(uint16_t)(wf_be16(t,O_X)+
            (bit(t,O_FACING,7u)?-56:56));
        if(destination>=wf_be16(a,O_X))setbit(a,O_FACING,7u);
        else pin_clear(a,O_FACING,0x80u);
        wf_set_be16(a,0x5cu,0x500u);wf_set_be16(a,0x5eu,0x48u);
        wf_set_be16(a,O_X,destination);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,72u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,72,-1,0);
        uint16_t target_facing=wf_be16(t,O_FACING);
        uint16_t facing_xor=(uint16_t)(wf_be16(a,O_FACING)^target_facing);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,facing_xor);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,target_facing);
        uint16_t route=facing_xor?4u:3u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
        if(wf_r17_trajectory_26ae(a,t,route)!=WF_OK)return block(tr,0x26aeu);
        return WF_OK;
    }
    if(!wf_be16(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_MOTION,2u);setbit(a,0x33u,3u);
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,O_B1B,0x20u);
    if(low(a,O_FRAME)==3u){
        wf_set_be16(a,0x42u,0x20u);
        if(!bit(a,0x37u,4u))return WF_OK;
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,0x42u,0u);
        sound_1110e(rt,a); /* $14C2E */
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
        if(wf_r17_helper_11152_carry(a,t)){
            setbit(a,O_SELECTOR,7u);wf_set_be16(t,O_STATE,4u);
            wf_set_be16(t,0x64u,7u);
            wf_set_be16(t,0xd8u,(uint16_t)(wf_be16(t,0xd8u)+1u));
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $14C66 */
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,42u);wf_runtime_emit_audio(rt,0x2au); /* $14C70 */
            wf_set_be16(t,0x68u,11u);return WF_OK;
        }
        wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        setbit(a,O_LINK,7u);chgbit(a,O_FACING,7u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffe0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-32,0,0);wf_set_be16(t,0x68u,3u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $14CBC */
        rt->g.global_15d2=15u;rt->g.global_15d3=26u;
    }
terminal:
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);}
    return WF_OK;
}

/* $14826: raw12 jumping attack. The early sound gate tests TIMER's high
 * BYTE, and successful hits/terminal exits retain147E6 rather than selecting
 * the normal-path alternate14806. */
static const desc_t D_147E6={0x147e6u,1u,6u,{12u,12u,12u,12u,32u,0xff00u},
    {0x8bu,0x8cu,0x8eu,0x8fu,0x15fu,0x1b1u}};
static const desc_t D_14806={0x14806u,1u,6u,{12u,12u,12u,12u,32u,0xff00u},
    {0x86u,0x87u,0x89u,0x8au,0x15fu,0x1b1u}};
static wf_status_t raw12(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_147E6;
    if(!bit(a,O_LATCH,7u)){wf_set_u8(a,O_MOTION,0u);goto normal;}
    if(low(a,O_FRAME)==1u&&!wf_u8(a,O_TIMER)&&!bit(a,WF_A_ROLE,7u)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,49u);wf_runtime_emit_audio(rt,0x31u); /* $14850 */wf_set_be16(a,O_FRAME,3u);
    }
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){
        wf_set_u8(a,O_MOTION,2u);rt->g.global_1670=0u;
        pin_clear(a,0x32u,2u);setbit(a,0x33u,3u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,32u);helper_10bd0(a,0,0,32);
        wf_set_be16(a,0x5cu,0x580u);wf_set_be16(a,0x5eu,0x48u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x148a2u);
        uint16_t route=low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x61u?5u:2u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
        if(wf_r17_trajectory_26ae(a,t,route)!=WF_OK)return block(tr,0x26aeu);
        goto normal;
    }
    if(low(a,O_FRAME)==5u){
        if(!bit(a,0x37u,4u))goto normal;
        sound_1110e(rt,a); /* $148D4 */
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x4au,0u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x148e2u);
        if(wf_r17_helper_11152_carry(a,t)){
            wf_set_be16(a,O_TIMER,24u);wf_set_be16(t,O_STATE,4u);
            wf_set_be16(t,0x64u,(uint16_t)((wf_be16(t,0x64u)&255u)-2u));
            wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0xd2u,(uint16_t)(wf_be16(t,0xd2u)+1u));
            wf_set_be16(t,0x68u,10u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $14920 */return WF_OK;
        }
        if(jump_hit_11192(a,t)){
            wf_set_be16(a,O_TIMER,24u);wf_set_be16(t,O_STATE,5u);
            wf_set_be16(t,0x64u,0x77u); /* Literal $14940 writes +64, not +60. */
            wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);
            wf_set_be16(t,0x68u,10u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $14960 */
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));return WF_OK;
        }
        setbit(a,O_LINK,7u);wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);wf_set_be16(a,0x68u,4u);wf_runtime_emit_audio(rt,0x32u); /* $149A2 */
        rt->g.global_15d2=15u;rt->g.global_15d3=26u;chgbit(a,O_FACING,7u);
        wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-48,0,0);return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){wf_set_be16(a,O_STATE,7u);setbit(a,O_LINK,7u);return WF_OK;}
normal:
    if(bit(a,0x45u,1u))*dp=&D_14806;
    return WF_OK;
}

/* $12614[$47] -> $14440/$14458. The shared ROM callback uses the
 * raw47-specific F8/F0 hit window and target WORD E2 counter. */
static const desc_t D_14440={0x14440u,1u,4u,{8u,12u,0xff00u,16u},
    {0x84u,0x85u,0x1c1u,0x1c2u}};
static wf_status_t raw47(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_14440;
    if(!bit(a,O_SELECTOR,7u)){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!bit(a,O_LATCH,7u)){
            if(!t)return block(tr,0x1447eu);
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x5cu,0x600u);wf_set_be16(a,0x5eu,0x48u);
            uint16_t route=low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x61u?5u:2u;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
            wf_status_t st=wf_r17_trajectory_26ae(a,t,route);
            if(st!=WF_OK)return block(tr,0x26aeu);
            wf_set_be16(a,O_FACING,(wf_be16(a,0x58u)&0x8000u)^0x8000u);return WF_OK;
        }
        if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
            wf_set_u8(a,O_MOTION,2u);return WF_OK;
        }
        if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
            wf_set_u8(a,0x19u,0xf8u);wf_set_u8(a,O_B1B,0xf0u);
        }
        if(bit(a,0x37u,4u)){
            setbit(a,0x18u,7u);wf_set_u8(a,O_MOTION,0u);
            sound_1110e(rt,a); /* $14500 */
            if(!t)return block(tr,0x11156u);
            if(wf_r17_helper_11152_carry(a,t)){
                /* Effective bus address and raw reciprocal LONG are distinct. */
                if(wf_be32(t,O_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors)))goto miss47;
                wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);
                wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
                wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,(uint16_t)((wf_be16(t,0x64u)&255u)-2u));
                wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0x68u,10u);wf_runtime_emit_audio(rt,0x2bu); /* $14552 */wf_runtime_emit_audio(rt,0x32u); /* $1455C */
                /* $14562 re-reads selector after the target writes. */
                size_t counter=low(a,O_SELECTOR)==0x46u?0xe0u:0xe2u;
                wf_set_be16(t,counter,(uint16_t)(wf_be16(t,counter)+1u));return WF_OK;
            }
            if(jump_hit_11192(a,t)){
                wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);
                wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);
                wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);wf_set_be16(t,0x68u,10u);wf_runtime_emit_audio(rt,0x2bu); /* $145B2 */wf_runtime_emit_audio(rt,0x32u); /* $145BC */
                wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));return WF_OK;
            }
miss47:
            wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);wf_set_be16(a,0x68u,3u);wf_runtime_emit_audio(rt,0x32u); /* $1461C */
            rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);chgbit(a,O_FACING,7u);
            wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
            /* $14642: the backstep belongs only to raw46. */
            if(low(a,O_SELECTOR)==0x46u){
                wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
                helper_10bd0(a,-48,0,0);
            }
            return WF_OK;
        }
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);
        wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));setbit(a,O_LINK,7u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,24u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,24,-1,0);
    }
    return WF_OK;
}

/* $12614[$46] -> $14424/$14458. Only raw46 is admitted here; raw47
 * shares the ROM callback but owns a different descriptor and counters. */
static const desc_t D_14424={0x14424u,1u,5u,{8u,12u,16u,0xff00u,8u},
    {0x84u,0x85u,0x1b0u,0x1b1u,0x1b1u}};
static wf_status_t raw46(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_14424;
    if(!bit(a,O_SELECTOR,7u)){
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
        if(!bit(a,O_LATCH,7u)){
            if(!t)return block(tr,0x1447eu);
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,0x5cu,0x600u);wf_set_be16(a,0x5eu,0x48u);
            uint16_t route=low(t,O_STATE)==5u&&low(t,O_SELECTOR)==0x61u?5u:2u;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,route);
            wf_status_t st=wf_r17_trajectory_26ae(a,t,route);
            if(st!=WF_OK)return block(tr,0x26aeu);
            wf_set_be16(a,O_FACING,(wf_be16(a,0x58u)&0x8000u)^0x8000u);
            return WF_OK;
        }
        if(!low(a,O_FRAME)&&!wf_be16(a,O_TIMER)){
            wf_set_u8(a,O_MOTION,2u);return WF_OK;
        }
        if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
            wf_set_u8(a,0x19u,0xf0u);wf_set_u8(a,O_B1B,0x28u);
        }
        if(bit(a,0x37u,4u)){
            setbit(a,0x18u,7u);wf_set_u8(a,O_MOTION,0u);
            sound_1110e(rt,a); /* $14500 */
            if(!t)return block(tr,0x11156u);
            if(wf_r17_helper_11152_carry(a,t)){
                /* $11412 resolves target through the 24-bit bus, then
                 * compares its stored LONG back-link literally with A0. */
                if(wf_be32(t,O_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors)))goto miss46;
                wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);
                wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));
                wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,(uint16_t)((wf_be16(t,0x64u)&255u)-2u));
                wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));wf_set_be16(t,0x68u,10u);wf_runtime_emit_audio(rt,0x2bu); /* $14552 */wf_runtime_emit_audio(rt,0x32u); /* $1455C */
                wf_set_be16(t,0xe0u,(uint16_t)(wf_be16(t,0xe0u)+1u));return WF_OK;
            }
            if(jump_hit_11192(a,t)){
                wf_set_be16(a,O_TIMER,0u);setbit(a,O_SELECTOR,7u);
                wf_set_be16(a,O_Y,(uint16_t)(wf_be16(t,O_Y)-1u));wf_set_u8(a,O_B1B,12u);
                wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x77u);wf_set_be16(t,0x68u,10u);wf_runtime_emit_audio(rt,0x2bu); /* $145B2 */wf_runtime_emit_audio(rt,0x32u); /* $145BC */
                wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+1u));return WF_OK;
            }
miss46:
            wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,5u);wf_set_be16(a,0x68u,3u);wf_runtime_emit_audio(rt,0x32u); /* $1461C */
            rt->g.global_15d2=15u;rt->g.global_15d3=26u;setbit(a,O_LINK,7u);chgbit(a,O_FACING,7u);
            wf_set_be16(a,O_VISUAL,0x1fu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffd0u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            helper_10bd0(a,-48,0,0);return WF_OK;
        }
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);wf_set_be16(a,O_VISUAL,0x68u);
        wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));setbit(a,O_LINK,7u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,24u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,24,-1,0);
    }
    return WF_OK;
}

/* $12614[$5C] -> $19436/$1944A. Release retains A1=$19436 while
 * $212A0 restores the original TAG pair or the Royal non-$53 prefix. */
static const desc_t D_19436={0x19436u,1u,3u,{16u,8u,8u},{0x178u,0x179u,0x17au}};
static wf_status_t raw5c(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19436;uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!bit(a,O_LATCH,7u)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x19452u);
        /* Null dispatch A2=$7F000 and tagged physical zero address ROM;
         * their stores cannot mutate the arcade RAM model. */
        if(t){wf_set_be16(t,O_STATE,0xffu);pin_clear(t,0x35u,2u);}
        pin_clear(a,0x35u,4u);return WF_OK;
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x19474u);
        if(t){wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x27u);wf_set_be16(t,0x68u,3u);interrupt_position(rt,a,t,16,-1,0);pin_clear(t,0x33u,0x40u);}
        else{
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0xfff0u:16u);
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        }
        pin_clear(a,0x33u,0x40u);
    }
    if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);
        wf_status_t st;
        if(rt->g.credited_route&&(rt->g.mode_0161&1u))st=action22_cleanup_21358(rt,a,tr);
        else st=interrupt_cleanup_212a0(rt,a,tr);
        if(st!=WF_OK)return st;
        setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }
    return WF_OK;
}

/* $12614[$2B] -> $1667A/$1669A. Preserve the selected descriptor through
 * the original RNG reversal, wall capture, landing and terminal exits. */
static const desc_t D_1667A={0x1667au,1u,6u,{16u,8u,10u,10u,0xff00u,28u},
    {0x11bu,0x11cu,0x11du,0x11eu,0x11fu,0x120u}};
static wf_status_t raw2b(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_1667A;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xacu);
    wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        if(!t&&(wf_be32(a,O_LINK)&0x00ffffffu))return block(tr,0x1141au);
        if(!t||wf_be32(t,O_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))){
            setbit(a,O_LINK,7u);wf_set_be16(a,O_STATE,0u);*dp=&D_125C0;return WF_OK;
        }
        wf_set_be16(t,O_STATE,0xffu);wf_set_u8(a,0x19u,0xb0u);wf_set_u8(a,O_B1B,0x50u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,1u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,0xecu));
        wf_set_be16(a,0xecu,(uint16_t)(wf_be16(a,0xecu)+1u));
        if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(tr,0x1129eu);
        wf_r17_1129e_trace_t rr;int carry=0;
        wf_status_t st=wf_r17_1129e_live(rt,(unsigned)(a-rt->actors),rt->g.mode_0161,
            rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&rr,&carry);
        if(st!=WF_OK)return block(tr,rr.blocked_dependency_rom);
        if(!carry){wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x7fu);return WF_OK;}
        if(wf_be16(a,0x46u)){
            setbit(a,O_LATCH,7u);wf_set_be16(a,O_FRAME,0u);wf_set_be16(a,O_TIMER,0u);chgbit(a,O_FACING,7u);
            interrupt_position(rt,a,t,-160,0,0);setbit(a,0x33u,6u);setbit(t,0x33u,6u);wf_set_u8(a,0x19u,0xf0u);
        }
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=8u;return WF_OK;
    }
    if(low(a,O_FRAME)==3u&&!wf_be16(a,O_TIMER)){
        /* $258E[30], literal row $267E=FF00/0180/0058. */
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,30u);
        wf_set_be16(a,0x58u,bit(a,O_FACING,7u)?0x100u:0xff00u);wf_set_be16(a,0x5au,0u);
        wf_set_be16(a,0x5cu,0x180u);wf_set_be16(a,0x5eu,0x58u);wf_set_u8(a,O_MOTION,2u);
    }
    if(low(a,O_FRAME)==4u&&!(rt->g.mode_0161&2u)){
        wf_set_be16(a,0x3eu,0xffb0u);wf_status_t st=wf_r17_resolver_280dc(rt,a);
        if(st!=WF_OK)return block(tr,0x280dcu);
        wf_set_be16(a,0x3eu,0u);uint8_t contact=(uint8_t)(wf_u8(a,0x37u)&3u);
        wf_r17_m68k_move_b(&rt->m68k_regs,0u,contact);
        if(contact){int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);if(!previous){
            wf_set_be16(a,O_VISUAL,(uint16_t)(0x120u|wf_be16(a,O_FACING)));
            if(!t)return block(tr,0x167aau);
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2025u);setbit(t,0x32u,3u);
            wf_set_be16(t,0x44u,1u);wf_set_be16(t,0x68u,24u);chgbit(t,O_FACING,7u);
            interrupt_position(rt,a,t,wf_be16(a,0x46u)?-64:-112,1,64);
            /* $11058: side1 has priority over side0, WORD stores. */
            if(bit(a,0x37u,1u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}
            else if(bit(a,0x37u,0u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
            pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        }}
    }
    if(low(a,O_FRAME)==4u&&bit(a,0x37u,4u)){
        sound_1110e(rt,a); /* $16814 */
        /* $1110E preserves D0. Landing RNG observes the live resolver or
         * wall-position D0 above, or selector*4 when mode.bit1 bypasses it. */
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_TIMER,0u);
        wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
        if(!(rt->g.mode_0161&1u)){
            if(!t)return block(tr,0x110eeu);
            wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110eeu);
            if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
        }
        rt->g.global_1800=14u;
        if(!bit(a,O_SELECTOR,7u)){
            if(!t)return block(tr,0x1683cu);
            wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,0x2024u);setbit(t,0x32u,3u);
            wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_be16(t,0x68u,24u);wf_runtime_emit_audio(rt,0x32u); /* $16864 */
            interrupt_position(rt,a,t,wf_be16(a,0x46u)?-128:-176,1,32);
            pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);return WF_OK;
        }
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,0x18u,7u);wf_set_be16(a,O_STATE,7u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xff90u);wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,-112,0,0);setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    }
    return WF_OK;
}

/* $12614[$36] -> $17912/$1792A. Original linked submission/result
 * controller; its incoming A1 remains17912 through every next-state exit. */
static const desc_t D_17912={0x17912u,1u,4u,{8u,16u,20u,20u},{0xf8u,0x12bu,0x12cu,0x12du}};
static wf_status_t raw36(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_actions_r11_bridge_t*b,
                         const desc_t**dp,wf_r17_actions_r11_trace_t*tr){
    *dp=&D_17912;uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!bit(a,O_LATCH,7u)){
        if(!t&&(link&0x00ffffffu))return block(tr,0x1141au);
        /* $11412 compares the stored LONG back-link literally with A0.
         * Null/physical-zero ROM links cannot be reciprocal actor links. */
        if(!t||wf_be32(t,O_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))||
           (low(t,O_SELECTOR)!=0x7bu&&low(t,O_LATCH)!=0xffu)){
            setbit(a,O_LINK,7u);wf_set_be16(a,O_STATE,0u);*dp=&D_125C0;return WF_OK;
        }
        wf_set_u8(a,O_MOTION,0u);wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));
        rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=6u;wf_set_be16(t,O_STATE,0xffu);
        setbit(a,0x35u,1u);setbit(t,0x35u,2u);return WF_OK;
    }
    wf_set_be16(a,O_PROFILE,bit(a,O_SELECTOR,7u)?0x8015u:0u);
    if(low(a,O_FRAME)!=0xfeu)return WF_OK;
    if(bit(a,0xfeu,7u)){
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x6fu);
        if(!t)return block(tr,0x17a32u);
        wf_set_be16(t,O_STATE,0u);
    }else{
        wf_set_be16(a,O_FRAME,1u);wf_set_be16(a,O_TIMER,0u);
        if(!t)return block(tr,0x179c0u);
        wf_set_be16(t,0x68u,5u);
        int carry=0;wf_status_t st=action22_result_111c8(rt,a,t,b,&carry,tr);
        if(st!=WF_OK)return st;
        if(!carry){
            int previous=bit(a,O_SELECTOR,7u);setbit(a,O_SELECTOR,7u);
            if(previous)return WF_OK;
            wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x63u);
            setbit(t,0x64u,5u);setbit(t,0x32u,3u);
            st=action22_partner_215b6(rt,a,tr);
            if(st==WF_OK){wf_runtime_emit_audio(rt,0x32u); /* $17A1E */}
            return st;
        }
        wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x6fu);
        wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x6eu);chgbit(t,O_FACING,7u);
    }
    /* Raw36 has become6F here; neither selector can reach21358's
     * separate53 continuation. Shared unported caller guards stay intact. */
    if(rt->g.credited_route&&(rt->g.mode_0161&1u))return action22_cleanup_21358(rt,a,tr);
    return interrupt_cleanup_212a0(rt,a,tr);
}

/* $12614[$63] aliases $19530/$19536. Its latched $11432/$10D04
 * continuation is identical to the closed raw5F owner; init uses argument7. */
static wf_status_t raw63(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19530;
    if(bit(a,O_LATCH,7u))return raw5f(rt,a,dp,tr);
    setbit(a,O_LATCH,7u);wf_set_be16(a,0x52u,0u);wf_set_u8(a,O_MOTION,0u);
    wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_TIMER,0xff00u);
    if(rt->g.credited_route&&(rt->g.mode_0161&1u)){
        /* $217FE/$21828: use literal raw-LONG candidate exclusions. Raw63
         * cannot enter the $4A-only random branch, and has no zero-link
         * diagnostic write from the distinct $2125C/$21018 callers. */
        uint32_t candidate,dependency=0u;int found;
        wf_status_t st=wf_r17_candidate_2187e(rt,(unsigned)(a-rt->actors),&candidate,&found,&dependency);
        if(st!=WF_OK)return block(tr,dependency);
        if(found){
            wf_actor_raw_t*c=actor_from_ptr(rt,candidate);if(!c)return block(tr,0x21828u);
            wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,candidate);
        }
    }else{
        wf_status_t st=raw5f_prepare_21732(rt,a,tr);if(st!=WF_OK)return st;
    }
    /* $195DA clears AA before $10C60[7]. Literal row $10CFC is4/12/24;
     * the original helper subtracts one after loading the low BYTE. */
    wf_set_be16(a,0xaau,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,7u);
    unsigned tier=wf_be16(a,0x70u);if(tier>=3u)return block(tr,0x10ca8u);
    static const uint16_t recovery[3]={3u,11u,23u};
    wf_set_be16(a,0xaau,recovery[tier]);return WF_OK;
}

/* $12614[$6E] -> $19D7C/$19D82: result victim, kind-zero timer owner.
 * Royal init returns before the result mask; TAG init falls through. */
static const desc_t D_19D7C={0x19d7cu,0u,0u,{0u},{0u}};
static wf_status_t raw6e(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_19D7C;int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_VISUAL,0x4eu);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        pin_clear(a,0x35u,4u);wf_set_be16(a,O_TIMER,0x80u);
        if(rt->g.mode_0161&1u)return WF_OK;
        wf_set_be16(a,O_TIMER,0xffffu);
    }
    if((wf_be16(a,0xfeu)&0x8001u)==0x8000u){wf_set_be16(a,O_STATE,0u);return WF_OK;}
    uint16_t timer=(uint16_t)(wf_be16(a,O_TIMER)-1u);wf_set_be16(a,O_TIMER,timer);
    if(!timer){
        if(!(rt->g.mode_0161&1u))wf_set_be16(a,O_TIMER,0xffffu);
        else{
            wf_set_be16(a,O_STATE,5u);wf_set_be16(a,O_SELECTOR,0x7au);wf_set_be16(a,0x44u,0u);setbit(a,0x32u,4u);
            rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=41u;
        }
    }
    return WF_OK;
}

/* $12614[$6F] -> $19E0E/$19E14: result attacker. Only init consumes
 * A2; the kind-zero 64-count continuation has no external dependency. */
static const desc_t D_19E0E={0x19e0eu,0u,0u,{0u},{0u}};
static wf_status_t raw6f(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19E0E;int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched){
        wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,O_VISUAL,0x4du);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_set_be16(a,O_TIMER,0x40u);
        uint32_t link=wf_be32(a,O_LINK);wf_actor_raw_t*t=actor_from_ptr(rt,link);
        if(!t&&(link&0x00ffffffu))return block(tr,0x19e32u);
        /* Zero LINK gives dispatcher A2=$7F000, ROM+A=$0000. Tagged
         * physical zero reads ROM $00000A=$FFFF. Both are read-only. */
        uint16_t y=t?wf_be16(t,O_Y):link?0xffffu:0u;
        wf_set_be16(a,O_Y,(uint16_t)(y+2u));wf_set_u8(a,O_B1B,32u);pin_clear(a,0x35u,2u);return WF_OK;
    }
    uint16_t timer=(uint16_t)(wf_be16(a,O_TIMER)-1u);wf_set_be16(a,O_TIMER,timer);
    if(!timer){
        setbit(a,0x18u,7u);setbit(a,O_LINK,7u);
        if(bit(a,0x32u,0u)){wf_set_be16(a,O_STATE,1u);wf_set_be16(a,0xaeu,1u);}
        else{wf_set_be16(a,O_STATE,0u);wf_set_be16(a,O_VISUAL,wf_be16(a,O_FACING));}
    }
    return WF_OK;
}

/* $12614[$2C] -> $168D8/$168F8. The descriptor stays selected across
 * release; only init/impact and the TAG terminal helper consume A2. */
static const desc_t D_168D8={0x168d8u,1u,6u,
    {8u,8u,12u,20u,12u,16u},{0x107u,0x108u,0x114u,0x115u,0x116u,0x117u}};
static wf_status_t raw2c(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_168D8;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xb0u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16904u);
        wf_set_be16(t,O_STATE,0xffu);rt->g.global_15d2=wf_u8(a,3u);rt->g.global_15d3=16u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,48u);return throw2f_offset(rt,a,48u,tr);
    }
    if(low(a,O_FRAME)==1u&&!wf_be16(a,O_TIMER))wf_set_u8(a,0x19u,8u);
    if(low(a,O_FRAME)==4u&&!wf_be16(a,O_TIMER)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,43u);wf_runtime_emit_audio(rt,0x2bu); /* $1693A */
        wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x16966u);
        wf_set_be16(t,O_STATE,4u);wf_set_be16(t,0x64u,21u);
        wf_set_u8(t,0xc7u,(uint8_t)(wf_u8(t,0xc7u)+5u));wf_set_be16(t,0x68u,16u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);wf_runtime_emit_audio(rt,0x32u); /* $16982 */
        wf_set_u8(t,O_FACING,wf_u8(a,O_FACING));chgbit(t,O_FACING,7u);
        interrupt_position(rt,a,t,48,-1,40);pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
        return WF_OK;
    }
    if(low(a,O_FRAME)==0xfeu){
        setbit(a,O_FLAGS18,7u);wf_set_be16(a,O_STATE,0u);
        if(wf_u8(a,3u)!=9u){
            wf_status_t st=landing_1108c(rt,a,tr);if(st!=WF_OK)return st;
            if(!(rt->g.mode_0161&1u)){
                wf_actor_raw_t*t=actor_from_ptr(rt,wf_be32(a,O_LINK));if(!t)return block(tr,0x110eeu);
                wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(t,0x86u));if(!p)return block(tr,0x110f2u);
                if(low(p,O_STATE)==1u&&low(p,0xaeu)==1u)setbit(p,0x34u,5u);
            }
        }
        setbit(a,O_LINK,7u);wf_set_be16(a,O_VISUAL,0x68u);wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0xffe0u:32u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);helper_10bd0(a,32,0,0);
    }
    return WF_OK;
}

/* $1AF04 is a compact, private four-frame clock, independent of $1C0C8. */
static void raw8e_script(wf_actor_raw_t*a,const uint8_t table[9]){
    int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched)wf_set_be16(a,O_FRAME,0u);
    else{
        uint16_t timer=wf_be16(a,O_TIMER);
        wf_set_be16(a,O_TIMER,(uint16_t)(timer-1u));
        if(timer)return;
        uint16_t frame=(uint16_t)(wf_be16(a,O_FRAME)+1u);
        wf_set_be16(a,O_FRAME,frame);
        if((uint8_t)frame>=table[0]){
            wf_set_be16(a,O_FRAME,0xfeu);return;
        }
    }
    unsigned i=1u+2u*wf_be16(a,O_FRAME);
    wf_set_be16(a,O_VISUAL,0u);wf_set_be16(a,O_TIMER,0u);
    wf_set_u8(a,O_VISUAL+1u,table[i]);
    wf_set_u8(a,O_TIMER+1u,table[i+1u]);
}
/* $1AE2E: scripted Royal interlude. The foreground clear happens once
 * on entry, in bounded batches so it does not reserve 2K write records. */
static const desc_t D_1AE28={0x1ae28u,0u,0u,{0u},{0u}};
static wf_status_t raw8e(wf_runtime_t*rt,wf_actor_raw_t*a,
                         wf_r17_actions_r11_bridge_t*b,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    static const uint8_t script0[9]={4u,0x11u,16u,0x12u,16u,0x13u,16u,0x14u,16u};
    static const uint8_t script1[9]={4u,7u,16u,6u,16u,5u,16u,4u,16u};
    *dp=&D_1AE28;
    switch(wf_be16(a,0x44u)){
    case 0u:
        if(!bit(a,O_LATCH,7u)){
            if(!b->video_batch)return block(tr,0x1f9eu);
            wf_r17_fg0_write_t writes[64];
            for(unsigned chunk=0;chunk<32u;chunk++){
                for(unsigned i=0;i<64u;i++)writes[i]=(wf_r17_fg0_write_t)
                    {0xc0000u+chunk*256u+i*4u,0u,4u};
                if(b->video_batch(rt,writes,64u,b->video_user)!=WF_OK)
                    return block(tr,0x1f9eu);
            }
            rt->g.global_0072|=0x8000u;
            wf_set_u8(a,O_MOTION,0u);wf_set_be16(a,2u,31u);
            wf_set_be16(a,O_FLAGS18,0u);wf_set_be16(a,0x1au,0u);
            setbit(a,0x32u,5u);setbit(a,0x32u,6u);
            wf_set_be16(a,O_X,0x278u);wf_set_be16(a,O_Y,0x199u);
            wf_set_be16(a,O_Z,0x14eu);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,31u);
            if(!b->resource_2aea||b->resource_2aea(rt,31u,b->video_user)!=WF_OK)
                return block(tr,0x2aeau);
        }else if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,5u);
            wf_set_be16(a,0x44u,(uint16_t)(wf_be16(a,0x44u)+1u));
            return WF_OK;
        }
        raw8e_script(a,script0);return WF_OK;
    case 1u:
        if(!bit(a,O_LATCH,7u)){
            wf_set_be16(a,O_X,0x278u);wf_set_be16(a,O_Y,0x199u);
            wf_set_be16(a,O_Z,0x14eu);
        }else if(low(a,O_FRAME)==0xfeu){
            wf_set_be16(a,O_STATE,5u);
            wf_set_be16(a,0x44u,(uint16_t)(wf_be16(a,0x44u)+1u));
            return WF_OK;
        }
        raw8e_script(a,script1);return WF_OK;
    case 2u:{
        wf_set_be16(a,O_VISUAL,0u);wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(a,O_Z,0x140u);
        wf_actor_raw_t*t=actor_from_ptr(rt,rt->g.global_0092);
        if(!t)return block(tr,0x1af62u);
        if(!low(t,O_STATE)&&!rt->g.global_15d4&&!rt->g.global_15d5){
            wf_set_be16(a,O_STATE,5u);
            wf_set_be16(a,0x44u,(uint16_t)(wf_be16(a,0x44u)+1u));
        }
        return WF_OK;
    }
    case 3u:{
        int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
        if(!latched){
            wf_actor_raw_t*t=actor_from_ptr(rt,rt->g.global_0092);
            if(!t)return block(tr,0x1af8cu);
            rt->g.global_15d2=(uint8_t)(wf_u8(t,3u)+12u);
            rt->g.global_15d3=43u;
            wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0x79u);
            wf_set_be16(a,O_VISUAL,0x800cu);wf_set_be16(a,O_TIMER,0x200u);
        }
        uint16_t timer=(uint16_t)(wf_be16(a,O_TIMER)-1u);
        wf_set_be16(a,O_TIMER,timer);
        if(!timer)rt->g.global_16c4=(uint16_t)((rt->g.global_16c4&0x00ffu)|0x0100u);
        return WF_OK;
    }
    default:return block(tr,0x1ae3au);
    }
}

#include "wf_r18_action87.inc"
#include "wf_r18_action20.inc"
#include "wf_r18_action54.inc"
#include "wf_r18_action62.inc"

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
        case 0x32u:tr->callback_rom=0x17194u;tr->descriptor_rom=0x1716cu;s=raw32(rt,a,&d,tr);break;
        case 0x42u:tr->callback_rom=0x17194u;tr->descriptor_rom=0x17180u;s=raw42(rt,a,&d,tr);break;
        case 0x31u:tr->callback_rom=0x17000u;tr->descriptor_rom=0x16fd8u;s=raw31(rt,a,&d,tr);break;
        case 0x30u:tr->callback_rom=0x16e48u;tr->descriptor_rom=0x16e28u;s=raw30(rt,a,&d,tr);break;
        case 0x3au:tr->callback_rom=0x17c18u;tr->descriptor_rom=0x17bf8u;s=raw3a(rt,a,&d,tr);break;
        case 0x3bu:tr->callback_rom=0x17d12u;tr->descriptor_rom=0x17cf6u;s=raw3b(rt,a,b,&d,tr);break;
        case 0x1au:tr->callback_rom=0x15228u;tr->descriptor_rom=0x1520cu;s=raw1a(rt,a,&d,tr);break;
        case 0x1bu:tr->callback_rom=0x153bcu;tr->descriptor_rom=0x15384u;s=raw1b(rt,a,b,&d,tr);break;
        case 0x3eu:tr->callback_rom=0x1831au;tr->descriptor_rom=0x18314u;s=raw3e(rt,a,&d,tr);break;
        case 0x45u:tr->callback_rom=0x128fau;tr->descriptor_rom=0x128deu;s=raw01_45(rt,a,&d,tr);break;
        case 0x46u:tr->callback_rom=0x14458u;tr->descriptor_rom=0x14424u;s=raw46(rt,a,&d,tr);break;
        case 0x47u:tr->callback_rom=0x14458u;tr->descriptor_rom=0x14440u;s=raw47(rt,a,&d,tr);break;
        case 0x12u:tr->callback_rom=0x14826u;tr->descriptor_rom=0x147e6u;s=raw12(rt,a,&d,tr);break;
        case 0x11u:tr->callback_rom=0x14670u;tr->descriptor_rom=0x14658u;s=raw11(rt,a,&d,tr);break;
        case 0x14u:tr->callback_rom=0x14b64u;tr->descriptor_rom=0x14b48u;s=raw14(rt,a,&d,tr);break;
        case 0x13u:tr->callback_rom=0x149f2u;tr->descriptor_rom=0x149e2u;s=raw13(rt,a,&d,tr);break;
        case 0x5cu:tr->callback_rom=0x1944au;tr->descriptor_rom=0x19436u;s=raw5c(rt,a,&d,tr);break;
        case 0x2bu:tr->callback_rom=0x1669au;tr->descriptor_rom=0x1667au;s=raw2b(rt,a,&d,tr);break;
        case 0x36u:tr->callback_rom=0x1792au;tr->descriptor_rom=0x17912u;s=raw36(rt,a,b,&d,tr);break;
        case 0x63u:tr->callback_rom=0x19536u;tr->descriptor_rom=0x19530u;s=raw63(rt,a,&d,tr);break;
        case 0x64u:tr->callback_rom=0x19618u;tr->descriptor_rom=0x19608u;s=raw64(rt,a,&d,tr);break;
        case 0x65u:tr->callback_rom=0x19670u;tr->descriptor_rom=0x19664u;s=raw_effect_hold(rt,a,b,&d,tr,0x65u);break;
        case 0x6eu:tr->callback_rom=0x19d82u;tr->descriptor_rom=0x19d7cu;s=raw6e(rt,a,&d);break;
        case 0x6fu:tr->callback_rom=0x19e14u;tr->descriptor_rom=0x19e0eu;s=raw6f(rt,a,&d,tr);break;
        case 0x27u:tr->callback_rom=0x1639cu;tr->descriptor_rom=0x16388u;s=raw27(rt,a,&d,tr);break;
        case 0x0fu:tr->callback_rom=0x13f90u;tr->descriptor_rom=0x13f78u;s=raw0f(rt,a,&d,tr);break;
        case 0x2eu:tr->callback_rom=0x16a7au;tr->descriptor_rom=0x16a56u;s=raw2e(rt,a,&d,tr);break;
        case 0x50u:tr->callback_rom=0x18b94u;tr->descriptor_rom=0x18b8eu;s=raw50(rt,a,&d,tr);break;
        case 0x19u:tr->callback_rom=0x15006u;tr->descriptor_rom=0x14fb6u;s=raw19(rt,a,&d,tr);break;
        case 0x2du:tr->callback_rom=0x16a16u;tr->descriptor_rom=0x169fau;s=raw2d(rt,a,&d);break;
        case 0x2cu:tr->callback_rom=0x168f8u;tr->descriptor_rom=0x168d8u;s=raw2c(rt,a,&d,tr);break;
        case 0x07u:tr->callback_rom=0x13116u;tr->descriptor_rom=0x130e6u;s=raw07(a,&d);break;
        case 0x02u:tr->callback_rom=0x12a08u;tr->descriptor_rom=0x129f0u;s=raw02(rt,a,&d,tr);break;
        case 0x01u:tr->callback_rom=0x128fau;tr->descriptor_rom=0x128deu;s=raw01_45(rt,a,&d,tr);break;
        case 0x04u:tr->callback_rom=0x12c22u;tr->descriptor_rom=0x12bdau;s=raw04(rt,a,&d,tr);break;
        case 0x7au:tr->callback_rom=0x1a364u;tr->descriptor_rom=0x1a348u;s=raw7a(rt,a,b,&d,tr);break;
        case 0x05u:tr->callback_rom=0x12f40u;tr->descriptor_rom=0x12f08u;s=raw05(rt,a,&d,tr);break;
        case 0x1eu:
        case 0x1fu:tr->callback_rom=0x15a20u;tr->descriptor_rom=0x159f8u;s=raw1f(rt,a,b,&d,tr);break;
        case 0x21u:tr->callback_rom=0x15b34u;tr->descriptor_rom=0x15b24u;s=raw21(a,&d);break;
        case 0x79u:tr->callback_rom=0x1a28eu;tr->descriptor_rom=0x1a1beu;s=raw79(rt,a,&d,tr);break;
        case 0x7du:tr->callback_rom=0x1a604u;tr->descriptor_rom=0x1a5f4u;s=raw_reversal(rt,a,&d,tr,&D_1A5F4,3u,0x1a632u);break;
        case 0x7eu:tr->callback_rom=0x1a688u;tr->descriptor_rom=0x1a674u;s=raw7e(rt,a,&d,tr);break;
        case 0x7fu:tr->callback_rom=0x1a718u;tr->descriptor_rom=0x1a704u;s=raw7f(rt,a,&d,tr);break;
        case 0x78u:tr->callback_rom=0x1a192u;tr->descriptor_rom=0x1a186u;s=raw78(a,&d);break;
        case 0x34u:tr->callback_rom=0x174aau;tr->descriptor_rom=0x1747eu;s=raw34(rt,a,&d,tr);break;
        case 0x35u:tr->callback_rom=0x175c4u;tr->descriptor_rom=0x175a8u;s=raw35(rt,a,b,&d,tr);break;
        case 0x44u:tr->callback_rom=0x18668u;tr->descriptor_rom=0x1863cu;s=raw44(rt,a,&d,tr);break;
        case 0x8eu:tr->callback_rom=0x1ae2eu;tr->descriptor_rom=0x1ae28u;s=raw8e(rt,a,b,&d,tr);break;
        case 0x75u:tr->callback_rom=0x1a0acu;tr->descriptor_rom=0x1a08cu;s=raw75(rt,a,&d);break;
        case 0x73u:tr->callback_rom=0x1a020u;tr->descriptor_rom=0x1a014u;s=raw_effect_hold(rt,a,b,&d,tr,0x73u);break;
        case 0x74u:tr->callback_rom=0x1a05cu;tr->descriptor_rom=0x1a050u;s=raw_effect_hold(rt,a,b,&d,tr,0x74u);break;
        case 0x09u:tr->callback_rom=0x135ecu;tr->descriptor_rom=0x135c8u;s=raw09(rt,a,b,&d,tr);break;
        case 0x67u:tr->callback_rom=0x1978au;tr->descriptor_rom=0x19776u;s=raw67(rt,a,&d,tr);break;
        case 0x6du:tr->callback_rom=0x19d06u;tr->descriptor_rom=0x19cf2u;s=raw6d(rt,a,b,&d,tr);break;
        case 0x57u:tr->callback_rom=0x1914eu;tr->descriptor_rom=0x1913eu;s=raw57(rt,a,&d,tr);break;
        case 0x76u:tr->callback_rom=0x1a10eu;tr->descriptor_rom=0x1a0feu;s=raw76(rt,a,b,&d,tr);break;
        case 0x58u:tr->callback_rom=0x191c8u;tr->descriptor_rom=0x191b4u;s=raw58(rt,a,b,&d,tr);break;
        case 0x52u:tr->callback_rom=0x18d2eu;tr->descriptor_rom=0x18d12u;s=raw52(rt,a,&d,tr);break;
        case 0x53u:tr->callback_rom=0x18e64u;tr->descriptor_rom=0x18e3cu;s=raw53(rt,a,&d,tr);break;
        case 0x1cu:tr->callback_rom=0x1567cu;tr->descriptor_rom=0x15668u;s=raw1c(rt,a,b,&d,tr);break;
        case 0x1du:tr->callback_rom=0x15870u;tr->descriptor_rom=0x1585cu;s=raw1d(rt,a,b,&d,tr);break;
        case 0x2fu:tr->callback_rom=0x16c9eu;tr->descriptor_rom=0x16c5eu;s=raw2f(rt,a,&d,tr);break;
        case 0x80u:tr->callback_rom=0x1a7b4u;tr->descriptor_rom=0x1a794u;s=raw_reversal(rt,a,&d,tr,&D_1A794,0u,0x1a7e2u);break;
        case 0x81u:tr->callback_rom=0x1a7b4u;tr->descriptor_rom=0x1a7a4u;s=raw81(rt,a,&d,tr);break;
        case 0x82u:tr->callback_rom=0x1a832u;tr->descriptor_rom=0x1a822u;s=raw_reversal(rt,a,&d,tr,&D_1A822,0u,0x1a866u);break;
        case 0x38u:tr->callback_rom=0x17b12u;tr->descriptor_rom=0x17afeu;s=raw38(rt,a,&d);break;
        case 0x39u:tr->callback_rom=0x17b70u;tr->descriptor_rom=0x17b5cu;s=raw39(rt,a,b,&d,tr);break;
          case 0x3cu:tr->callback_rom=0x18028u;tr->descriptor_rom=0x18022u;s=raw3c(a,&d,tr);break;
        case 0x2au:tr->callback_rom=0x1662cu;tr->descriptor_rom=0x16614u;s=raw2a(rt,a,&d);break;
        case 0x6cu:tr->callback_rom=0x19c48u;tr->descriptor_rom=0x19c28u;s=raw6c(rt,a,&d);break;
        case 0x18u:tr->callback_rom=0x14f08u;tr->descriptor_rom=0x14ef0u;s=raw18(rt,a,&d,tr);break;
        case 0x03u:tr->callback_rom=0x12b16u;tr->descriptor_rom=0x12ae6u;s=raw03(rt,a,&d,tr);break;
        case 0x5du:
        case 0x5fu:tr->callback_rom=0x19536u;tr->descriptor_rom=0x19530u;s=raw5f(rt,a,&d,tr);break;
        case 0x5eu:tr->callback_rom=0x19536u;tr->descriptor_rom=0x19530u;s=raw5f(rt,a,&d,tr);break;
        case 0x22u:tr->callback_rom=0x15b8cu;tr->descriptor_rom=0x15b6cu;s=raw22(rt,a,b,&d,tr);break;
        case 0x3fu:tr->callback_rom=0x1836eu;tr->descriptor_rom=0x1835eu;s=raw3f(rt,a,b,&d,tr);break;
        case 0x40u:tr->callback_rom=0x18404u;tr->descriptor_rom=0x183ecu;s=raw40(rt,a,&d);break;
        case 0x41u:tr->callback_rom=0x18498u;tr->descriptor_rom=0x1848cu;s=raw41(rt,a,&d,tr);break;
          case 0x8au:tr->callback_rom=0x1ac72u;tr->descriptor_rom=0x1ac62u;s=raw8a(rt,a,&d,tr);break;
          case 0x60u:tr->callback_rom=0x19536u;tr->descriptor_rom=0x19530u;s=raw60(rt,a,&d,tr);break;
          case 0x25u:tr->callback_rom=0x1612eu;tr->descriptor_rom=0x16116u;s=raw25(rt,a,b,&d,tr);break;
          case 0x28u:tr->callback_rom=0x16436u;tr->descriptor_rom=0x1641eu;s=raw28(rt,a,&d,tr);break;
          case 0x43u:tr->callback_rom=0x1853cu;tr->descriptor_rom=0x18518u;s=raw43(rt,a,&d,tr);break;
        case 0x0eu:tr->callback_rom=0x13d46u;tr->descriptor_rom=0x13d26u;s=raw0e(rt,a,&d,tr);break;
        case 0x51u:tr->callback_rom=0x18c24u;tr->descriptor_rom=0x18bf4u;s=raw51(rt,a,&d,tr);break;
        case 0x33u:tr->callback_rom=0x17282u;tr->descriptor_rom=0x17266u;s=raw33(rt,a,&d,tr);break;
        case 0x84u:tr->callback_rom=0x1a950u;tr->descriptor_rom=0x1a940u;s=raw84(rt,a,&d,tr);break;
        case 0x83u:tr->callback_rom=0x1a8b6u;tr->descriptor_rom=0x1a8a6u;s=raw83(rt,a,&d,tr);break;
        case 0x26u:tr->callback_rom=0x16258u;tr->descriptor_rom=0x1623cu;s=raw26(rt,a,&d,tr);break;
        case 0x85u:tr->callback_rom=0x1a9e4u;tr->descriptor_rom=0x1a9d4u;s=raw85(rt,a,&d,tr);break;
        case 0x86u:tr->callback_rom=0x1aa9au;tr->descriptor_rom=0x1aa76u;s=raw87(rt,a,&d,tr);break;
        case 0x87u:tr->callback_rom=0x1aa9au;tr->descriptor_rom=0x1aa8au;s=raw87(rt,a,&d,tr);break;
        case 0x88u:tr->callback_rom=0x1ab38u;tr->descriptor_rom=0x1ab32u;s=raw88(a,&d);break;
        case 0x89u:tr->callback_rom=0x1abc0u;tr->descriptor_rom=0x1aba4u;s=raw89(rt,a,&d,tr);break;
        case 0x69u:tr->callback_rom=0x19a20u;tr->descriptor_rom=0x199fcu;s=raw69(rt,a,b,&d,tr);break;
        case 0x6au:tr->callback_rom=0x19b50u;tr->descriptor_rom=0x19b3cu;s=raw6a(a,&d);break;
        case 0x6bu:tr->callback_rom=0x19bc0u;tr->descriptor_rom=0x19bbau;s=raw6b(rt,a,&d,tr);break;
        case 0x68u:tr->callback_rom=0x19832u;tr->descriptor_rom=0x1981eu;s=raw68(rt,a,&d,tr);break;
        case 0x24u:tr->callback_rom=0x16056u;tr->descriptor_rom=0x1603au;s=raw24(rt,a,&d,tr);break;
        case 0x08u:tr->callback_rom=0x13180u;tr->descriptor_rom=0x1316cu;s=raw08(rt,a,&d,tr);break;
        case 0x4eu:tr->callback_rom=0x18aa0u;tr->descriptor_rom=0x18a90u;s=raw4e(a,&d);break;
        case 0x3du:tr->callback_rom=0x114b2u;tr->descriptor_rom=0x114acu;d=&D_114AC;s=wf_r17_action_idle_114b2(rt,a,tr);break;
        case 0x48u:tr->callback_rom=0x132f4u;tr->descriptor_rom=0x132d8u;s=raw48(rt,a,&d,tr);break;
        case 0x49u:tr->callback_rom=0x13444u;tr->descriptor_rom=0x13434u;s=raw49(rt,a,&d,tr);break;
        case 0x4au:tr->callback_rom=0x180c4u;tr->descriptor_rom=0x180acu;s=raw4a(rt,a,&d,tr);break;
        case 0x4bu:tr->callback_rom=0x1820cu;tr->descriptor_rom=0x181ecu;s=raw4b(rt,a,&d,tr);break;
        case 0x8bu:tr->callback_rom=0x1ad24u;tr->descriptor_rom=0x1ad10u;s=raw8b(rt,a,&d,tr);break;
        case 0x8cu:tr->callback_rom=0x1ad7cu;tr->descriptor_rom=0x1ad76u;s=raw8c(rt,a,&d,tr);break;
        case 0x8du:tr->callback_rom=0x17fb2u;tr->descriptor_rom=0x17f9au;s=raw8d(rt,a,&d,tr);break;
        case 0x00u:tr->callback_rom=0x00012888u;s=raw00(rt,a,&d,tr);
                   if(s==WF_OK)tr->descriptor_rom=d->rom;
                   break;
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
        case 0x0bu:tr->callback_rom=0x13a2eu;tr->descriptor_rom=0x13a1eu;s=raw0b(rt,a,&d,tr);break;
        case 0x0cu:tr->callback_rom=0x13bc8u;tr->descriptor_rom=0x13b98u;s=raw0c(rt,a,&d,tr);break;
        case 0x20u:tr->callback_rom=0x16a16u;tr->descriptor_rom=0x16a06u;s=raw20(rt,a,&d);break;
        case 0x54u:tr->callback_rom=0x18f82u;tr->descriptor_rom=0x18f62u;s=raw54(rt,a,&d,tr);break;
        case 0x55u:tr->callback_rom=0x19002u;tr->descriptor_rom=0x18feeu;s=raw55(rt,a,&d,tr);break;
        case 0x56u:tr->callback_rom=0x19086u;tr->descriptor_rom=0x19076u;s=raw56(rt,a,&d,tr);break;
        case 0x5au:tr->callback_rom=0x19302u;tr->descriptor_rom=0x192f2u;s=raw5a_5b(rt,a,&d,tr,0x5au);break;
        case 0x5bu:tr->callback_rom=0x193aau;tr->descriptor_rom=0x1939au;s=raw5a_5b(rt,a,&d,tr,0x5bu);break;
        case 0x62u:tr->callback_rom=0x19536u;tr->descriptor_rom=0x19530u;s=raw62(rt,a,&d,tr);break;
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
