#include "wf_r18_transition.h"
#include "wf_r17_dependencies.h"
#include "wf_r17_fixed_four_10120.h"
#include "wf_r18_referee.h"
#include "wf_r18_ring.h"
#include "wf_r18_hud.h"
#include <string.h>
static wf_r18_transition_context_t *bound;
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
void wf_r18_transition_bind(wf_r18_transition_context_t *c){bound=c;}
static int valid(void*v){return bound&&bound->video==v&&bound->world&&bound->display&&bound->camera&&bound->render_queue&&bound->priority_140010;}
static wf_status_t block(uint32_t pc){bound->blocked_pc=pc;return WF_BLOCKED;}
static wf_status_t read_rom(uint32_t at,void*dst,size_t n){
 wf_r18_video_t*v=bound->video;
 if(!v->read_rom||at>0x80000u||n>0x80000u-at||v->read_rom(v->read_user,at,dst,n)!=WF_OK){v->failed_address=at;return WF_BLOCKED;}
 return WF_OK;
}
static wf_status_t pointer(uint32_t at,uint32_t*out){uint8_t b[4];wf_status_t s=read_rom(at,b,4);if(s==WF_OK)*out=be32(b);return s;}
wf_status_t wf_r18_transition_reset(wf_runtime_t*r,uint8_t royal,void*v){
 (void)royal;if(!r)return WF_BAD_STATE;if(!valid(v))return WF_BLOCKED;bound->blocked_pc=0;
 /* $1F6C/$1FDE: the native queue replaces the external pointer lists at
  * $19B2..$1CD1. Invalidate the borrowed scene before its buffer is reused. */
 bound->video->ring_cell=NULL;(void)wf_r18_ring_bind(NULL,0);
 wf_status_t s=wf_r17_transition_reset_ram(r);if(s!=WF_OK)return s;
 memset(bound->world->cache,0,sizeof bound->world->cache);
 memset(bound->world->animation_queue,0,sizeof bound->world->animation_queue);
 memset(bound->world->animation_count,0,sizeof bound->world->animation_count);
 memset(bound->video->background,0,0x1000);memset(bound->video->background+0x2000,0,0x1000);
 memset(bound->video->fg0+0x2000,0,0x1000);
 wf_r17_render_queue_clear(bound->render_queue);bound->sprite_control_140008=0;
 return WF_OK;
}
static wf_status_t load_scene(wf_runtime_t*r){
 const char*name=r->g.global_007e==0?"R18RING.BIN":r->g.global_007e==2?"R18MAP2.BIN":NULL;
 uint32_t bytes=0;
 if(!name||!bound->scene_blob||!bound->file_size||!bound->file_read||bound->file_size(name,&bytes)||bytes<544u||bytes>bound->scene_capacity)return block(0x26e66);
 /* Never leave an old scene callable while CD reads replace its storage. */
 bound->video->ring_cell=NULL;(void)wf_r18_ring_bind(NULL,0);
 if(bound->file_read(name,0,bound->scene_blob,bytes))return block(0x26e66);
 const uint8_t*p=bound->scene_blob;
 if(be16(p+6)!=r->g.global_007e||be16(p+8)!=r->g.global_007e||wf_r18_ring_bind(p,bytes))return block(0x26e66);
 return WF_OK;
}
static wf_status_t map_26e66(wf_runtime_t*r){
 uint16_t x=r->g.global_1804,y=r->g.global_1806,bx=x>>5,by=y>>5;
 uint16_t left=(uint16_t)(bx-3u);uint8_t priority;
 if(read_rom(0x26e9eu+r->g.global_007e,&priority,1)!=WF_OK)return block(0x26e66);
 r->g.global_17f6=r->g.global_17e6=r->g.global_17ea=x;
 r->g.global_17fa=r->g.global_17ee=r->g.global_17f2_word=y;
 /* $26F14 fills a 16x16 window of 32px blocks. The cache retained by
  * $26DAC is the final row, one row above the final $17E4 cursor. */
 for(unsigned side=0;side<2;side++){
  uint32_t root;if(pointer((side?0x270bcu:0x2709cu)+r->g.global_007e*4u,&root)!=WF_OK)return block(0x26e66);
  for(unsigned row=0;row<16;row++)for(unsigned col=0;col<16;col++){
   uint16_t xx=(uint16_t)(left+col),yy=(uint16_t)(0x40u-(uint16_t)(by+4u-row));
   uint16_t offset=(uint16_t)((xx/10u)*4u+(yy>>3)*32u);uint32_t page;
   if(pointer((uint32_t)(root+(int16_t)offset),&page)!=WF_OK)return block(0x26e66);
   uint32_t source=page+(xx%10u)*(side?4u:8u)+(yy&7u)*(side?0x50u:0xa0u);
   uint32_t dest=(side?0x82000u:0x80000u)+((xx+16u)&15u)*(side?4u:8u)+(yy&15u)*(side?0x80u:0x100u);
   size_t width=side?4u:8u;uint8_t top[8],bottom[8];
   if(read_rom(source,top,width)!=WF_OK||read_rom(source+(side?0x28u:0x50u),bottom,width)!=WF_OK)return block(0x26e66);
   memcpy(bound->video->background+(dest-0x80000u),top,width);
   memcpy(bound->video->background+(dest-0x80000u)+(side?0x40u:0x80u),bottom,width);
   bound->world->cache[side][col]=(wf_r18_world_transport_t){source,dest};
  }
 }
 r->g.global_17d6=r->g.global_17da=r->g.global_17de=bx;
 r->g.global_17d8=r->g.global_17dc=r->g.global_17e0=by;
 r->g.global_17e2=left;r->g.global_17e4=(uint16_t)(by-12u);
 r->g.global_16d4=bx==left?(by==(uint16_t)(by-12u)?0xffu:0u):(bx<left?3u:1u);
 r->g.global_17fe|=0x8000u;r->g.global_16d2&=0x3fffu;r->branchb_prev_185a[0]=255;
 r->m68k_regs.d[1]=0;wf_r17_m68k_move_w(&r->m68k_regs,2,0xffff);wf_r17_m68k_move_w(&r->m68k_regs,3,16);
 wf_r17_m68k_move_w(&r->m68k_regs,0,(uint16_t)((r->g.global_007e&0xff00u)|priority));*bound->priority_140010=priority;
 if(!(r->g.frame_handshake[1]&16u)){
  bound->display->scroll[0]=x;bound->display->scroll[1]=(uint16_t)(2048u-y);
  bound->display->scroll[2]=x;bound->display->scroll[3]=(uint16_t)(2048u-y);
 }
 wf_camera_state_t*c=bound->camera;
 c->scroll0_x=c->scroll1_x=(int16_t)x;c->scroll0_y=c->scroll1_y=(int16_t)y;
 c->target_x=(int16_t)x;c->target_y=(int16_t)y;c->scene_index=r->g.global_007e;
 return WF_OK;
}
static wf_status_t palette_bank(wf_runtime_t*r,uint32_t source,uint32_t dest){
 uint8_t b[32];wf_r17_fg0_write_t w[8];if(read_rom(source,b,sizeof b)!=WF_OK)return WF_BLOCKED;
 for(unsigned i=0;i<8;i++)w[i]=(wf_r17_fg0_write_t){dest+i*4u,be32(b+i*4u),4};
 return wf_r18_video_batch(r,w,8,bound->video);
}
static wf_status_t palettes_2a06(wf_runtime_t*r){
 for(unsigned layer=0;layer<2;layer++){
  uint32_t source;if(pointer((layer?0x2ab4u:0x2a98u)+r->g.global_15f4*4u,&source)!=WF_OK)return block(0x2a06);
  for(unsigned bank=0;bank<16;bank++)if(palette_bank(r,source+bank*32u,(layer?0x186000u:0x188000u)+bank*128u)!=WF_OK)return block(0x2a06);
 }
 for(unsigned bank=0;bank<16;bank++){
  uint32_t source;if(pointer(0x2b82u+r->g.global_15fc*64u+bank*4u,&source)!=WF_OK||palette_bank(r,source,0x180000u+bank*128u)!=WF_OK)return block(0x2a06);
 }
 wf_r17_m68k_move_w(&r->m68k_regs,1,0x3c);wf_r17_m68k_move_w(&r->m68k_regs,2,0xffff);return WF_OK;
}
static wf_status_t palettes_c1ea(wf_runtime_t*r){
 if(!r->g.credited_route||(r->g.mode_0161&1u))return WF_OK;
 for(unsigned i=0;i<4;i++){
  wf_actor_raw_t*a=&r->actors[i];if(!(wf_u8(a,0)&128u))continue;unsigned id=wf_be16(a,0x56)&15u;uint32_t source;
  if(pointer((wf_u8(a,0x33)&1u?0xc264u:0xc294u)+id*4u,&source)!=WF_OK||palette_bank(r,source,0x180100u+id*128u)!=WF_OK)return block(0xc1ea);
 }
 return WF_OK;
}
wf_status_t wf_r18_transition_barrier(wf_runtime_t*r,uint8_t royal,void*v){
 if(!r)return WF_BAD_STATE;if(!valid(v))return WF_BLOCKED;bound->blocked_pc=0;
 wf_status_t s=load_scene(r);if(s!=WF_OK)return s;
 s=map_26e66(r);if(s!=WF_OK)return s;r->g.global_15f4=r->g.global_15f8=r->g.global_007e;
 s=palettes_2a06(r);if(s!=WF_OK)return s;s=palettes_c1ea(r);if(s!=WF_OK)return s;
 if(royal){s=wf_r18_referee_clear_206fe(r,v);if(s!=WF_OK)return block(0x206fe);wf_r17_fixed_four_init_1004c(r);}
 /* ring_bind uploads native VDP2 characters; update commits its new cells
  * before gameplay resumes. Palette work above uses original ROM bytes. */
 wf_r18_video_ring_bind(bound->video);
 for(unsigned role=0;role<4;role++)if(wf_r18_palettes_commit(bound->video,role)!=WF_OK)return block(0x2a06);
 if(wf_r18_hud_commit(bound->video)!=WF_OK)return block(0xc1ea);
 if(wf_r18_ring_update((int16_t)bound->display->scroll[0],(int16_t)(2048u-bound->display->scroll[1]),(int16_t)bound->display->scroll[2],(int16_t)(2048u-bound->display->scroll[3]),*bound->priority_140010))return block(0x26e66);
 return WF_OK;
}
