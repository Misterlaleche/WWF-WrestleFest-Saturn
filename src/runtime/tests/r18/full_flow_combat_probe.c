/* Test-only composition. Frontend fixture owns the immutable controller tape;
 * gameplay uses the same callbacks as direct_combat_probe, with no direct boot. */
#define main full_flow_frontend_fixture_main
#include "frontend_driver_host.c"
#undef main
#include <stdlib.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_match_driver.h"
#include "wf_r18_match_support.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_r18_frontend_world.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_result_screen.h"
#include "wf_r18_present.h"
#include "wf_r18_rom_hot_pages.h"
#include "wf_r18_ring.h"
#include "wf_r18_display.h"
#include "wf_r18_hud.h"
#include "wf_r18_transition.h"

static wf_r18_match_driver_t combat_paid;
static wf_gameplay_binding_context_t combat_gc;
static wf_phase_binding_t combat_bindings[48];
static wf_r18_world_ctx_t combat_world;
static wf_r18_display_t combat_display;
static wf_r18_transition_context_t combat_transition;
static uint8_t combat_ring[WF_SATURN_LWRAM_SCENE_BYTES];
static uint8_t combat_rom_cache[WF_SATURN_LWRAM_ROM_BYTES];
static uint8_t combat_tile_cache[WF_SATURN_LWRAM_TILE_BYTES];
static FILE *combat_sprites,*combat_hot;
static uint8_t combat_priority;
static uint32_t combat_waits;
static const char *combat_output;
static int combat_capture_f4c2_frame=-1;
static int combat_captured_f4c2;

