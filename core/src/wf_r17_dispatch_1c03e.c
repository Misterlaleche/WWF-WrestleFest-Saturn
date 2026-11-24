#include <stddef.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_animation.h"
#include "wf_r17_resolver_280dc.h"

static uint8_t low(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o+1u);}
/* Original $1110E selects the mat/ringside impact without changing D0. */
static void landing_audio_1110e(wf_runtime_t *rt,const wf_actor_raw_t *a){
    wf_runtime_emit_audio(rt,(wf_u8(a,WF_A_FLAGS33)&4u)?0x2du:0x29u);
}
static wf_status_t blocked(wf_r17_dispatch_1c03e_ctx_t *c,uint32_t pc){if(c)c->blocked_dependency_rom=pc;return WF_BLOCKED;}
/* $1BFFE/$1C004: raw5C release victim. Kind-zero descriptor; the
 * callback owns the timer, including the decrement on the init call. */
static void state4_1c004(wf_actor_raw_t*a){
    int latched=(wf_u8(a,WF_A_LATCHED)&0x80u)!=0u;
    wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
    if(!latched){wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0x17du|wf_be16(a,WF_A_FACING)));wf_set_be16(a,WF_A_ANIM_COUNTER,8u);}
    uint16_t timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
    if(!timer){wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,2u);wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^0x80u));}
}
/* $1BECC/$1BED8: raw2B release pose, then grounded BYTE subtype5. */
static void state4_1bed8(wf_actor_raw_t*a){
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){wf_set_u8(a,WF_A_MOTION_MODE,0u);return;}
    if(low(a,WF_A_ANIM_INDEX)==0xfeu){
        wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
        wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
    }
}
static void exact_tail_1c0b4(wf_actor_raw_t *a){
    if((wf_u8(a,WF_A_FIELD18)&0x80u)!=0u){wf_set_be16(a,WF_A_FIELD18,0u);wf_set_be16(a,WF_A_FIELD1A,0u);}
}
/* Main state zero and raw $3D share ROM $114B2, with a kind-zero
 * descriptor. Use the same fully owned callback for both dispatch domains. */
static wf_status_t main_zero_114b2(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    wf_r17_actions_r11_trace_t trace={0};
    wf_status_t status=wf_r17_action_idle_114b2(rt,a,&trace);
    if(status!=WF_OK)return blocked(ctx,trace.blocked_dependency_rom);
    return WF_OK;
}
/* ROM $11652 movement family: AE 0/5..11 share $116C6; the other
 * table entries are $11796/$1192C/$11A20/$11ABA/$11B6C. */
