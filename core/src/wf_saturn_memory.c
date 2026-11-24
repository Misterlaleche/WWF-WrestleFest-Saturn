#include "wf_saturn_memory.h"
#ifdef WF_JO_ENGINE
#include "wf_jo_video.h"
#endif
wf_status_t wf_saturn_memory_contract_validate(void){
    if(WF_SATURN_LWRAM_SCENE<0x00200000u||WF_SATURN_LWRAM_SCENE+WF_SATURN_LWRAM_SCENE_BYTES>WF_SATURN_LWRAM_ROM_CACHE)return WF_BAD_STATE;
    if((WF_SATURN_LWRAM_ROM_CACHE&127u)||(WF_SATURN_LWRAM_ROM_BYTES&1023u)||(WF_SATURN_LWRAM_TILE_CACHE&127u)||(WF_SATURN_LWRAM_TILE_BYTES&127u)||(WF_SATURN_HWRAM_HOT_INDEX_BYTES&31u))return WF_BAD_STATE;
    if(WF_SATURN_LWRAM_ROM_CACHE+WF_SATURN_LWRAM_ROM_BYTES>WF_SATURN_LWRAM_VIDEO_WRITES)return WF_BAD_STATE;
    if(WF_SATURN_LWRAM_VIDEO_WRITES+WF_SATURN_LWRAM_VIDEO_WRITE_BYTES>WF_SATURN_LWRAM_TILE_CACHE)return WF_BAD_STATE;
    if(WF_SATURN_LWRAM_TILE_CACHE+WF_SATURN_LWRAM_TILE_BYTES>0x00300000u)return WF_BAD_STATE;
#ifdef WF_JO_ENGINE
    if(WF_SATURN_LWRAM_VIDEO_WRITES+WF_SATURN_LWRAM_VIDEO_WRITE_BYTES>WF_SATURN_LWRAM_SCENE_RESIDENT)return WF_BAD_STATE;
    if(WF_SATURN_LWRAM_SCENE_RESIDENT+WF_SATURN_LWRAM_SCENE_RESIDENT_BYTES>WF_SATURN_LWRAM_TILE_CACHE)return WF_BAD_STATE;
    if(WF_JO_TEXTURE_FIRST<0x10000u || WF_JO_TEXTURE_FIRST+WF_JO_TEXTURE_BYTES>0x7e4f8u)return WF_BAD_STATE;
    if(WF_JO_TEXTURE_BANK_BYTES<256u*128u || WF_JO_MAP_GROUP_BYTES*WF_JO_TEXTURE_BANKS>=WF_SATURN_VDP2_BACK_COLOR)return WF_BAD_STATE;
#else
    if(WF_SATURN_LWRAM_FRONT_SCENE<WF_SATURN_LWRAM_TILE_CACHE||WF_SATURN_LWRAM_FRONT_SCENE+WF_SATURN_LWRAM_FRONT_SCENE_BYTES>WF_SATURN_LWRAM_FRONT_TILES)return WF_BAD_STATE;
    if((WF_SATURN_LWRAM_FRONT_TILES&127u)||(WF_SATURN_LWRAM_FRONT_TILE_BYTES&127u)||WF_SATURN_LWRAM_FRONT_TILES+WF_SATURN_LWRAM_FRONT_TILE_BYTES>WF_SATURN_LWRAM_FRONT_RAM)return WF_BAD_STATE;
    if(WF_SATURN_LWRAM_FRONT_RAM+WF_SATURN_LWRAM_FRONT_RAM_BYTES>WF_SATURN_LWRAM_TILE_CACHE+WF_SATURN_LWRAM_TILE_BYTES)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_CMD_OFFSET!=WF_SATURN_VDP1_CMD_RESERVED_BYTES)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_CMD_BYTES/32u!=WF_SATURN_VDP1_CMD_COUNT)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_DYNAMIC_END>WF_SATURN_VDP1_MUSIC_OFFSET||
       WF_SATURN_VDP1_MUSIC_END!=WF_SATURN_VDP1_VRAM_BYTES)return WF_BAD_STATE;
    if(WF_SATURN_VDP1_STREAM_TILE_CAPACITY<256u)return WF_BAD_STATE;
