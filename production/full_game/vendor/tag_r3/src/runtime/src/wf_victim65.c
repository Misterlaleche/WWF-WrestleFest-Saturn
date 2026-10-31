#include "wf_victim65.h"
static int c37(const wf_actor_raw_t*a){return (wf_u8(a,WF_A_CONTACT+1u)&0x10u)!=0;}
static wf_v65_result_t low(wf_actor_raw_t*a,uint8_t v){wf_state4_write_low(a,v);return WF_V65_RETYPE;}
static wf_v65_result_t word(wf_actor_raw_t*a,uint16_t v){wf_state4_write_word(a,v);return WF_V65_RETYPE;}
void wf_victim65_first_entry(wf_actor_raw_t*a){
 if(wf_main_state(a)==4u && wf_state4_subtype(a)==0x05u && wf_u8(a,WF_A_TIER_LO)==0x02u){
  wf_set_u8(a,WF_A_STATE4,(uint8_t)(wf_state4_flags(a)|0x80u));
 }
}
wf_v65_result_t wf_victim65_tick_transition(wf_actor_raw_t*a,uint8_t global_0161){
 if(wf_main_state(a)!=4u){return WF_V65_STAY;}
 uint8_t s=wf_state4_subtype(a),p=wf_u8(a,WF_A_ACTION_PHASE);
 switch(s){
 case 0x02:case 0x03:case 0x04:if(c37(a))return low(a,0x05);break;
 case 0x05:
  /* First-entry critical-tier BSET belongs to state4 entry semantics and is not repeated here. */
  if(c37(a)){return low(a,(wf_state4_flags(a)&0x80u)?0x09u:0x08u);}
  break;
 case 0x2A:if(c37(a))return low(a,0x08);break;
 case 0x06:if(p==0xFEu)return low(a,0x08);break;
 case 0x07:if(p==0xFEu)return low(a,0x09);break;
 case 0x08:case 0x09:
  if(p==0xFEu){wf_set_be16(a,WF_A_STATE,7);wf_set_u8(a,WF_A_STATE4,(uint8_t)(wf_state4_flags(a)&~0x80u));return WF_V65_RETYPE;}break;
 case 0x0A:
  if(p==0xFEu){wf_set_be16(a,WF_A_STATE,(wf_u8(a,WF_A_FLAGS32)&1u)?1u:0u);return WF_V65_RETYPE;}break;
 case 0x20:if(p==0xFEu){wf_set_be16(a,WF_A_STATE,4);return word(a,0x0002);}break;
 case 0x0B:case 0x0C:case 0x0D:return low(a,(uint8_t)(s-9u));
 case 0x0E:
  if(p==3u&&c37(a)&&(((wf_u8(a,WF_A_FLAGS33)&0x04u)==0u)||(global_0161&0x02u))){return low(a,0x05);}
  break;
 case 0x0F:case 0x13:case 0x15:case 0x17:case 0x19:case 0x23:case 0x26:if(c37(a))return low(a,0x05);break;
 case 0x14:case 0x1D:
  if(p==0xFEu){if(wf_u8(a,0x1F)==2u)return low(a,0x1B);wf_set_be16(a,WF_A_STATE,4);return word(a,0x0002);}break;
 case 0x16:
  if(p==1u&&c37(a)&&(((wf_u8(a,WF_A_FLAGS33)&0x04u)==0u)||(global_0161&0x02u))){return low(a,0x05);}
  break;
 case 0x18:if(p==0xFEu){wf_set_be16(a,WF_A_STATE,0);return WF_V65_RETYPE;}break;
 case 0x29:if(p==0xFEu){wf_set_be16(a,WF_A_STATE,4);return word(a,0x0003);}break;
 case 0x1A:if(c37(a))return low(a,0x09);break;
 case 0x1B:if(c37(a)){wf_set_be16(a,WF_A_STATE,4);return word(a,0x0005);}break;
 case 0x1C:if(p==0xFEu){wf_set_be16(a,WF_A_STATE,4);return word(a,0x0002);}break;
 case 0x1E:if(p==0xFEu){wf_set_be16(a,WF_A_STATE,4);return word(a,0x100F);}break;
 case 0x21:if(c37(a))return low(a,wf_u8(a,WF_A_TIER_LO)==2u?0x09u:0x08u);break;
 case 0x22:if(p==1u&&c37(a))return word(a,0x0005);break;
 case 0x24:if(p==0xFEu)return low(a,0x05);break;
 case 0x25:if(p==2u&&c37(a))return word(a,0x0005);break;
 case 0x27:if(wf_be16(a,WF_A_ANIM_COUNTER)==0u){wf_set_be16(a,WF_A_STATE,4);return word(a,0x0002);}break;
 default:break;
 }
 return WF_V65_STAY;
}