static int movement_bit(const wf_actor_raw_t*a,unsigned off,unsigned bit){return (wf_u8(a,off)&(1u<<bit))!=0u;}
static void movement_set(wf_actor_raw_t*a,unsigned off,unsigned mask){wf_set_u8(a,off,(uint8_t)(wf_u8(a,off)|mask));}
static void movement_clear(wf_actor_raw_t*a,unsigned off,unsigned mask){wf_set_u8(a,off,(uint8_t)(wf_u8(a,off)&~mask));}
static wf_status_t movement_unlink(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_dispatch_1c03e_ctx_t*ctx){
    return wf_actor_unlink_115d2(rt,a)==WF_OK?WF_OK:blocked(ctx,0x115deu);
}
static void movement_magnitude(wf_actor_raw_t*a){
    static const uint8_t magnitude[12]={27,29,28,25,30,27,24,30,22,27,28,27};
    unsigned v=movement_bit(a,0x32u,3u)?12u:magnitude[wf_be16(a,2u)];
    if(movement_bit(a,0x56u,7u)&&movement_bit(a,0x33u,5u))v=(v+6u)&255u;
    if(movement_bit(a,0x74u,7u))v=(v-8u)&65535u;
    wf_set_be16(a,0x2au,(uint16_t)v);
}
static unsigned movement_descriptor(wf_actor_raw_t*a){
    if(movement_bit(a,0x74u,7u)){wf_set_u8(a,0x1bu,16u);return movement_bit(a,0x75u,0u)?3u:2u;}
    return movement_bit(a,0x32u,3u)?1u:0u;
}
static int movement_tag_1d526(wf_runtime_t*rt,wf_actor_raw_t*a){
    uint16_t timer=wf_be16(a,0xe6u);if(!movement_bit(a,0x34u,0u)||!timer)return 0;
    wf_set_be16(a,0xe6u,(uint16_t)(timer-1u));if(timer!=1u)return 0;
    movement_set(a,0x56u,0x40u);movement_clear(a,0x34u,3u);
    uint32_t ptr=wf_actor_rom_base((unsigned)(a-rt->actors));if(movement_bit(a,0x33u,7u))rt->g.global_1688=ptr;else rt->g.global_1684=ptr;
    if(movement_bit(a,0xc6u,6u)){movement_clear(a,0xc6u,0x40u);movement_set(a,0x33u,0x20u);}
    rt->g.global_1682=0x177u;wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x4eu);return 1;
}
static void movement_heal(wf_actor_raw_t*a){
    uint16_t timer=(uint16_t)(wf_be16(a,0x44u)-1u);wf_set_be16(a,0x44u,timer);
    if(!timer){wf_set_be16(a,0x44u,148u);if(wf_be16(a,0x72u)>wf_be16(a,0x66u)){wf_set_be16(a,0x66u,(uint16_t)(wf_be16(a,0x66u)+1u));wf_set_be16(a,0x6au,1u);}}
}
static int movement_arrived(wf_runtime_t*rt,wf_actor_raw_t*a){
    uint16_t x=wf_be16(a,6u),y=wf_be16(a,10u),tx=wf_be16(a,0xbeu),ty=wf_be16(a,0xc0u);
    if((unsigned)(x>=tx?x-tx:tx-x)<8u&&(unsigned)(y>=ty?y-ty:ty-y)<8u)return 1;
    static const uint8_t base[4]={0,192,64,128};unsigned quadrant=0;
    uint16_t dx=(uint16_t)(tx-x),dy=(uint16_t)(ty-y);
    if(dx&0x8000u){quadrant|=1u;dx=(uint16_t)(0u-dx);}if(dy&0x8000u){quadrant|=2u;dy=(uint16_t)(0u-dy);}
    if(dx>255u)dx=255u;
    if(dy>255u)dy=255u;
    unsigned sum=(unsigned)dx+dy,component=(quadrant==0u||quadrant==3u)?dx:dy;
    uint8_t angle=(uint8_t)(base[quadrant]+component*64u/(sum?sum:1u));uint8_t*s=rt->g.direction_scratch_15e4;
    s[0]=angle;s[1]=(uint8_t)(wf_actor_rom_base((unsigned)(a-rt->actors))>>16);s[2]=(uint8_t)(dx>>8);s[3]=(uint8_t)dx;s[4]=(uint8_t)(dy>>8);s[5]=(uint8_t)dy;s[6]=(uint8_t)(ty>>8);s[7]=(uint8_t)ty;
    wf_set_u8(a,0x2du,angle);return 0;
}
static void movement_tick(wf_actor_raw_t*a,unsigned descriptor){
    static const uint16_t periods[4][4]={{10,10,10,10},{12,12,12,12},{20,16,20,16},{20,16,20,16}};
    static const uint16_t first[4]={1,5,0x7c,0x80};unsigned index=wf_be16(a,0x24u);
    if(movement_bit(a,0x1cu,7u)){
        uint16_t old=wf_be16(a,0x22u);if(old==0xff00u)return;wf_set_be16(a,0x22u,(uint16_t)(old-1u));if(old)return;
        index=(index+1u)&65535u;if(descriptor==4u){wf_set_be16(a,0x24u,0xfeu);wf_set_be16(a,0x22u,0u);return;}if(index>=4u)index=0;
    }else{index=0;movement_set(a,0x1cu,0x80u);}
    wf_set_be16(a,0x24u,(uint16_t)index);wf_set_be16(a,0x22u,descriptor==4u?0xff00u:periods[descriptor][index]);wf_set_be16(a,4u,(uint16_t)((descriptor==4u?0xffffu:first[descriptor]+index)^wf_be16(a,0x2eu)));
}
static wf_status_t main_one_11652(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    unsigned sub=wf_be16(a,0xaeu)&63u,descriptor=0u;int latched=movement_bit(a,0x1cu,7u);wf_status_t status;
    if(sub>12u||wf_be16(a,2u)>=12u)return blocked(ctx,0x11652u);
    if(!(rt->g.credited_route&0x8000u))movement_clear(a,0x32u,8u);
    if(sub==0u||(sub>=5u&&sub<=11u)){
        if(!latched){wf_set_u8(a,1u,1u);movement_clear(a,0x33u,0x40u);}movement_magnitude(a);
        status=movement_unlink(rt,a,ctx);if(status!=WF_OK)return status;wf_set_be16(a,0x4cu,low(a,0xaeu)<5u&&movement_bit(a,0x74u,7u)?14u:1u);
        descriptor=movement_descriptor(a);if(wf_be16(a,0xfeu))wf_set_be16(a,0x20u,0u);
    }else if(sub==4u){
        if(!latched){
            wf_set_be16(a,0x18u,0u);wf_set_be16(a,0x1au,0u);wf_set_u8(a,1u,1u);movement_clear(a,0x34u,0x10u);
            int release=(wf_u8(a,0x34u)&0xc0u)!=0;movement_clear(a,0x34u,0xc0u);
            if(release){wf_actor_raw_t*t=wf_actor_from_rom_ptr(rt,wf_be32(a,0x7au),0);if(!t)return blocked(ctx,0x11aecu);movement_clear(t,0x34u,0xc0u);}
            movement_magnitude(a);uint8_t face=wf_u8(a,0x33u)&0x80u;wf_set_u8(a,0x2eu,face);wf_set_be16(a,0x2cu,(uint8_t)((face^0x80u)+0x40u));
        }else if(movement_bit(a,0x34u,0u)){movement_clear(a,0x34u,2u);wf_set_be16(a,0x20u,0u);wf_set_be16(a,0xaeu,0u);}
        else{status=movement_unlink(rt,a,ctx);if(status!=WF_OK)return status;if(wf_u8(a,0x37u)&(movement_bit(a,0x33u,7u)?1u:2u)){wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x4fu);}}
        descriptor=movement_descriptor(a);
    }else if(sub==1u){
        if(!latched){
            movement_set(a,0x32u,1u);movement_clear(a,0x32u,8u);movement_clear(a,0x33u,0x40u);wf_set_be16(a,0x2au,16u);wf_set_be16(a,0x12u,4u);wf_set_be16(a,0x44u,148u);wf_set_be16(a,0x46u,0u);
            uint16_t y=wf_be16(a,10u),x=movement_bit(a,0x33u,7u)?(uint16_t)(0xe48u-y):(uint16_t)(y+0x580u);wf_set_be16(a,6u,x>>2);
        }else{
            wf_set_u8(a,0x2eu,(wf_u8(a,0x33u)&0x80u)^0x80u);
            if(movement_bit(a,0xfeu,7u)){wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x4eu);goto tick;}
            if(movement_tag_1d526(rt,a))goto tick;
            status=movement_unlink(rt,a,ctx);if(status!=WF_OK)return status;
            if(!movement_bit(a,0x33u,5u)){wf_set_be16(a,0xc8u,(uint16_t)(wf_be16(a,0xc8u)+1u));if(movement_bit(a,0xc6u,6u)){wf_set_be16(a,0x20u,1u);wf_set_be16(a,0xaeu,3u);goto tick;}}
            movement_heal(a);
            if(movement_bit(a,0x34u,5u)){
                uint16_t t=wf_be16(a,0x46u);if(!t)t=32u;wf_set_be16(a,0x46u,--t);
                if(!t){movement_clear(a,0x34u,0x20u);wf_set_be16(a,4u,wf_be16(a,0x2eu));}
                else{wf_set_u8(a,1u,0u);wf_set_be16(a,4u,0x1d3u|wf_be16(a,0x2eu));wf_set_be16(a,0x22u,2u);}
            }else{
                uint8_t angle=wf_u8(a,0x2du);if(angle){wf_set_u8(a,1u,1u);uint16_t y=wf_be16(a,10u);if(angle>=0x40u&&angle<0xc0u){if(y>0x120u)goto tick;wf_set_be16(a,10u,0x120u);}else{if(y<0x192u)goto tick;wf_set_be16(a,10u,0x192u);}}
                wf_set_u8(a,1u,0u);wf_set_u8(a,4u,wf_u8(a,0x2eu));descriptor=4u;
            }
        }
    }else if(sub==2u){
        if(!latched){wf_set_u8(a,1u,1u);wf_set_be16(a,0x2au,16u);wf_set_be16(a,0x44u,1u);movement_clear(a,0xaeu,0x80u);wf_set_u8(a,0x2eu,(wf_u8(a,0x33u)&0x80u)^0x80u);wf_set_be16(a,0x2cu,movement_bit(a,0x33u,7u)?0x78u:8u);}
        if(!movement_bit(a,0x33u,5u))wf_set_be16(a,0xc8u,(uint16_t)(wf_be16(a,0xc8u)+1u));
        if(movement_bit(a,0xc6u,6u)){movement_clear(a,0xc6u,0x40u);movement_set(a,0x33u,0x20u);}movement_heal(a);
        if(movement_bit(a,0xfeu,7u)){wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x4eu);goto tick;}
        if(movement_tag_1d526(rt,a))goto tick;
        if(!movement_bit(a,0xaeu,7u)){
            uint16_t y=wf_be16(a,10u);if(movement_bit(a,0x33u,7u)){if(y>=0x130u)goto tick;wf_set_be16(a,10u,0x130u);}else{if(y<=0x182u)goto tick;wf_set_be16(a,10u,0x182u);}movement_set(a,0xaeu,0x80u);
        }
        wf_set_u8(a,1u,0u);wf_set_u8(a,4u,wf_u8(a,0x2eu));wf_set_be16(a,0x22u,2u);
    }else if(sub==3u){
        if(!latched){wf_set_be16(a,0x2au,16u);uint16_t angle=movement_bit(a,0x33u,7u)?0xf8u:8u;if(wf_be16(a,10u)>=0x158u)angle^=0x80u;wf_set_be16(a,0x2cu,angle);wf_set_u8(a,1u,1u);}
        if(movement_tag_1d526(rt,a))goto tick;
        uint8_t angle=wf_u8(a,0x2du);int lowangle=angle==8u||angle==0xf8u;
        if((wf_be16(a,10u)>=0x158u)==lowangle){
            if(movement_bit(a,0xc6u,6u)){wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x71u);}else{wf_set_be16(a,0x20u,1u);wf_set_be16(a,0xaeu,1u);}
        }
    }else if(sub==12u){
        if(!latched){wf_set_be16(a,0x34u,0x400u);wf_set_u8(a,1u,1u);movement_magnitude(a);wf_set_be16(a,0xc0u,0x160u);wf_set_be16(a,0xbeu,(rt->g.mode_0161&1u)?0x2c0u:movement_bit(a,0x33u,0u)?0x254u:0x29cu);}
        if(!movement_bit(a,0xaeu,7u)){
            wf_set_u8(a,0x2eu,(wf_u8(a,0x2du)&0x80u)^0x80u);descriptor=movement_descriptor(a);if(!movement_arrived(rt,a))goto tick;
            movement_set(a,0xaeu,0x80u);wf_set_u8(a,1u,0u);wf_set_be16(a,0x22u,0xffffu);wf_set_be16(a,6u,wf_be16(a,0xbeu));wf_set_be16(a,10u,wf_be16(a,0xc0u));
        }
        if(rt->g.mode_0161&1u){wf_set_be16(a,0x20u,0u);wf_set_be16(a,4u,0u);wf_set_be16(a,0x2eu,0u);movement_clear(a,0x33u,2u);}
        else{
            movement_set(a,0x2eu,0x80u);wf_actor_raw_t*t=wf_actor_from_rom_ptr(rt,wf_be32(a,0x86u),0);if(!t)return blocked(ctx,0x11c08u);
            if(wf_be16(t,6u)<wf_be16(a,6u))movement_clear(a,0x2eu,0x80u);
            wf_set_be16(a,4u,wf_be16(a,0x2eu));
            if(low(t,0x20u)==1u&&wf_be16(t,0xaeu)==0x800cu){wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x8bu);wf_set_be16(t,0x20u,5u);wf_set_be16(t,0x60u,0x8bu);}
        }
    }
    tick:movement_tick(a,descriptor);return WF_OK;
}
/* $10B62 preserves D0 across $280DC and commits only the X correction.
 * $124FA uses it twice to settle the linked actors at the arcade offsets. */
