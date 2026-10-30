#include "wf_pin.h"
void wf_pin_assert_trigger(wf_actor_raw_t*w){wf_set_u8(w,WF_A_PINFLAGS,(uint8_t)(wf_u8(w,WF_A_PINFLAGS)|1u));}
int wf_pin_trigger(const wf_actor_raw_t*w){return (wf_u8(w,WF_A_PINFLAGS)&1u)!=0;}
void wf_ref_count_enter(wf_ref_count_t*r,uint8_t inherited_phase){r->action_state=6;r->phase=inherited_phase;r->timer=20;r->audio_cmd=0x3162;r->display_acc=0;}
int wf_ref_prelock_cancel(const wf_ref_count_t*r,const wf_actor_raw_t*l){return r->phase<6u && !wf_pin_trigger(l);}
wf_ref_event_t wf_ref_count_tick(wf_ref_count_t*r){
 wf_ref_event_t e={0,0,0};
 if(r->timer){r->timer--;return e;}
 r->phase++;
 if((r->phase&1u)==0u){r->display_acc++;e.count_event=1;e.audio_cmd=r->audio_cmd++;}
 if(r->phase==7u){e.terminal=1;return e;}
 /* Canonical selector 5/6 alternation: 56 / 32; phase6 adds 16. */
 r->timer=(r->phase&1u)?32u:56u;
 if(r->phase==6u)r->timer=(uint8_t)(r->timer+16u);
 return e;
}
