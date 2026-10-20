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
/* $11D56 writes the pair of rope-animation words on the contacted side. */
static void rope_contact_11d56(wf_runtime_t *rt,const wf_actor_raw_t*a){
    if(wf_u8(a,WF_A_CONTACT+1u)&1u){
        rt->g.global_11b0=2u;rt->g.global_11e0=2u;
    }else{
        rt->g.global_1150=2u;rt->g.global_1180=2u;
    }
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
                rope_contact_11d56(rt,a);
                wf_set_be16(a,WF_A_STATE,4u);
                wf_set_be16(a,WF_A_STATE4,0x17u);
                wf_set_be16(a,WF_A_TIMER44,0u);
                goto thrown_tick;
            }
            wf_set_be16(a,WF_A_STATE,6u);
            rope_contact_11d56(rt,a);
            uint16_t timer=wf_be16(a,WF_A_TIMER44);
            wf_set_be16(a,WF_A_TIMER44,timer?(uint16_t)(timer-1u):0u);
            goto thrown_tick;
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

/* $11DE0 and descriptor $11DD0: two phases of the rope rebound.
 * The callback does not alter D registers; $1C0C8 runs even on its exit. */
static void main_six_11de0(wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^0x80u));
        wf_set_u8(a,0x2du,(uint8_t)(wf_u8(a,0x2du)^0x80u));
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        wf_set_be16(a,WF_A_MAGNITUDE,12u);
        wf_set_u8(a,0x19u,0x20u);
        wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~4u));
    }else{
        if(wf_u8(a,0x25u)==1u)wf_set_u8(a,WF_A_MOTION_MODE,1u);
        if(wf_u8(a,0x25u)==0xfeu){
            wf_set_be16(a,WF_A_STATE,2u);
            wf_set_u8(a,WF_A_FIELD18,(uint8_t)(wf_u8(a,WF_A_FIELD18)|0x80u));
        }
    }
    wf_set_be16(a,WF_A_PROFILE,0x20u);
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,8u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0x69u^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
        if(timer==0xffffu){
            uint16_t index=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);
            wf_set_be16(a,WF_A_ANIM_INDEX,index<2u?index:0xfeu);
            wf_set_be16(a,WF_A_ANIM_COUNTER,index<2u?12u:0u);
            if(index<2u)wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0x69u^wf_be16(a,WF_A_FACING)));
        }
    }
}

/* $11F3A -> $12078, jumping into the ring after the TAG handover.
 * Routes 4/5 use $11F26, routes 6/7 use $11ECA. */
static wf_status_t main_eight_12078(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    unsigned route=wf_be16(a,WF_A_TIMER44);
    if(route<4u||route>7u)return blocked(ctx,0x11f3au);
    static const uint16_t coords[4][2]={{0x1dcu,0x193u},{0x314u,0x193u},{0x1bdu,0x120u},{0x338u,0x120u}};
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_AE,0u);
        wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|2u));
        wf_set_be16(a,WF_A_RENDER_LIST,(route&2u)?4u:0u);
        wf_set_be16(a,WF_A_X,(uint16_t)(coords[route&3u][0]+((route&1u)?24u:0xffe8u)));
        wf_set_be16(a,WF_A_Y,coords[route&3u][1]);wf_set_be16(a,WF_A_Z,0x180u);
        wf_set_u8(a,WF_A_FACING,(route&1u)?0u:128u);
        rt->g.global_15d2=wf_u8(a,WF_A_CHARACTER+1u);rt->g.global_15d3=0x17u;
        /* $2052 is the silent sound output device. */
    }else if(wf_u8(a,WF_A_ANIM_INDEX+1u)==0xfeu){
        wf_set_be16(a,WF_A_STATE,9u);wf_set_be16(a,WF_A_RENDER_LIST,0u);
    }
    unsigned count=(route&2u)?5u:3u,pose=(route&2u)?0x31u:0x2du;
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,12u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(pose^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
        if(timer==0xffffu){
            uint16_t index=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);
            wf_set_be16(a,WF_A_ANIM_INDEX,index<count?index:0xfeu);
            wf_set_be16(a,WF_A_ANIM_COUNTER,index<count?12u:0u);
            if(index<count)wf_set_be16(a,WF_A_MIRROR,(uint16_t)((pose+index)^wf_be16(a,WF_A_FACING)));
        }
    }
    return WF_OK;
}

