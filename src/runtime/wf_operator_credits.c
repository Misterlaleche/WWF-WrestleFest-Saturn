#include "wf_operator_credits.h"
#include <string.h>
static const uint8_t k_threshold[4]={1u,1u,2u,3u};
static const uint8_t k_award[4]={1u,2u,1u,1u};
static const int16_t k_difficulty[4]={0,-15,15,30};
static const uint8_t k_clear_stage[4]={24u,32u,12u,0u};
uint16_t wf_operator_pack_config(uint16_t p20,uint16_t p22,uint16_t p24,uint16_t p26){
 uint32_t packed=((uint32_t)(p20&0x3000u)<<2)|(uint32_t)(p22&0x3f00u)|((uint32_t)(p24&0x3f00u)>>8)|((uint32_t)(p26&0x0300u)>>2);return(uint16_t)(~packed);
}
void wf_credit_boot_init(wf_credit_state_t*s,uint16_t config){uint8_t i;if(!s)return;i=(uint8_t)(config&3u);memset(s,0,sizeof(*s));s->coin_threshold=k_threshold[i];s->credit_award=k_award[i];}
void wf_credit_accept_pulse(wf_credit_state_t*s,wf_coin_event_t*e){
 uint16_t candidate;if(e)memset(e,0,sizeof(*e));if(!s)return;s->partial=(uint8_t)(s->partial+1u);s->accepted_pulse_bookkeeping=(uint16_t)(s->accepted_pulse_bookkeeping+1u);
 if(e){e->sound_command=true;e->sound_command_byte=WF_OPERATOR_COIN_SOUND_COMMAND;}if(s->partial<s->coin_threshold)return;candidate=(uint16_t)s->credits+(uint16_t)s->credit_award;
 if(candidate>=WF_OPERATOR_MAX_CREDITS){s->credits=WF_OPERATOR_MAX_CREDITS;s->partial=0u;if(e){e->converted=true;e->capped=true;}return;}
 s->credits=(uint8_t)candidate;s->awarded_bookkeeping=(uint16_t)(s->awarded_bookkeeping+s->credit_award);s->partial=(uint8_t)(s->partial-s->coin_threshold);if(e)e->converted=true;
}
bool wf_operator_pulse_sample(wf_operator_pulse_filter_t*f,bool asserted){
 bool accepted=false;if(!f)return false;if(asserted){f->held_duration=(uint16_t)(f->held_duration+1u);return false;}
 if(f->held_duration<1u){f->held_duration=0u;if(f->release_stability<WF_OPERATOR_PULSE_STABILITY_MAX)f->release_stability++;return false;}
 if(f->release_stability>=WF_OPERATOR_PULSE_STABILITY_MIN&&f->held_duration<WF_OPERATOR_PULSE_HELD_MAX_EXCLUSIVE){accepted=true;f->release_stability=0u;}f->held_duration=0u;return accepted;
}
static bool pay_whole(wf_credit_state_t*s,uint8_t n){if(s->credits<n)return false;s->credits=(uint8_t)(s->credits-n);return true;}
static bool pay_fixed_one(wf_credit_state_t*s){if(s->partial>0u){s->partial--;return true;}if(s->credits==0u)return false;s->credits--;s->partial=s->coin_threshold;s->partial--;return true;}
static bool pay_fixed_two(wf_credit_state_t*s){if(s->partial>=2u){s->partial=(uint8_t)(s->partial-2u);return true;}if(s->credits==0u)return false;s->credits--;s->partial=(uint8_t)(s->partial+s->coin_threshold);s->partial=(uint8_t)(s->partial-2u);return true;}
bool wf_credit_pay(wf_credit_state_t*s,uint16_t config,uint8_t mode){
 uint16_t bit;if(!s)return false;switch(mode){case 0:return pay_whole(s,1u);case 1:bit=WF_OPERATOR_CONFIG_CONTINUE_AS_START;break;case 2:bit=WF_OPERATOR_CONFIG_REGAIN_AS_START;break;
 case 3:if((config&WF_OPERATOR_CONFIG_CONTINUE_AS_START)!=0u)return pay_whole(s,2u);if((config&3u)==0u)return pay_whole(s,2u);return pay_fixed_two(s);
 case 4:return pay_whole(s,2u);case 5:bit=WF_OPERATOR_CONFIG_BUYIN_AS_START;break;default:return false;}if((config&bit)!=0u)return pay_whole(s,1u);return pay_fixed_one(s);
}
bool wf_credit_available(const wf_credit_state_t*s,uint16_t config,uint8_t mode){wf_credit_state_t copy;if(!s)return false;copy=*s;return wf_credit_pay(&copy,config,mode);}
uint8_t wf_title_prompt_id(const wf_credit_state_t*s,uint16_t config,bool second){
 uint8_t idx;if(!s)return 0u;idx=(uint8_t)(config&3u);if(s->credits==0u){if(s->partial==0u)return 0x02u;if(s->partial>=2u)return 0x53u;if(idx==3u)return 0x54u;return 0x53u;}
 if((config&WF_OPERATOR_CONFIG_PLAYERS_MASK)!=WF_OPERATOR_CONFIG_PLAYERS_2P)return 0x03u;if(s->credits==1u)return 0x55u;return second?0x56u:0x55u;
}
bool wf_title_prompt_blink_phase(uint16_t counter){return(counter&0x0040u)!=0u;}
int16_t wf_operator_difficulty_stamina_adjust(uint16_t config){return k_difficulty[(config>>8)&3u];}
uint8_t wf_operator_clear_stage_power(uint16_t config){return k_clear_stage[(config>>13)&3u];}
bool wf_operator_demo_sound_allowed(uint16_t config,bool presentation_context){return !presentation_context||((config&WF_OPERATOR_CONFIG_DEMO_SOUNDS_OFF)==0u);}
bool wf_operator_flip_enabled(uint16_t config){return(config&WF_OPERATOR_CONFIG_FLIP_ON)!=0u;}
bool wf_operator_fbi_logo_enabled(uint16_t config){return(config&WF_OPERATOR_CONFIG_FBI_OFF)==0u;}
bool wf_operator_championship_is_4th(uint16_t config){return(config&WF_OPERATOR_CONFIG_CHAMPIONSHIP_4TH)!=0u;}
