#include "arcade_attract_transition.h"
#include "arcade_interstitial.h"
arcade_attract_entry_transition_t arcade_attract_entry_transition(unsigned entry_state){
 arcade_attract_entry_transition_t r={0};
 if(entry_state==1U){
  r.active=true;r.entry_state=1;r.interstitial_scene=0;r.records_mode=ARCADE_RECORD_TAG;r.records_hold=0x0100;r.transition_wipes=2;
 }else if(entry_state==2U){
  r.active=true;r.entry_state=2;r.interstitial_scene=1;r.records_mode=ARCADE_RECORD_ROYAL;r.records_hold=0x0100;r.transition_wipes=2;
 }
 return r;
}
