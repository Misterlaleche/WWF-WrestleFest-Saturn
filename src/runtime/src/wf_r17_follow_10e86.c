#include "wf_r17_follow_10e86.h"
static int bit(const wf_actor_raw_t*a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static void clrbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~(1u<<b)));}
wf_status_t wf_r17_follow_10e86(wf_runtime_t *rt,wf_actor_raw_t *a,uint8_t mode){
 wf_actor_raw_t*p;uint16_t x,y;int path_e;
 if(!rt||!a)return WF_BAD_STATE;
 if((mode&1u)!=0u)return WF_OK;
 if(!bit(a,WF_A_FLAGS33,0)||bit(a,WF_A_FLAGS33,2))return WF_OK;
 p=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TEAM_LINK),0);if(!p)return WF_BLOCKED;
 if(!bit(p,WF_A_FLAGS32,0))return WF_OK;
 x=wf_be16(a,WF_A_X);y=wf_be16(a,WF_A_Y);
 if(!bit(a,WF_A_FLAGS33,7)) path_e=(x>=0x0230u)||(y<=0x0170u);
 else path_e=(x<=0x0300u)||(y>=0x0150u);
 if(path_e){
  if(wf_u8(p,WF_A_STATE+1u)!=1u||wf_u8(p,WF_A_CPU_SUBSTATE)!=2u)return WF_OK;
  wf_set_be16(p,WF_A_STATE,1u);wf_set_u8(p,WF_A_CPU_SUBSTATE,1u);return WF_OK;
 }
 if(wf_u8(p,WF_A_STATE+1u)==1u&&wf_u8(p,WF_A_CPU_SUBSTATE)==1u){wf_set_be16(p,WF_A_STATE,1u);wf_set_u8(p,WF_A_CPU_SUBSTATE,2u);return WF_OK;}
 if(wf_u8(p,WF_A_STATE+1u)!=5u||wf_u8(p,WF_A_SELECTOR_LO)!=0x71u)return WF_OK;
 clrbit(p,WF_A_C6,6);setbit(p,WF_A_FLAGS33,5);wf_set_be16(p,WF_A_STATE,1u);wf_set_u8(p,WF_A_CPU_SUBSTATE,2u);return WF_OK;
}