/* $121E6, kind-zero descriptor $121E0: wait on the turnbuckle. */
static wf_status_t main_nine_121e6(wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    static const uint16_t coords[10][2]={
        {0x1e4u,0x190u},{0x310u,0x190u},{0x1d0u,0x118u},{0x31cu,0x118u},
        {0x1e4u,0x190u},{0x310u,0x190u},{0x1d0u,0x118u},{0x31cu,0x118u},
        {0x268u,0x158u},{0x3d8u,0x158u}};
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        unsigned route=wf_be16(a,WF_A_TIMER44);if(route>=10u)return blocked(ctx,0x12208u);
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
        wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_RENDER_LIST,0u);wf_set_be16(a,WF_A_TIMER46,128u);
        wf_set_be16(a,WF_A_X,coords[route][0]);wf_set_be16(a,WF_A_Y,coords[route][1]);wf_set_be16(a,WF_A_Z,0x180u);
        wf_set_u8(a,WF_A_MIRROR+1u,(route&2u)?0x36u:0x30u);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
    }else{
        if(wf_u8(a,0xfeu)&128u)wf_set_be16(a,WF_A_STATE,10u);
        else{
            uint16_t timer=(uint16_t)(wf_be16(a,WF_A_TIMER46)-1u);wf_set_be16(a,WF_A_TIMER46,timer);
            if(!timer)wf_set_be16(a,WF_A_STATE,10u);
        }
    }
    wf_set_be32(a,WF_A_LINK,0u);return WF_OK;
}

/* $1B1D0/$1B1B8: rebound after a knockdown. The alternate $1B1C4
 * descriptor has the same indefinite duration and pose $16 instead of $1F. */
static wf_status_t state4_1b1d0(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~16u));
        /* $10B62 saves/restores D0/D1 and the original contact word. */
        uint16_t contact=wf_be16(a,WF_A_CONTACT);
        wf_set_be16(a,WF_A_FIELD3E,80u);
        if(wf_r17_resolver_280dc(rt,a)!=WF_OK)return blocked(ctx,0x10b62u);
        if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
        wf_set_be16(a,WF_A_CONTACT,contact);
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+wf_be16(a,WF_A_CORR_X)));
        wf_set_be16(a,WF_A_FIELD3E,0u);
        /* $258E preset 3: $0000,$0300,$0038. */
        wf_set_be16(a,WF_A_VX,0u);wf_set_be16(a,WF_A_VY,0u);
        wf_set_be16(a,WF_A_VZ,768u);wf_set_be16(a,WF_A_VZ_DEC,56u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        /* $10C60 selector 1. Its private registers are all restored. */
        uint16_t unit=(uint16_t)(wf_be16(a,0x72u)>>2),sum=unit;unsigned level=3u;
        while(sum<wf_be16(a,WF_A_STAMINA)&&level){sum=(uint16_t)(sum+unit);--level;}
        static const uint8_t recovery[4]={1u,3u,7u,21u};wf_set_be16(a,0xaau,recovery[level]);
        if(low(a,WF_A_STATE4)!=0x2au&&low(a,0x70u)==2u)
            wf_set_u8(a,WF_A_STATE4,(uint8_t)(wf_u8(a,WF_A_STATE4)|128u));
    }else{
        /* $10D04 records player recovery input. */
        if(!(wf_u8(a,WF_A_ROLE)&0xc0u)){
            rt->g.global_167a|=128u;
            if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
                uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,timer?timer:0x4000u);
            }
        }
        if(wf_u8(a,WF_A_CONTACT+1u)&16u){
            /* $1110E only emits a sound command. */
            wf_set_be16(a,WF_A_STATE,4u);
            wf_set_u8(a,WF_A_STATE4_LO,(low(a,WF_A_STATE4)!=0x2au&&(wf_u8(a,WF_A_STATE4)&128u))?9u:8u);
        }
    }
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,0xff00u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(((wf_u8(a,WF_A_STATE4)&128u)?0x16u:0x1fu)^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
        if(timer==0xffffu){wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);}
    }
    return WF_OK;
}