#endif
    if(WF_SATURN_CRAM_FG_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>WF_SATURN_CRAM_SPRITE_FIRST)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_SPRITE_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>WF_SATURN_CRAM_TEXT_FIRST)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_TEXT_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>WF_SATURN_CRAM_BG_FIRST)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_BG_FIRST+WF_SATURN_CRAM_WINDOW_ENTRIES>1024u)return WF_BAD_STATE;
    if(WF_SATURN_CRAM_SPRITE_BANKS*WF_SATURN_CRAM_BANK_COLORS!=WF_SATURN_CRAM_WINDOW_ENTRIES)return WF_BAD_STATE;
    return WF_OK;
}
wf_status_t wf_saturn_stream_tile_offset(uint16_t slot,uint32_t*out){
#ifdef WF_JO_ENGINE
    /* Bank-zero reference only. The live Jo backend selects its fenced bank. */
    return wf_jo_texture_offset(0,slot,out);
#else
    if(!out)return WF_BAD_STATE;
    if(slot>=WF_SATURN_VDP1_STREAM_TILE_CAPACITY)return WF_BLOCKED;
    uint32_t off=WF_SATURN_VDP1_DYNAMIC_OFFSET+(uint32_t)slot*WF_SATURN_VDP1_STREAM_TILE_BYTES;
    if(off+WF_SATURN_VDP1_STREAM_TILE_BYTES>WF_SATURN_VDP1_DYNAMIC_END)return WF_BLOCKED;
    *out=off;return WF_OK;
#endif
}
wf_status_t wf_saturn_sprite_cram_entry(uint8_t bank,uint8_t color,uint16_t*out){
    if(!out)return WF_BAD_STATE;
    if(bank>=WF_SATURN_CRAM_SPRITE_BANKS||color>=WF_SATURN_CRAM_BANK_COLORS)return WF_BLOCKED;
    *out=(uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+(uint16_t)bank*16u+color);return WF_OK;
}
uint32_t wf_saturn_xbgr444_to_rgb888_raw(uint16_t v){
    uint32_t r=(uint32_t)(v&0x0Fu);uint32_t g=(uint32_t)((v>>4)&0x0Fu);uint32_t b=(uint32_t)((v>>8)&0x0Fu);
    r=(r<<4)|r;g=(g<<4)|g;b=(b<<4)|b;
    /* Saturn CRAM mode 2 in big-endian SH-2 memory: 0x00BBGGRR. */
    return (b<<16)|(g<<8)|r;
}

/* One owner for all game CRAM stores. The high nibble is unused on the
 * arcade DAC; compare effective colors, then convert only changed entries. */
#include <string.h>
static uint8_t cram_color[2048];
static uint8_t cram_valid[128];
uint32_t wf_saturn_cram_writes;
void wf_saturn_cram_reset(void){memset(cram_valid,0,sizeof cram_valid);wf_saturn_cram_writes=0;}
wf_status_t wf_saturn_cram_write(uint16_t entry,uint16_t color){
 if(entry>=1024u)return WF_BLOCKED;
 color&=0x0fffu;unsigned mask=1u<<(entry&7u);
 if((cram_valid[entry>>3]&mask)&&((uint16_t)((cram_color[entry*2u]<<8)|cram_color[entry*2u+1u]))==color)return WF_OK;
#if defined(WF_PLATFORM_SATURN) || defined(__sh__)
 wf_jo_video_cram_write(entry,wf_saturn_xbgr444_to_rgb888_raw(color));
#endif
 cram_color[entry*2u]=(uint8_t)(color>>8);cram_color[entry*2u+1u]=(uint8_t)color;cram_valid[entry>>3]|=(uint8_t)mask;++wf_saturn_cram_writes;
 return WF_OK;
}

wf_status_t wf_saturn_cram_write_bank(uint16_t first,const uint8_t*p){
 if(!p||(first&15u)||first>1008u)return WF_BAD_STATE;
 if(cram_valid[first>>3]==255u&&cram_valid[(first>>3)+1u]==255u&&!memcmp(cram_color+first*2u,p,32))return WF_OK;
 for(unsigned pen=0;pen<16;pen++)wf_saturn_cram_write((uint16_t)(first+pen),(uint16_t)((p[pen*2u]<<8)|p[pen*2u+1u]));
 return WF_OK;
}
