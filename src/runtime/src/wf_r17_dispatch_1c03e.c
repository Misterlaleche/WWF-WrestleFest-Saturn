#include <stddef.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_animation.h"
#include "wf_r17_resolver_280dc.h"

static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}
static wf_status_t blocked(wf_r17_dispatch_1c03e_ctx_t *c,uint32_t pc){if(c)c->blocked_dependency_rom=pc;return WF_BLOCKED;}
static void exact_tail_1c0b4(wf_actor_raw_t *a){
    if((wf_u8(a,WF_A_FIELD18)&0x80u)!=0u){wf_set_be16(a,WF_A_FIELD18,0u);wf_set_be16(a,WF_A_FIELD1A,0u);}
}
/* ROM $114B2, main state zero. The uncommon +$74/+FE continuations retain
 * their explicit dependency until their complete register effects are owned. */
static wf_status_t main_zero_114b2(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    uint8_t old=wf_u8(a,WF_A_LATCHED);
    wf_set_u8(a,WF_A_LATCHED,(uint8_t)(old|0x80u));
    if(old&0x80u){
        /* $115D2: discard a stale composition link, then restore facing pose. */
        if(wf_be16(a,0xfeu)==0u&&wf_be32(a,WF_A_LINK)!=0u){
            wf_actor_raw_t *p=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
            if(!p)return blocked(ctx,0x000115deu);
            if(wf_be32(p,WF_A_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))){
                wf_set_be32(a,WF_A_LINK,0u);wf_set_be32(a,WF_A_SECONDARY_LINK,0u);
            }
        }
        wf_set_be16(a,WF_A_MIRROR,wf_be16(a,WF_A_FACING));
        if((wf_u8(a,WF_A_REL_FLAGS)&0x80u)||(wf_u8(a,0xfeu)&0x80u))return blocked(ctx,0x0001156au);
        return WF_OK;
    }
    wf_set_u8(a,WF_A_MOTION_MODE,0u);
    wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x58u));
    wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~0x04u));
    wf_set_be16(a,WF_A_RENDER_LIST,0u);wf_set_be16(a,WF_A_MASH,0u);
    for(unsigned off=0x44u;off<=0x46u;off+=2u)wf_set_be16(a,off,0u);
    wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);
    for(unsigned off=0x18u;off<=0x1au;off+=2u)wf_set_be16(a,off,0u);
    for(unsigned off=0x3eu;off<=0x42u;off+=2u)wf_set_be16(a,off,0u);
    if((wf_u8(a,WF_A_FLAGS32)&8u)!=0u&&(rt->g.credited_route&0x80u)==0u)
        wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~8u));
    if(wf_be16(a,0xd0u)==0u)wf_set_be16(a,0xd0u,0xd0u);
    if(wf_u8(a,WF_A_FLAGS32)&0x10u){wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x7au);}
    if(wf_u8(a,WF_A_FLAGS34)&0x40u)return blocked(ctx,0x00011546u);
    if((wf_u8(a,WF_A_REL_FLAGS)&0x80u)||(wf_u8(a,0xfeu)&0x80u))return blocked(ctx,0x0001156au);
    return WF_OK;
}
/* ROM $11652 -> $11ABA (AE=4), $1174C, $11710, then kind-2 $1C0C8.
 * This bounded route is used by the freshly selected reserve wrestler. */