/* $1B478, state-four subtype $0E, the collision victim's flying recoil. */
static wf_status_t state4_1b478(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_actor_raw_t *hit=wf_actor_from_rom_ptr(rt,wf_be32(a,0x92u),NULL);
        if(!hit)return blocked(ctx,0x1b486u);
        wf_set_be16(a,WF_A_TIMER44,0u);wf_set_be32(a,WF_A_LINK,wf_be32(a,0x92u));
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(hit,WF_A_X)+((wf_u8(hit,WF_A_FACING)&128u)?16u:0xfff0u)));
        wf_set_be16(a,WF_A_Y,(uint16_t)(wf_be16(hit,WF_A_Y)-1u));
        wf_set_be16(a,WF_A_Z,(uint16_t)(wf_be16(hit,WF_A_Z)+0x48u));
        int inside=1;
        if((rt->g.global_007e&255u)!=1u&&!(rt->g.mode_0161&2u)){
            int32_t dividend=(int32_t)wf_be16(a,WF_A_Y)*256;
            int facing=(wf_u8(a,WF_A_FACING)&128u)!=0;
            dividend+=facing?-0x96000:0x4b000;
            int32_t q=dividend/736,rem=dividend%736;
            uint16_t edge=facing?(uint16_t)-q:(uint16_t)q;
            wf_r17_m68k_move_l(&rt->m68k_regs,3u,((uint32_t)(uint16_t)rem<<16)|edge);
            inside=facing?edge>=wf_be16(a,WF_A_X):edge<wf_be16(a,WF_A_X);
        }
        /* $258E preset 4 or 14, unchanged ROM WORD triplets. */
        uint16_t vx=inside?0xfe80u:0xfd00u;
        if(wf_u8(a,WF_A_FACING)&128u)vx=(uint16_t)(0u-vx);
        wf_set_be16(a,WF_A_VX,vx);wf_set_be16(a,WF_A_VY,0u);
        wf_set_be16(a,WF_A_VZ,inside?0x600u:0x780u);wf_set_be16(a,WF_A_VZ_DEC,inside?0x38u:0x48u);
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        if(inside){
            uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
            if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
            else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,48u);}
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
        }else{
            wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)|4u));
            if(rt->g.mode_0161&1u){
                wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|16u));
                wf_set_be16(hit,0xc4u,(uint16_t)(wf_be16(hit,0xc4u)+1u));
            }
        }
    }else{
        uint8_t contact=wf_u8(a,WF_A_CONTACT),bits=wf_u8(a,WF_A_CONTACT+1u);
        if(contact==6u&&!(wf_u8(a,WF_A_TIMER44+1u)&1u)){
            wf_set_u8(a,WF_A_TIMER44+1u,(uint8_t)(wf_u8(a,WF_A_TIMER44+1u)|1u));
            wf_set_be16(a,WF_A_VX,0u);rt->g.global_1802=14u;
        }
        if(contact==5u)rope_contact_11d56(rt,a);
        /* $10FC6, including its rope-animation writes. */
        if(!(rt->g.mode_0161&2u)&&(contact==1u||contact==5u)){
            if((bits&2u)&&!(rt->g.global_1150&255u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}
            else if(!(bits&2u)&&(bits&1u)&&!(rt->g.global_11b0&255u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
        }else if(contact==3u){wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,0x17u);}
        if(wf_u8(a,WF_A_ANIM_INDEX+1u)==0u&&wf_be16(a,WF_A_ANIM_COUNTER)==0u)
            wf_set_u8(a,WF_A_MOTION_MODE,2u);
        else if(wf_u8(a,WF_A_ANIM_INDEX+1u)==3u){
            wf_set_be16(a,WF_A_FIELD42,0xffe0u);
            int ringside=0,land=0,sound_only=0;
            if(bits&16u){
                ringside=(wf_u8(a,WF_A_FLAGS33)&4u)&&!(rt->g.mode_0161&2u);
                land=!ringside;
            }else if(contact==3u){
                ringside=!(rt->g.mode_0161&2u);sound_only=!ringside;
            }
            if(ringside||land){
                wf_set_be16(a,WF_A_STATE,land?4u:5u);
                if(land)wf_set_u8(a,WF_A_STATE4_LO,5u);
                else{wf_set_be16(a,WF_A_SELECTOR,0x68u);wf_set_be16(a,WF_A_TIMER44,0u);}
                wf_set_be16(a,WF_A_MIRROR,land?0x1fu:0x16u);
                wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
                uint16_t dx=land?0xffc8u:16u;if(wf_u8(a,WF_A_FACING)&128u)dx=(uint16_t)(0u-dx);
                wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            }
            if(ringside||land||sound_only)wf_set_be16(a,WF_A_FIELD42,0u);
        }
    }
    /* $1C0C8 consumes $1B460, even after the callback changes state. */
    static const uint16_t ticks[4]={4u,16u,16u,0xff00u},poses[4]={0xb1u,0xb1u,0xb2u,0xb3u};
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,ticks[0]);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[0]^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
        if(timer==0xffffu){
            unsigned index=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);
            wf_set_be16(a,WF_A_ANIM_INDEX,index<4u?(uint16_t)index:0xfeu);
            wf_set_be16(a,WF_A_ANIM_COUNTER,index<4u?ticks[index]:0u);
            if(index<4u)wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[index]^wf_be16(a,WF_A_FACING)));
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
        if(d->callback_rom==0x1b1d0u){
            s=state4_1b1d0(rt,a,ctx);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b478u){
            s=state4_1b478(rt,a,ctx);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
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
    if(id==6u&&d->callback_rom==0x00011de0u){
        main_six_11de0(a);exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==8u&&d->callback_rom==0x00011f3au){
        s=main_eight_12078(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==9u&&d->callback_rom==0x000121e6u){
        s=main_nine_121e6(a,ctx);if(s!=WF_OK)return s;
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
