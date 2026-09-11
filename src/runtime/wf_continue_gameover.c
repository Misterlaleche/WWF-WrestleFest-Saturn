#include "wf_continue_gameover.h"
#include <string.h>

static bool valid_mask(uint8_t m){return m>=1u&&m<=3u;}
static void clear_event(wf_continue_event_t *e){if(e)memset(e,0,sizeof(*e));}

uint8_t wf_continue_request_code(uint8_t m){if(m==3u)return 3u;if(m==1u||m==2u)return 1u;return 0u;}

uint32_t wf_continue_input_word_a_rom(wf_continue_pair_t pair,uint8_t m){
 if(!valid_mask(m))return 0u;
 if(pair==WF_CONTINUE_PAIR_A)return m==2u?0x00140022u:0x00140020u;
 if(pair==WF_CONTINUE_PAIR_B)return m==2u?0x00140026u:0x00140024u;
 return 0u;
}
uint32_t wf_continue_input_word_b_rom(wf_continue_pair_t pair,uint8_t m){
 if(!valid_mask(m))return 0u;
 if(m!=3u)return wf_continue_input_word_a_rom(pair,m);
 if(pair==WF_CONTINUE_PAIR_A)return 0x00140022u;
 if(pair==WF_CONTINUE_PAIR_B)return 0x00140026u;
 return 0u;
}
bool wf_continue_start_level(uint8_t m,const wf_continue_pair_inputs_t *in){
 if(!in||!valid_mask(m))return false;
 if((m&1u)&&((in->player_a_active_low&0x0080u)==0u))return true;
 if((m&2u)&&((in->player_b_active_low&0x0080u)==0u))return true;
 return false;
}
bool wf_continue_begin(wf_continue_runtime_t *rt,wf_continue_pair_t pair,uint8_t m){
 if(!rt||!valid_mask(m)||(pair!=WF_CONTINUE_PAIR_A&&pair!=WF_CONTINUE_PAIR_B))return false;
 memset(rt,0,sizeof(*rt));rt->pair=pair;rt->participation_mask=m;rt->digit=WF_CONTINUE_INITIAL_DIGIT;rt->active=true;return true;
}
void wf_continue_step(wf_continue_runtime_t *rt,const wf_continue_pair_inputs_t *in,const wf_continue_credit_ops_t *credit,wf_continue_event_t *e){
 uint8_t request;bool available;bool start;clear_event(e);
 if(!rt||!rt->active||!in||!credit||!credit->available||!credit->spend)return;
 if(e){e->render_digit=true;e->render_word=(uint16_t)(0x8000u|((uint16_t)rt->digit&0x7fffu));}
 rt->iterations++;rt->subcounter++;
 if(rt->subcounter>=WF_CONTINUE_DIGIT_QUANTUM){rt->subcounter=0u;rt->digit--;}
 request=wf_continue_request_code(rt->participation_mask);
 available=credit->available(request,credit->user);
 if(e){
  const uint16_t off=rt->pair==WF_CONTINUE_PAIR_B?WF_CONTINUE_PAIR_B_TEXT_OFFSET:0u;
  e->credit_available=available;e->no_credit_text_id=WF_CONTINUE_NO_CREDIT_TEXT_ID;e->no_credit_visible=!available;
  e->player_prompt_a_id=(uint16_t)(WF_CONTINUE_PAIR_A_PROMPT_BASE+off);
  e->player_prompt_b_id=(uint16_t)(WF_CONTINUE_PAIR_A_PROMPT_BASE+1u+off);
  e->player_prompt_a_visible=available&&((rt->participation_mask&1u)!=0u);
  e->player_prompt_b_visible=available&&((rt->participation_mask&2u)!=0u);
 }
 start=wf_continue_start_level(rt->participation_mask,in);if(e)e->start_level_seen=start;
 if(start){
  bool spent;if(e)e->spend_attempted=true;spent=credit->spend(request,credit->user);
  if(spent){rt->active=false;rt->accepted=true;if(e){e->accepted=true;e->set_global_markers=true;e->set_pair_actor_markers=true;e->retry_same_campaign_position=true;}return;}
 }
 if(rt->digit<0){rt->active=false;rt->expired=true;if(e){e->expired=true;e->clear_pair_actor_domains=true;e->clear_bytes=WF_CONTINUE_ACTOR_CLEAR_BYTES;e->preserve_word_offset=WF_CONTINUE_PRESERVE_WORD_OFFSET;}}
}
void wf_continue_session_begin(wf_continue_session_t *s,uint8_t ma,bool ea,uint8_t mb,bool eb){
 if(!s)return;memset(s,0,sizeof(*s));s->pair_mask_a=ma;s->pair_mask_b=mb;s->pair_eligible_a=ea;s->pair_eligible_b=eb;s->next_pair=WF_CONTINUE_PAIR_A;
}
bool wf_continue_session_next(wf_continue_session_t *s){
 if(!s||s->finished||s->current.active)return false;
 while(s->next_pair!=WF_CONTINUE_PAIR_NONE){
  wf_continue_pair_t p=s->next_pair;uint8_t m;bool eligible;
  if(p==WF_CONTINUE_PAIR_A){m=s->pair_mask_a;eligible=s->pair_eligible_a;s->next_pair=WF_CONTINUE_PAIR_B;}
  else{m=s->pair_mask_b;eligible=s->pair_eligible_b;s->next_pair=WF_CONTINUE_PAIR_NONE;}
  if(eligible&&valid_mask(m))return wf_continue_begin(&s->current,p,m);
 }
 s->finished=true;return false;
}
void wf_continue_session_complete_current(wf_continue_session_t *s){if(!s)return;if(s->current.active)return;if(s->next_pair==WF_CONTINUE_PAIR_NONE)s->finished=true;}
