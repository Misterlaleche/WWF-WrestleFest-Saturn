#include "wf_platform.h"
#include "wf_live_video.h"
#include "wf_transport.h"
#include "wf_combat_saturn.h"
#include "wf_r18_present.h"
#include "wf_workspace.h"
#include "wf_failure_screen.h"
#ifdef WF_ALPHA_RETURN
#include "wf_frontend_resume.h"
#include "wf_postmatch.h"
#include "wf_scene_checkpoint.h"
static bool returning;
#endif
#ifdef WF_ALPHA_SAVE
#include "wf_frontend_save.h"
volatile uint32_t wf_save_status;
#endif
#ifdef WF_COMBAT_OVERLAY
#include "wf_combat_loader.h"
#endif
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
#include "r05/wf_fixed_attract.h"
#include "r05/wf_fixed_attract_state.h"
#include "wf_front_resource_pack.h"
static bool fixed_owned, fixed_paused;
static wf_arcade_clock_t fixed_clock;
static uint8_t fixed_initial_credit;
volatile uint32_t wf_attract_irq2,wf_attract_irq3,wf_attract_runs,wf_attract_ticks;
#endif
static bool combat_started;
#include <string.h>
static wf_session_t session;
static wf_board_t board;
static uint8_t cold[0x60];
#define latched_sprites (wf_workspace.front.latched_sprites)
static unsigned sprite_count,latched_count;

static smpc_peripheral_digital_t pads[2];
volatile uint32_t wf_session_phase,wf_session_pc,wf_session_owner,wf_session_generation,wf_credits,wf_paid,wf_state,wf_slot,wf_partner,wf_royal,wf_scroll,wf_inputs,wf_irq2_count,wf_irq3_count,wf_board_line,wf_snapshot_sequence,wf_cold_rng;
static unsigned word(const uint8_t*p){return (p[0]<<8)|p[1];}
static void publish(void){
 ++wf_snapshot_sequence;wf_session_phase=session.phase;wf_state=session.frontend.phase;wf_session_pc=session.frontend.pc;wf_session_owner=session.error_owner;wf_session_generation=session.frontend.generation;wf_credits=session.ram[0x4f];wf_paid=word(session.ram+0x7c);wf_slot=word(session.ram+0x5d2);wf_partner=word(session.ram+0x6de);wf_royal=session.ram[0x161]&1;wf_scroll=word(board.scroll);wf_board_line=board.line;++wf_snapshot_sequence;
}
void wf_failure_context_read(wf_failure_context_t *out){
 out->session_phase=session.phase;out->phase=session.frontend.phase;
 out->pc=session.phase==WF_SESSION_FATAL?session.error_pc:session.frontend.pc;
 out->owner=session.error_owner?session.error_owner:session.frontend.active_owner;
 if(!out->owner)out->owner=session.frontend.blocked_dependency;
 out->address=session.frontend.blocked_address;
}
void user_init(void){
#ifdef WF_COMBAT_OVERLAY
#ifdef WF_ALPHA_RETURN
 returning=WF_COMBAT_MAILBOX[0]==5;
 if(!returning)
#endif
 WF_COMBAT_MAILBOX[0]=0;
#endif
 /* Capture actual LWRAM power-on bytes before CD resources occupy it.
  * RESET654 clears$60 onward. Never seed RNG/credits from a test fixture. */
#ifdef WF_ALPHA_RETURN
 if(!returning){
#endif
 for(unsigned i=0;i<0x60;i++)cold[i]=*(volatile uint8_t*)(0x00200000u+i);
 wf_cold_rng=((uint32_t)cold[0x5c]<<24)|((uint32_t)cold[0x5d]<<16)|((uint32_t)cold[0x5e]<<8)|cold[0x5f];
#ifdef WF_ALPHA_RETURN
 }
#endif
 wf_platform_init();
}
static wf_status_t rom(void*u __unused,uint32_t at,void*out,size_t n){if(combat_started)return wf_r18_rom_read(NULL,at,out,n);if(at>0x80000||n>0x80000-at)return WF_BLOCKED;const uint8_t*p=(const uint8_t*)(0x00200000u+at);uint8_t*q=out;
 if(n<=4){for(unsigned i=0;i<n;i++)q[i]=p[i];}else memcpy(out,p,n);return WF_OK;}
