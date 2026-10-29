/* $12614[$20] -> $16A06/$16A16. The original BTST at entry is followed
 * by NOP, so both initial and latched entries execute the rope/countdown. */
static const desc_t D_16A06={0x16a06u,1u,2u,{6u,4u},{0xadu,0xaeu}};
static wf_status_t raw20(wf_runtime_t*rt,wf_actor_raw_t*a,const desc_t**dp){
    *dp=&D_16A06;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x80u);
    if(raw75_edge_10fc6(rt,a)){wf_set_be16(a,O_STATE,6u);return WF_OK;}
    wf_set_be16(a,O_PROFILE,11u);
    uint16_t counter=(uint16_t)(wf_be16(a,0x2au)-2u);
    wf_set_be16(a,0x2au,counter);
    /* BPL consumes the subtraction's N flag, not signed-overflow logic. */
    if(!(counter&0x8000u))return WF_OK;
    wf_set_be16(a,O_STATE,0u);
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,bit(a,O_FACING,7u)?0xfff8u:8u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
    wf_r17_m68k_move_w(&rt->m68k_regs,2u,0u);
    helper_10bd0(a,8,0,0);setbit(a,O_LINK,7u);return WF_OK;
}
