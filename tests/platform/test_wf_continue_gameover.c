#include <assert.h>
#include "../../src/runtime/wf_continue_gameover.h"

typedef struct mock_credit{unsigned avail_calls,spend_calls;bool available,spend_ok;}mock_credit_t;
static bool avail(uint8_t req,void*u){mock_credit_t*m=u;m->avail_calls++;assert(req==1u||req==3u);return m->available;}
static bool spend(uint8_t req,void*u){mock_credit_t*m=u;m->spend_calls++;assert(req==1u||req==3u);return m->spend_ok;}

static void countdown_full(void){
 wf_continue_runtime_t rt;wf_continue_event_t ev;wf_continue_pair_inputs_t in={0xffff,0xffff};mock_credit_t m={0,0,false,false};wf_continue_credit_ops_t ops={avail,spend,&m};
 assert(wf_continue_begin(&rt,WF_CONTINUE_PAIR_A,1));
 for(int digit=9;digit>=0;--digit)for(unsigned i=0;i<64;i++){assert(rt.digit==digit);wf_continue_step(&rt,&in,&ops,&ev);assert(ev.render_digit&&(ev.render_word&0x7fff)==(unsigned)digit);if(digit!=0||i!=63)assert(!ev.expired);}
 assert(rt.expired&&!rt.active&&rt.iterations==640u);
 assert(ev.expired&&ev.clear_pair_actor_domains&&ev.clear_bytes==0x10cu&&ev.preserve_word_offset==0x56u);
 assert(m.avail_calls==640u&&m.spend_calls==0u);
}
static void accept_final_zero_iteration(void){
 wf_continue_runtime_t rt;wf_continue_event_t ev;wf_continue_pair_inputs_t in={0xffff,0xffff};mock_credit_t m={0,0,true,true};wf_continue_credit_ops_t ops={avail,spend,&m};
 assert(wf_continue_begin(&rt,WF_CONTINUE_PAIR_A,1));
 for(unsigned i=0;i<639;i++)wf_continue_step(&rt,&in,&ops,&ev);
 assert(rt.digit==0&&rt.subcounter==63&&rt.active);in.player_a_active_low=0xff7fu;
 wf_continue_step(&rt,&in,&ops,&ev);
 assert(rt.accepted&&!rt.expired&&!rt.active&&ev.accepted&&ev.set_global_markers&&ev.set_pair_actor_markers&&ev.retry_same_campaign_position);
}
static void held_level_and_live_credit(void){
 wf_continue_runtime_t rt;wf_continue_event_t ev;wf_continue_pair_inputs_t in={0xff7f,0xffff};mock_credit_t m={0,0,false,false};wf_continue_credit_ops_t ops={avail,spend,&m};
 assert(wf_continue_begin(&rt,WF_CONTINUE_PAIR_A,1));wf_continue_step(&rt,&in,&ops,&ev);
 assert(ev.start_level_seen&&ev.spend_attempted&&!ev.accepted&&ev.no_credit_visible&&!ev.player_prompt_a_visible);
 m.available=true;m.spend_ok=true;wf_continue_step(&rt,&in,&ops,&ev);
 assert(ev.credit_available&&ev.accepted&&!ev.no_credit_visible&&ev.player_prompt_a_visible&&!ev.player_prompt_b_visible&&ev.player_prompt_a_id==0x1du);
}
static void pairs_and_request(void){
 assert(wf_continue_request_code(1)==1&&wf_continue_request_code(2)==1&&wf_continue_request_code(3)==3);
 assert(wf_continue_input_word_a_rom(WF_CONTINUE_PAIR_A,1)==0x00140020u);
 assert(wf_continue_input_word_a_rom(WF_CONTINUE_PAIR_A,2)==0x00140022u);
 assert(wf_continue_input_word_b_rom(WF_CONTINUE_PAIR_A,3)==0x00140022u);
 assert(wf_continue_input_word_a_rom(WF_CONTINUE_PAIR_B,1)==0x00140024u);
 assert(wf_continue_input_word_a_rom(WF_CONTINUE_PAIR_B,2)==0x00140026u);
 assert(wf_continue_input_word_b_rom(WF_CONTINUE_PAIR_B,3)==0x00140026u);
 wf_continue_session_t s;wf_continue_session_begin(&s,3,true,1,true);
 assert(wf_continue_session_next(&s)&&s.current.pair==WF_CONTINUE_PAIR_A&&s.current.participation_mask==3);
 s.current.active=false;s.current.accepted=true;wf_continue_session_complete_current(&s);
 assert(wf_continue_session_next(&s)&&s.current.pair==WF_CONTINUE_PAIR_B&&s.current.participation_mask==1);
 s.current.active=false;s.current.expired=true;wf_continue_session_complete_current(&s);assert(s.finished);
}
int main(void){countdown_full();accept_final_zero_iteration();held_level_and_live_credit();pairs_and_request();assert(WF_GAMEOVER_NORMAL_WAIT_EACH==65u&&WF_GAMEOVER_NORMAL_WAIT_TOTAL==130u&&WF_GAMEOVER_ROYAL_WAIT==97u);return 0;}