static wf_status_t hold_offset_10b62(wf_runtime_t *rt,wf_actor_raw_t *a,
                                      uint16_t displacement,
                                      wf_r17_dispatch_1c03e_ctx_t *ctx){
    uint16_t contact=wf_be16(a,WF_A_CONTACT);
    wf_set_be16(a,WF_A_FIELD3E,displacement);
    wf_status_t s=wf_r17_resolver_280dc(rt,a);
    if(s!=WF_OK)return blocked(ctx,0x000280dcu);
    if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
    wf_set_be16(a,WF_A_CONTACT,contact);
    wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+wf_be16(a,WF_A_CORR_X)));
    wf_set_be16(a,WF_A_FIELD3E,0u);
    return WF_OK;
}
/* $1C0C8 with the one-entry $125C0 descriptor. Its pose still XORs facing. */
static void held_tick_125c0(wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,0xff00u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0xffffu^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t timer=wf_be16(a,WF_A_ANIM_COUNTER);
        wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(timer-1u));
        if(!timer){wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);}
    }
}
/* $125CC is selected directly by latched low state FF, before $1C082. */
static wf_status_t held_pose_125cc(wf_runtime_t *rt,wf_actor_raw_t *a,
                                    wf_r17_dispatch_1c03e_ctx_t *ctx){
    wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
    if(!linked)return blocked(ctx,0x000125d6u);
    uint32_t back=wf_be32(linked,WF_A_LINK)&0x00ffffffu;
    wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_MIRROR,0xffffu);
    wf_r17_m68k_move_l(&rt->m68k_regs,1u,back);
    if(back!=wf_actor_rom_base((unsigned)(a-rt->actors))){
        wf_set_be16(a,WF_A_STATE,0u);wf_set_be32(a,WF_A_LINK,0u);wf_set_be32(a,WF_A_SECONDARY_LINK,0u);
        wf_set_be16(a,WF_A_FLAGS32,(uint16_t)(wf_be16(a,WF_A_FLAGS32)&0x0087u));
        wf_set_be16(a,WF_A_FLAGS34,(uint16_t)(wf_be16(a,WF_A_FLAGS34)&0x0300u));
        wf_set_be16(a,WF_A_CPU_B4,0u);wf_set_be16(a,WF_A_CPU_B6,0u);
    }
    held_tick_125c0(a);return WF_OK;
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
        /* $124B6 selects the held descriptor for the animation tail only. */
        wf_set_be16(a,WF_A_MIRROR,0xffffu);
        held_tick_125c0(a);
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
            unsigned idx=(unsigned)wf_be16(a,WF_A_ANIM_INDEX)+1u;
            if(idx>=4u)idx=0u; /* $1C10A/$1C11E: kind-2 FE sentinel wraps to pose zero. */
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
                unsigned idx=(unsigned)wf_be16(a,WF_A_ANIM_INDEX)+1u;
                if(idx>=4u)idx=0u; /* $1C10A/$1C11E: kind-2 FE sentinel wraps to pose zero. */
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
                wf_runtime_emit_audio(rt,0x28u); /* $11CF2 */
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
    if(!wf_be16(a,WF_A_ANIM_COUNTER)&&
       (low(a,WF_A_ANIM_INDEX)==1u||low(a,WF_A_ANIM_INDEX)==4u))
        wf_runtime_emit_audio(rt,0x33u); /* $11D42 -> $1112E */
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

/* $11F3A selects $11FAE (routes0..3), $12078 (4..7), or
 * $12122 (8..9), then the matching original animation descriptor. */
static wf_status_t main_eight_12078(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    unsigned route=wf_be16(a,WF_A_TIMER44);
    if(route>=10u)return blocked(ctx,0x11f3au);
    static const uint16_t coords[6][2]={{0x1dcu,0x193u},{0x314u,0x193u},{0x1bdu,0x120u},{0x338u,0x120u},{0x278u,0x160u},{0x3c8u,0x160u}};
    unsigned staged=route<4u||route>=8u;
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|2u));
        unsigned coord=route<8u?(route&3u):route-4u;
        wf_set_be16(a,WF_A_X,coords[coord][0]);wf_set_be16(a,WF_A_Y,coords[coord][1]);
        if(staged){
            wf_set_be16(a,WF_A_Z,0x140u);wf_set_u8(a,WF_A_FACING,(route&1u)?128u:0u);
            if(route<4u){
                wf_set_be16(a,WF_A_RENDER_LIST,2u);
                if(!(rt->g.mode_0161&1u))wf_set_be16(a,0x4au,1u);
            }
        }else{
            wf_set_be16(a,WF_A_AE,0u);wf_set_be16(a,WF_A_RENDER_LIST,(route&2u)?4u:0u);
            wf_set_be16(a,WF_A_X,(uint16_t)(coords[coord][0]+((route&1u)?24u:0xffe8u)));
            wf_set_be16(a,WF_A_Z,0x180u);wf_set_u8(a,WF_A_FACING,(route&1u)?0u:128u);
        }
        rt->g.global_15d2=wf_u8(a,WF_A_CHARACTER+1u);rt->g.global_15d3=0x17u;
        wf_runtime_emit_audio(rt,0x32u); /* $12008/$120FE/$1217A */
    }else if(staged&&wf_u8(a,WF_A_ANIM_INDEX+1u)==1u&&wf_be16(a,WF_A_ANIM_COUNTER)==0u){
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+((route&1u)?24u:0xffe8u)));
        wf_set_be16(a,WF_A_Z,0x180u);wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));
        if(route<4u)wf_set_be16(a,WF_A_RENDER_LIST,(route&2u)?0u:4u);
    }else if(wf_u8(a,WF_A_ANIM_INDEX+1u)==0xfeu){
        wf_set_be16(a,WF_A_STATE,9u);
        if(route<8u)wf_set_be16(a,WF_A_RENDER_LIST,0u);
    }
    /* Descriptors $11F0A/$11EA6/$11F26/$11ECA/$11EE6; every delay is12. */
    static const uint8_t counts[5]={5,7,3,5,7};
    static const uint16_t poses[5][7]={{0x66,0x67,0x2d,0x2e,0x2f},{0x66,0x67,0x31,0x32,0x33,0x34,0x35},{0x2d,0x2e,0x2f},{0x31,0x32,0x33,0x34,0x35},{0x66,0x67,0x31,0x32,0x33,0x34,0x37}};
    unsigned row=route>=8u?4u:route>=4u?2u+((route&2u)?1u:0u):((route&2u)?1u:0u),count=counts[row];
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,12u);
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[row][0]^wf_be16(a,WF_A_FACING)));
        wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
    }else if(wf_be16(a,WF_A_ANIM_COUNTER)!=0xff00u){
        uint16_t timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);
        wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
        if(timer==0xffffu){
            uint16_t index=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);
            wf_set_be16(a,WF_A_ANIM_INDEX,index<count?index:0xfeu);
            wf_set_be16(a,WF_A_ANIM_COUNTER,index<count?12u:0u);
            if(index<count)wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[row][index]^wf_be16(a,WF_A_FACING)));
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

/* $122E6: finish the turnbuckle descent selected by state9. The callback
 * chooses one of three original descriptors; all use 16-tick, kind-one poses. */
static wf_status_t main_ten_122e6(wf_runtime_t *rt,wf_actor_raw_t *a,
                                   wf_r17_dispatch_1c03e_ctx_t *ctx,unsigned *variant){
    /* TAG indexes from $121C8, including the following descriptor bytes for
     * route 6/7. Royal indexes the two coordinate pairs at $121D8. */
    static const uint16_t tag_coords[8][2]={
        {0x01dcu,0x0193u},{0x0314u,0x0193u},{0x01bdu,0x0120u},{0x0338u,0x0120u},
        {0x0278u,0x0160u},{0x03c8u,0x0160u},{0x0001u,0x21e6u},{0x0000u,0x0828u}};
    static const uint16_t royal_coords[2][2]={{0x0278u,0x0160u},{0x03c8u,0x0160u}};
    uint16_t route=wf_be16(a,WF_A_TIMER44);
    uint8_t low44=wf_u8(a,WF_A_TIMER44+1u);
    unsigned royal=(rt->g.mode_0161&2u)!=0u;
    unsigned latched=(wf_u8(a,WF_A_LATCHED)&0x80u)!=0u;
    unsigned royal_select=0u;
    if(!latched){
        if((royal&&(route<8u||route>=10u))||(!royal&&route>=8u))return blocked(ctx,0x12318u);
        if(royal)wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~4u));
        const uint16_t *xy=royal?royal_coords[route-8u]:tag_coords[route];
        wf_set_be16(a,WF_A_X,xy[0]);wf_set_be16(a,WF_A_Y,xy[1]);
        wf_set_be16(a,WF_A_Z,0x180u);
        uint16_t offset=(low44&1u)?24u:0xffe8u;
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,offset);
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+offset));
        wf_set_be16(a,WF_A_RENDER_LIST,(low44&2u)?0u:4u);
        royal_select=royal;
    }else if(!royal){
        if(((low44&2u)?wf_u8(a,WF_A_ACTION_PHASE)==4u:
             wf_u8(a,WF_A_ACTION_PHASE)==2u)&&!wf_be16(a,WF_A_ANIM_COUNTER)){
            wf_set_be16(a,WF_A_RENDER_LIST,2u);wf_set_be16(a,WF_A_Z,0x140u);
        }
        if(wf_u8(a,WF_A_ACTION_PHASE)==0xfeu){
            wf_set_be16(a,WF_A_STATE,0u);
            wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
            rt->g.global_1670=0u;wf_set_be16(a,0x4au,0u);
            wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~2u));
            uint16_t offset=(low44&1u)?0xffd8u:40u;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,offset);
            wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+offset));
        }
    }else{
        if(wf_u8(a,WF_A_ACTION_PHASE)==4u&&!wf_be16(a,WF_A_ANIM_COUNTER))
            wf_set_be16(a,WF_A_Z,0x140u);
        if(wf_u8(a,WF_A_ACTION_PHASE)==0xfeu){
            royal_select=1u;
            wf_set_be16(a,WF_A_STATE,0u);
            wf_set_be16(a,WF_A_MIRROR,wf_be16(a,WF_A_FACING));
            rt->g.global_1670=0u;
            wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~4u));
            wf_set_be16(a,0x4au,0u);
            wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~2u));
            uint16_t offset=(low44&1u)?0xffe8u:24u;
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,offset);
            wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+offset));
        }
    }
    *variant=royal?(royal_select?2u:0u):((low44&2u)?0u:1u);
    if(royal_select)wf_set_be16(a,WF_A_RENDER_LIST,0u);
    return WF_OK;
}
static void main_ten_tick(wf_actor_raw_t *a,unsigned variant){
    static const uint16_t poses[3][7]={
        {0x35u,0x34u,0x33u,0x32u,0x31u,0x66u,0x67u},
        {0x2fu,0x2eu,0x2du,0x66u,0x67u,0u,0u},
        {0x37u,0x34u,0x33u,0x32u,0x31u,0x66u,0x67u}};
    unsigned count=variant==1u?5u:7u;
    uint16_t index=wf_be16(a,WF_A_ANIM_INDEX);
    if(!(wf_u8(a,WF_A_LATCHED)&0x80u))index=0u;
    else{
        uint16_t timer=wf_be16(a,WF_A_ANIM_COUNTER);
        if(timer==0xff00u)return;
        wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(timer-1u));if(timer)return;
        index=(uint16_t)(index+1u);
        if(index>=count){wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);
            wf_set_be16(a,WF_A_ANIM_COUNTER,0u);return;}
    }
    wf_set_be16(a,WF_A_ANIM_INDEX,index);wf_set_be16(a,WF_A_ANIM_COUNTER,16u);
    wf_set_be16(a,WF_A_MIRROR,(uint16_t)(poses[variant][index]^wf_be16(a,WF_A_FACING)));
    wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|0x80u));
}

