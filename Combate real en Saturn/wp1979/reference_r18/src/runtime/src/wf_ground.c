#include "wf_ground.h"
static int c37(const wf_actor_raw_t*a){return (wf_u8(a,WF_A_CONTACT+1u)&0x10u)!=0;}
wf_ground_result_t wf_ground_linked_retype(uint8_t raw,wf_actor_raw_t*v){
 if(!(raw==0x0Au||raw==0x0Bu||raw==0x10u)) return WF_GROUND_STAY;
 if(wf_main_state(v)!=4u) return WF_GROUND_STAY;
 uint8_t s=wf_state4_subtype(v);
 if(s==0x08u){wf_state4_write_low(v,0x06u);return WF_GROUND_ADVANCE;}
 if(s==0x09u){wf_state4_write_low(v,0x07u);return WF_GROUND_ADVANCE;}
 return WF_GROUND_STAY;
}
wf_ground_result_t wf_ground_contact_release(uint8_t raw,wf_actor_raw_t*a){
 if(!c37(a)) return WF_GROUND_STAY;
 switch(raw){
  case 0x0B: case 0x14: case 0x23: case 0x26:
   wf_set_u8(a,WF_A_MOTION_MODE,0); wf_set_u8(a,WF_A_ANIM_COUNTER,0); return WF_GROUND_ADVANCE;
  case 0x13:
   /* Exact proven helper path writes $0018, but helper carry decides reachability. */
   return WF_GROUND_BLOCKED;
  case 0x0E: case 0x10: case 0x35: case 0x46: case 0x47:
   return WF_GROUND_BLOCKED;
  default:return WF_GROUND_STAY;
 }
}
