#include "wf_r18_result_screen.h"
#include "wf_r18_frontend_palette.h"

wf_status_t wf_r18_result_royal_game_over(wf_runtime_t *rt,wf_r18_video_t *video,
 wf_r18_result_level_fn level,wf_r18_result_wait_fn wait_frames,
 wf_r18_result_commit_fn commit,void *user){
 wf_status_t st;
 if(!rt||!video||!rt->m68k_regs.valid||!level||!wait_frames||!commit)
  return WF_BAD_STATE;
 if(!video->read_rom)return WF_BLOCKED;
 if(rt->g.global_15f4>=7u)return WF_BLOCKED;
 rt->g.global_15fc=1u;
 st=level(rt,1u,user);if(st!=WF_OK)return st;
 st=wf_r18_frontend_palette_2a06(rt,video,rt->g.global_15f4,1u);if(st!=WF_OK)return st;
 st=commit(user);if(st!=WF_OK)return st;
 st=level(rt,0u,user);if(st!=WF_OK)return st;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x50u);
 st=wf_r18_text_2503c(rt,0x50u,video);if(st!=WF_OK)return st;
 st=commit(user);if(st!=WF_OK)return st;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x60u);
 return wait_frames(rt,0x61u,user);
}

wf_status_t wf_r18_result_in_ring(wf_runtime_t *rt,wf_r18_video_t *video,
                                 wf_r18_result_wait_fn wait_frames,
                                 wf_r18_result_commit_fn commit,void *user){
    wf_status_t status;
    unsigned slot;
    if(!rt||!video||!rt->m68k_regs.valid||!wait_frames||!commit)return WF_BAD_STATE;
    if(!video->read_rom)return WF_BLOCKED;

    /* $11CE: $21E6 saves D0, then DBRA waits D0.W+1 VBlank periods. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x20u);
    status=wait_frames(rt,0x21u,user);
    if(status!=WF_OK)return status;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x8050u);
    status=wf_r18_text_2503c(rt,0x8050u,video);
    if(status!=WF_OK)return status;
    status=commit(user);
    if(status!=WF_OK)return status;
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x20u);
    status=wait_frames(rt,0x21u,user);
    if(status!=WF_OK)return status;

    /* $11EC..$121A: BTST.B reads the high result byte at actor+$A0;
     * DBRA affects only D1.W, including $FFFF on the exhausted path. */
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x51u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,3u);
    for(slot=0;slot<4u;slot++){
        if(wf_u8(&rt->actors[slot],WF_A_RESULT)&0x40u)break;
        wf_r17_m68k_move_w(&rt->m68k_regs,1u,
                         (uint16_t)(rt->m68k_regs.d[1]-1u));
    }
    if(slot==4u)wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x50u);
    /* $1202 compares BYTE $0163, not WORD $0162. With no winner, $50
     * is always drawn, including progression nine. */
    if(slot==4u||(uint8_t)rt->g.progression_0162!=9u){
        status=wf_r18_text_2503c(rt,(uint16_t)rt->m68k_regs.d[0],video);
        if(status!=WF_OK)return status;
        status=commit(user);
        if(status!=WF_OK)return status;
    }
    wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x40u);
    return wait_frames(rt,0x41u,user);
}