/* Exact $1C0C8/$1C12C animation tail for a ROM-derived descriptor.
 * The callback may change state but A1 still names the original descriptor. */
static wf_status_t tick_descriptor(wf_actor_raw_t *a,const wf_anim_desc_t *d){
    if(!d->kind)return WF_OK;
    if(d->stored_entries<d->entry_count||!d->entry_count)return WF_BAD_STATE;
    uint16_t index=wf_be16(a,WF_A_ANIM_INDEX);
    if(!(wf_u8(a,WF_A_LATCHED)&128u))index=0u;
    else{
        uint16_t timer=wf_be16(a,WF_A_ANIM_COUNTER);if(timer==0xff00u)return WF_OK;
        wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(timer-1u));if(timer)return WF_OK;
        index=(uint16_t)(index+1u);
        if(index>=d->entry_count){
            if(d->kind!=2u){wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);return WF_OK;}
            index=0u;
        }
    }
    const wf_anim_entry_t *entry=&wf_anim_entries[d->entry_start+index];
    wf_set_be16(a,WF_A_ANIM_INDEX,index);wf_set_be16(a,WF_A_ANIM_COUNTER,entry->counter_raw);
    wf_set_be16(a,WF_A_MIRROR,(uint16_t)(entry->pose_raw^wf_be16(a,WF_A_FACING)));
    wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));return WF_OK;
}
/* $1BCF6/$1BD0E, raw54's victim subtype $1E. EXG A0,A2 makes $10B9A
 * position the attacker's own linked actor, which need not be this victim. */
static wf_status_t state4_1bd0e(wf_runtime_t*rt,wf_actor_raw_t*a,
                               wf_r17_dispatch_1c03e_ctx_t*ctx,unsigned*alternate){
    *alternate=0u;
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        uint32_t saved_d0=rt->m68k_regs.d[0];wf_set_u8(a,WF_A_MOTION_MODE,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,40u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,20u);
        wf_actor_raw_t*attacker=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
        if(!attacker)return blocked(ctx,0x10b9eu);
        wf_actor_raw_t*recipient=wf_actor_from_rom_ptr(rt,wf_be32(attacker,WF_A_LINK),0);
        if(!recipient)return blocked(ctx,0x10ba2u);
        /* Literal $10BA2..$10BC6: copy all coordinates before translating. */
        wf_set_be16(recipient,WF_A_X,wf_be16(attacker,WF_A_X));
        wf_set_be16(recipient,WF_A_Y,wf_be16(attacker,WF_A_Y));
        wf_set_be16(recipient,WF_A_Z,wf_be16(attacker,WF_A_Z));
        uint16_t dx=(wf_u8(attacker,WF_A_FACING)&128u)?0xffd8u:40u;
        wf_set_be16(recipient,WF_A_X,(uint16_t)(wf_be16(recipient,WF_A_X)+dx));
        wf_set_be16(recipient,WF_A_Y,(uint16_t)(wf_be16(recipient,WF_A_Y)-1u));
        wf_set_be16(recipient,WF_A_Z,(uint16_t)(wf_be16(recipient,WF_A_Z)+20u));
        wf_runtime_emit_audio(rt,0x29u); /* $1BD34; caller D0 is restored below. */
        rt->m68k_regs.d[0]=saved_d0;
    }else{
        if(low(a,WF_A_ANIM_INDEX)!=0xfeu)return WF_OK;
        wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,0x100fu);
    }
    *alternate=low(a,WF_A_SELECTOR)!=0x0fu;return WF_OK;
}
static wf_status_t state4_1bd0e_tick(wf_actor_raw_t*a,const wf_anim_desc_t*d,unsigned alternate){
    int loads=!(wf_u8(a,WF_A_LATCHED)&128u)||
        (wf_be16(a,WF_A_ANIM_COUNTER)==0u&&wf_be16(a,WF_A_ANIM_INDEX)==0xffffu);
    wf_status_t status=tick_descriptor(a,d);
    /* $1BD02 has the same kind/count/period16 as $1BCF6, pose $1AE. */
    if(status==WF_OK&&alternate&&loads&&wf_be16(a,WF_A_ANIM_INDEX)==0u)
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0x1aeu^wf_be16(a,WF_A_FACING)));
    return status;
}

/* $1BB08, shared by state4 $18/$29. The callback keeps the original
 * descriptor for $1C0C8 even after $29 returns to state4/subtype3. */
static void state4_1bb08(wf_actor_raw_t*a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,0x54u,64u);
    }else if(low(a,WF_A_ANIM_INDEX)==0xfeu){
        wf_set_be16(a,WF_A_STATE,0u);
        if(low(a,WF_A_STATE4)==0x29u){wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,3u);}
    }
}

/* $1B7A2: linked victim, descriptor $1B78A or the local $1B796 alternate.
 * Terminal exits retain the incoming descriptor even after changing subtype. */
static wf_status_t state4_1b7a2(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_dispatch_1c03e_ctx_t*ctx,unsigned*alternate){
    *alternate=0u;
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));wf_set_be32(a,WF_A_LINK,wf_be32(a,WF_A_RECIP_LINK));
        wf_actor_raw_t*t=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),NULL);if(!t)return blocked(ctx,0x1b7b6u);
        wf_set_be16(a,WF_A_X,wf_be16(t,WF_A_X));wf_set_be16(a,WF_A_Y,wf_be16(t,WF_A_Y));wf_set_be16(a,WF_A_Y,(uint16_t)(wf_be16(a,WF_A_Y)-1u));wf_set_be16(a,WF_A_Z,wf_be16(t,WF_A_Z));wf_set_u8(a,WF_A_MOTION_MODE,0u);
    }else if(low(a,WF_A_ANIM_INDEX)==0xfeu){
        unsigned high=low(a,WF_A_PRIOR)==2u;wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,high?0x1bu:2u);
        uint8_t face=(uint8_t)(wf_u8(a,WF_A_FACING)^128u);wf_set_u8(a,WF_A_FACING,face);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0xffffu);wf_r17_m68k_move_w(&rt->m68k_regs,2u,high?64u:16u);
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+(face&128u?0xfff8u:8u)));wf_set_be16(a,WF_A_Y,(uint16_t)(wf_be16(a,WF_A_Y)-1u));wf_set_be16(a,WF_A_Z,(uint16_t)(wf_be16(a,WF_A_Z)+(high?64u:16u)));
        if(!high){wf_set_be16(a,WF_A_MIRROR,0x11u);wf_set_u8(a,WF_A_MIRROR,face);}return WF_OK;
    }
    *alternate=low(a,WF_A_STATE4)!=0x14u;return WF_OK;
}
/* Both original descriptors have kind1/count1/counter12. $1B796 is a local
 * callback-selected descriptor absent from the generated dispatch catalog. */
static void state4_1b7a2_tick(wf_actor_raw_t*a,unsigned alternate){
    uint16_t index=0;
    if(wf_u8(a,WF_A_LATCHED)&128u){
        uint16_t timer=wf_be16(a,WF_A_ANIM_COUNTER);if(timer==0xff00u)return;
        wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)(timer-1u));if(timer)return;
        index=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);
        if(index>=1u){wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);wf_set_be16(a,WF_A_ANIM_COUNTER,0);return;}
    }
    wf_set_be16(a,WF_A_ANIM_INDEX,index);wf_set_be16(a,WF_A_ANIM_COUNTER,12u);wf_set_be16(a,WF_A_MIRROR,(uint16_t)((alternate?0x196u:0x195u)^wf_be16(a,WF_A_FACING)));wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
}

/* $1B706/$1B700, state4 subtypes $10/$11/$12: hide for 32 owner calls.
 * Kind-zero animation leaves the callback's $FF00 counter unchanged. */
