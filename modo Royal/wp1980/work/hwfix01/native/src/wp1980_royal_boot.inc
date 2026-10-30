/* WP1980 Royal launch transport. Derived from the existing Saturn frontend;
 * the original portable RESET/coin/START/mode/roster/match owners are unchanged.
 * Automatic launch is ordinary cabinet input, never an actor/RAM fixture. */
#include "wf_r18_frontend_driver.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_r18_match_flow.h"
#include "wf_r18_match_hud.h"
#include "wf_r18_match_flash.h"
#include "wf_r18_match_driver.h"
#include "wf_r18_match_support.h"
#include "wf_r18_frontend_world.h"

static wf_r18_frontend_driver_t frontend;
static wf_r18_match_driver_t frontend_match;
static volatile uint32_t frontend_observed[8];
static uint8_t frontend_scroll_ports[0x20],frontend_control_ports[0x20];
static uint32_t frontend_serviced_clock;
static unsigned frontend_bound_world=0xffffu,frontend_servicing,frontend_pending;
static unsigned frontend_scene_gate; /* 1=clear in progress, 2=ready to unblank. */
static uint16_t frontend_back_color;
static uint16_t fr_u16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t fr_u32(const uint8_t*p){return ((uint32_t)fr_u16(p)<<16)|fr_u16(p+2);}
static void fr_w32(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static wf_status_t frontend_service(void);
static void frontend_settle(void);
static void frontend_sprite_priority_set(unsigned priority){
 for(unsigned i=0;i<8u;i++)vdp2_sprite_priority_set(i,priority);
}
static wf_status_t frontend_scene_clear(void*u,uint32_t owner,unsigned complete){
 (void)u;(void)owner;
 if(complete){if(frontend_scene_gate)frontend_scene_gate=2u;return WF_OK;}
 frontend_settle();vdp1_sync_wait();vdp2_sync_wait();
 /* Keep TVMD running: the original $2A06/$2988 waits still need real
  * VBlank levels while the temporary $FFC5FF04 name pattern is written. */
 vdp2_scrn_display_set(0);frontend_sprite_priority_set(0);
 vdp2_sync();vdp2_sync_wait();frontend_scene_gate=1u;
 return WF_OK;
}
static wf_status_t frontend_reset_ready(void*u,uint8_t*ram,size_t bytes){
 (void)u;if(!ram||bytes<0xc2u+WF_R18_SAVE_RANKING_BYTES)return WF_BAD_STATE;
 (void)wf_r18_save_saturn_load(ram+0xc2u);
 return WF_OK; /* Missing/full/unformatted backup remains a playable default. */
}
static uint8_t frontend_controls(const smpc_peripheral_digital_t*p){
 uint8_t v=0xffu;if(!p->connected)return v;
 if(p->pressed.button.right)v&=(uint8_t)~1u;
 if(p->pressed.button.left)v&=(uint8_t)~2u;
 if(p->pressed.button.up)v&=(uint8_t)~4u;
 if(p->pressed.button.down)v&=(uint8_t)~8u;
 if(p->pressed.button.a)v&=(uint8_t)~16u;
 if(p->pressed.button.b)v&=(uint8_t)~32u;
 if(p->pressed.button.c)v&=(uint8_t)~64u;
 if(p->pressed.button.start)v&=(uint8_t)~128u;
 return v;
}
static uint16_t frontend_port(unsigned port){
 /* Transport configuration: DIP word$0300, matching the existing NTSC
  * build. C maps to each cabinet coin input; original credit owners decide
  * acceptance. Directions use the frontend UP/DOWN order. */
 uint16_t v=port==1u?0xfcffu:0xffffu;
 if(port<4u)v=(uint16_t)((v&0xff00u)|frontend_controls(player_pads[port]));
 if(!port){
  if((pad.connected&&pad.pressed.button.c)||(frontend_pad3.connected&&frontend_pad3.pressed.button.c))v&=(uint16_t)~0x0100u;
  if((frontend_pad2.connected&&frontend_pad2.pressed.button.c)||(frontend_pad4.connected&&frontend_pad4.pressed.button.c))v&=(uint16_t)~0x0200u;
 }
 if(port==3u){v&=(uint16_t)~0x0400u;if(vdp2_tvmd_vblank_in())v|=0x0400u;}
 return wp1980_frontend_input(frontend.phase,frontend.ram,video_clock,port,v);
}
static wf_status_t frontend_bus_read(void*u,uint32_t at,uint32_t*out,unsigned width){
 (void)u;if(!out||!width||width>4u)return WF_BAD_STATE;
 at&=0xffffffu;uint32_t value=0;
 if(at>=0x140020u&&at<0x140028u){
  saturn_controllers_sample();
 }
 for(unsigned i=0;i<width;i++){
  uint32_t a=at+i;uint8_t b;
  if(a>=0x140020u&&a<0x140028u){uint16_t v=frontend_port((a-0x140020u)/2u);b=(uint8_t)(v>>((a&1u)?0:8));}
  else if(a>=0x100000u&&a<0x100020u)b=frontend_scroll_ports[a-0x100000u];
  else if(a>=0x140000u&&a<0x140020u)b=frontend_control_ports[a-0x140000u];
  else if(a>=0x184000u&&a<0x184002u)b=(uint8_t)(frontend_back_color>>((a&1u)?0:8));
  else return WF_BLOCKED;
  value=(value<<8)|b;
 }
 *out=value;return WF_OK;
}
static wf_status_t frontend_bus_store(void*u,uint32_t at,uint32_t value,unsigned width){
 (void)u;at&=0xffffffu;
 if(!width||width>4u)return WF_BAD_STATE;
 /* Reset's $3100 is a stop command. Before the SCSP boots there is no
  * voice to stop; later commands still go through the normal queue. */
 if(at==0x14000cu&&width==2u&&
    (scsp_boot_complete||(uint16_t)value!=0x3100u)&&
    wf_r18_scsp_audio(NULL,(uint16_t)value,NULL)!=WF_OK)return WF_BAD_STATE;
 uint8_t *p=NULL;
 if(at>=0x100000u&&at+width<=0x100020u)p=frontend_scroll_ports+at-0x100000u;
 else if(at>=0x140000u&&at+width<=0x140020u)p=frontend_control_ports+at-0x140000u;
 else if(at==0x184000u&&width==2u){frontend_back_color=(uint16_t)value;return WF_OK;}
 if(!p)return WF_BLOCKED;
 for(unsigned i=0;i<width;i++)p[i]=(uint8_t)(value>>(8u*(width-i-1u)));
 return WF_OK;
}
static int frontend_file_read(const char*name,uint32_t at,void*out,size_t n){
 return wf_r18_file_read(!strcmp(name,"HOTSPR.BIN")?"FRONTSPR.BIN":name,at,out,n);
}
static int frontend_combat_file_read(const char*name,uint32_t at,void*out,size_t n){
 return wf_r18_file_read(!strcmp(name,"HOTSPR.BIN")&&(rt.g.mode_0161&1u)?"ROYSPR.BIN":name,at,out,n);
}
static wf_status_t frontend_world_bind(void*u,unsigned mode){
 (void)u;if(frontend_bound_world==mode){if(frontend_scene_gate)frontend_scene_gate=2u;return WF_OK;}
 /* Empty the old VDP1 command list before changing the legal plate or map.
  * The ERASE_CHANGE buffer can otherwise retain old central sprites during
  * the first scan of the next pack on a physical Saturn. */
 frontend_settle();
 if(wf_r18_present_records_commit(&video,0)!=WF_OK)return WF_BLOCKED;
 vdp1_sync();
 vdp1_sync_wait();
 vdp2_scrn_display_set(0);frontend_sprite_priority_set(0);
 vdp2_sync();vdp2_sync_wait();frontend_scene_gate=1u;
 /* Drawing an empty VDP1 list completes before its framebuffer becomes
  * the displayed buffer. Cross one full VBlank while the VDP2 layers are
  * still gated, so a newly exposed ring cannot inherit title portraits. */
 vdp2_tvmd_vblank_out_wait();vdp2_tvmd_vblank_in_wait();
 /* A different immutable pack uploads characters and initial CRAM. Keep
  * scanout blank until the IRQ commits the shared live palette state. */
 const char*name=mode==0u?"R18RING.BIN":mode==1u?"R18MAP1.BIN":mode==2u?"R18MAP2.BIN":mode==3u?"R18MENU.BIN":
  mode==4u?"R18MAP4.BIN":mode==5u?"R18MAP5.BIN":NULL;
 uint32_t bytes;int combat_world=mode==0u||mode==1u||mode==2u||mode==5u;
 void*blob;size_t capacity;
 if(combat_world){blob=RING_BLOB;capacity=WF_SATURN_LWRAM_SCENE_BYTES;}
 else{blob=(void*)WF_SATURN_LWRAM_FRONT_SCENE;capacity=WF_SATURN_LWRAM_FRONT_SCENE_BYTES;}
 /* Rings live outside the combat tile lease from their first bind. The
  * larger interview/menu packs may borrow that lease only before F8C. */
 if(!name||wf_r18_file_size(name,&bytes)||bytes>capacity||
    wf_r18_file_read(name,0,blob,bytes)||fr_u16((uint8_t*)blob+6)!=mode)return WF_BLOCKED;
 if(wf_r18_ring_bind(blob,bytes))return WF_BLOCKED;
 wf_r18_video_ring_bind(&video);
 /* bind uploads initial pack colors. Restore live raw palette state at
  * the next commit, including an in-progress original black/fade state. */
 video.palette_committed[WF_R18_PALETTE_FG]=video.palette_committed[WF_R18_PALETTE_BG]=0;
 frontend_bound_world=mode;frontend_scene_gate=2u;return WF_OK;
}
static wf_status_t frontend_submit(void*u,const wf_r18_sprite_record_t*records,size_t count){
 (void)u;frontend_settle();size_t cap;wf_r18_sprite_record_t*workspace=wf_r18_present_records_buffer(&cap);
 if(count>cap||(!records&&count))return WF_BAD_STATE;
 if(count)memcpy(workspace,records,count*sizeof *records);
 wf_status_t st=wf_r18_present_records_commit(&video,count);if(st!=WF_OK)return st;
 vdp1_sync();frontend_pending=1;return WF_OK;
}
static wf_status_t frontend_commit(void*u,wf_r18_frontend_driver_t*d){
 (void)u;
 if(wf_r18_hud_commit(&video)!=WF_OK)return WF_BLOCKED;
 for(unsigned role=WF_R18_PALETTE_SPRITES;role<=WF_R18_PALETTE_BG;role++)
  if(wf_r18_palettes_commit(&video,role)!=WF_OK)return WF_BLOCKED;
 uint32_t visible;unsigned sprite_priority=3u;
 if(d->world_visible){
  uint16_t sx0=fr_u16(frontend_scroll_ports),sy0=(uint16_t)(2048u-fr_u16(frontend_scroll_ports+2));
  uint16_t sx1=fr_u16(frontend_scroll_ports+4),sy1=(uint16_t)(2048u-fr_u16(frontend_scroll_ports+6));
  uint8_t priority=(uint8_t)fr_u16(frontend_control_ports+0x10);
  sprite_priority=priority==0x7cu?2u:3u;
  if(wf_r18_ring_update_deferred((int16_t)sx0,(int16_t)sy0,(int16_t)sx1,(int16_t)sy1,priority))return WF_BLOCKED;
  /* ring_update can retain unchanged scroll/priority. Showing a world
   * after a legal-text screen must also restore its display enables. */
  visible=VDP2_SCRN_DISPTP_NBG2|(priority==0x78u?
   (VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):(VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1));
 }else visible=VDP2_SCRN_DISP_NBG2;
 if(title_loading_hidden&&d->phase==WF_R18_DRIVER_TITLE&&d->scene.title.pc>=0x837eu)
  title_loading_hidden=0;
 if(title_loading_hidden)visible=0;
 vdp2_scrn_display_set(frontend_scene_gate?0:visible);
 frontend_sprite_priority_set(frontend_scene_gate||title_loading_hidden?0:sprite_priority);
 vdp2_sync();
 if(frontend_scene_gate==2u){
  frontend_settle();vdp1_sync_wait();vdp2_sync_wait();
  vdp2_scrn_display_set(visible);
  frontend_sprite_priority_set(title_loading_hidden?0:sprite_priority);
  vdp2_sync();vdp2_sync_wait();
  frontend_scene_gate=0u;
 }
 vdp2_tvmd_display_set();
 return WF_OK;
}
static void frontend_settle(void){
 if(frontend_pending){vdp1_sync_wait();frontend_pending=0;}
}
static wf_status_t frontend_service(void){
 if(scsp_boot_complete&&!wf_r18_scsp_service())return WF_BAD_STATE;
 frontend_observed[0]=frontend.phase;frontend_observed[1]=frontend.pc;
 frontend_observed[2]=video_clock;frontend_observed[3]=frontend.ram?frontend.ram[0x4f]:0;
 frontend_observed[4]=frontend.ram?((uint32_t)fr_u16(frontend.ram+0x76)<<16)|fr_u16(frontend.ram+0x6e):0;
 frontend_observed[5]=frontend.generation;frontend_observed[6]=frontend.ram?fr_u16(frontend.ram+0x160):0;
 saturn_controllers_sample();
 unsigned reset_event=wf_r18_reset_combo_poll(&reset_combo,reset_combo_held(),
  frontend.phase==WF_R18_DRIVER_TITLE);
 if(reset_event==WF_R18_RESET_BIOS){
  frontend_settle();vdp1_sync_wait();vdp2_sync_wait();
  vdp2_tvmd_display_clear();bios_cd_player_execute();
 }
 if(reset_event==WF_R18_RESET_TITLE){reset_to_title_requested=1;return WF_BLOCKED;}
 if(frontend_servicing)return WF_OK;
 frontend_servicing=1;
 uint32_t target_clock=video_clock;
 while(frontend_serviced_clock!=target_clock){
  ++frontend_serviced_clock;
  /* Primary timing reference: MAME technos/ddragon3.cpp, scanline callback
   * every16 of272 lines, IRQ3 at248. Preserve its17 coin IRQ quanta per
   * NTSC display period; no gameplay/frame counter is advanced here. */
  for(unsigned i=0;i<17;i++)if((frontend.control.sr&0x0700u)<0x0200u)
   if(wf_r18_frontend_driver_on_raster(&frontend)!=WF_OK){frontend_servicing=0;return WF_BLOCKED;}
  if((frontend.control.sr&0x0700u)<0x0300u&&wf_r18_frontend_driver_on_vblank(&frontend)!=WF_OK){frontend_servicing=0;return WF_BLOCKED;}
 }
 frontend_servicing=0;return WF_OK;
}
static wf_status_t frontend_wait_level(void*u,wf_r18_frontend_driver_t*d,unsigned high){
 (void)u;uint32_t generation=d->generation;frontend_settle();
 for(;;){
  if(frontend_service()!=WF_OK)return WF_BLOCKED;
  if(d->generation!=generation||!!vdp2_tvmd_vblank_in()==!!high)return WF_OK;
 }
}
static wf_status_t frontend_wait_vblank(void*u,wf_r18_frontend_driver_t*d){
 (void)u;uint32_t generation=d->generation,previous=frontend_serviced_clock;frontend_settle();
 do {if(frontend_service()!=WF_OK)return WF_BLOCKED;}while(frontend_serviced_clock==previous&&d->generation==generation);
 return WF_OK;
}
static void frontend_boot_scsp(void){
 uint32_t bytes;
 if(scsp_boot_complete)return;
 /* The first legal plate is already in VDP2. SCSP firmware and the bulk SFX
  * uploader may borrow the scene buffer while that static plate is shown. */
 if(wf_r18_file_size("SCSP.ELF",&bytes)!=0||bytes>WF_SATURN_LWRAM_SCENE_BYTES||
    wf_r18_file_read("SCSP.ELF",0,RING_BLOB,bytes)!=0)fail(0x2052u,0x68000u);
 boot_timing[1]=video_clock;
 if(!wf_r18_scsp_boot(&video_clock,RING_BLOB,bytes))fail(0x2052u,0x68000u);
 boot_timing[2]=video_clock;
 scsp_boot_complete=1u;
}
static wf_status_t frontend_begin(uint32_t seed,unsigned title_only){
 frontend_observed[7]=1;
 wp1980_frontend_input_disable();
 frontend_scene_gate=0u;
 wf_r18_cdfs_combat_window_set(0u);
 wf_r18_title_portrait_divisor_set(2u);
 uint8_t*raw=(uint8_t*)WF_SATURN_LWRAM_FRONT_RAM;
 /* The lower work bytes are explicit fresh-cabinet inputs. Saturn BIOS
  * scratch at00200000 is not arcade credit/service RAM. Preserve the
  * explicit reproducible power-on seed; original654 clears$60 upward. */
 memset(raw,0,0x60u);fr_w32(raw+0x5c,seed);
 wf_r18_present_init(frontend_file_read,ROM_CACHE,WF_SATURN_LWRAM_ROM_BYTES,
  (void*)WF_SATURN_LWRAM_FRONT_TILES,WF_SATURN_LWRAM_FRONT_TILE_BYTES);
 boot_timing[6]=video_clock;
 wf_r18_frontend_driver_config_t config={.ram=raw,.ram_bytes=WF_SATURN_LWRAM_FRONT_RAM_BYTES,
  .runtime=&rt,.video=&video,.ssp=0x1c3bd4u,.usp=0x1c3fd4u,.sr=0x2700u,
  .platform={.read_rom=wf_r18_rom_read,.read_bus=frontend_bus_read,.store_bus=frontend_bus_store,
   .wait_level=frontend_wait_level,.wait_vblank=frontend_wait_vblank,.commit=frontend_commit,
   .bind_world=frontend_world_bind,.submit_records=frontend_submit,
   .reset_ready=frontend_reset_ready,.scene_clear=frontend_scene_clear}};
 if(wf_r18_frontend_driver_init(&frontend,&config)!=WF_OK)return WF_BLOCKED;
 frontend_observed[7]=2;
 frontend_serviced_clock=video_clock;
 if(title_only){
  /* Run the cabinet reset and initial palette setup, then enter the normal
   * title owner after its three legal plates. Portraits are rendered while
   * display is hidden and become visible together at $837E. */
  while(frontend.phase==WF_R18_DRIVER_RESET||
        (frontend.phase==WF_R18_DRIVER_BOOT&&frontend.scene.boot.pc!=0x760u)){
   int step=wf_r18_frontend_driver_step(&frontend);
   if(step==WF_R18_DRIVER_BLOCKED||reset_to_title_requested)return WF_BLOCKED;
  }
  if(frontend.phase!=WF_R18_DRIVER_BOOT||frontend.scene.boot.pc!=0x760u)
   return WF_BLOCKED;
  frontend.scene.boot.pc=0x830u;
 }else{
  /* The legal plates use only VDP2. Show the first complete plate before
   * fetching the title/roster atlas, then resume the original hold counter.
   * This moves CD latency off the black screen after the SEGA logo. */
  while(frontend.phase==WF_R18_DRIVER_RESET||
        (frontend.phase==WF_R18_DRIVER_BOOT&&
         !(frontend.scene.boot.pc==0x1e7cu&&frontend.scene.boot.hold_return==0x78au))){
   int step=wf_r18_frontend_driver_step(&frontend);
   if(step==WF_R18_DRIVER_BLOCKED||reset_to_title_requested)return WF_BLOCKED;
  }
  if(frontend.phase!=WF_R18_DRIVER_BOOT||frontend.scene.boot.pc!=0x1e7cu||
     frontend.scene.boot.hold_return!=0x78au)return WF_BLOCKED;
  legal_first_visible=video_clock;
 }
 frontend_boot_scsp();
 frontend_settle();vdp1_sync_wait();vdp2_sync_wait();
 if(wf_r18_hot_sprites_load()!=WF_OK)return WF_BLOCKED;
 /* The display held a static legal plate during the synchronous CD read.
  * Its 128-frame hold starts now rather than fast-forwarding past the read. */
 frontend_serviced_clock=video_clock;
 boot_timing[7]=video_clock;
 wp1980_frontend_input_begin(video_clock);
 return wf_r18_frontend_driver_run(&frontend);
}
static wf_status_t frontend_support_rom(void*u,uint32_t at,uint8_t*p,size_t n){
 (void)u;return wf_r18_rom_read(NULL,at,p,n);
}
static wf_status_t frontend_support_palette(void*u,uint32_t at,uint16_t word){
 (void)u;wf_r17_fg0_write_t w={at,word,2};return wf_r18_video_batch(&rt,&w,1,&video);
}
static wf_status_t frontend_extension(void*u,uint32_t owner,wf_r18_prematch_t*m){
 (void)u;
 if(owner==0x285dau){
  wf_r18_frontend_world_t w={.ram=m->ram,.ram_bytes=0x4000u,.regs=m->regs,.a=m->a,.read_rom=wf_r18_rom_read};
  wf_status_t st=wf_r18_frontend_world_call(&w,owner);if(st!=WF_OK)m->dependency=w.blocked_dependency;return st;
 }
 wf_r18_match_support_t s={.machine=m,.fg_0c0000=video.fg0,.fg_bytes=sizeof video.fg0,
  .read_rom=frontend_support_rom,.write_palette=frontend_support_palette};
 wf_status_t st=wf_r18_match_support_call(&s,owner);memset(video.hud_dirty,0xff,sizeof video.hud_dirty);return st;
}
static wf_status_t frontend_to_gameplay(void){
 frontend_observed[7]=3;
 for(;;){
  if(frontend.phase==WF_R18_DRIVER_BOOT||frontend.phase==WF_R18_DRIVER_MODE_PLATE){
   frontend_observed[7]=2;
   if(wf_r18_frontend_driver_run(&frontend)!=WF_OK)return WF_BLOCKED;
   frontend_observed[7]=3;
  }
  uint32_t generation=frontend.generation;
  if(wf_r18_match_driver_init(&frontend_match,&frontend,frontend_extension,NULL)!=WF_OK)return WF_BLOCKED;
  wf_r18_prematch_speech_divisor_set(1u);
  wf_status_t st=wf_r18_match_driver_run(&frontend_match);
  if(generation!=frontend.generation&&frontend.phase==WF_R18_DRIVER_MODE_PLATE){
   if(wf_r18_frontend_driver_run(&frontend)!=WF_OK)return WF_BLOCKED;
   continue;
  }
  if(st!=WF_OK)return WF_BLOCKED;
  break;
 }
 frontend_settle();
 if(!(frontend.ram[0x161]&1u)||!fr_u16(frontend.ram+0x7c))return WF_BLOCKED;
 if(wf_r18_runtime_handoff_f8c(&rt,frontend.ram,frontend.ram_bytes,&frontend.regs)!=WF_OK)return WF_BLOCKED;
 /* The selected ring must already live in the independent scene range. */
 if(frontend_bound_world!=rt.g.global_007e||fr_u16((const uint8_t*)RING_BLOB+6)!=rt.g.global_007e)return WF_BLOCKED;
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
  const uint8_t*p=frontend.ram+0x16d6u+side*128u+i*8u;
  world.cache[side][i]=(wf_r18_world_transport_t){fr_u32(p),fr_u32(p+4)};
 }
 world.animation_count[0]=world.animation_count[1]=0;
 frontend_observed[7]=4;
 wp1980_frontend_input_disable();
 boot.priority_140010=(uint8_t)fr_u16(frontend_control_ports+0x10);
 video.read_rom=wf_r18_rom_read;video.read_ram=read_ram;video.read_user=NULL;
 return WF_OK;
}
static wf_status_t frontend_combat_cache(void){
 uint32_t bytes;
 const char*atlas=(rt.g.mode_0161&1u)?"ROYSPR.BIN":"HOTSPR.BIN";
 wf_r18_present_init(frontend_combat_file_read,ROM_CACHE,WF_SATURN_LWRAM_ROM_BYTES,TILE_CACHE,WF_SATURN_LWRAM_TILE_BYTES);
 if(wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof wf_r18_rom_hot_pages[0])!=WF_OK)return WF_BLOCKED;
 if((rt.g.mode_0161&1u)&&
    wf_r18_rom_extra_pages_preload(wf_r18_rom_royal_extra_pages,
      sizeof wf_r18_rom_royal_extra_pages/sizeof wf_r18_rom_royal_extra_pages[0])!=WF_OK)
  return WF_BLOCKED;
 if(!(rt.g.mode_0161&1u)&&
    wf_r18_rom_extra_pages_preload(wf_r18_rom_tag_extra_pages,
      sizeof wf_r18_rom_tag_extra_pages/sizeof wf_r18_rom_tag_extra_pages[0])!=WF_OK)
  return WF_BLOCKED;
 if(wf_r18_file_size(atlas,&bytes)||wf_r18_hot_sprites_load()!=WF_OK||bytes!=wf_r18_hot_sprites_bytes())return WF_BLOCKED;
 wf_r18_cdfs_combat_window_set(1u);
 return WF_OK;
}
