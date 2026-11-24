#include "wp1980_royal_init.h"
#include <string.h>

static uint16_t word(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t lng(const uint8_t *p){return ((uint32_t)word(p)<<16)|word(p+2);}
static uint32_t crc32(const uint8_t *p,size_t bytes)
{
 uint32_t crc=0xffffffffu;
 for(size_t i=0;i<bytes;i++){
  crc^=p[i];
  for(unsigned bit=0;bit<8u;bit++)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
 }
 return ~crc;
}
wf_status_t wp1980_royal_init_apply(wf_runtime_t *r,wf_r18_video_t *v,
 wf_r18_world_ctx_t *world,void *image,size_t bytes,wp1980_royal_init_info_t *info)
{
 uint8_t *p=image;
 if(!r||!v||!world||!p||!info||bytes!=WP1980_ROYINIT_BYTES)return WF_BAD_STATE;
 if(memcmp(p,"ROYINIT1",8)||lng(p+8)!=1u||lng(p+12)!=bytes||
    lng(p+16)!=0xf94u||lng(p+20)>255u||
    crc32(p+WP1980_ROYINIT_HEADER,bytes-WP1980_ROYINIT_HEADER)!=lng(p+28))return WF_BLOCKED;
 uint8_t *raw=p+WP1980_ROYINIT_RAM;
 if(!(raw[0x161]&1u)||!word(raw+0x7cu))return WF_BLOCKED;
 wf_r17_m68k_shadow_t regs={.valid=1};
 for(unsigned i=0;i<8u;i++)regs.d[i]=lng(p+WP1980_ROYINIT_REGS+i*4u);
 info->source_pc=lng(p+16);info->source_rng=lng(raw+0x5cu);
 info->source_sr=(uint16_t)lng(p+24);info->priority=(uint8_t)lng(p+20);
 for(unsigned i=0;i<4u;i++)info->scroll[i]=word(p+WP1980_ROYINIT_SCROLL+i*2u);
 memcpy(v->fg0,p+WP1980_ROYINIT_TEXT,8192u);
 memcpy(v->fg0+0x2000u,p+WP1980_ROYINIT_SPRITES,8192u);
 memcpy(v->background,p+WP1980_ROYINIT_FGMAP,4096u);
 memcpy(v->background+0x2000u,p+WP1980_ROYINIT_BGMAP,4096u);
 memcpy(v->palettes,p+WP1980_ROYINIT_PALETTE,sizeof v->palettes);
 memcpy(v->bg_palette,p+WP1980_ROYINIT_PALETTE+0x6000u,sizeof v->bg_palette);
 memcpy(v->fg_palette,p+WP1980_ROYINIT_PALETTE+0x8000u,sizeof v->fg_palette);
 memcpy(v->palette_used,raw+0x1600u,sizeof v->palette_used);
 memcpy(v->palette_map,raw+0x1610u,sizeof v->palette_map);
 memset(v->hud_dirty,255,sizeof v->hud_dirty);
 memset(v->palette_dirty,0,sizeof v->palette_dirty);
 memset(v->palette_committed,0,sizeof v->palette_committed);
 memset(world->animation_queue,0,sizeof world->animation_queue);
 memset(world->animation_count,0,sizeof world->animation_count);
 for(unsigned side=0;side<2u;side++)for(unsigned i=0;i<16u;i++){
  const uint8_t *cache=raw+0x16d6u+side*128u+i*8u;
  world->cache[side][i]=(wf_r18_world_transport_t){lng(cache),lng(cache+4)};
 }
 /* The capture tap precedes the F94 CLR.L $1C0080 bus write. The existing
  * original F8C continuation performs idempotent BSET76, then CLR80,
  * handshake clear and the first $21B4 call exactly once. */
 return wf_r18_runtime_handoff_f8c(r,raw,16384u,&regs);
}
