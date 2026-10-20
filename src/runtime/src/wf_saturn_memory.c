#include "wf_saturn_memory.h"
wf_status_t wf_saturn_memory_contract_validate(void){
    if(WF_SATURN_VDP1_CMD_OFFSET!=WF_SATURN_VDP1_CMD_RESERVED_BYTES)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_CMD_BYTES/32u!=WF_SATURN_VDP1_CMD_COUNT)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_DYNAMIC_END>WF_SATURN_VDP1_VRAM_BYTES)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_STREAM_TILE_CAPACITY<256u)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_FG_FIRST!=WF_SATURN_CRAM_TEXT_FIRST)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_FG_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>WF_SATURN_CRAM_SPRITE_FIRST)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_SPRITE_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>WF_SATURN_CRAM_BG_FIRST)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_BG_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>1024u)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_SPRITE_BANKS*WF_SATURN_CRAM_BANK_COLORS!=WF_SATURN_CRAM_WINDOW_ENTRIES)return WF_BAD_STATE;
    return WF_OK;
}
wf_status_t wf_saturn_stream_tile_offset(uint16_t slot,uint32_t*out){
    if(!out)return WF_BAD_STATE;
    if(slot>=WF_SATURN_VDP1_STREAM_TILE_CAPACITY)return WF_BLOCKED;
    uint32_t off=WF_SATURN_VDP1_DYNAMIC_OFFSET+(uint32_t)slot*WF_SATURN_VDP1_STREAM_TILE_BYTES;
    if(off+WF_SATURN_VDP1_STREAM_TILE_BYTES>WF_SATURN_VDP1_DYNAMIC_END)return WF_BLOCKED;
    *out=off;return WF_OK;
}
wf_status_t wf_saturn_sprite_cram_entry(uint8_t bank,uint8_t color,uint16_t*out){
    if(!out)return WF_BAD_STATE;
    if(bank>=WF_SATURN_CRAM_SPRITE_BANKS||color>=WF_SATURN_CRAM_BANK_COLORS)return WF_BLOCKED;
    *out=(uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+(uint16_t)bank*16u+color);return WF_OK;
}
uint32_t wf_saturn_xbgr444_to_rgb888_raw(uint16_t v){
    uint32_t r=(uint32_t)(v&0x0Fu);uint32_t g=(uint32_t)((v>>4)&0x0Fu);uint32_t b=(uint32_t)((v>>8)&0x0Fu);
    r=(r<<4)|r;g=(g<<4)|g;b=(b<<4)|b;
    /* Yaul rgb888_t on SH-2 / CRAM mode 2: 0x00BBGGRR. */
    return (b<<16)|(g<<8)|r;
}
