#include <string.h>
#include "wf_r18_hud.h"
#include "wf_saturn_memory.h"
#include "wp2015_display_pipeline.h"
#ifdef WF_PLATFORM_SATURN
#include <yaul.h>
#endif
void wf_r18_hud_reset(wf_r18_video_t*v){if(v){memset(v->hud_dirty,255,sizeof v->hud_dirty);memset(v->palette_committed,0,sizeof v->palette_committed);}}
wf_status_t wf_r18_hud_map_update(wf_r18_video_t*v,volatile uint16_t*map,size_t*writes){
 if(!v||!map)return WF_BAD_STATE;
 size_t n=0;wp2015_name_write_t names[24];unsigned queued=0;
 for(unsigned byte=0;byte<256;byte++){
  unsigned bits=v->hud_dirty[byte];if(!bits)continue;
  for(unsigned bit=0;bit<8;bit++)if(bits&(1u<<bit)){
   unsigned cell=byte*8u+bit;
   /* Board FG0 is 8-bit VRAM on the 68000 low byte: tile low, then
    * attribute high nibble=palette, low nibble=tile bits8..11. */
   const uint8_t*p=v->fg0+cell*4u;
   uint16_t name=(uint16_t)((p[3]<<8)|p[1]);
   names[queued++]=(wp2015_name_write_t){(uint16_t)cell,name};
   names[queued++]=(wp2015_name_write_t){(uint16_t)(cell+2048u),name};n+=2;
   if(queued==24u){wp2015_display_names_write(map,names,queued);queued=0;}
  }
  v->hud_dirty[byte]=0;
 }
 wp2015_display_names_write(map,names,queued);
 if(writes)*writes=n;
 return WF_OK;
}
wf_status_t wf_r18_hud_bind(wf_r18_video_t*v,wf_r18_hud_file_fn read_file){
 if(!v||!read_file)return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
 /* Sector-sized boot reads use the CDFS HWRAM window, not B-bus->B-bus
  * DMA. No full font allocation competes with the combat HWRAM image. */
 uint8_t sector[2048] __aligned(32);
 volatile uint16_t *font=(volatile uint16_t *)VDP2_VRAM_ADDR(3,0);
 for(unsigned at=0;at<WF_SATURN_FONT_BYTES;at+=sizeof sector){
  if(read_file("FONT8.BIN",at,sector,sizeof sector))return WF_BLOCKED;
  for(unsigned i=0;i<sizeof sector;i+=2){uint16_t word;memcpy(&word,sector+i,2);font[(at+i)/2u]=word;}
 }
 vdp2_scrn_cell_format_t f={.scroll_screen=VDP2_SCRN_NBG2,.ccc=VDP2_SCRN_CCC_PALETTE_16,.char_size=VDP2_SCRN_CHAR_SIZE_1X1,.pnd_size=1,.aux_mode=VDP2_SCRN_AUX_MODE_1,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,.palette_base=VDP2_CRAM_MODE_2_OFFSET(2,0,0),.cpd_base=VDP2_VRAM_ADDR(3,0)};
 vdp2_vram_t names=VDP2_VRAM_ADDR(1,WF_SATURN_VDP2_HUD_NAMES);
 vdp2_scrn_normal_map_t map={.plane_a=names,.plane_b=names,.plane_c=names,.plane_d=names};
 vdp2_scrn_cell_format_set(&f,&map);vdp2_scrn_priority_set(VDP2_SCRN_NBG2,4);
 vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG2,0);vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG2,0);
#endif
 wf_r18_hud_reset(v);return WF_OK;
}
wf_status_t wf_r18_palettes_commit(wf_r18_video_t *v,unsigned role){
 if(!v||role>WF_R18_PALETTE_BG)return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
 const uint8_t *colors;uint16_t first;
 switch(role){
 case WF_R18_PALETTE_TEXT:colors=v->palettes;first=WF_SATURN_CRAM_TEXT_FIRST;break;
 case WF_R18_PALETTE_SPRITES:colors=v->palettes+0x2000u;first=WF_SATURN_CRAM_SPRITE_FIRST;break;
 case WF_R18_PALETTE_FG:colors=v->fg_palette;first=WF_SATURN_CRAM_FG_FIRST;break;
 default:colors=v->bg_palette;first=WF_SATURN_CRAM_BG_FIRST;break;
 }
 uint16_t pending=wf_r18_video_palette_pending(v,(wf_r18_palette_role_t)role);
 for(unsigned bank=0;bank<16;bank++)if(pending&(1u<<bank)){
  wf_status_t status=wf_saturn_cram_write_bank((uint16_t)(first+bank*16u),colors+bank*128u);
  if(status!=WF_OK)return status;
  wf_r18_video_palette_ack(v,(wf_r18_palette_role_t)role,(uint16_t)(1u<<bank));
 }
#endif
 return WF_OK;
}
wf_status_t wf_r18_hud_commit(wf_r18_video_t*v){
 wf_status_t status=wf_r18_palettes_commit(v,WF_R18_PALETTE_TEXT);
 if(status!=WF_OK)return status;
#ifdef WF_PLATFORM_SATURN
 return wf_r18_hud_map_update(v,(volatile uint16_t*)VDP2_VRAM_ADDR(1,WF_SATURN_VDP2_HUD_NAMES),NULL);
#else
 return WF_OK;
#endif
}

wf_status_t wf_r18_hud_irq(wf_runtime_t*r,wf_r18_video_t*v){
 if(!r||!v)return WF_BAD_STATE;
 if(!(r->g.gameplay_vblank_enable&128u)||(r->g.frame_handshake[1]&129u)!=1u||!(r->g.global_169e&0x2000u))return WF_OK;
 /* Original $264AA: flash the clock using two text-palette entries.
  * $169E.bit5 is the high byte of the WORD; $16A5.bit0 its subcounter. */
 unsigned flash=r->g.global_16a4&1u;
 wf_r17_fg0_write_t w[2]={{0x18071cu,flash?0x000fu:0x00ffu,2},{0x18071eu,flash?0x0003u:0x000fu,2}};
 return wf_r18_video_batch(r,w,2,v);
}
