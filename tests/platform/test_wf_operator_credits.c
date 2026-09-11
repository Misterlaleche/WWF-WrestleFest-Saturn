#include <assert.h>
#include "../../src/runtime/wf_operator_credits.h"

static void coinage(void){
 for(unsigned i=0;i<4;i++){wf_credit_state_t s;wf_coin_event_t e;wf_credit_boot_init(&s,(uint16_t)i);const unsigned th[4]={1,1,2,3},aw[4]={1,2,1,1};assert(s.coin_threshold==th[i]&&s.credit_award==aw[i]);for(unsigned n=1;n<th[i];n++){wf_credit_accept_pulse(&s,&e);assert(!e.converted&&s.credits==0);}wf_credit_accept_pulse(&s,&e);assert(e.converted&&s.credits==aw[i]&&e.sound_command&&e.sound_command_byte==0x2a);}
 wf_credit_state_t s={1,1,0,98,7,0};wf_coin_event_t e;wf_credit_accept_pulse(&s,&e);assert(s.credits==99&&s.partial==0&&s.awarded_bookkeeping==7&&e.capped);
}
static void pulses(void){
 wf_operator_pulse_filter_t f={0,0};for(unsigned i=0;i<4;i++){assert(!wf_operator_pulse_sample(&f,false));}assert(f.release_stability==4);
 assert(!wf_operator_pulse_sample(&f,true));assert(wf_operator_pulse_sample(&f,false));assert(f.held_duration==0&&f.release_stability==0);
 f=(wf_operator_pulse_filter_t){0,4};for(unsigned i=0;i<0x35f;i++){assert(!wf_operator_pulse_sample(&f,true));}assert(wf_operator_pulse_sample(&f,false));
 f=(wf_operator_pulse_filter_t){0,4};for(unsigned i=0;i<0x360;i++){assert(!wf_operator_pulse_sample(&f,true));}assert(!wf_operator_pulse_sample(&f,false));
}
static void payments(void){
 wf_credit_state_t s;wf_credit_boot_init(&s,0);s.credits=2;assert(wf_credit_pay(&s,0,0)&&s.credits==1);assert(!wf_credit_pay(&s,0,4));
 s=(wf_credit_state_t){3,1,0,1,0,0};assert(wf_credit_pay(&s,0,1));assert(s.credits==0&&s.partial==2);
 s=(wf_credit_state_t){3,1,2,0,0,0};assert(wf_credit_pay(&s,0,2)&&s.partial==1);
 s=(wf_credit_state_t){3,1,1,1,0,0};assert(wf_credit_pay(&s,0,5)&&s.partial==0&&s.credits==1);
 s=(wf_credit_state_t){1,1,0,2,0,0};assert(wf_credit_pay(&s,WF_OPERATOR_CONFIG_CONTINUE_AS_START,3)&&s.credits==0);
 s=(wf_credit_state_t){1,2,0,2,0,0};assert(wf_credit_pay(&s,1u,3)&&s.credits==1&&s.partial==0xffu);
 s=(wf_credit_state_t){2,1,1,1,0,0};assert(wf_credit_pay(&s,2u,3)&&s.credits==0&&s.partial==1);
 s=(wf_credit_state_t){3,1,1,0,0,0};assert(!wf_credit_pay(&s,3u,3));
 s=(wf_credit_state_t){3,1,0,1,0,0};wf_credit_state_t before=s;assert(wf_credit_available(&s,0,1));assert(s.partial==before.partial&&s.credits==before.credits);
 for(unsigned m=0;m<6;m++){wf_credit_state_t x={3,1,1,4,0,0};bool a=wf_credit_available(&x,0,(uint8_t)m);bool b=wf_credit_pay(&x,0,(uint8_t)m);assert(a==b);}
}
static void prompts(void){
 wf_credit_state_t s={3,1,0,0,0,0};assert(wf_title_prompt_id(&s,3u,false)==0x02);s.partial=1;assert(wf_title_prompt_id(&s,3u,false)==0x54);assert(wf_title_prompt_id(&s,2u,false)==0x53);s.partial=2;assert(wf_title_prompt_id(&s,3u,false)==0x53);
 s.credits=1;assert(wf_title_prompt_id(&s,0u,false)==0x03);assert(wf_title_prompt_id(&s,WF_OPERATOR_CONFIG_PLAYERS_2P,false)==0x55);s.credits=2;assert(wf_title_prompt_id(&s,WF_OPERATOR_CONFIG_PLAYERS_2P,true)==0x56);
 assert(!wf_title_prompt_blink_phase(63)&&wf_title_prompt_blink_phase(64));
}
static void dips(void){
 assert(wf_operator_difficulty_stamina_adjust(0x0000)==0);assert(wf_operator_difficulty_stamina_adjust(0x0100)==-15);assert(wf_operator_difficulty_stamina_adjust(0x0200)==15);assert(wf_operator_difficulty_stamina_adjust(0x0300)==30);
 assert(wf_operator_clear_stage_power(0x0000)==24);assert(wf_operator_clear_stage_power(0x2000)==32);assert(wf_operator_clear_stage_power(0x4000)==12);assert(wf_operator_clear_stage_power(0x6000)==0);
 assert(!wf_operator_demo_sound_allowed(WF_OPERATOR_CONFIG_DEMO_SOUNDS_OFF,true));assert(wf_operator_demo_sound_allowed(WF_OPERATOR_CONFIG_DEMO_SOUNDS_OFF,false));
 assert(wf_operator_flip_enabled(WF_OPERATOR_CONFIG_FLIP_ON));assert(!wf_operator_fbi_logo_enabled(WF_OPERATOR_CONFIG_FBI_OFF));assert(wf_operator_championship_is_4th(WF_OPERATOR_CONFIG_CHAMPIONSHIP_4TH));
}
static void pack(void){assert(wf_operator_pack_config(0xffff,0xffff,0xffff,0xffff)==0u);assert(wf_operator_pack_config(0xcfff,0xc0ff,0xc0ff,0xfcff)==0xffffu);}
int main(void){pack();coinage();pulses();payments();prompts();dips();return 0;}
