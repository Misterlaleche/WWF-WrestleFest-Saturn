#include "wf_r18_frontend_palette.h"
#include <string.h>

static uint32_t be32(const uint8_t p[4]){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

static wf_status_t block(wf_r18_video_t *v,uint32_t pointer,void *dst,size_t n){
    uint8_t p[4];
    wf_status_t st=v->read_rom(v->read_user,pointer,p,4);
    if(st!=WF_OK)return st;
    return v->read_rom(v->read_user,be32(p),dst,n);
}

wf_status_t wf_r18_frontend_palette_2a06(wf_runtime_t *r,wf_r18_video_t *v,
                                      uint16_t world,uint16_t text)
{
    uint8_t fg[512],bg[512],chars[512];
    wf_status_t st;
    if(!r||!v||!v->read_rom||!r->m68k_regs.valid||world>=7u||text>=4u)
        return WF_BAD_STATE;
    st=block(v,0x2a98u+world*4u,fg,sizeof fg);if(st!=WF_OK)return st;
    st=block(v,0x2ab4u+world*4u,bg,sizeof bg);if(st!=WF_OK)return st;
    for(unsigned bank=0;bank<16u;bank++){
        st=block(v,0x2b82u+text*64u+bank*4u,chars+bank*32u,32);
        if(st!=WF_OK)return st;
    }
    for(unsigned bank=0;bank<16u;bank++){
        memcpy(v->fg_palette+bank*128u,fg+bank*32u,32);
        memcpy(v->bg_palette+bank*128u,bg+bank*32u,32);
        memcpy(v->palettes+bank*128u,chars+bank*32u,32);
    }
    v->palette_dirty[WF_R18_PALETTE_FG]=0xffffu;
    v->palette_dirty[WF_R18_PALETTE_BG]=0xffffu;
    v->palette_dirty[WF_R18_PALETTE_TEXT]=0xffffu;
    /* MOVEM saves D0 but not D1/D2. The final16-bank text loop leaves
     * D1.W=15*4 and DBRA D2.W=FFFF, retaining each high word. */
    wf_r17_m68k_move_w(&r->m68k_regs,1,60);
    wf_r17_m68k_move_w(&r->m68k_regs,2,0xffffu);
    return WF_OK;
}