static void state4_1b706(wf_actor_raw_t *a){
    wf_set_be16(a,WF_A_MIRROR,0xffffu);wf_set_be16(a,WF_A_ANIM_COUNTER,0xff00u);
    int latched=(wf_u8(a,WF_A_LATCHED)&128u)!=0u;
    wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
    if(!latched)wf_set_be16(a,WF_A_TIMER44,32u);
    uint16_t timer=(uint16_t)(wf_be16(a,WF_A_TIMER44)-1u);wf_set_be16(a,WF_A_TIMER44,timer);
    if(!timer)wf_set_be16(a,WF_A_STATE,0u);
}
/* $1BCAA/$1BC9E: victim of the linked jumping attack. */
static wf_status_t state4_1bcaa(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)||low(a,WF_A_ANIM_INDEX)==0xfeu){
        wf_actor_raw_t *linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),NULL);
        if(!linked)return blocked(ctx,0x1bcb2u);
        if(!(wf_u8(a,WF_A_LATCHED)&128u))wf_set_be16(linked,WF_A_STATE,0xffu);
        else{
            wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,2u);
            wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));
            wf_set_be16(linked,WF_A_STATE,0u);
            wf_set_u8(a,WF_A_LINK,(uint8_t)(wf_u8(a,WF_A_LINK)|128u));
            wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~64u));
            wf_set_u8(linked,WF_A_FLAGS33,(uint8_t)(wf_u8(linked,WF_A_FLAGS33)&~64u));
        }
    }
    return WF_OK;
}

/* $115D2 removes stale composition links without altering the current pose. */
static wf_status_t unlink_115d2(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    return wf_actor_unlink_115d2(rt,a)==WF_OK?WF_OK:blocked(ctx,0x115deu);
}
/* $1B090/$1B0CC/$1B3DE: hit stun and its recovery continuations. */
static wf_status_t state4_recover(wf_runtime_t *rt,wf_actor_raw_t *a,uint32_t pc,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);
        if(pc==0x1b0ccu){
            wf_set_be16(a,0x46u,128u);
            if(wf_be16(a,0x52u))return WF_OK;
            wf_set_be16(a,0x52u,4u);
        }else wf_set_be16(a,0x52u,(uint16_t)(wf_be16(a,0x52u)+1u));
        wf_set_be16(a,0x54u,64u);return WF_OK;
    }
    if(pc!=0x1b090u){
        wf_status_t s=unlink_115d2(rt,a,ctx);if(s!=WF_OK)return s;
    }
    if(pc==0x1b0ccu){
        wf_set_be16(a,WF_A_PROFILE,1u);
        uint16_t timer=(uint16_t)(wf_be16(a,0x46u)-1u);wf_set_be16(a,0x46u,timer);
        if(!timer){wf_set_be16(a,WF_A_STATE,0u);rt->g.global_1697=1u;}
    }else if(low(a,WF_A_ANIM_INDEX)==0xfeu){
        if(pc==0x1b3deu&&low(a,WF_A_STATE4)==0x20u){
            wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,2u);
        }else if(pc==0x1b3deu&&(wf_u8(a,WF_A_FLAGS32)&1u)){
            wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,1u);
        }else wf_set_be16(a,WF_A_STATE,0u);
    }
    return WF_OK;
}
static wf_status_t push_clearance_10b62(wf_runtime_t *rt,wf_actor_raw_t *a,uint16_t clearance,wf_r17_dispatch_1c03e_ctx_t *ctx){
    uint16_t contact=wf_be16(a,WF_A_CONTACT);wf_set_be16(a,WF_A_FIELD3E,clearance);
    if(wf_r17_resolver_280dc(rt,a)!=WF_OK)return blocked(ctx,0x280dcu);
    if(wf_be16(a,WF_A_CONTACT)&3u)wf_set_be16(a,WF_A_VX,0u);
    wf_set_be16(a,WF_A_CONTACT,contact);wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+wf_be16(a,WF_A_CORR_X)));
    wf_set_be16(a,WF_A_FIELD3E,0u);return WF_OK;
}
/* $10FC6: rope animation / ring-edge reaction; return its carry result.
 * D0 is saved by $1C03E. Existing victim callers consume only RAM effects. */
static int edge_reaction_10fc6(wf_runtime_t *rt,wf_actor_raw_t *a){
    uint8_t contact=wf_u8(a,WF_A_CONTACT),bits=wf_u8(a,WF_A_CONTACT+1u);
    if(!(rt->g.mode_0161&2u)&&(contact==1u||contact==5u)){
        if(bits&2u){if(!(rt->g.global_1150&255u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}return 1;}
        if(bits&1u){if(!(rt->g.global_11b0&255u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}return 1;}
    }else if(contact==3u){
        wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,0x17u);
        wf_runtime_emit_audio(rt,0x28u);wf_runtime_emit_audio(rt,0x32u);
    }
    return 0;
}
/* $1B73E/$1B72E, victim subtype LOW13. The descriptor remains $1B72E
 * after the landing changes LOW subtype to5; edge carry is not consumed. */
static void state4_1b73e(wf_runtime_t*rt,wf_actor_raw_t*a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E preset6 at$25EE, with caller D1 restored before$10F9C. */
        wf_set_be16(a,WF_A_VX,(wf_u8(a,WF_A_FACING)&128u)?0x100u:0xff00u);
        wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x300u);
        wf_set_be16(a,WF_A_VZ_DEC,0x38u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
        if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
        else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,48u);}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);return;
    }
    (void)edge_reaction_10fc6(rt,a);
    if(wf_u8(a,WF_A_CONTACT+1u)&16u){
        landing_audio_1110e(rt,a);
        wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
        wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
    }
}

/* $1B684/$1B66C, victim subtype LOW0F. The original landing tail only
 * loads D0.W=-64/D1.W=0/D2.W=0; it does not call $10BD0 or move X. */
static void state4_1b684(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E preset5 at$25E8: VX=-256,VZ=1280,gravity56. */
        wf_set_be16(a,WF_A_VX,(wf_u8(a,WF_A_FACING)&128u)?0x100u:0xff00u);
        wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x500u);
        wf_set_be16(a,WF_A_VZ_DEC,0x38u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
        if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
        else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,0x30u);}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
        uint8_t flags=wf_u8(a,WF_A_STATE4);
        wf_set_u8(a,WF_A_STATE4,(uint8_t)(flags&~16u));
        if(flags&16u){
            wf_set_u8(a,WF_A_LATCHED,(uint8_t)(wf_u8(a,WF_A_LATCHED)|128u));
            wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);
        }
    }else{
        edge_reaction_10fc6(rt,a);
        if(low(a,WF_A_ANIM_INDEX)==3u){
            wf_set_be16(a,WF_A_FIELD42,0xffe0u);
            if(wf_u8(a,WF_A_CONTACT+1u)&16u){
                landing_audio_1110e(rt,a);
                wf_set_be16(a,WF_A_FIELD42,0u);wf_set_be16(a,WF_A_STATE,4u);
                wf_set_u8(a,WF_A_STATE4_LO,5u);
                wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            }
        }
    }
}
/* $11D8E/$11D82: slow a released runner, or rebound when $10FC6
 * reports rope contact. The dispatcher still ticks the original descriptor. */
static void main_three_11d8e(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x10u));
        return;
    }
    wf_set_be16(a,WF_A_PROFILE,1u);
    if(edge_reaction_10fc6(rt,a)){wf_set_be16(a,WF_A_STATE,6u);return;}
    uint16_t speed=(uint16_t)(wf_be16(a,WF_A_MAGNITUDE)-5u);
    wf_set_be16(a,WF_A_MAGNITUDE,speed);
    if(speed&0x8000u){
        wf_set_be16(a,WF_A_STATE,0u);wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_MAGNITUDE,0u);
    }
}
/* $1BA92/$1BA82, subtype $17: edge recoil, shake and landing. The
 * $2052/$1110E calls preserve every live D register and emit only sound. */
static void state4_1ba92(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~16u));
        /* $258E row13 at $2618; facing applies only to the VX WORD. */
        wf_set_be16(a,WF_A_VX,(wf_u8(a,WF_A_FACING)&128u)?0xffc0u:0x40u);
        wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x100u);wf_set_be16(a,WF_A_VZ_DEC,0x38u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        rt->g.global_1802=14u;
        if((rt->g.global_007e&255u)==1u)wf_runtime_emit_audio(rt,0x28u); /* $1BAC0 */
    }else if(wf_u8(a,WF_A_CONTACT+1u)&16u){
        landing_audio_1110e(rt,a); /* $1BAD0 */
        wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
        wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
    }
}
/* $1BB4C/$1BB40, subtype $19: horizontal recoil and landing. */
static void state4_1bb4c(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        uint16_t vx=(wf_u8(a,WF_A_FLAGS34)&8u)?0x500u:0x300u;
        wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~8u));
        if(wf_u8(a,WF_A_FACING)&128u)vx=(uint16_t)(0u-vx);
        /* $258E rows $13/$24 at $263C/$26A2 preserve all live registers. */
        wf_set_be16(a,WF_A_VX,vx);wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0u);wf_set_be16(a,WF_A_VZ_DEC,0x58u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
    }else{
        edge_reaction_10fc6(rt,a);
        if(wf_u8(a,WF_A_CONTACT+1u)&16u){
            landing_audio_1110e(rt,a);
            wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
            wf_set_be16(a,WF_A_MIRROR,low(a,WF_A_TIER)==2u?0x1fu:0x14u);
            wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
            uint16_t dx=(wf_u8(a,WF_A_FACING)&128u)?64u:0xffc0u;
            wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        }
    }
}
/* $1BBC6/$1BBB6, state-4 subtype $1A: small launch, alternating impact
 * effect, then return to subtype 9 on contact. $1C03E saves D0 around the
 * callback, while the byte move to D1 remains visible to later owners. */
