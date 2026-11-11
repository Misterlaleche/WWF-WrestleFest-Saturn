#include "wf_combat_loader.h"
#include "wf_combat_saturn.h"
#include "wf_platform.h"
#include "wf_saturn_yaul_backend.h"
#include "wp2015_irq_mac.h"
#include "wf_failure_screen.h"
static wf_session_t session;
static wf_board_t board;
static smpc_peripheral_digital_t pads[2];
extern volatile uint32_t wf_combat_finished;
volatile uint32_t wf_session_phase,wf_session_pc,wf_session_owner,wf_session_generation,wf_credits,wf_paid,wf_state,wf_slot,wf_partner,wf_royal,wf_scroll,wf_inputs,wf_irq2_count,wf_irq3_count,wf_board_line,wf_snapshot_sequence,wf_cold_rng;
/* Frontend counters are carried only as observer fields; no frontend owner
 * or renderer is linked into this executable. */
volatile uint32_t wf_video_error,wf_video_commits,wf_video_sprites,wf_video_tile,wf_render_work,wf_render_wait;
static unsigned word(const uint8_t*p){return (p[0]<<8)|p[1];}
static void publish(void){
 ++wf_snapshot_sequence;wf_session_phase=session.phase;wf_state=session.frontend.phase;wf_session_pc=session.frontend.pc;
 wf_session_owner=session.error_owner;wf_session_generation=session.frontend.generation;wf_credits=session.runtime.g.credit_count_004f;
 wf_paid=session.runtime.g.credited_route;wf_slot=word(session.ram+0x5d2);wf_partner=word(session.ram+0x6de);
 wf_royal=session.runtime.g.mode_0161&1;wf_scroll=word(board.scroll);wf_board_line=board.line;++wf_snapshot_sequence;
}
__attribute__((cold,noinline)) void wf_failure_context_read(wf_failure_context_t *out){
 extern volatile uint32_t wf_combat_error;
 const wf_runtime_t *r=&session.runtime;
 out->session_phase=session.phase;out->phase=r->frame_stage;
 out->pc=r->trace_count&&r->trace_count<=64?r->trace[r->trace_count-1]:session.frontend.pc;
 out->owner=wf_combat_error?wf_combat_error:out->pc;
 out->address=session.video.failed_address;
}
void user_init(void){
 wf_platform_init();
 if(wp2015_irq_mac_install())wf_platform_fail(39);
 vdp2_tvmd_display_clear();vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
}
int main(void){
 wf_pacer_t clock;
 if(wf_error||!wf_combat_handoff_restore(WF_COMBAT_CAPSULE,&session,&board,&clock)||
    !wf_platform_clock_restore(&clock,WF_COMBAT_CAPSULE->fields,WF_COMBAT_CAPSULE->clock_origin)){
  WF_COMBAT_MAILBOX[0]=0xdead0002u;wf_platform_fail(40);goto failed;
 }
 wf_cold_rng=WF_COMBAT_CAPSULE->cold_rng;publish();WF_COMBAT_MAILBOX[0]=3;
 if(wf_saturn_yaul_video_init()!=WF_OK||!wf_combat_saturn_begin(&session,&board)||!wf_platform_load(false)){wf_platform_fail(36);goto failed;}
 for(;;){
  if(!wf_platform_tick_wait()){wf_platform_fail(37);break;}
  smpc_peripheral_process();uint8_t inputs[4]={255,255,255,255};
  for(unsigned i=0;i<2;i++){
   smpc_peripheral_digital_port(i+1,pads+i);unsigned p=0;
   if(pads[i].pressed.button.right)p|=1;if(pads[i].pressed.button.left)p|=2;
   /* Sprite-Y inversion belongs to rendering, not the physical pad bits. */
   if(pads[i].pressed.button.up)p|=4;if(pads[i].pressed.button.down)p|=8;
   if(pads[i].pressed.button.a)p|=0x10;if(pads[i].pressed.button.b)p|=0x20;
   if(pads[i].pressed.button.c)p|=0x40;if(pads[i].pressed.button.start)p|=0x80;
   inputs[i]=(uint8_t)~p;if(!i)wf_inputs=p;
  }
  if(!wf_combat_saturn_step(inputs)){wf_platform_fail(38);break;}publish();
  if(wf_combat_finished){
#ifdef WF_ALPHA_RETURN
   if(!wf_combat_saturn_export_scene()||!wf_combat_return_frontend(&session,&board,wf_cold_rng))wf_platform_fail(41);
#endif
   break;
  }
 }
 failed:for(;;)wf_platform_hold_fields(1);
}
