#include "wf_platform.h"
#include "wf_frontend_video.h"
#include "wf_frontend_resources.h"
#include "wf_roster_identity.h"
volatile uint32_t wf_state,wf_slot,wf_page,wf_royal,wf_scroll,wf_pad_bits;
volatile uint32_t wf_selected_identity=0xffffffffu;
static wf_frontend_t frontend;
static smpc_peripheral_digital_t pad;
void user_init(void){wf_platform_init();}
static void publish(void){wf_state=frontend.state;wf_slot=frontend.slot;wf_page=frontend.page;wf_royal=frontend.royal;wf_scroll=frontend.scroll;}
int main(void){
 wf_frontend_init(&frontend);
 if(!wf_error&&!wf_frontend_resources_load())wf_platform_fail(21);
 if(!wf_error&&!wf_frontend_video_mode())wf_platform_fail(10);
 if(wf_error)wf_state=0;else{wf_platform_scene_begin(0);publish();}
 for(;;){
  if(wf_error||frontend.state==WF_FRONTEND_DONE){wf_platform_hold_fields(1);continue;}
  if(!wf_platform_tick_wait())continue;
  /* TEST03 does not process SMPC during its scroll loop: preserve button edges. */
  if(frontend.state!=WF_FRONTEND_SCROLL)wf_platform_pad(&pad);
  unsigned d=0;
  if(pad.pressed.button.right)d|=1;if(pad.pressed.button.left)d|=2;
  if(pad.pressed.button.up)d|=4;if(pad.pressed.button.down)d|=8;
  bool confirm=pad.held.button.a||pad.held.button.b;
  wf_pad_bits=d|(confirm?16u:0u);
  wf_frontend_effect_t effect;
  if(!wf_frontend_tick(&frontend,d,confirm,&effect)){wf_platform_fail(11);publish();continue;}
  if(effect.confirm){uint8_t raw;if(!wf_roster_raw_identity((uint16_t)frontend.slot,&raw)){wf_platform_fail(22);continue;}wf_selected_identity=raw;}
  wf_frontend_video_apply(&frontend,&effect);publish();
  if(frontend.state==WF_FRONTEND_LOAD_ROSTER){
   if(!wf_frontend_video_roster()){wf_platform_fail(12);continue;}
   wf_platform_hold_fields(17);wf_frontend_roster_ready(&frontend);
   wf_platform_scene_begin(0);publish();
  }
 }
}
