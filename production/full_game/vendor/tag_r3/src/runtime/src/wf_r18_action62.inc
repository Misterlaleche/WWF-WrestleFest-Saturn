/* $12614[$62] -> $19530/$19536. Raw62 uses $10C60 argument7;
 * its latched $11432/$10D04 route has no raw5E zero-health exception.
 * Included after the existing private action helpers, before dispatch. */
static wf_status_t raw62_prepare_21732(wf_runtime_t*rt,wf_actor_raw_t*a,
                                      wf_r17_actions_r11_trace_t*tr){
    if(!rt->g.credited_route)return WF_OK;
    if(rt->g.mode_0161&1u){
        uint32_t candidate,dependency=0u;int found;
        wf_status_t s=wf_r17_candidate_2187e(rt,(unsigned)(a-rt->actors),&candidate,&found,&dependency);
        if(s!=WF_OK)return block(tr,dependency);
        if(found){
            wf_actor_raw_t*c=actor_from_ptr(rt,candidate);
            if(!c)return block(tr,0x21828u);
            wf_set_be32(c,0x7eu,wf_be32(a,O_LINK));wf_set_be32(a,0x7eu,candidate);
        }
        return WF_OK;
    }
    if(!bit(a,0x33u,0u))return WF_OK;
    wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));
    if(!p)return block(tr,0x21756u);
    setbit(p,0x34u,0u);pin_clear(p,0x34u,2u);
    if(!bit(p,0x32u,0u)){
        rt->g.global_1682=0xfau;
        if(!bit(p,0x33u,1u)&&!bit(p,0x74u,7u)){
            setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));
            wf_set_be32(a,0x7eu,wf_be32(a,0x86u));
        }
        return WF_OK;
    }
    /* $2179C precedes the WORD tier table, even when the read is guarded. */
    wf_set_be16(p,0xe6u,1u);
    uint32_t saved_d0=rt->m68k_regs.d[0];unsigned tier=wf_be16(p,0x70u);
    wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(tier<<1));
    if(tier>=3u)return block(tr,0x217c8u);
    static const uint16_t timer[3]={62u,93u,125u};wf_set_be16(p,0xe6u,timer[tier]);
    if(bit(p,0x33u,1u))wf_set_be32(p,0x7au,wf_be32(a,O_LINK));
    else{setbit(p,0x34u,2u);setbit(p,0xb5u,7u);wf_set_be32(p,0x7eu,wf_be32(a,O_LINK));}
    if(bit(p,0x33u,1u))setbit(p,0x34u,2u);
    rt->m68k_regs.d[0]=saved_d0; /* $21796 MOVEM restores the caller D0. */
    return WF_OK;
}

static wf_status_t raw62(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_19530;wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x188u);
    int latched=bit(a,O_LATCH,7u);setbit(a,O_LATCH,7u);
    if(!latched){
        wf_set_be16(a,0x52u,0u);wf_set_u8(a,O_MOTION,0u);
        wf_set_be16(a,O_VISUAL,0xffffu);wf_set_be16(a,O_TIMER,0xff00u);
        wf_status_t s=raw62_prepare_21732(rt,a,tr);if(s!=WF_OK)return s;
        wf_set_be16(a,0xaau,0u);wf_r17_m68k_move_w(&rt->m68k_regs,0u,7u);
        unsigned tier=wf_be16(a,0x70u);
        if(tier>=3u){
            /* $10CA8 after SUBQ.W/MULU.W/ADD.W inside the saved-D helper. */
            wf_r17_m68k_move_l(&rt->m68k_regs,0u,(uint16_t)(18u+tier));
            return block(tr,0x10ca8u);
        }
        static const uint16_t recovery[3]={3u,11u,23u};
        wf_set_be16(a,0xaau,recovery[tier]);return WF_OK;
    }
    if(!(rt->g.mode_0161&1u)&&!bit(a,0x56u,7u)){
        wf_actor_raw_t*p=actor_from_ptr(rt,wf_be32(a,0x86u));
        if(!p)return block(tr,0x11448u);
        uint8_t flags=(uint8_t)(wf_u8(p,0x34u)&0xc0u);
        wf_r17_m68k_move_b(&rt->m68k_regs,0u,flags);
        if(flags){
            pin_clear(p,0x34u,0xc0u);p=actor_from_ptr(rt,wf_be32(p,0x7au));
            if(!p)return block(tr,0x11462u);
            pin_clear(p,0x34u,0xc0u);
        }
    }
    if(!(wf_u8(a,0x56u)&0xc0u)){
        rt->g.global_167a|=0x80u;
        if(wf_be16(a,0xaau)!=0x4000u&&wf_u8(a,0xa3u)){
            uint16_t timer=(uint16_t)(wf_be16(a,0xaau)-1u);
            wf_set_be16(a,0xaau,timer?timer:0x4000u);
        }
    }
    return WF_OK;
}