static wf_status_t main_one_11652(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    static const uint8_t magnitude[12]={0x1b,0x1d,0x1c,0x19,0x1e,0x1b,0x18,0x1e,0x16,0x1b,0x1c,0x1b};
    uint8_t high=wf_u8(a,WF_A_LATCHED),flags=wf_u8(a,WF_A_FLAGS34);
    uint16_t sub=(uint16_t)(wf_be16(a,WF_A_AE)&0x3fu);
    if((sub!=0u&&sub!=1u&&sub!=4u)||wf_u8(a,WF_A_REL_FLAGS)&0x80u||wf_be16(a,WF_A_CHARACTER)>=12u)return blocked(ctx,0x00011652u);
    if(sub==1u){
        /* $11796 and kind-2 descriptor $115F2. The per-character $11914
         * recovery period consists of twelve WORDs, each $0094. */
        if(!(high&0x80u)){
            wf_set_u8(a,WF_A_FLAGS32,(uint8_t)((wf_u8(a,WF_A_FLAGS32)|1u)&~8u));
            wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x40u));
            wf_set_be16(a,WF_A_MAGNITUDE,16u);
            wf_set_be16(a,WF_A_RENDER_LIST,4u);
            wf_set_be16(a,0x44u,148u);
            wf_set_be16(a,0x46u,0u);
            uint16_t y=wf_be16(a,WF_A_Y);
            uint16_t x=(wf_u8(a,WF_A_FLAGS33)&0x80u)?(uint16_t)(0x0e48u-y):(uint16_t)(y+0x0580u);
            wf_set_be16(a,WF_A_X,(uint16_t)(x>>2));
            wf_set_be16(a,WF_A_ANIM_INDEX,0u);
            wf_set_be16(a,WF_A_ANIM_COUNTER,10u);
            wf_set_be16(a,WF_A_MIRROR,(uint16_t)(1u^wf_be16(a,WF_A_FACING)));
            wf_set_u8(a,WF_A_LATCHED,(uint8_t)(high|0x80u));
            return WF_OK;
        }
        wf_set_u8(a,WF_A_FACING,(uint8_t)((wf_u8(a,WF_A_FLAGS33)&0x80u)^0x80u));
        if(wf_u8(a,0xfeu)&0x80u){
            wf_set_be16(a,WF_A_STATE,5u);
            wf_set_be16(a,WF_A_SELECTOR,0x4eu);
        }else{
            /* $1D526 only takes its global-state continuation when the
             * actor's E6 countdown reaches zero. Preserve that dependency. */
            if((wf_u8(a,WF_A_FLAGS34)&1u)&&wf_be16(a,0xe6u)){
                if(wf_be16(a,0xe6u)==1u)return blocked(ctx,0x0001d546u);
                wf_set_be16(a,0xe6u,(uint16_t)(wf_be16(a,0xe6u)-1u));
            }
            if(wf_be16(a,0xfeu)==0u&&wf_be32(a,WF_A_LINK)!=0u){
                wf_actor_raw_t *p=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
                if(!p)return blocked(ctx,0x000115deu);
                if(wf_be32(p,WF_A_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))){
                    wf_set_be32(a,WF_A_LINK,0u);wf_set_be32(a,WF_A_SECONDARY_LINK,0u);
                }
            }
            if(!(wf_u8(a,WF_A_FLAGS33)&0x20u)){
                wf_set_be16(a,0xc8u,(uint16_t)(wf_be16(a,0xc8u)+1u));
                if(wf_u8(a,0xc6u)&0x40u){
                    wf_set_be16(a,WF_A_STATE,1u);
                    wf_set_be16(a,WF_A_AE,3u);
                    goto state_one_tick;
                }
            }
            uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);
            if(!timer){
                timer=148u;
                if(wf_be16(a,0x72u)>wf_be16(a,0x66u)){
                    wf_set_be16(a,0x66u,(uint16_t)(wf_be16(a,0x66u)+1u));
                    wf_set_be16(a,0x6au,1u);
                }
            }
            wf_set_be16(a,0x44u,timer);
            if(wf_u8(a,WF_A_FLAGS34)&0x20u){
                uint16_t t=wf_be16(a,0x46u);
                if(!t)t=32u;
                --t;wf_set_be16(a,0x46u,t);
                if(!t){
                    wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~0x20u));
                    wf_set_be16(a,WF_A_MIRROR,wf_be16(a,WF_A_FACING));
                }else{
                    wf_set_u8(a,WF_A_MOTION_MODE,0u);
                    wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0x01d3u|wf_be16(a,WF_A_FACING)));
                    wf_set_be16(a,WF_A_ANIM_COUNTER,2u);
                }
            }else{
                uint8_t angle=wf_u8(a,WF_A_ANGLE);
                if(angle){
                    wf_set_u8(a,WF_A_MOTION_MODE,1u);
                    uint16_t y=wf_be16(a,WF_A_Y);
                    if(angle>=0x40u&&angle<0xc0u){
                        if(y>0x120u)goto state_one_tick;
                        wf_set_be16(a,WF_A_Y,0x120u);
                    }else{
                        if(y<0x192u)goto state_one_tick;
                        wf_set_be16(a,WF_A_Y,0x192u);
                    }
                }
                wf_set_u8(a,WF_A_MOTION_MODE,0u);
                wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
            }
        }