static wf_status_t state4_1bbc6(wf_runtime_t *rt,wf_actor_raw_t *a,
                                wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row $17 at $2654: VX=0, VZ=$200, gravity=$40. */
        wf_set_be16(a,WF_A_VX,0u);wf_set_be16(a,WF_A_VY,0u);
        wf_set_be16(a,WF_A_VZ,0x200u);wf_set_be16(a,WF_A_VZ_DEC,0x40u);
        wf_set_u8(a,WF_A_MOTION_MODE,2u);
    }else if(wf_u8(a,WF_A_CONTACT+1u)&16u){
        landing_audio_1110e(rt,a); /* $1BBE2, before the subtype BYTE write. */
        wf_set_be16(a,0x9au,0u);wf_set_be16(a,WF_A_STATE,4u);
        wf_set_u8(a,WF_A_STATE4_LO,9u);return WF_OK;
    }
    uint8_t d1=wf_u8(a,WF_A_ANIM_INDEX+1u);
    if(!wf_be16(a,WF_A_ANIM_COUNTER))d1=(uint8_t)(d1+1u);
    wf_r17_m68k_move_b(&rt->m68k_regs,1u,d1);
    ptrdiff_t slot=a-rt->actors;
    if(!ctx->actions||slot<0||slot>=(ptrdiff_t)WF_ACTOR_SLOTS)
        return blocked(ctx,0x10d3au);
    wf_r17_actions_r11_trace_t *trace=&ctx->actions->trace[(unsigned)slot];
    uint32_t saved_d0=rt->m68k_regs.d[0];
    trace->blocked_dependency_rom=0u;
    wf_status_t st=wf_r17_action_effect_10d3a(rt,a,d1?16u:17u,
                                                ctx->actions,trace);
    rt->m68k_regs.d[0]=saved_d0;
    return st==WF_OK?WF_OK:blocked(ctx,trace->blocked_dependency_rom?
                                  trace->blocked_dependency_rom:0x10d3au);
}
/* $1BC36/$1BC22, subtype $1B: launched linked victim. $1C0C8 keeps
 * the original descriptor even when the contact tail changes subtype. */
static void state4_1bc36(wf_runtime_t*rt,wf_actor_raw_t*a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row20=$FEC0,$0500,$0038, then $10F9C D0.W=$30. */
        wf_set_be16(a,WF_A_VX,(wf_u8(a,WF_A_FACING)&128u)?0x140u:0xfec0u);
        wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x500u);wf_set_be16(a,WF_A_VZ_DEC,0x38u);
        wf_set_u8(a,WF_A_MOTION_MODE,2u);
        uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
        if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
        else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,48u);}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);return;
    }
    (void)edge_reaction_10fc6(rt,a);wf_set_be16(a,WF_A_FIELD42,0xfff0u);
    if(!(wf_u8(a,WF_A_CONTACT+1u)&16u))return;
    landing_audio_1110e(rt,a);
    wf_set_be16(a,WF_A_FIELD42,0u);wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,5u);
    wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
    wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+(wf_u8(a,WF_A_FACING)&128u?56u:0xffc8u)));
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
}

/* $1B85E/$1B84E, subtype15: short drop and the original byte-only
 * transition to grounded subtype5. Rope reaction still precedes landing. */
static void state4_1b85e(wf_runtime_t*rt,wf_actor_raw_t*a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row8=$0000,$0010,$0038. */
        wf_set_be16(a,WF_A_VX,0u);wf_set_be16(a,WF_A_VY,0u);
        wf_set_be16(a,WF_A_VZ,16u);wf_set_be16(a,WF_A_VZ_DEC,56u);
        wf_set_u8(a,WF_A_MOTION_MODE,2u);return;
    }
    (void)edge_reaction_10fc6(rt,a);
    if(!(wf_u8(a,WF_A_CONTACT+1u)&16u))return;
    landing_audio_1110e(rt,a); /* $1B880 */
    wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
    wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
    wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+(wf_u8(a,WF_A_FACING)&128u?56u:0xffc8u)));
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
}

/* $1BDE8/$1BDD8, subtype $22: descending throw victim and landing. */
static void state4_1bde8(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row $18 at $265A; $10F9C uses D0.W=$30. */
        wf_set_be16(a,WF_A_VX,(wf_u8(a,WF_A_FACING)&128u)?0x180u:0xfe80u);
        wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0xff00u);wf_set_be16(a,WF_A_VZ_DEC,0x58u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
        if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
        else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,48u);}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
    }else{
        if(!low(a,WF_A_ANIM_INDEX)&&!wf_be16(a,WF_A_ANIM_COUNTER))wf_set_be16(a,WF_A_Z,(uint16_t)(wf_be16(a,WF_A_Z)-16u));
        edge_reaction_10fc6(rt,a);
        if(low(a,WF_A_ANIM_INDEX)==1u){
            wf_set_be16(a,WF_A_FIELD42,0xffe0u);
            if(wf_u8(a,WF_A_CONTACT+1u)&16u){
                landing_audio_1110e(rt,a);
                wf_set_be16(a,WF_A_FIELD42,0u);wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,5u);
                wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
                uint16_t dx=(wf_u8(a,WF_A_FACING)&128u)?56u:0xffc8u;
                wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            }
        }
    }
}
/* $1BE7C/$1BE6C, subtype LOW23: descending fall. The ROM does not
 * branch on health; zero-health actors retain the same landing behavior. */
static void state4_1be7c(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row $1A at $2666; only VX is negated by facing. */
        wf_set_be16(a,WF_A_VX,(wf_u8(a,WF_A_FACING)&128u)?0x100u:0xff00u);
        wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x100u);wf_set_be16(a,WF_A_VZ_DEC,0x58u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
    }else{
        edge_reaction_10fc6(rt,a);
        if(wf_u8(a,WF_A_CONTACT+1u)&16u){
            landing_audio_1110e(rt,a);
            wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
            wf_set_be16(a,WF_A_MIRROR,0x1fu);wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
            uint16_t dx=(wf_u8(a,WF_A_FACING)&128u)?56u:0xffc8u;
            wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        }
    }
}
/* $1BF1C/$1BF08: subtype LOW25 keeps its high flags until landing. */
static void state4_1bf1c(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row $25 at $26A8, followed by optional WORD44 reversal. */
        uint16_t vx=(wf_u8(a,WF_A_FACING)&128u)?0x140u:0xfec0u;
        if(wf_be16(a,WF_A_TIMER44))vx=(uint16_t)(0u-vx);
        wf_set_be16(a,WF_A_VX,vx);wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x200u);wf_set_be16(a,WF_A_VZ_DEC,0x40u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));
        uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
        if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
        else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,48u);}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
    }else{
        edge_reaction_10fc6(rt,a);
        if(low(a,WF_A_ANIM_INDEX)==2u){
            wf_set_be16(a,WF_A_FIELD42,0xfff0u);
            if(wf_u8(a,WF_A_CONTACT+1u)&16u){
                landing_audio_1110e(rt,a);
                wf_set_be16(a,WF_A_FIELD42,0u);wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,5u);
                wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
                uint16_t dx=(wf_u8(a,WF_A_FACING)&128u)?56u:0xffc8u;
                wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));
                wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
            }
        }
    }
}
/* $1BFA6/$1BF96, subtype LOW26: rearward fall and its first-pose
 * offset. Only the low animation-index BYTE selects $10BD0. */
static void state4_1bfa6(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E[3], then the callback's distinct VY WORD overwrite. */
        wf_set_be16(a,WF_A_VX,0u);wf_set_be16(a,WF_A_VY,0x80u);wf_set_be16(a,WF_A_VZ,0x300u);wf_set_be16(a,WF_A_VZ_DEC,0x38u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
    }else{
        if(!low(a,WF_A_ANIM_INDEX)&&!wf_be16(a,WF_A_ANIM_COUNTER)){
            wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));
            /* $10BD0 applies the new facing to -56; Y remains unchanged. */
            uint16_t dx=(wf_u8(a,WF_A_FACING)&128u)?56u:0xffc8u;
            wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));wf_set_be16(a,WF_A_Z,(uint16_t)(wf_be16(a,WF_A_Z)-16u));
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0xfff0u);
        }
        if(wf_u8(a,WF_A_CONTACT+1u)&16u){
            landing_audio_1110e(rt,a);
            wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);
        }
    }
}
/* $1B8CA: victim of raw $24. Edge classification uses the linked attacker's
 * facing; the velocity preset uses the victim's own facing. */
