/* $12614[$54] -> $18F62/$18F82. Included after the shared action helpers.
 * A2 is consumed only by the latched WORD-phase-zero branch. */
static const desc_t D_18F62={0x18f62u,1u,2u,{0xff00u,32u},{0x1c0u,0x1c0u}};
static const desc_t D_18F72={0x18f72u,1u,2u,{0xff00u,32u},{0x220u,0x220u}};
static wf_status_t raw54(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp,
                         wf_r17_actions_r11_trace_t*tr){
    *dp=&D_18F62;
    /* $1C08A/$1C08E leaves the table offset live until the callback writes D0. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x150u);
    if(!bit(a,O_LATCH,7u)){
        wf_set_u8(a,O_MOTION,0u);
    }else if(low(a,O_FRAME)==0xfeu){
        wf_set_be16(a,O_STATE,7u);wf_set_be16(a,0x64u,0u);
        setbit(a,O_LINK,7u);
        return WF_OK; /* $18FA8 skips alternate descriptor selection. */
    }else if(!wf_be16(a,O_FRAME)){
        uint32_t link=wf_be32(a,O_LINK);
        wf_actor_raw_t*t=actor_from_ptr(rt,link);
        /* Literal zero supplies the dispatcher's ROM dummy $7F000
         * (BYTE $7F021=$50); tagged physical zero reads ROM $21=$FF.
         * Neither can satisfy the exact BYTE state-four comparison. */
        if(!t&&(link&0x00ffffffu))return block(tr,0x18fb0u);
        if(t&&low(t,O_STATE)==4u){
            wf_set_be16(a,O_TIMER,0u);
            interrupt_position(rt,a,t,64,-1,20); /* Whole ROM $10B9A. */
            wf_set_be16(t,0x68u,5u);
            wf_r17_m68k_move_w(&rt->m68k_regs,0u,50u);
            /* $2052 emits sound $32 and preserves D0; device output is
             * handled outside the typed action state, as for other owners. */
        }
    }
    if(low(a,0x64u)!=9u)*dp=&D_18F72;
    return WF_OK;
}