static void combat_dump(const char *name,const void *p,size_t n){
 char path[4096];int len=snprintf(path,sizeof path,"%s/%s",combat_output,name);
 assert(len>0&&(size_t)len<sizeof path);FILE*f=fopen(path,"wb");assert(f);
 assert(fwrite(p,1,n,f)==n);assert(fclose(f)==0);
}
static wf_status_t combat_capture_f4c2(wf_runtime_t*r,uint32_t at,void*u){
 if(!combat_captured_f4c2&&combat_capture_f4c2_frame>=0&&
    r->g.frame_counter==(uint32_t)combat_capture_f4c2_frame){
  uint8_t snapshot[24u+32u+0x4000u+0x4000u]={0};
  uint32_t header[6]={r->g.frame_counter,0,0,0x0000f4c2u,0,1};
  memcpy(snapshot,header,sizeof header);
  memcpy(snapshot+24u,r->m68k_regs.d,32u);
  assert(wf_r18_runtime_export(r,snapshot+56u,0x4000u)==WF_OK);
  memcpy(snapshot+56u+0x4000u,h.video.fg0,0x4000u);
  combat_dump("pre-f4c2-snapshot.bin",snapshot,sizeof snapshot);
  combat_captured_f4c2=1;
 }
 return wf_phase_wrestlers_f4c2(r,at,u);
}
static int combat_file(const char *name,uint32_t at,void *out,size_t n){
 if(!strcmp(name,"ROM68K.BIN"))return rom(&h,at,out,n)==WF_OK?0:-1;
 FILE*f=!strcmp(name,"SPRITES.PAK")?combat_sprites:!strcmp(name,"HOTSPR.BIN")?combat_hot:NULL;
 return !f||fseek(f,(long)at,SEEK_SET)||fread(out,1,n,f)!=n?-1:0;
}
static int combat_scene_size(const char *name,uint32_t *out){
 char path[256];snprintf(path,sizeof path,"audit/runtime18-cd/%s",name);
 FILE*f=fopen(path,"rb");if(!f)return -1;
 if(fseek(f,0,SEEK_END)){fclose(f);return -1;}
 long n=ftell(f);fclose(f);if(n<0)return -1;*out=(uint32_t)n;return 0;
}
static int combat_scene_read(const char *name,uint32_t at,void *out,size_t n){
 char path[256];snprintf(path,sizeof path,"audit/runtime18-cd/%s",name);
 FILE*f=fopen(path,"rb");if(!f)return -1;
 int failed=fseek(f,(long)at,SEEK_SET)||fread(out,1,n,f)!=n;
 fclose(f);return failed?-1:0;
}
static wf_status_t combat_readrom(void*u,uint32_t at,void*out,size_t n){
 if(at>sizeof h.rom||n>sizeof h.rom-at)return WF_BLOCKED;
 return wf_r18_rom_read(u,at,out,n);
}
static wf_status_t combat_readram(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 (void)u;
 switch(at){
 case 0x1c0062:case 0x1c0063:*out=r->g.global_0063;return WF_OK;
 case 0x1c004e:*out=(uint16_t)((r->g.credit_remainder_004e<<8)|r->g.credit_count_004f);return WF_OK;
 case 0x1c004a:*out=r->g.credit_cycle_units_004b;return WF_OK;
 default:fprintf(stderr,"RAM dependency=%06x\n",at);return WF_BLOCKED;
 }
}
static wf_status_t combat_audio(wf_runtime_t*r,uint16_t cmd,void*u){
 (void)r;(void)cmd;(void)u;return WF_OK; /* Explicit silent output device. */
}
static wf_status_t combat_wait(wf_runtime_t*r,uint16_t raw,void*u){
 (void)r;(void)u;combat_waits+=(uint32_t)raw+1u;return WF_OK;
}
static wf_status_t combat_result_wait(wf_runtime_t*r,uint16_t n,void*u){
 return n?combat_wait(r,(uint16_t)(n-1u),u):WF_BAD_STATE;
}
static wf_status_t combat_result_commit(void*u){return wf_r18_hud_commit(u);}
static wf_status_t combat_result_level(wf_runtime_t*r,unsigned high,void*u){
 (void)r;(void)high;(void)u;return WF_OK;
}
static wf_status_t combat_input(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 (void)r;(void)u;at&=0x00ffffffu;
 if(!out||at<0x140020||at>0x140026||(at&1u))return WF_BAD_STATE;
 *out=at==0x140020?(uint16_t)(0xff00u|combat_gc.input_hw_low[0]):0xffffu;return WF_OK;
}
static wf_status_t combat_world288(wf_runtime_t*r,uint8_t a[4],uint8_t b,void*u){
 (void)u;return wf_r18_world_288f0(r,a,b,&combat_world);
}
static wf_status_t combat_world26(wf_runtime_t*r,uint8_t s,uint16_t d,void*u){
 (void)u;return wf_r18_world_26b68(r,s,d,&combat_world);
}
static wf_status_t combat_render(wf_runtime_t*r,wf_actor_raw_t *const*a,size_t n,void*u){
 (void)u;
 if((uint16_t)combat_gc.camera.scroll0_x!=r->g.global_17e6||
    (uint16_t)combat_gc.camera.scroll1_x!=r->g.global_17ea||
    (uint16_t)combat_gc.camera.scroll0_y!=r->g.global_17ee||
    (uint16_t)combat_gc.camera.scroll1_y!=r->g.global_17f2_word)return WF_BAD_STATE;
 wf_status_t st=wf_r18_present_batch(r,a,n,&h.video);
 if(r->g.frame_counter%1000u==0u)printf("RENDER frame=%u actors=%zu commands=%u status=%u rng=%08x\n",r->g.frame_counter,n,wf_r18_present_commands(),st,r->g.rng_state);
 return st;
}
static wf_status_t combat_paid_rom(void*u,uint32_t at,uint8_t*p,size_t n){return rom(u,at,p,n);}
static wf_status_t combat_palette(void*u,uint32_t at,uint16_t value){
 (void)u;wf_r17_fg0_write_t w={at,value,2};return wf_r18_video_batch(&h.runtime,&w,1,&h.video);
}
static wf_status_t combat_extension(void*u,uint32_t owner,wf_r18_prematch_t*m){
 (void)u;
 if(owner==0x285da){
  wf_r18_frontend_world_t w={.ram=m->ram,.ram_bytes=sizeof h.ram,.regs=m->regs,.a=m->a,.read_rom=rom,.user=&h};
  wf_status_t st=wf_r18_frontend_world_call(&w,owner);if(st!=WF_OK)m->dependency=w.blocked_dependency;return st;
 }
 if(owner==0x98ba||owner==0x8eb6){
  wf_r18_match_support_t c={.machine=m,.fg_0c0000=h.video.fg0,.fg_bytes=sizeof h.video.fg0,.read_rom=combat_paid_rom,.user=&h,.write_palette=combat_palette};
  return wf_r18_match_support_call(&c,owner);
 }
 m->dependency=owner;return WF_BLOCKED;
}
static void combat_frontend(unsigned royal){
 /* cold() supplies the documented power-on fixture once. Every subsequent
  * seed/selection write belongs to the original frontend owner. */
 cold(royal);h.manual_roster=1;
 size_t n=wf_gameplay_bindings_build(&combat_gc,combat_bindings,48);
 assert(n);wf_runtime_init(&h.runtime,combat_bindings,n);
 const char *capture_f4c2=getenv("WF_CAPTURE_PRE_F4C2_FRAME");
 if(capture_f4c2){
  char *end=0;long frame=strtol(capture_f4c2,&end,10);
  assert(end&&*end=='\0'&&frame>=0&&frame<20000);
  combat_capture_f4c2_frame=(int)frame;
  assert(combat_bindings[10].rom_addr==0x00f4c2u);
  combat_bindings[10].fn=combat_capture_f4c2;
 }
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_TITLE;i++){assert(i<100000);step();}
 for(unsigned i=0;h.driver.scene.title.pc<0x837e;i++){assert(i<100000);step();}
 tick(0,12);tick(1,3);tick(0,1);assert(h.ram[0x4f]==1);
 h.start=1;
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_MODE_PLATE;i++){assert(i<100000);step();}
 h.start=0;assert(h.driver.credited_start&&!h.ram[0x4f]);
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_AC0;i++){assert(i<200000);step();}
 assert((h.ram[0x161]&1u)==royal&&h.roster_max_timer<1024u);
 assert(wf_r18_match_driver_init(&combat_paid,&h.driver,combat_extension,NULL)==WF_OK);
 wf_status_t st=wf_r18_match_driver_run(&combat_paid);
 printf("PAID status=%u pc=%06x dependency=%06x flow=%06x frames=%u mode=%u picks=%u,%u,%u,%u seed=%08x\n",st,combat_paid.blocked_pc,combat_paid.blocked_dependency,combat_paid.flow.pc,h.frames,h.ram[0x161],h.ram[0x607],h.ram[0x713],h.ram[0x81f],h.ram[0x92b],read_be(h.ram+0x5c,4));
 combat_dump("frontend-f8c-ram.bin",h.ram,sizeof h.ram);
 assert(st==WF_OK&&combat_paid.flow.pc==0xf8c);
 wf_phase_binding_t *before=h.runtime.bindings;size_t count=h.runtime.binding_count;
 assert(wf_r18_runtime_handoff_f8c(&h.runtime,h.ram,sizeof h.ram,&h.driver.regs)==WF_OK);
 const char *captured=getenv("WF_CAPTURED_HANDOFF_RAM");
 if(captured){
  /* Test-only exploration of a read-only native F8C capture. This is
   * explicitly distinct from the normal HOST frontend trajectory above. */
  const char *dpath=getenv("WF_CAPTURED_HANDOFF_DREGS");assert(dpath);
  FILE*f=fopen(captured,"rb");assert(f);assert(fread(h.ram,1,sizeof h.ram,f)==sizeof h.ram);assert(fgetc(f)==EOF);fclose(f);
  uint8_t bytes[32];f=fopen(dpath,"rb");assert(f);assert(fread(bytes,1,sizeof bytes,f)==sizeof bytes);assert(fgetc(f)==EOF);fclose(f);
  wf_r17_m68k_shadow_t registers={0};registers.valid=1;
  for(unsigned i=0;i<8;i++)registers.d[i]=read_be(bytes+i*4u,4);
  assert(read_be(h.ram+0x80,4)==0u);
  assert(wf_r18_runtime_import(&h.runtime,h.ram,sizeof h.ram,&registers)==WF_OK);
  memcpy(h.video.palette_used,h.ram+0x1600,sizeof h.video.palette_used);
  memcpy(h.video.palette_map,h.ram+0x1610,sizeof h.video.palette_map);
  h.runtime.frame_ready=h.runtime.vblank_ack=0;h.runtime.frame_stage=3;
  printf("CAPTURED NATIVE F8C exploration ram=%s dregs=%s; no RNG/actor/selection replacement beyond exact import\n",captured,dpath);
 }
 assert(h.runtime.frame_stage==3&&h.runtime.g.frame_counter==0);
 assert(before==h.runtime.bindings&&count==h.runtime.binding_count);
 combat_dump("handoff-fa4-runtime.bin",&h.runtime,sizeof h.runtime);
 printf("HANDOFF FA4 mode=%u stage=%u rng=%08x bindings=%zu actors=%u,%u,%u,%u\n",h.runtime.g.mode_0161,h.runtime.frame_stage,h.runtime.g.rng_state,count,wf_u8(&h.runtime.actors[0],0x57),wf_u8(&h.runtime.actors[1],0x57),wf_u8(&h.runtime.actors[4],0x57),wf_u8(&h.runtime.actors[5],0x57));
}
static void combat_setup(void){
 wf_r18_video_ring_bind(&h.video);h.video.read_rom=combat_readrom;h.video.read_ram=combat_readram;
 combat_gc.cabinet_words_valid=1;combat_gc.cabinet_word_140020=combat_gc.cabinet_word_140022=0xffff;
 combat_gc.camera_valid=1;combat_gc.camera.scene_index=0;
 combat_gc.camera.scroll0_x=h.runtime.g.global_17e6;combat_gc.camera.scroll1_x=h.runtime.g.global_17ea;
 combat_gc.camera.scroll0_y=h.runtime.g.global_17ee;combat_gc.camera.scroll1_y=h.runtime.g.global_17f2_word;
 combat_gc.input_valid_mask=0x1ff;memset(combat_gc.input_hw_low,0xff,sizeof combat_gc.input_hw_low);
 combat_gc.audio_command=combat_audio;
 combat_gc.referee_direction_20c8=wf_r18_referee_direction_20c8;
 combat_gc.referee_display_2067c=wf_r18_referee_display_2067c;
 combat_gc.referee_number_2067c=wf_r18_referee_number_2067c;
 combat_gc.referee_clear_206fe=wf_r18_referee_clear_206fe;
 combat_gc.referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;
 combat_gc.referee_result_20156=wf_r18_result_sound_20156;
 combat_gc.post7_wait_21e6=combat_wait;combat_gc.participant_input_word=combat_input;
 combat_gc.participant_commit_batch=wf_r18_video_commit;
 combat_gc.post7_fg0_batch=combat_gc.post7_video_batch=wf_r18_video_batch;
 combat_gc.post7_resource_2aea=wf_r18_palette_2aea;combat_gc.post7_resource_2b58=wf_r18_palette_2b58;
 combat_gc.post7_raw_2503c=wf_r18_text_2503c;combat_gc.post7_raw_9aca=wf_r18_strip_9aca;
 combat_gc.post7_fg0_peek_long=wf_r18_video_peek;combat_gc.post7_render_2836_batch=combat_render;
 combat_world.read_rom=combat_readrom;combat_gc.branchb_288f0=combat_world288;
 combat_gc.branchb_26b68=combat_world26;combat_gc.r17_owner_user=&h.video;
 assert(wf_r18_display_init(&combat_display,&h.runtime,combat_readrom,NULL)==WF_OK);
 combat_priority=h.board[0x10];
 combat_transition=(wf_r18_transition_context_t){.video=&h.video,.world=&combat_world,.display=&combat_display,.render_queue=&combat_gc.render_queue,.camera=&combat_gc.camera,.scene_blob=combat_ring,.scene_capacity=sizeof combat_ring,.file_size=combat_scene_size,.file_read=combat_scene_read,.priority_140010=&combat_priority};
 wf_r18_transition_bind(&combat_transition);combat_gc.transition_reset=wf_r18_transition_reset;combat_gc.transition_barrier=wf_r18_transition_barrier;
}
static uint8_t combat_pad(unsigned frame,const char *profile){
 if(!strcmp(profile,"idle"))return 0xff;
 if(!strcmp(profile,"a600"))return frame>=60&&frame<180?0xfe:frame>=180&&frame<184?0xef:0xff;
 /* Validation controller only: observes positions and sends normal pad bits.
  * It never writes actor, damage, credit, seed or result state. */
 if(!strcmp(profile,"pressure")||!strcmp(profile,"punch")||!strcmp(profile,"grapple")||!strcmp(profile,"rush")||!strcmp(profile,"pin")){
  wf_actor_raw_t*a=&h.runtime.actors[0],*t=wf_actor_from_rom_ptr(&h.runtime,wf_be32(a,WF_A_TARGET),NULL);
  if(!t||!(wf_u8(t,0)&0x80u))t=&h.runtime.actors[4];
  if(!(wf_u8(t,0)&0x80u))t=&h.runtime.actors[5];
  int rush=!strcmp(profile,"rush"),pin=!strcmp(profile,"pin");
  if(pin){
   wf_actor_raw_t*b=&h.runtime.actors[4],*c=&h.runtime.actors[5];
   if((wf_u8(b,0)&0x80u)&&(wf_u8(c,0)&0x80u))t=wf_be16(b,WF_A_STAMINA)<=wf_be16(c,WF_A_STAMINA)?b:c;
  }
  int dx=(int16_t)(wf_be16(t,WF_A_X)-wf_be16(a,WF_A_X));
  int dy=(int16_t)(wf_be16(t,WF_A_Y)-wf_be16(a,WF_A_Y));
  uint8_t bits=0xffu;
  int reach=rush||pin?8:20;
  if(dx>reach)bits&=(uint8_t)~1u;else if(dx< -reach)bits&=(uint8_t)~2u;
  if(dy>6)bits&=(uint8_t)~4u;else if(dy< -6)bits&=(uint8_t)~8u;
  /* Two sampled quanta on/off avoid aliasing the original paired $514E
   * accumulator. These remain ordinary physical button levels. */
  if(dx> -72&&dx<72&&dy> -24&&dy<24&&((rush||pin)?frame%4u<2u:frame%12u<4u)){
   if(rush)bits&=(uint8_t)~0x30u;
   else if(pin)bits&=(uint8_t)~((frame/4u)%3u==0u?0x30u:(frame/4u)%3u==1u?0x10u:0x20u);
   else if(!strcmp(profile,"punch"))bits&=(uint8_t)~0x10u;
   else if(!strcmp(profile,"grapple"))bits&=(uint8_t)~0x20u;
   else bits&=(uint8_t)~((frame/12u)%3u==0u?0x30u:(frame/12u)%3u==1u?0x10u:0x20u);
  }
  return bits;
 }
 /* Four held directions followed by distinct A/B/C press/release edges.
  * Gameplay bit2=DOWN,bit3=UP; frontend fixture uses its own ROM ordering. */
 unsigned t=frame%600u;
 if(t<60)return 0xff;
 if(t<150)return 0xfe;
 if(t<210)return 0xfd;
 if(t<240)return 0xfb;
 if(t<270)return 0xf7;
 if(t<510){unsigned k=(t-270)%60u;if(k<4)return 0xef;if(k>=20&&k<24)return 0xdf;if(k>=40&&k<44)return 0xbf;}
 return 0xff;
}
static void combat_block(unsigned frame,wf_status_t st){
 wf_runtime_t*r=&h.runtime;
 extern volatile uint32_t wf_collision_blocked_probe[12];
 printf("BLOCK frame=%u runtime_frame=%u status=%u phase=%06x dependency=%06x trace=%zu video=%06x transition=%06x stage=%u\n",frame,r->g.frame_counter,st,combat_gc.last_blocked_phase,combat_gc.last_blocked_dependency,r->trace_count,h.video.failed_address,combat_transition.blocked_pc,r->frame_stage);
 if(combat_gc.last_blocked_phase==0x24062u){
  printf("COLLISION PROBE");for(unsigned i=0;i<12;i++)printf(" %08x",wf_collision_blocked_probe[i]);puts("");
 }
 printf("TRACE");for(size_t j=0;j<r->trace_count;j++)printf(" %06x",r->trace[j]);puts("");
 printf("GLOBALS rng=%08x mode=%02x credited=%04x pipeline=%02x referee=%04x/%04x count=%04x timer=%04x\n",r->g.rng_state,r->g.mode_0161,r->g.credited_route,r->g.pipeline_mode,wf_be16(&r->referee,0x20),wf_be16(&r->referee,0x60),r->g.global_169a,r->g.global_16a0);
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)printf("actor%u state=%04x pose=%04x id=%02x hp=%u target=%08x team=%08x link=%08x pos=%u,%u flags=%04x ae=%04x selector=%04x cpu=%06x/%06x action=%06x/%06x\n",i,wf_be16(&r->actors[i],0x20),wf_be16(&r->actors[i],4),wf_u8(&r->actors[i],0x57),wf_be16(&r->actors[i],0x66),wf_be32(&r->actors[i],0x7a),wf_be32(&r->actors[i],0x86),wf_be32(&r->actors[i],0x26),wf_be16(&r->actors[i],6),wf_be16(&r->actors[i],10),wf_be16(&r->actors[i],0x32),wf_be16(&r->actors[i],0xae),wf_be16(&r->actors[i],0x60),combat_gc.cpu_trace[i].handler_rom,combat_gc.cpu_trace[i].blocked_dependency_rom,combat_gc.f4c2_actions.trace[i].callback_rom,combat_gc.f4c2_actions.trace[i].blocked_dependency_rom);
 combat_dump("blocked-runtime.bin",r,sizeof *r);combat_dump("blocked-actors.bin",r->actors,sizeof r->actors);
 combat_dump("blocked-globals.bin",&r->g,sizeof r->g);combat_dump("blocked-dregs.bin",r->m68k_regs.d,sizeof r->m68k_regs.d);
 combat_dump("blocked-fg.bin",h.video.fg0,sizeof h.video.fg0);
}
int main(int argc,char**argv){
 assert(argc==9);setvbuf(stdout,NULL,_IOLBF,0);combat_output=argv[8];
 unsigned frames=(unsigned)strtoul(argv[5],NULL,10),royal=(unsigned)strtoul(argv[7],NULL,10);
 const char *profile=argv[6];assert(frames&&royal<=1u);
 FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(h.rom,1,sizeof h.rom,f)==sizeof h.rom);fclose(f);
 combat_sprites=fopen(argv[2],"rb");combat_hot=fopen(argv[4],"rb");assert(combat_sprites&&combat_hot);
 f=fopen(argv[3],"rb");assert(f);assert(fread(combat_ring,1,114918u,f)==114918u);fclose(f);
 assert(!wf_r18_ring_bind(combat_ring,114918u));
 wf_r18_present_init(combat_file,combat_rom_cache,sizeof combat_rom_cache,combat_tile_cache,sizeof combat_tile_cache);
 assert(wf_r18_hot_sprites_load()==WF_OK);
 assert(wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof wf_r18_rom_hot_pages[0])==WF_OK);
 combat_frontend(royal);combat_setup();printf("INPUT profile=%s frames=%u no post-selection seed reset\n",profile,frames);
 const int trace_ref_link=getenv("WF_TRACE_REF_LINK")!=NULL;
 uint32_t prior_links[WF_ACTOR_SLOTS]={0};
 uint16_t prior_ref_state=0xffffu;
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)prior_links[i]=wf_be32(&h.runtime.actors[i],WF_A_LINK);
 for(unsigned frame=0;frame<frames;frame++){
  combat_gc.input_hw_low[0]=combat_pad(frame,profile);
  wf_status_t st=wf_runtime_frame(&h.runtime);
  if(st==WF_BLOCKED&&h.runtime.frame_ready){
   st=wf_r18_display_irq(&h.runtime,&combat_display);
   if(st==WF_OK)st=wf_r18_world_commit(&h.runtime,&combat_world);
   if(st==WF_OK)st=wf_r18_hud_irq(&h.runtime,&h.video);
   if(st==WF_OK){wf_runtime_vblank_ack(&h.runtime);st=wf_runtime_frame(&h.runtime);}
  }
  if(trace_ref_link){
   uint16_t ref_state=wf_be16(&h.runtime.referee,0x20);
   if(ref_state!=prior_ref_state){
    printf("REF-LINK frame=%u referee=%04x latch=%04x visual=%04x ptr=%08x\n",frame,ref_state,wf_be16(&h.runtime.referee,0x1c),wf_be16(&h.runtime.referee,0x04),wf_be32(&h.runtime.referee,0x56));
    prior_ref_state=ref_state;
   }
   for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
    uint32_t link=wf_be32(&h.runtime.actors[i],WF_A_LINK);
    if(link!=prior_links[i]){
     printf("ACTOR-LINK frame=%u slot=%u from=%08x to=%08x state=%04x selector=%04x pin=%02x\n",frame,i,prior_links[i],link,wf_be16(&h.runtime.actors[i],WF_A_STATE),wf_be16(&h.runtime.actors[i],WF_A_SELECTOR_LO),wf_u8(&h.runtime.actors[i],WF_A_PINFLAGS));
     prior_links[i]=link;
    }
   }
  }
  if(st==WF_BLOCKED&&wf_r18_tag_match_complete(&h.runtime)){
   combat_dump("match-complete-runtime.bin",&h.runtime,sizeof h.runtime);
   printf("TAG RESULT actor0=%04x actor2=%04x\n",wf_be16(&h.runtime.actors[0],WF_A_RESULT),wf_be16(&h.runtime.actors[2],WF_A_RESULT));
   uint32_t before=combat_waits;
   assert(wf_r18_result_in_ring(&h.runtime,&h.video,combat_result_wait,combat_result_commit,&h.video)==WF_OK);
   assert(combat_waits-before==131u);
   printf("MATCH COMPLETE frame=%u endpoint=0011B6 result-waits=%u\n",h.runtime.g.frame_counter,combat_waits-before);return 0;
  }
  if(st==WF_BLOCKED&&wf_r18_royal_game_over(&h.runtime)){
   combat_dump("match-complete-runtime.bin",&h.runtime,sizeof h.runtime);
   uint32_t before=combat_waits;
   assert(wf_r18_result_royal_game_over(&h.runtime,&h.video,combat_result_level,combat_result_wait,combat_result_commit,&h.video)==WF_OK);
   assert(combat_waits-before==97u);
   printf("MATCH COMPLETE frame=%u endpoint=001AFC presentation-end=001B1E result-waits=%u\n",h.runtime.g.frame_counter,combat_waits-before);return 0;
  }
  if(st!=WF_OK){combat_block(frame,st);return 1;}
 }
 printf("PASS full-flow manual %s profile=%s frames=%u runtime_frame=%u rng=%08x\n",royal?"Royal":"TAG",profile,frames,h.runtime.g.frame_counter,h.runtime.g.rng_state);
 combat_dump("final-runtime.bin",&h.runtime,sizeof h.runtime);return 0;
}