static wf_status_t state4_1b8ca(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        int inside=1;
        wf_actor_raw_t *t=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),NULL);
        if(!t)return blocked(ctx,0x1b8f0u);
        if((rt->g.global_007e&255u)!=1u&&!(rt->g.mode_0161&2u)){
            wf_set_be16(a,0x44u,0u);
            int facing=(wf_u8(t,WF_A_FACING)&128u)!=0;
            int32_t dividend=(int32_t)wf_be16(a,WF_A_Y)*256+(facing?-0x9c000:0x48000);
            int32_t q=dividend/736,rem=dividend%736;
            uint16_t edge=facing?(uint16_t)-q:(uint16_t)q;
            wf_r17_m68k_move_l(&rt->m68k_regs,3u,((uint32_t)(uint16_t)rem<<16)|edge);
            inside=facing?edge>=wf_be16(a,WF_A_X):edge<wf_be16(a,WF_A_X);
        }
        uint16_t vx=inside?0xff00u:0xfb80u;
        if(wf_u8(a,WF_A_FACING)&128u)vx=(uint16_t)(0u-vx);
        wf_set_be16(a,WF_A_VX,vx);wf_set_be16(a,WF_A_VY,0u);
        wf_set_be16(a,WF_A_VZ,inside?0u:0x400u);wf_set_be16(a,WF_A_VZ_DEC,inside?0x28u:0x58u);
        wf_set_u8(a,WF_A_MOTION_MODE,2u);
        if(inside){
            uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
            if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffd0u);
            else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,48u);}
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
        }else{
            wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)|4u));
            if(rt->g.mode_0161&1u){
                wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|16u));
                wf_set_be16(t,0xc4u,(uint16_t)(wf_be16(t,0xc4u)+1u));
            }
        }
    }else{
        uint8_t contact=wf_u8(a,WF_A_CONTACT),bits=wf_u8(a,WF_A_CONTACT+1u);
        if(contact==6u){
            if(!(wf_u8(a,0x45u)&1u)){
                wf_runtime_emit_audio(rt,0x28u);wf_runtime_emit_audio(rt,0x32u);
            }
            wf_set_u8(a,0x45u,(uint8_t)(wf_u8(a,0x45u)|1u));
            if(wf_be16(a,WF_A_Z)<0x180u){
                if(bits&2u){rt->g.global_1150=1u;rt->g.global_1180=1u;}
                else{rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
            }
        }
        if(low(a,WF_A_ANIM_INDEX)!=1u)return WF_OK;
        wf_set_be16(a,0x42u,0xffe0u);edge_reaction_10fc6(rt,a);
        if(!(bits&16u)&&contact!=3u)return WF_OK;
        if(!(bits&16u)&&contact==3u&&(rt->g.mode_0161&2u)){
            wf_runtime_emit_audio(rt,0x28u); /* $1BA72 sound-only branch. */
            wf_set_be16(a,0x42u,0u);return WF_OK;
        }
        int ringout=!(rt->g.mode_0161&2u)&&((!(bits&16u)&&contact==3u)||(wf_u8(a,WF_A_FLAGS33)&4u));
        uint16_t dx=ringout?16u:0xffc8u;
        if(ringout){
            wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x68u);wf_set_be16(a,0x44u,0u);
            wf_set_be16(a,WF_A_MIRROR,0x16u);
        }else{
            wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);wf_set_be16(a,WF_A_MIRROR,0x1fu);
        }
        wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
        if(wf_u8(a,WF_A_FACING)&128u)dx=(uint16_t)(0u-dx);
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        if(ringout)wf_runtime_emit_audio(rt,0x28u);else landing_audio_1110e(rt,a);
        wf_set_be16(a,0x42u,0u);
    }
    return WF_OK;
}
/* $1B134, victim subtypes 2/3/4. */
static wf_status_t state4_1b134(wf_runtime_t *rt,wf_actor_raw_t *a,const wf_anim_desc_t *d){
    int alternate=0;
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        rt->g.global_1697=1u;
        static const uint16_t speed[3]={0xc0u,0x180u,0x200u};
        unsigned sub=low(a,WF_A_STATE4);if(sub<2u||sub>4u)return WF_BAD_STATE;
        uint16_t vx=speed[sub-2u];if(wf_u8(a,WF_A_FACING)&128u)vx=(uint16_t)(0u-vx);
        wf_set_be16(a,WF_A_VX,vx);wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x300u);wf_set_be16(a,WF_A_VZ_DEC,0x38u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        uint16_t y=wf_be16(a,WF_A_Y),d1=(uint16_t)(y+32u);
        if(d1>=0x198u)wf_set_be16(a,WF_A_VY,0xffc0u);
        else{d1=(uint16_t)(y-32u);if(d1<0x118u)wf_set_be16(a,WF_A_VY,64u);}
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);alternate=(wf_u8(a,WF_A_STATE4)&128u)!=0;
    }else{
        edge_reaction_10fc6(rt,a);
        if(wf_be16(a,WF_A_ANIM_INDEX)==0u&&wf_be16(a,WF_A_ANIM_COUNTER)==0u){
            uint16_t dx=(wf_u8(a,WF_A_FACING)&128u)?16u:0xfff0u;
            wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+dx));wf_set_be16(a,WF_A_Z,(uint16_t)(wf_be16(a,WF_A_Z)+16u));
            wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,16u);
        }
        wf_set_be16(a,WF_A_FIELD3E,64u);
        if(wf_u8(a,WF_A_CONTACT+1u)&16u){wf_set_be16(a,WF_A_FIELD3E,0u);wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,5u);landing_audio_1110e(rt,a);}
        else alternate=(wf_u8(a,WF_A_STATE4)&128u)!=0;
    }
    int loads=!(wf_u8(a,WF_A_LATCHED)&128u)||wf_be16(a,WF_A_ANIM_COUNTER)==0u;
    wf_status_t status=tick_descriptor(a,d);
    /* $1B124 differs only in the second pose WORD ($16 instead of $14). */
    if(status==WF_OK&&alternate&&loads&&wf_be16(a,WF_A_ANIM_INDEX)==1u)
        wf_set_be16(a,WF_A_MIRROR,(uint16_t)(0x16u^wf_be16(a,WF_A_FACING)));
    return status;
}
static void recovery_input_10d04(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_ROLE)&0xc0u)){
        rt->g.global_167a|=128u;
        if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
            uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);wf_set_be16(a,0xaau,timer?timer:0x4000u);
        }
    }
}
/* $1BD6C, subtype LOW21. Airborne paths select $1B1B8 for the tick;
 * the landing keeps the incoming kind-zero $1BD66 descriptor. */
static wf_status_t state4_1bd6c(wf_runtime_t*rt,wf_actor_raw_t*a,
                               wf_r17_dispatch_1c03e_ctx_t*ctx,unsigned*ground_pose){
    *ground_pose=1u;
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row3=$0000,$0300,$0038, followed by CLR.W AA and
         * $10C60 selector1. All helper registers are restored. */
        wf_set_be16(a,WF_A_VX,0u);wf_set_be16(a,WF_A_VY,0u);
        wf_set_be16(a,WF_A_VZ,0x300u);wf_set_be16(a,WF_A_VZ_DEC,0x38u);
        wf_set_u8(a,WF_A_MOTION_MODE,2u);wf_set_be16(a,0xaau,0u);
        uint16_t unit=(uint16_t)(wf_be16(a,WF_A_STAMINA_MAX)>>2),sum=unit;unsigned level=3u;
        while(sum<wf_be16(a,WF_A_STAMINA)&&level){sum=(uint16_t)(sum+unit);--level;}
        static const uint8_t delay[4]={1u,3u,7u,21u};wf_set_be16(a,0xaau,delay[level]);
        return WF_OK;
    }
    recovery_input_10d04(rt,a);
    wf_status_t status=push_clearance_10b62(rt,a,80u,ctx);if(status!=WF_OK)return status;
    (void)edge_reaction_10fc6(rt,a);
    if(!(wf_u8(a,WF_A_CONTACT+1u)&16u))return WF_OK;
    landing_audio_1110e(rt,a);
    wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,low(a,WF_A_TIER)==2u?9u:8u);
    *ground_pose=0u;return WF_OK;
}
/* $1B288: descriptors $1B268/$1B278 (subtypes 6/7). Ground contact
 * releases the indefinite first pose; the original descriptor still ticks. */
