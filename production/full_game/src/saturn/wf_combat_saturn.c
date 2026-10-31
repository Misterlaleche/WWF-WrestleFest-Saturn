#include "wf_combat_saturn.h"
#include "wf_combat.h"
#include "wf_platform.h"
#include "wf_live_video.h"
#include "wf_workspace.h"
#include "wf_cd.h"
#include "wf_r18_cdfs.h"
#include "wf_r18_present.h"
#include "wf_r18_ring.h"
#include "wf_r18_hud.h"
#include "wf_r18_display.h"
#include "wf_r18_transition.h"
#include "wf_r18_rom_hot_pages.h"
#include "wf_saturn_memory.h"
#include <string.h>
#include <dram-cart.h>
static wf_combat_t combat;
static wf_session_t *session;
static wf_board_t *board;
static wf_r18_display_t display;
static wf_r18_transition_context_t transition;
static uint8_t priority;
static bool raw_atlas;
volatile uint32_t wf_handoff_rng,wf_handoff_frame,wf_handoff_ids[9];
volatile uint32_t wf_combat_stage,wf_combat_frame,wf_combat_ack,wf_combat_error,wf_combat_dependency,wf_combat_sprites,wf_combat_rng,wf_combat_audio_count,wf_combat_audio_last,wf_combat_work,wf_combat_wait;
static int file_read(const char*name,uint32_t at,void*dst,size_t n){
 if(!strcmp(name,"HOTSPR.BIN")&&(session->runtime.g.mode_0161&1))name=raw_atlas?"ROYRAW.BIN":"ROYSPR.BIN";
 return wf_cd_asset_io.read(NULL,name,at,dst,n);
}
static int file_size(const char*name,uint32_t*n){return wf_cd_asset_io.size(NULL,name,n);}
static wf_status_t input(wf_runtime_t*r __unused,uint32_t at,uint16_t*out,void*u __unused){
 at&=0xffffff;if(!out||at<0x140020||at>0x140026||(at&1))return WF_BAD_STATE;
 *out=(uint16_t)(0xff00|combat.gameplay.input_hw_low[(at-0x140020)/2]);return WF_OK;
}
static wf_status_t audio(wf_runtime_t*r __unused,uint16_t command,void*u __unused){
 /* Audio disabled by the phase spec; retain every original command. */
 wf_combat_audio_last=command;++wf_combat_audio_count;
 return wf_board_write(board,0x14000c,command,2)?WF_OK:WF_BLOCKED;
}
static wf_status_t wait_ticks(wf_runtime_t*r __unused,uint16_t raw,void*u __unused){
 for(unsigned i=0;i<=(unsigned)raw;i++)if(!wf_platform_tick_wait())return WF_BLOCKED;
 return WF_OK;
}
static wf_status_t world_animation(wf_runtime_t*r,uint8_t record[4],uint8_t bank,void*u __unused){return wf_r18_world_288f0(r,record,bank,&combat.world);}
static wf_status_t world_scroll(wf_runtime_t*r,uint8_t side,uint16_t direction,void*u __unused){return wf_r18_world_26b68(r,side,direction,&combat.world);}
static wf_status_t reset(wf_runtime_t*r,uint8_t royal,void*u){return wf_r18_transition_reset(r,royal,u);}
static wf_status_t barrier(wf_runtime_t*r,uint8_t royal,void*u){
 if(!wf_platform_load(true))return WF_BLOCKED;
 wf_status_t st=wf_r18_transition_barrier(r,royal,u);
 return wf_platform_load(false)?st:WF_BLOCKED;
}
static bool palettes_ring(void){
 wf_r18_video_t*v=&session->video;
 if(wf_r18_hud_commit(v)!=WF_OK)return false;
 for(unsigned i=WF_R18_PALETTE_TEXT;i<=WF_R18_PALETTE_BG;i++)if(wf_r18_palettes_commit(v,i)!=WF_OK)return false;
 return !wf_r18_ring_update_deferred((int16_t)display.scroll[0],(int16_t)(2048-display.scroll[1]),
  (int16_t)display.scroll[2],(int16_t)(2048-display.scroll[3]),priority);
}
bool wf_combat_saturn_begin(wf_session_t*s,wf_board_t*b){
 if(!s||!b||s->phase!=WF_SESSION_GAMEPLAY||wf_combat_stage)return false;
 session=s;board=b;wf_combat_stage=1;
 wf_handoff_rng=s->runtime.g.rng_state;wf_handoff_frame=s->runtime.g.frame_counter;
 for(unsigned i=0;i<9;i++)wf_handoff_ids[i]=wf_be16(s->runtime.actors+i,WF_A_CHARACTER);
 raw_atlas=(s->runtime.g.mode_0161&1)&&wf_cart_valid;
 if(!wf_platform_load(true))return false;
 wf_live_video_finish();
 const wf_combat_io_t io={wf_r18_rom_read,input,audio,wait_ticks,wf_r18_present_batch,reset,barrier};
 if(wf_combat_join(&combat,s,&io)!=WF_OK)goto failed;
 combat.gameplay.branchb_288f0=world_animation;combat.gameplay.branchb_26b68=world_scroll;
 priority=b->control[17];
 /* PROGRAM/BACK cease to be readers before native caches take their LWRAM. */
 wf_workspace_combat_begin();
 wf_r18_present_init(file_read,(void*)WF_SATURN_LWRAM_ROM_CACHE,WF_SATURN_LWRAM_ROM_BYTES,
  (void*)WF_SATURN_LWRAM_TILE_CACHE,WF_SATURN_LWRAM_TILE_BYTES);
 wf_combat_stage=2;
 if(wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/2)!=WF_OK)goto failed;
 const uint16_t*extra=(s->runtime.g.mode_0161&1)?wf_r18_rom_royal_extra_pages:wf_r18_rom_tag_extra_pages;
 if(wf_r18_rom_extra_pages_preload(extra,14)!=WF_OK)goto failed;
 uint32_t size;const char*atlas=(s->runtime.g.mode_0161&1)?(raw_atlas?"ROYRAW.BIN":"ROYSPR.BIN"):"HOTSPR.BIN";
 if(file_size(atlas,&size))goto failed;
 wf_status_t loaded=raw_atlas?wf_r18_hot_sprites_load_raw(dram_cart_area_get(),wf_cart_bytes):wf_r18_hot_sprites_load();
 if(loaded!=WF_OK||size!=wf_r18_hot_sprites_bytes())goto failed;
 wf_combat_stage=3;
 unsigned world=s->runtime.g.global_007e;const char*map=world==0?"R18RING.BIN":world==1?"R18MAP1.BIN":world==2?"R18MAP2.BIN":world==5?"R18MAP5.BIN":NULL;
 uint8_t*scene=(uint8_t*)WF_SATURN_LWRAM_SCENE;
 if(!map||file_size(map,&size)||size>WF_SATURN_LWRAM_SCENE_BYTES||file_read(map,0,scene,size)||
  (((unsigned)scene[6]<<8)|scene[7])!=world||wf_r18_ring_bind(scene,size))goto failed;
 if(wf_r18_hud_bind(&s->video,file_read)!=WF_OK)goto failed;
 wf_r18_video_ring_bind(&s->video);wf_r18_hud_reset(&s->video);
 if(wf_r18_display_init(&display,&s->runtime,wf_r18_rom_read,NULL)!=WF_OK)goto failed;
 transition=(wf_r18_transition_context_t){.video=&s->video,.world=&combat.world,.display=&display,
  .camera=&combat.gameplay.camera,.render_queue=&combat.gameplay.render_queue,
  .scene_blob=scene,.scene_capacity=WF_SATURN_LWRAM_SCENE_BYTES,.file_size=file_size,.file_read=file_read,
  .priority_140010=&priority,.unblank_pending=1};
 wf_r18_transition_bind(&transition);
 if(!palettes_ring())goto failed;
 wf_r18_cdfs_combat_window_set(1);
 wf_combat_stage=4;wf_combat_rng=s->runtime.g.rng_state;
 return wf_platform_load(false);
 failed:wf_combat_error=wf_combat_stage;wf_platform_load(false);return false;
}
bool wf_combat_saturn_step(const uint8_t inputs[4]){
 if(wf_combat_stage<4)return false;
 wf_runtime_t*r=&session->runtime;
 for(unsigned i=0;i<4;i++)combat.gameplay.input_hw_low[i]=inputs[i];
 combat.gameplay.cabinet_word_140020=(uint16_t)(0xff00|inputs[0]);combat.gameplay.cabinet_word_140022=(uint16_t)(0xff00|inputs[1]);
 unsigned begin=cpu_frt_count_get();
 wf_status_t st=wf_runtime_frame(r);
 if(st!=WF_BLOCKED||!r->frame_ready)goto failed;
 if(wf_r18_display_irq(r,&display)!=WF_OK||wf_r18_world_commit(r,&combat.world)!=WF_OK||wf_r18_hud_irq(r,&session->video)!=WF_OK||!palettes_ring())goto failed;
 wf_combat_work=(uint16_t)(cpu_frt_count_get()-begin);begin=cpu_frt_count_get();
 vdp1_sync();vdp2_sync();vdp1_sync_wait();vdp2_sync_wait();
 wf_combat_wait=(uint16_t)(cpu_frt_count_get()-begin);
 if(transition.unblank_pending){
  vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG2|(priority==0x78?(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):(VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1)));
  vdp2_sprite_priority_set(0,priority==0x7c?2:3);vdp2_tvmd_display_set();transition.unblank_pending=0;
 }
 wf_runtime_vblank_ack(r);++wf_combat_ack;st=wf_runtime_frame(r);
 wf_combat_frame=r->g.frame_counter;wf_combat_rng=r->g.rng_state;wf_combat_sprites=wf_r18_present_commands();wf_combat_stage=5;
 if(st==WF_OK)return true;
 failed:wf_combat_error=combat.gameplay.last_blocked_phase;wf_combat_dependency=combat.gameplay.last_blocked_dependency;return false;
}