state_one_tick:
        if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
            uint16_t count=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
            wf_set_be16(a,WF_A_ANIM_COUNTER,count);
            if(count==0xffffu){
                unsigned idx=(wf_be16(a,WF_A_ANIM_INDEX)+1u)&0xffffu;
                if(idx>=4u)idx=0u;
                wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)idx);
                wf_set_be16(a,WF_A_ANIM_COUNTER,10u);
                wf_set_be16(a,WF_A_MIRROR,(uint16_t)((idx+1u)^wf_be16(a,WF_A_FACING)));
            }
        }
        return WF_OK;
    }
    if(high&0x80u){
        if(sub==4u&&(flags&1u))return blocked(ctx,0x00011b26u);
        if(sub==4u&&((wf_u8(a,WF_A_FLAGS33)&0x80u)?(wf_u8(a,WF_A_CONTACT+1u)&1u):(wf_u8(a,WF_A_CONTACT+1u)&2u))){
            /* $11B5A..$11B66 changes the action, then $11710 only selects A1. */
            wf_set_be16(a,WF_A_STATE,5u);
            wf_set_be16(a,WF_A_SELECTOR,0x004fu);
        }
        if(wf_be16(a,0xfeu)==0u&&wf_be32(a,WF_A_LINK)!=0u){
            wf_actor_raw_t *p=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
            if(!p)return blocked(ctx,0x000115deu);
            if(wf_be32(p,WF_A_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))){wf_set_be32(a,WF_A_LINK,0u);wf_set_be32(a,WF_A_SECONDARY_LINK,0u);}
        }
        if(sub==0u){
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,WF_A_CHARACTER));
            wf_set_be16(a,WF_A_MAGNITUDE,magnitude[wf_be16(a,WF_A_CHARACTER)]);
            wf_set_be16(a,WF_A_PROFILE,1u);
            wf_set_be16(a,WF_A_MIRROR,wf_be16(a,WF_A_FACING));
            if(wf_be16(a,0xfeu))return blocked(ctx,0x00011702u);
        }else wf_set_be16(a,WF_A_MIRROR,wf_be16(a,WF_A_FACING));
        uint16_t count=wf_be16(a,WF_A_ANIM_COUNTER);
        if(count!=0xff00u){
            count=(uint16_t)(count-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,count);
            if(count==0xffffu){unsigned idx=(wf_be16(a,WF_A_ANIM_INDEX)+1u)&0xffffu;if(idx>=4u)idx=0;
                wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)idx);wf_set_be16(a,WF_A_ANIM_COUNTER,10u);
                wf_set_be16(a,WF_A_MIRROR,(uint16_t)((idx+1u)^wf_be16(a,WF_A_FACING)));
            }
        }
        return WF_OK;
    }
    if(sub==0u){
        if(wf_be16(a,0xfeu)!=0u)return blocked(ctx,0x00011702u);
        wf_set_u8(a,WF_A_MOTION_MODE,1u);
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x40u));
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(a,WF_A_CHARACTER));
        wf_set_be16(a,WF_A_MAGNITUDE,magnitude[wf_be16(a,WF_A_CHARACTER)]);
        wf_set_be16(a,WF_A_PROFILE,1u);
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,10u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(1u^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(high|0x80u));return WF_OK;
    }
    if(flags&0xc0u)return blocked(ctx,0x00011abau);
    if(!(rt->g.credited_route&0x8000u))wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~8u));
    wf_set_be16(a,WF_A_FIELD18,0u);wf_set_be16(a,WF_A_FIELD1A,0u);
    wf_set_u8(a,WF_A_MOTION_MODE,1u);wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(flags&~0xd0u));
    wf_set_be16(a,WF_A_MAGNITUDE,magnitude[wf_be16(a,WF_A_CHARACTER)]);
    uint8_t d0=(uint8_t)((wf_u8(a,WF_A_FLAGS33)&0x80u)^0x80u);d0=(uint8_t)(d0+0x40u);
    wf_r17_m68k_move_b(&rt->m68k_regs,0u,d0);
    wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&0x80u));
    wf_set_be16(a,WF_A_ANGLE_WORD,d0);
    wf_set_be16(a,WF_A_PROFILE,1u);
    wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,10u);
    wf_set_be16(a,WF_A_MIRROR,(uint16_t)(1u^wf_be16(a,WF_A_FACING)));
    wf_set_u8(a,WF_A_LATCHED,(uint8_t)(high|0x80u));
    return WF_OK;
}
/* $10B62 preserves D0 across $280DC and commits only the X correction.
 * $124FA uses it twice to settle the linked actors at the arcade offsets. */