static wf_status_t state4_1b288(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        /* $258E row $15: $0000,$0300,$0068, before $10B62. */
        wf_set_be16(a,WF_A_VX,0u);wf_set_be16(a,WF_A_VY,0u);wf_set_be16(a,WF_A_VZ,0x300u);wf_set_be16(a,WF_A_VZ_DEC,0x68u);wf_set_u8(a,WF_A_MOTION_MODE,2u);
        wf_status_t status=push_clearance_10b62(rt,a,80u,ctx);if(status!=WF_OK)return status;
        /* $10C60 selector1 leaves a positive existing recovery WORD alone. */
        uint16_t recovery=wf_be16(a,0xaau);
        if(!recovery||(recovery&0x8000u)){
            uint16_t unit=(uint16_t)(wf_be16(a,WF_A_STAMINA_MAX)>>2),sum=unit;unsigned level=3u;
            while(sum<wf_be16(a,WF_A_STAMINA)&&level){sum=(uint16_t)(sum+unit);--level;}
            static const uint8_t delay[4]={1u,3u,7u,21u};wf_set_be16(a,0xaau,delay[level]);
        }
        return WF_OK;
    }
    recovery_input_10d04(rt,a);
    if(!wf_be16(a,WF_A_ANIM_INDEX)&&(wf_u8(a,WF_A_CONTACT+1u)&16u)){
        landing_audio_1110e(rt,a);
        wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);return WF_OK;
    }
    wf_status_t status=unlink_115d2(rt,a,ctx);if(status!=WF_OK)return status;
    if(low(a,WF_A_ANIM_INDEX)==0xfeu){wf_set_be16(a,WF_A_STATE,4u);wf_set_u8(a,WF_A_STATE4_LO,low(a,WF_A_STATE4)==6u?8u:9u);}
    return WF_OK;
}
/* $1B318: grounded recovery, followed by the standing-up state 7. */
static wf_status_t state4_1b318(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_dispatch_1c03e_ctx_t *ctx){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x58u));
        wf_set_u8(a,WF_A_SELECTOR,(uint8_t)(wf_u8(a,WF_A_SELECTOR)&~128u));
        wf_set_be16(a,0x52u,0u);wf_set_be16(a,0x54u,0u);wf_set_be16(a,0xd0u,0u);
        if(!wf_be16(a,0x9au)){
            uint16_t delay=48u;
            if(!(wf_u8(a,WF_A_ROLE)&128u)||!(wf_u8(a,WF_A_FLAGS33)&0x24u)){
                if(low(a,WF_A_TIER)==2u)delay=224u;else if(low(a,WF_A_TIER)==1u)delay=128u;
            }
            wf_set_be16(a,0x9au,delay);
        }
        if(low(a,WF_A_STATE4)!=9u)return push_clearance_10b62(rt,a,80u,ctx);
        wf_set_be16(a,WF_A_FIELD3E,48u);wf_set_be16(a,WF_A_FIELD40,0xffe8u);
        if(wf_r17_resolver_280dc(rt,a)!=WF_OK)return blocked(ctx,0x280dcu);
        wf_set_be16(a,WF_A_X,(uint16_t)(wf_be16(a,WF_A_X)+wf_be16(a,WF_A_CORR_X)));
        wf_set_be16(a,WF_A_Y,(uint16_t)(wf_be16(a,WF_A_Y)+wf_be16(a,WF_A_CORR_Y)));
        wf_set_be16(a,WF_A_FIELD3E,0u);wf_set_be16(a,WF_A_FIELD40,0u);return WF_OK;
    }
    if(!(wf_u8(a,WF_A_FLAGS32)&16u)){
        wf_status_t status=unlink_115d2(rt,a,ctx);if(status!=WF_OK)return status;
        if(!(wf_u8(a,0xfeu)&128u)){
            wf_actor_raw_t *target=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TARGET),NULL);if(!target)return blocked(ctx,0x1b3a6u);
            if(wf_u8(target,WF_A_FLAGS34)&16u)return WF_OK;
            recovery_input_10d04(rt,a);
            uint16_t delay=wf_be16(a,0x9au);
            if(delay){--delay;wf_set_be16(a,0x9au,delay);if(delay)return WF_OK;}
        }
    }
    wf_set_be16(a,WF_A_STATE,7u);wf_set_u8(a,WF_A_STATE4,(uint8_t)(wf_u8(a,WF_A_STATE4)&~128u));return WF_OK;
}
static void main_seven_11e42(wf_runtime_t *rt,wf_actor_raw_t *a){
    if(!(wf_u8(a,WF_A_LATCHED)&128u)){
        wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x58u));
        wf_set_be16(a,0xaau,0u);wf_set_be16(a,0x9au,0u);
    }else if(low(a,WF_A_ANIM_INDEX)==0xfeu){
        rt->g.global_1697=1u;
        if(!wf_be16(a,0xfeu)&&low(a,WF_A_PRIOR)!=5u){
            unsigned old=wf_u8(a,WF_A_STATE4);wf_set_u8(a,WF_A_STATE4,(uint8_t)(old&~32u));
            if(old&32u){wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,1u);return;}
        }
        wf_set_be16(a,WF_A_STATE,0u);
    }
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
            landing_audio_1110e(rt,a);
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
            wf_runtime_emit_audio(rt,0x28u);wf_runtime_emit_audio(rt,0x32u);
            wf_set_be16(a,WF_A_VX,0u);rt->g.global_1802=14u;
        }
        if(contact==5u)rope_contact_11d56(rt,a);
        /* $10FC6, including its rope-animation writes. */
        if(!(rt->g.mode_0161&2u)&&(contact==1u||contact==5u)){
            if((bits&2u)&&!(rt->g.global_1150&255u)){rt->g.global_1150=1u;rt->g.global_1180=1u;}
            else if(!(bits&2u)&&(bits&1u)&&!(rt->g.global_11b0&255u)){rt->g.global_11b0=1u;rt->g.global_11e0=1u;}
        }else if(contact==3u){
            wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,0x17u);
            wf_runtime_emit_audio(rt,0x28u);wf_runtime_emit_audio(rt,0x32u);
        }
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
            if(land)landing_audio_1110e(rt,a);
            else if(ringside||sound_only)wf_runtime_emit_audio(rt,0x28u);
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
    /* $1C07A jumps past $1C082 when latched low state is FF. */
    if(domain==0xffu){s=held_pose_125cc(rt,a,ctx);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;}
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
        if(d->callback_rom==0x1bed8u){
            state4_1bed8(a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1c004u){
            state4_1c004(a);exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b73eu){
            state4_1b73e(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b684u){
            state4_1b684(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b8cau){
            s=state4_1b8ca(rt,a,ctx);if(s!=WF_OK)return s;
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b440u){
            wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));
            wf_set_be16(a,WF_A_STATE,4u);
            wf_set_u8(a,WF_A_STATE4_LO,(uint8_t)(low(a,WF_A_STATE4)-9u));
            wf_set_be16(a,WF_A_MIRROR,0xffffu);
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b090u||d->callback_rom==0x1b0ccu||d->callback_rom==0x1b3deu){
            s=state4_recover(rt,a,d->callback_rom,ctx);if(s!=WF_OK)return s;
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b134u){s=state4_1b134(rt,a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;}
        if(d->callback_rom==0x1b288u){
            s=state4_1b288(rt,a,ctx);if(s!=WF_OK)return s;
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b318u){
            s=state4_1b318(rt,a,ctx);if(s!=WF_OK)return s;
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bcaau){
            s=state4_1bcaa(rt,a,ctx);if(s!=WF_OK)return s;
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b1d0u){
            s=state4_1b1d0(rt,a,ctx);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b478u){
            s=state4_1b478(rt,a,ctx);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bb08u){
            state4_1bb08(a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b7a2u){
            unsigned alternate;s=state4_1b7a2(rt,a,ctx,&alternate);if(s!=WF_OK)return s;state4_1b7a2_tick(a,alternate);exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b706u){
            state4_1b706(a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1ba92u){
            state4_1ba92(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bb4cu){
            state4_1bb4c(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bbc6u){
            s=state4_1bbc6(rt,a,ctx);if(s!=WF_OK)return s;
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bc36u){
            state4_1bc36(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1b85eu){
            state4_1b85e(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bd6cu){
            unsigned ground_pose;s=state4_1bd6c(rt,a,ctx,&ground_pose);if(s!=WF_OK)return s;
            if(ground_pose)d=&wf_anim_descs[wf_anim_state4_dispatch[5u]]; /* $1B1B8 */
            s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bd0eu){
            unsigned alternate;s=state4_1bd0e(rt,a,ctx,&alternate);if(s!=WF_OK)return s;
            s=state4_1bd0e_tick(a,d,alternate);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bde8u){
            state4_1bde8(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bf1cu){
            state4_1bf1c(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1be7cu){
            state4_1be7c(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        if(d->callback_rom==0x1bfa6u){
            state4_1bfa6(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
            exact_tail_1c0b4(a);return WF_OK;
        }
        /* The two demonstrated callback-RTS entries are subtypes $1F/$28. */
        if(d->callback_rom!=0x00012612u)return blocked(ctx,d->callback_rom);
        /* $1C0C8 sees a kind-0 descriptor here and performs no animation mutation. */
        exact_tail_1c0b4(a);return WF_OK;
    }
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
    if(id==3u&&d->callback_rom==0x11d8eu){
        main_three_11d8e(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==6u&&d->callback_rom==0x00011de0u){
        main_six_11de0(a);exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==7u&&d->callback_rom==0x11e42u){main_seven_11e42(rt,a);s=tick_descriptor(a,d);if(s!=WF_OK)return s;exact_tail_1c0b4(a);return WF_OK;}
    if(id==8u&&d->callback_rom==0x00011f3au){
        s=main_eight_12078(rt,a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==9u&&d->callback_rom==0x000121e6u){
        s=main_nine_121e6(a,ctx);if(s!=WF_OK)return s;
        exact_tail_1c0b4(a);return WF_OK;
    }
    if(id==10u&&d->callback_rom==0x000122e6u){
        unsigned variant=0u;
        s=main_ten_122e6(rt,a,ctx,&variant);if(s!=WF_OK)return s;
        main_ten_tick(a,variant);exact_tail_1c0b4(a);return WF_OK;
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
