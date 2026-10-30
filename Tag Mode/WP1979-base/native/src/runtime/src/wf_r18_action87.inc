/* $12614[$87] -> $1AA8A/$1AA9A. The reversal translates on entry,
 * counts72 calls and releases both actors in literal A0/A2 write order. */
static const desc_t D_1AA8A={0x1aa8au,2u,2u,{18u,10u},{0x144u,0x145u}};
static const desc_t D_1AA76={0x1aa76u,2u,3u,{16u,10u,10u},
    {0x90u,0x91u,0x138u}};
static wf_status_t raw87(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    uint8_t selector=low(a,O_SELECTOR);
    *dp=selector==0x86u?&D_1AA76:&D_1AA8A;
    /* Whole1C03E leaves the chosen selector*4 in D0 until the callback changes it. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)selector*4u);
    if(!bit(a,O_LATCH,7u)){
        wf_r17_m68k_move_w(&rt->m68k_regs,0u,16u);
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
        wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
        helper_10bd0(a,16,0,0);
        if(bit(a,O_FACING,7u))wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xfff0u);
        wf_set_be16(a,0x44u,72u);
    }
    uint16_t counter=(uint16_t)(wf_be16(a,0x44u)-1u);
    wf_set_be16(a,0x44u,counter);if(counter)return WF_OK;
    rt->g.global_15d2=15u;rt->g.global_15d3=26u;
    setbit(a,O_FLAGS18,7u);
    uint32_t link=wf_be32(a,O_LINK);
    wf_actor_raw_t*t=actor_from_ptr(rt,link);
    if(!t)return block(tr,0x1aacau);
    setbit(t,O_FLAGS18,7u);
    wf_set_be16(a,O_STATE,4u);wf_set_be16(a,0x64u,14u);
    wf_set_be16(a,0x68u,8u);wf_set_be16(a,O_VISUAL,0x2bu);
    wf_set_u8(a,O_VISUAL,wf_u8(a,O_FACING));
    wf_set_be16(t,O_STATE,5u);wf_set_be16(t,O_SELECTOR,0xc006u);
    wf_set_be16(t,O_VISUAL,0x60u);wf_set_u8(t,O_VISUAL,wf_u8(t,O_FACING));
    wf_set_be32(a,0x92u,link);
    wf_set_be32(t,0x92u,wf_actor_rom_base((unsigned)(a-rt->actors)));
    wf_set_be16(a,O_Z,(uint16_t)(wf_be16(a,O_Z)+72u));
    pin_clear(a,0x33u,0x40u);pin_clear(t,0x33u,0x40u);
    wf_set_be16(a,0x18u,0u);wf_set_be16(t,0x18u,0u);
    wf_set_be16(a,0x1au,0u);wf_set_be16(t,0x1au,0u);return WF_OK;
}
