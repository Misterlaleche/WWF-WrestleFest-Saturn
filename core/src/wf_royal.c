#include "wf_royal.h"
wf_royal_scan_result_t wf_royal_survivor_scan(wf_actor_raw_t a[WF_ACTOR_SLOTS],wf_royal_scan_t*out,uint16_t m[WF_ACTOR_SLOTS]){
 uint8_t d3=0,d4=0;int8_t survivor=-1;
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
  if((wf_u8(&a[i],WF_A_ACTIVE)&0x80u)==0u)continue;
  int b32=(wf_u8(&a[i],WF_A_FLAGS32)&0x10u)!=0;int b56=(wf_u8(&a[i],WF_A_ROLE)&0x80u)!=0;
  if(!b32&&!b56){d3++;survivor=(int8_t)i;}
  else if(!b32&&b56)d4++;
  else if(b32&&!b56&&m)m[i]=0x3001u;
 }
 if(out){out->eligible=d3;out->noneligible=d4;out->survivor_slot=survivor;}
 if(d3==0u)return WF_ROYAL_SCAN_ZERO;
 if(d3==1u&&d4==0u)return WF_ROYAL_SCAN_WINNER;
 return WF_ROYAL_SCAN_CONTINUE;
}