static wf_status_t hold_offset_10b62(wf_runtime_t *rt,wf_actor_raw_t *a,
                                      uint16_t displacement,
                                      wf_r17_dispatch_1c03e_ctx_t *ctx){
    wf_set_be16(a,WF_A_FIELD3E,displacement);
    wf_status_t s=wf_r17_resolver_280dc(rt,a);
    if(s!=WF_OK)return blocked(ctx,0x000280dcu);
    if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
    wf_set_be16(a,WF_A_CONTACT,displacement);
    wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+wf_be16(a,WF_A_CORR_X)));
    wf_set_be16(a,WF_A_FIELD3E,0u);
    return WF_OK;
}
/* $12474 with the kind-2 $12450 descriptor: linked hold, four 16-tick
 * poses $39,$3A,$39,$3B; $125C0 is the held-pose descriptor. */
static wf_status_t main_eleven_12474(wf_runtime_t *rt,wf_actor_raw_t *a,
                                      wf_r17_dispatch_1c03e_ctx_t *ctx){
    wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
    uint8_t alternate=(uint8_t)((wf_u8(a,WF_A_TIMER44)&0x20u)!=0u);
    if(!linked)return blocked(ctx,0x00012490u);
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        wf_status_t s=hold_offset_10b62(rt,a,0x0018u,ctx);
        if(s!=WF_OK)return s;
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)|0x40u));
        wf_set_u8(linked,WF_A_FLAGS33,(uint8_t)(wf_u8(linked,WF_A_FLAGS33)|0x40u));
    }
    if(wf_u8(a,WF_A_LOCKOUT)&0x80u){
        wf_set_be16(a,WF_A_STATE,5u);
        wf_set_be16(a,WF_A_SELECTOR,0x50u);
    }else if(wf_u8(a,WF_A_TIMER44)&0x80u){
        /* $125CC compares the linked actor's return link on the 24-bit bus. */
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        wf_set_be16(a,WF_A_MIRROR,0xffffu);
        if((wf_be32(linked,WF_A_LINK)&0x00ffffffu)!=
           wf_actor_rom_base((unsigned)(a-rt->actors))){
            wf_set_be16(a,WF_A_STATE,0u);
            wf_set_be32(a,WF_A_LINK,0u);
            wf_set_be32(a,WF_A_SECONDARY_LINK,0u);
            wf_set_be16(a,WF_A_FLAGS32,(uint16_t)(wf_be16(a,WF_A_FLAGS32)&0x0087u));
            wf_set_be16(a,WF_A_FLAGS34,(uint16_t)(wf_be16(a,WF_A_FLAGS34)&0x0300u));
            wf_set_be16(a,WF_A_CPU_B4,0u);
            wf_set_be16(a,WF_A_CPU_B6,0u);
        }
        if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
            wf_set_be16(a,WF_A_ANIM_INDEX,0u);
            wf_set_be16(a,WF_A_ANIM_COUNTER,0xff00u);
            wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
        }
        return WF_OK;
    }else if(alternate&&wf_u8(a,WF_A_ACTION_PHASE)==0xfeu){
        wf_set_u8(a,WF_A_TIMER44,(uint8_t)(wf_u8(a,WF_A_TIMER44)&~0x20u));
    }
    static const uint16_t poses[4]={0x39u,0x3au,0x39u,0x3bu};
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,16u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)((alternate?0x01d2u:poses[0])^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t count=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,count);
        if(count==0xffffu){
            unsigned idx=(wf_be16(a,WF_A_ANIM_INDEX)+1u)&3u;
            if(alternate){
                wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);
                wf_set_be16(a,WF_A_ANIM_COUNTER,0u);
            }else{
                wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)idx);
                wf_set_be16(a,WF_A_ANIM_COUNTER,16u);
                wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[idx]^wf_be16(a,WF_A_FACING)));
            }
        }
    }
    return WF_OK;
}
static wf_status_t main_twelve_124fa(wf_runtime_t *rt,wf_actor_raw_t *a,
                                      wf_r17_dispatch_1c03e_ctx_t *ctx){
    uint8_t alternate=0u;
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
        if(!linked)return blocked(ctx,0x0001c094u);
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        wf_status_t s=hold_offset_10b62(rt,a,0x0018u,ctx);
        if(s!=WF_OK)return s;
        s=hold_offset_10b62(rt,a,0xffe8u,ctx);
        if(s!=WF_OK)return s;
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)|0x40u));
        wf_set_u8(linked,WF_A_FLAGS33,(uint8_t)(wf_u8(linked,WF_A_FLAGS33)|0x40u));
        wf_set_be16(a,WF_A_TIMER46,0xe0u);
        wf_set_u8(a,WF_A_TIMER44+1u,0u);
        wf_set_be16(a,WF_A_CPU_BC,0u);
    }
    if(wf_u8(a,WF_A_LOCKOUT)&0x80u){
        wf_set_be16(a,WF_A_STATE,5u);
        wf_set_be16(a,WF_A_SELECTOR,0x50u);
    }else{
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_TIMER46)-1u);
        wf_set_be16(a,WF_A_TIMER46,timer);
        if(timer==0xa0u)wf_set_u8(a,WF_A_TIMER44+1u,1u);
        if(!timer){
            wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
            if(!linked)return blocked(ctx,0x00012570u);
            wf_set_be16(a,WF_A_STATE,0xffu);
            wf_set_be16(linked,WF_A_STATE,12u);
            wf_set_be16(a,WF_A_CPU_BC,0u);
            wf_set_be16(linked,WF_A_TIMER44,0u);
            wf_set_be16(linked,WF_A_MIRROR,0x39u);
            wf_set_u8(linked,WF_A_MIRROR,wf_u8(linked,WF_A_FACING));
            ctx->extra_render_actor=linked;
            wf_set_be16(a,WF_A_MIRROR,0xffffu);
        }
        alternate=(uint8_t)((wf_u8(a,WF_A_TIMER44)&0x20u)!=0u);
        if(alternate&&wf_u8(a,WF_A_ACTION_PHASE)==0xfeu)
            wf_set_u8(a,WF_A_TIMER44,(uint8_t)(wf_u8(a,WF_A_TIMER44)&~0x20u));
    }
    /* Kind-2 descriptor $124E2: four 16-frame poses $39,$3A,$3F,$3A. */
    static const uint16_t poses[4]={0x39u,0x3au,0x3fu,0x3au};
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,16u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)((alternate?0x01d2u:poses[0])^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t count=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,count);
        if(count==0xffffu){
                unsigned idx=(wf_be16(a,WF_A_ANIM_INDEX)+1u)&3u;
                if(alternate){
                    wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);
                    wf_set_be16(a,WF_A_ANIM_COUNTER,0u);
                }else{
                    wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)idx);
                    wf_set_be16(a,WF_A_ANIM_COUNTER,16u);
                    wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[idx]^wf_be16(a,WF_A_FACING)));
                }
        }
    }
    return WF_OK;
}
/* ROM $11C82, state two reached by the $14E40 linked throw. */
static wf_status_t main_two_11c82(wf_runtime_t *rt,wf_actor_raw_t *a,
                                   wf_r17_dispatch_1c03e_ctx_t *ctx){
    static const uint8_t magnitude[12]={0x2f,0x35,0x31,0x2d,0x38,0x30,
                                         0x2c,0x38,0x28,0x2f,0x30,0x2f};
    static const uint16_t poses[6]={9u,10u,11u,12u,13u,14u};
    if(wf_be16(a,WF_A_CHARACTER)>=12u)return blocked(ctx,0x00011d4au);
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_u8(a,WF_A_MOTION_MODE,1u);
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)|0x10u));
        wf_set_be16(a,WF_A_MAGNITUDE,magnitude[wf_be16(a,WF_A_CHARACTER)]);
    }else{
        if(wf_u8(a,WF_A_LOCKOUT)&0x80u){
            wf_set_be16(a,WF_A_STATE,3u);
            goto thrown_tick;
        }
        if(wf_be16(a,WF_A_LOCKOUT)==0u&&wf_be32(a,WF_A_LINK)!=0u){
            wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
            if(!linked)return blocked(ctx,0x000115deu);
            if(wf_be32(linked,WF_A_LINK)!=wf_actor_rom_base((unsigned)(a-rt->actors))){
                wf_set_be32(a,WF_A_LINK,0u);
                wf_set_be32(a,WF_A_SECONDARY_LINK,0u);
            }
        }
        uint8_t contact=wf_u8(a,WF_A_CONTACT+1u);
        uint8_t edge=(wf_u8(a,WF_A_FACING)&0x80u)?1u:2u;
        if(contact&edge){
            uint8_t code=wf_u8(a,WF_A_CONTACT);
            if(code==3u){
                wf_set_be16(a,WF_A_PENDING_DAMAGE,10u);
                wf_set_be16(a,WF_A_STATE,4u);
                wf_set_be16(a,WF_A_STATE4,0x17u);
                wf_set_be16(a,WF_A_TIMER44,0u);
                goto thrown_tick;
            }
            if(code==5u&&wf_be16(a,WF_A_TIMER44)!=0u){
                wf_set_be16(a,WF_A_PENDING_DAMAGE,8u);
                return blocked(ctx,0x00011d56u);
            }
            wf_set_be16(a,WF_A_STATE,6u);
            return blocked(ctx,0x00011d56u);
        }
    }
    wf_set_be16(a,WF_A_PROFILE,3u);
thrown_tick:
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,6u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[0]^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t count=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,count);
        if(count==0xffffu){
            unsigned idx=(wf_be16(a,WF_A_ANIM_INDEX)+1u)%6u;
            wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)idx);
            wf_set_be16(a,WF_A_ANIM_COUNTER,6u);
            wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[idx]^wf_be16(a,WF_A_FACING)));
        }
    }
    return WF_OK;
}

wf_status_t wf_r17_dispatch_1c03e_bounded(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    uint8_t domain,state,id;uint16_t di;const wf_anim_desc_t *d;wf_status_t s;ptrdiff_t slot;
    if(!rt||!a||!ctx)return WF_BAD_STATE;
    ctx->blocked_dependency_rom=0u;ctx->extra_render_actor=NULL;
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
    if(id==0u&&d->callback_rom==0x000114b2u){
        s=main_zero_114b2(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==1u&&d->callback_rom==0x00011652u){
        s=main_one_11652(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==2u&&d->callback_rom==0x00011c82u){
        s=main_two_11c82(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==11u&&d->callback_rom==0x00012474u){
        s=main_eleven_12474(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==12u&&d->callback_rom==0x000124fau){
        s=main_twelve_124fa(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    return blocked(ctx,d->callback_rom);
}