static wf_status_t read_bus(void*u __unused,uint32_t at,uint32_t*out,unsigned n){return wf_board_read(&board,at,out,n)?WF_OK:WF_BLOCKED;}
static wf_status_t write_bus(void*u __unused,uint32_t at,uint32_t v,unsigned n){return wf_board_write(&board,at,v,n)?WF_OK:WF_BLOCKED;}
static void poll(void){
 smpc_peripheral_process();
 for(unsigned i=0;i<2;i++){
  smpc_peripheral_digital_port(i+1,pads+i);unsigned p=0;
  if(pads[i].pressed.button.right)p|=1;if(pads[i].pressed.button.left)p|=2;
  if(pads[i].pressed.button.up)p|=4;if(pads[i].pressed.button.down)p|=8;
  if(pads[i].pressed.button.a)p|=0x10;if(pads[i].pressed.button.b)p|=0x20;
  if(pads[i].pressed.button.start)p|=0x80;
  wf_board_input(&board,i,(uint8_t)p,pads[i].pressed.button.c);if(!i)wf_inputs=p|(pads[i].pressed.button.c?256:0);
 }
}
static bool irq(void*u,unsigned level){
 wf_r18_frontend_driver_t*d=u;uint32_t generation=d->generation;
 wf_status_t st;
 if(level==3){wf_irq3_count++;st=wf_r18_frontend_driver_on_vblank(d);}else{wf_irq2_count++;st=wf_r18_frontend_driver_on_raster(d);}
 /* A native START can arrive while stopped at the unplayed attract AC0.
  * Carry its new frontend generation; never keep the stale MATCH_READY. */
 if(generation!=d->generation&&d->phase!=WF_R18_DRIVER_FATAL)session.phase=WF_SESSION_FRONTEND;
 publish();return st==WF_OK;
}
static wf_status_t event(wf_r18_frontend_driver_t*d){
 if(board.line>=256){if(!wf_platform_presentation_tick_wait())return WF_BLOCKED;poll();}
 unsigned line=wf_board_advance(&board);
 if(line==248){memcpy(latched_sprites,session.video.fg0+0x2000,sizeof latched_sprites);latched_count=sprite_count;}
 if(!wf_board_dispatch(&board,d->control.sr,irq,d))return WF_BLOCKED;
 publish();return WF_OK;
}
static wf_status_t wait_level(void*u __unused,wf_r18_frontend_driver_t*d,unsigned high){
 uint32_t gen=d->generation;
 while(wf_board_high(&board)!=!!high){if(event(d)!=WF_OK)return WF_BLOCKED;if(gen!=d->generation)break;}return WF_OK;
}
static wf_status_t wait_vblank(void*u __unused,wf_r18_frontend_driver_t*d){
 uint32_t count=wf_irq3_count,gen=d->generation;
 do{if(event(d)!=WF_OK)return WF_BLOCKED;}while(wf_irq3_count==count&&gen==d->generation);return WF_OK;
}
static wf_status_t commit(void*u __unused,wf_r18_frontend_driver_t*d){
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 if(fixed_owned)return WF_OK;
#endif
if(d->world_visible)wf_live_video_reveal();return wf_live_video_commit(&session,&board,latched_sprites,latched_count)?WF_OK:WF_BLOCKED;}
static wf_status_t world(void*u __unused,unsigned mode){
 unsigned atlas=0;
#ifdef WF_ALPHA_RETURN
 if(session.phase==WF_SESSION_POSTMATCH&&session.frontend.pc==0x1ba6u)
  return mode<7&&!wf_video_error&&wf_live_video_finale_prepare(&session)?WF_OK:WF_BLOCKED;
 if(session.phase==WF_SESSION_POSTMATCH||
    (session.phase==WF_SESSION_MATCH_READY&&session.frontend.pc==0xae5eu))atlas=3;
 else
#endif
 if(session.frontend.phase==WF_R18_DRIVER_AC0&&session.frontend.pc!=0x84be)atlas=mode==4?1:2;
 return mode<7&&!wf_video_error&&wf_live_video_atlas(atlas)?WF_OK:WF_BLOCKED;
}
static wf_status_t submit(void*u __unused,const wf_r18_sprite_record_t*r,size_t n){if((const uint8_t*)r!=session.video.fg0+0x2000||n>512)return WF_BLOCKED;sprite_count=n;return WF_OK;}
static wf_status_t clear(void*u __unused,uint32_t owner,unsigned complete){
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 if(fixed_owned){if(!complete&&(owner==0x1fc0||owner==0x1fde))latched_count=0;return WF_OK;}
#endif
if(!complete){wf_live_video_hide();if(owner==0x1fc0||owner==0x1fde)latched_count=0;}else wf_live_video_reveal();return WF_OK;}
static wf_status_t transport(wf_r18_frontend_driver_t*d __unused,uint32_t owner,unsigned begin,wf_status_t result){
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 if(fixed_owned)return result;
#endif
 if(begin)return wf_platform_load(true)?WF_OK:WF_BLOCKED;
 if(result==WF_OK&&owner==0x26e66&&!wf_live_video_prepare(&session))result=WF_BLOCKED;
 bool ok=wf_platform_load(false);return ok?result:WF_BLOCKED;
}
static wf_status_t reset_ready(void*u __unused,uint8_t*r __unused,size_t n __unused){
 wf_platform_scene_begin(0);
#ifdef WF_ALPHA_SAVE
 wf_save_status=wf_frontend_save_load(r,n);
#endif
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 if(fixed_owned&&!wf_error){fixed_paused=wf_platform_attract_pause(true);if(!fixed_paused)return WF_BLOCKED;}
#endif
 return wf_error?WF_BLOCKED:WF_OK;
}
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
static int fixed_field(void*u __unused){
 poll();
 if(!wf_arcade_clock_field(&fixed_clock))return -1;
 while(wf_arcade_clock_take(&fixed_clock)){
  uint32_t r2=0,r3=0;
  if(!wf_fixed_attract_raster_frame(&session,&board,&r2,&r3))return -1;
  wf_attract_irq2+=r2;wf_attract_irq3+=r3;wf_irq2_count+=r2;wf_irq3_count+=r3;++wf_attract_ticks;
 }
 publish();return session.ram[0x4f]>fixed_initial_credit?1:0;
}
static bool fixed_enter(bool cold_start){
 if(!cold_start){
  wf_live_video_finish();fixed_owned=true;
  fixed_paused=wf_platform_attract_pause(true);if(!fixed_paused)return false;
  wf_fixed_attract_video_begin(false);
 }
 /* Full cold RESET654, or exactly the original postmatch/unpaid BOOT6FC
  * cleanup. No original plate/renderer is allowed to touch this VDP lease. */
 for(unsigned steps=0;session.frontend.phase!=WF_R18_DRIVER_BOOT||session.frontend.pc!=0x724u;steps++){
  if(steps>=64||session.phase!=WF_SESSION_FRONTEND||wf_session_step(&session)!=WF_OK)return false;
 }
 if(!fixed_paused)return false;
 fixed_initial_credit=session.ram[0x4f];++wf_attract_runs;
 if(!wf_arcade_clock_init(&fixed_clock,0))return false;
 if(!wf_fixed_attract_run(fixed_initial_credit!=0,fixed_field,NULL))return false;
 /* R05 has drained/released CD, DMA and sprite IRQ. The union can now be
  * reused, but every atlas/map/palette/cache residency must be re-established. */
 sprite_count=latched_count=0;
 if(!wf_live_video_restore_attract(&session))return false;
 if(wf_fixed_attract_title_enter(&session)!=WF_OK)return false;
 fixed_owned=false;
 if(!wf_platform_attract_pause(false))return false;fixed_paused=false;
 publish();return true;
}
#endif
int main(void){
 wf_board_init(&board,0xef,0xff);
 wf_r18_frontend_platform_t platform={.read_rom=rom,.read_bus=read_bus,.store_bus=write_bus,.wait_level=wait_level,.wait_vblank=wait_vblank,.commit=commit,.bind_world=world,.submit_records=submit,.scene_clear=clear,.reset_ready=reset_ready};
#ifdef WF_ALPHA_RETURN
 if(returning){
  /* Consume the LWRAM capsule before PROGRAM/BACK overwrite that area. */
  wf_pacer_t clock;
  if(wf_error||!wf_combat_handoff_restore_return(WF_COMBAT_CAPSULE,&session,&board,&clock)||
     !wf_platform_clock_restore(&clock,WF_COMBAT_CAPSULE->fields,WF_COMBAT_CAPSULE->clock_origin)){
   wf_platform_fail(42);goto failed;
  }
  wf_cold_rng=WF_COMBAT_CAPSULE->cold_rng;
  if(wf_scene_checkpoint_restore(&session.runtime,session.ram,sizeof session.ram,&session.video)!=WF_OK){wf_platform_fail(44);goto failed;}
  wf_r18_frontend_driver_config_t resume={.ram=session.ram,.ram_bytes=sizeof session.ram,
   .runtime=&session.runtime,.video=&session.video,.platform=platform,
   .ssp=session.frontend.control.ssp,.usp=session.frontend.control.usp,.sr=session.frontend.control.sr};
  if(wf_frontend_resume_bind(&session.frontend,&resume)!=WF_OK||wf_session_bind_match(&session)!=WF_OK){wf_platform_fail(43);goto failed;}
  sprite_count=latched_count=word(session.ram+0x19b2);
  if(sprite_count>512){wf_platform_fail(44);goto failed;}
  memcpy(latched_sprites,session.video.fg0+0x2000,sizeof latched_sprites);
  if(!wf_live_video_init_return()||!wf_live_video_return_prepare(&session,&board)){
   wf_platform_fail(45);goto failed;
  }
#ifdef WF_ALPHA_SAVE
  wf_save_status=wf_frontend_save_rebind(session.ram,sizeof session.ram);
#endif
  if(!wf_platform_load(false)){wf_platform_fail(45);goto failed;}
  WF_COMBAT_MAILBOX[0]=6;
 }else
#endif
 {
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
  fixed_owned=true;wf_fixed_attract_video_begin(true);
  if(wf_error||!wf_live_video_program_load()){wf_platform_fail(30);goto failed;}
#else
  if(wf_error||!wf_live_video_init()){wf_platform_fail(30);goto failed;}
#endif
  uint8_t*cold_image=wf_workspace.front.stage;memset(cold_image,0,0x4000);memcpy(cold_image,cold,sizeof cold);
  wf_session_config_t config={.cold_ram=cold_image,.cold_ram_bytes=0x4000,.ssp=0x1c3bd4,.usp=0x1c3fd4,.sr=0x2700,.platform=platform};
  if(wf_session_init(&session,&config)!=WF_OK){wf_platform_fail(31);goto failed;}
 }
 wf_transport_bind(transport);
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 if(fixed_owned&&!fixed_enter(true)){wf_platform_fail(47);goto failed;}
#endif
 for(;;){
#ifdef WF_ALPHA_RETURN
  if(session.phase==WF_SESSION_POSTMATCH){
   if(wf_postmatch_run(&session)!=WF_OK){wf_platform_fail(46);break;}
#ifdef WF_ALPHA_SAVE
   wf_save_status=wf_frontend_save_store(session.ram,sizeof session.ram);
   if(wf_error)break;
#endif
   WF_COMBAT_MAILBOX[0]=0;publish();
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
   if(session.phase==WF_SESSION_FRONTEND&&session.frontend.phase==WF_R18_DRIVER_BOOT&&session.frontend.pc==0x6fcu){
    if(!fixed_enter(false)){wf_platform_fail(47);break;}
   }
#endif
  }else
#endif
  if(session.phase==WF_SESSION_FRONTEND){if(wf_session_step(&session)!=WF_OK){wf_platform_fail(32);break;}}
  else if(session.phase==WF_SESSION_MATCH_READY){
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
   if(!word(session.ram+0x7c)){
    if(wf_r18_frontend_driver_resume_boot(&session.frontend,0x6fcu)!=WF_OK){wf_platform_fail(47);break;}
    session.phase=WF_SESSION_FRONTEND;
    if(!fixed_enter(false)){wf_platform_fail(47);break;}
   }else
#endif
   if(wf_session_prepare_match(&session)!=WF_OK){wf_platform_fail(35);break;}
  }
  else if(session.phase==WF_SESSION_GAMEPLAY){
#ifdef WF_COMBAT_OVERLAY
   if(!wf_combat_load_module(&session,&board,wf_cold_rng)){wf_platform_fail(36);break;}
#else
   if(!combat_started){if(!wf_combat_saturn_begin(&session,&board)){wf_platform_fail(36);break;}combat_started=true;}
   if(!wf_platform_tick_wait()){wf_platform_fail(37);break;}poll();
   uint8_t inputs[4]={255,255,255,255};
   for(unsigned i=0;i<2;i++){
    unsigned p=(~board.ports[i])&0xb3u;
    if(pads[i].pressed.button.up)p|=4;if(pads[i].pressed.button.down)p|=8;if(pads[i].pressed.button.c)p|=64;
    inputs[i]=(uint8_t)~p;
   }
   if(!wf_combat_saturn_step(inputs)){wf_platform_fail(38);break;}publish();continue;
#endif
  }
  else if(event(&session.frontend)!=WF_OK){wf_platform_fail(33);break;}
  if(!wf_board_dispatch(&board,session.frontend.control.sr,irq,&session.frontend)){wf_platform_fail(34);break;}
 }
 publish();
 failed:for(;;)wf_platform_hold_fields(1);
}
