#include "wf_target_relation.h"

static int bit(const wf_actor_raw_t *a,size_t off,unsigned b){return (wf_u8(a,off)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t off,unsigned b){wf_set_u8(a,off,(uint8_t)(wf_u8(a,off)|(uint8_t)(1u<<b)));}
static void clrbit(wf_actor_raw_t *a,size_t off,unsigned b){wf_set_u8(a,off,(uint8_t)(wf_u8(a,off)&(uint8_t)~(1u<<b)));}
static uint16_t abs16diff(uint16_t a,uint16_t b){int32_t d=(int16_t)a-(int16_t)b; if(d<0)d=-d; return (uint16_t)d;}
static uint32_t ptr_of(unsigned slot){return wf_actor_rom_base(slot);}
static wf_actor_raw_t *ptr(wf_runtime_t *rt,uint32_t p){return wf_actor_from_rom_ptr(rt,p,0);}

/* $20C88. Carry clear means candidate A3 is already owned as +$7A by an
 * active slot4..8 actor whose +$32.bit4 is clear. Carry set means usable. */
static wf_status_t royal_candidate_free_20c88(wf_runtime_t *rt,uint32_t source_ptr,uint32_t cand_ptr,int *carry_set){
 if(!rt||!carry_set)return WF_BAD_STATE;
 for(unsigned slot=4;slot<WF_ACTOR_SLOTS;slot++){
  wf_actor_raw_t *a=&rt->actors[slot];
  if(!bit(a,WF_A_ACTIVE,7))continue;
  if(ptr_of(slot)==source_ptr)continue;
  if(wf_be32(a,WF_A_TARGET)!=cand_ptr)continue;
  if(bit(a,WF_A_FLAGS32,4))continue;
  *carry_set=0; return WF_OK;
 }
 *carry_set=1; return WF_OK;
}

/* $20E00 primary Royal metric. */
static wf_status_t royal_primary_20e00(wf_runtime_t *rt,unsigned source_slot){
 wf_actor_raw_t *src=&rt->actors[source_slot];
 uint16_t best=0x0800u; uint32_t winner=0u; uint8_t d3=0u;
 for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++){
  wf_actor_raw_t *c=&rt->actors[slot]; uint32_t cp=ptr_of(slot);
  if(d3==(uint8_t)source_slot){d3++;continue;}
  if(!bit(c,WF_A_ACTIVE,7)){d3++;continue;}
  if(wf_u8(c,WF_A_STATE+1)==0xffu){d3++;continue;}
  if((rt->g.mode_0161&1u)==0u){
   if(bit(c,WF_A_FLAGS32,0)){d3++;continue;}
   if(bit(c,WF_A_FLAGS32,7)){d3++;continue;}
  }
  if(wf_be32(src,WF_A_TEAM_LINK)==cp){d3++;continue;}
  if(bit(c,WF_A_FLAGS32,4)){d3++;continue;}
  int32_t x=(int16_t)wf_be16(c,WF_A_X);
  if(wf_u8(c,WF_A_STATE+1)==4u && wf_u8(c,WF_A_STATE4_LO)==8u){x += bit(c,WF_A_MIRROR,7)?-0x48:0x48;}
  else if(wf_u8(c,WF_A_STATE+1)==5u && wf_u8(c,WF_A_SELECTOR_LO)==0x5du){x += bit(c,WF_A_MIRROR,7)?-0x48:0x48;}
  uint16_t dx=(uint16_t)(x-(int16_t)wf_be16(src,WF_A_X)); if((int16_t)dx<0)dx=(uint16_t)-(int16_t)dx;
  dx=(uint16_t)(dx>>2);
  uint16_t dy=abs16diff(wf_be16(src,WF_A_Y),wf_be16(c,WF_A_Y));
  uint16_t m=(uint16_t)(dx+dy);
  if(m<best){best=m;winner=cp;}
  d3++;
 }
 rt->g.global_1692=best; rt->g.global_168c=winner; return WF_OK;
}

/* $20BDA secondary Royal ownership/candidate scan. */
static wf_status_t royal_secondary_20bda(wf_runtime_t *rt,unsigned source_slot){
 wf_actor_raw_t *src=&rt->actors[source_slot]; uint32_t sp=ptr_of(source_slot);
 rt->g.global_1692=0x0400u; rt->g.global_168c=0u;
 for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++){
  wf_actor_raw_t *c=&rt->actors[slot]; uint32_t cp=ptr_of(slot);
  if(cp==sp)continue;
  if(!bit(c,WF_A_ACTIVE,7))continue;
  if(wf_u8(c,WF_A_STATE+1)==0xffu)continue;
  if(bit(c,WF_A_FLAGS33,6))continue;
  if(wf_be32(c,WF_A_LINK)!=0u)continue;
  if(bit(c,WF_A_FLAGS32,4)||bit(c,WF_A_FLAGS34,4)||bit(c,WF_A_FLAGS33,2)||bit(c,WF_A_FLAGS32,7))continue;
  if(!bit(c,WF_A_ROLE,7)){
   if(bit(c,WF_A_TARGET,7))continue;
   int carry=1; wf_status_t st=royal_candidate_free_20c88(rt,sp,cp,&carry); if(st!=WF_OK)return st;
   if(carry){rt->g.global_168c=cp;return WF_OK;}
   continue;
  }
  uint16_t m=(uint16_t)((abs16diff(wf_be16(src,WF_A_X),wf_be16(c,WF_A_X))>>1)+abs16diff(wf_be16(src,WF_A_Y),wf_be16(c,WF_A_Y)));
  if(m<rt->g.global_1692){rt->g.global_1692=m;rt->g.global_168c=cp;}
 }
 return WF_OK;
}

/* $20B44 TAG role-target helper. */
static wf_status_t tag_role_target_20b44(wf_runtime_t *rt,unsigned source_slot){
 wf_actor_raw_t *src=&rt->actors[source_slot]; uint32_t sp=ptr_of(source_slot); uint32_t partner=wf_be32(src,WF_A_TEAM_LINK);
 for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++){
  wf_actor_raw_t *a3=&rt->actors[slot]; uint32_t p3=ptr_of(slot); if(!bit(a3,WF_A_ACTIVE,7)||p3==sp||p3==partner)continue;
  wf_actor_raw_t *a4=0; uint32_t p4=0;
  if(bit(src,WF_A_FLAGS33,0)){
   if(!bit(a3,WF_A_FLAGS33,0)){p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED; wf_actor_raw_t *tmp=a3;a3=a4;a4=tmp; uint32_t tp=p3;p3=p4;p4=tp;}
   else {p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;}
   if(bit(a3,WF_A_FLAGS33,6)){
    wf_actor_raw_t *a1=ptr(rt,partner);if(!a1)return WF_BLOCKED;
    if(!bit(a1,WF_A_FLAGS32,0))continue;
   }
   wf_set_be32(src,WF_A_TARGET,p3);return WF_OK;
  }
  if(bit(src,WF_A_FLAGS32,0))continue;
  if(!bit(a3,WF_A_FLAGS33,0)){p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED; wf_actor_raw_t *tmp=a3;a3=a4;a4=tmp; uint32_t tp=p3;p3=p4;p4=tp;}
  else {p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;}
  if(bit(a4,WF_A_FLAGS33,6)){wf_set_be32(src,WF_A_TARGET,p3);return WF_OK;}
  if(bit(src,WF_A_FLAGS33,2))continue;
  if(bit(a4,WF_A_FLAGS33,2)){wf_set_be32(src,WF_A_TARGET,p3);return WF_OK;}
  wf_set_be32(src,WF_A_TARGET,p4);return WF_OK;
 }
 return WF_OK;
}

/* $20CCC exact TAG pair resolver, preserving +$7A/+82 separation. */
static wf_status_t tag_pair_20ccc(wf_runtime_t *rt,unsigned source_slot){
 wf_actor_raw_t *src=&rt->actors[source_slot]; uint32_t sp=ptr_of(source_slot), partner=wf_be32(src,WF_A_TEAM_LINK);
 for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++){
  wf_actor_raw_t *a3=&rt->actors[slot]; uint32_t p3=ptr_of(slot); if(!bit(a3,WF_A_ACTIVE,7)||p3==sp||p3==partner)continue;
  wf_actor_raw_t *a4=0;uint32_t p4=0;
  if(bit(src,WF_A_FLAGS33,0)){
   if(!bit(a3,WF_A_FLAGS33,0)){p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;wf_actor_raw_t*t=a3;a3=a4;a4=t;uint32_t q=p3;p3=p4;p4=q;}
   else {p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;}
   if(bit(src,WF_A_FLAGS33,2)&&!bit(a3,WF_A_FLAGS33,2)){wf_set_be32(src,WF_A_TARGET,p4);wf_set_be32(src,WF_A_TARGET2,p3);return WF_OK;}
  }
  if(!a4){p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;}
  if(bit(a4,WF_A_FLAGS32,0)||!bit(a3,WF_A_FLAGS33,6))goto alternate;
  if(wf_u8(a3,WF_A_STATE+1)==5u && wf_u8(a3,WF_A_SELECTOR_LO)==0x61u)goto primary_a3;
  if(!bit(a3,WF_A_PINFLAGS,1))goto primary_a3;
  {
   uint16_t m3=(uint16_t)(abs16diff(wf_be16(a3,WF_A_X),wf_be16(src,WF_A_X))+abs16diff(wf_be16(a3,WF_A_Y),wf_be16(src,WF_A_Y)));
   uint16_t m4=(uint16_t)(abs16diff(wf_be16(a4,WF_A_X),wf_be16(src,WF_A_X))+abs16diff(wf_be16(a4,WF_A_Y),wf_be16(src,WF_A_Y)));
   if(m4<m3)goto primary_a4; else goto primary_a3;
  }
alternate:
  if(!bit(a3,WF_A_FLAGS33,0)){uint32_t op=wf_be32(a3,WF_A_TEAM_LINK);wf_actor_raw_t*o=ptr(rt,op);if(!o)return WF_BLOCKED;a4=a3;p4=p3;a3=o;p3=op;}
  else {p4=wf_be32(a3,WF_A_TEAM_LINK);a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;}
  if(bit(src,WF_A_FLAGS32,0)||bit(a4,WF_A_FLAGS32,0))goto primary_a4;
  if(!bit(a3,WF_A_FLAGS33,6)){
   if(bit(a3,WF_A_PINFLAGS,0)||bit(a3,WF_A_PINFLAGS,1)||wf_u8(a3,WF_A_STATE+1)!=5u||wf_u8(a3,WF_A_SELECTOR_LO)==0x1du)goto primary_a4;
  }
  if(bit(a4,WF_A_FLAGS33,6))goto primary_a4;
primary_a3: wf_set_be32(src,WF_A_TARGET,p3);wf_set_be32(src,WF_A_TARGET2,p4);return WF_OK;
primary_a4: wf_set_be32(src,WF_A_TARGET,p4);wf_set_be32(src,WF_A_TARGET2,p3);return WF_OK;
 }
 return WF_OK;
}

wf_status_t wf_phase_target_relation_20958(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;(void)user;if(!rt)return WF_BAD_STATE;
 wf_r17_m68k_moveq(&rt->m68k_regs,6u,3);wf_r17_m68k_moveq(&rt->m68k_regs,5u,0);
 /* First ROM loop is exactly four wrestler records. */
 for(unsigned slot=0;slot<4u;slot++,wf_r17_m68k_move_b(&rt->m68k_regs,5u,(uint8_t)slot),wf_r17_m68k_move_w(&rt->m68k_regs,6u,(uint16_t)(3u-slot))){
  wf_actor_raw_t *a=&rt->actors[slot];
  if(!bit(a,WF_A_ACTIVE,7)||bit(a,WF_A_ROLE,6)||bit(a,WF_A_ROLE,7)||bit(a,WF_A_FLAGS34,6)||bit(a,WF_A_FLAGS34,7))continue;
  uint8_t st=wf_u8(a,WF_A_STATE+1);if(st!=0u&&st!=1u&&st!=9u)continue;
  if((rt->g.mode_0161&1u)==0u){wf_status_t s=tag_pair_20ccc(rt,slot);if(s!=WF_OK)return s;continue;}
  wf_status_t s=royal_primary_20e00(rt,slot);if(s!=WF_OK)return s;
  wf_actor_raw_t *c=ptr(rt,rt->g.global_168c);if(!c){if(rt->g.global_168c==0u)continue;return WF_BLOCKED;}
  /* $20A20 supplies the normal candidate, including on the first Royal
   * frame when +7A is still zero. Only the two literal linked-action
   * branches bypass that store. Preserve their original bit5 polarity. */
  uint32_t chosen=rt->g.global_168c;
  if(wf_u8(c,WF_A_STATE+1)==5u){
   if(wf_u8(c,WF_A_SELECTOR_LO)==0x4au)
    chosen=bit(c,WF_A_SELECTOR,5)?wf_be32(c,WF_A_SECONDARY_LINK):wf_be32(c,WF_A_LINK);
   else if(wf_u8(c,WF_A_SELECTOR_LO)==0x48u){
    wf_actor_raw_t*l=ptr(rt,wf_be32(c,WF_A_LINK));if(!l)return WF_BLOCKED;
    uint32_t q=wf_be32(l,WF_A_SECONDARY_LINK);
    if(q&&bit(l,WF_A_SELECTOR,5))chosen=q;
   }
  }
  if(chosen&&!ptr(rt,chosen))return WF_BLOCKED;
  wf_set_be32(a,WF_A_TARGET,chosen);
 }
 wf_r17_m68k_moveq(&rt->m68k_regs,6u,8);
 if((rt->g.mode_0161&1u)==0u){
  for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++,wf_r17_m68k_move_w(&rt->m68k_regs,6u,(uint16_t)(8u-slot))){
   wf_actor_raw_t*a=&rt->actors[slot];if(!bit(a,WF_A_ACTIVE,7))continue;
   uint8_t roles=(uint8_t)(wf_u8(a,WF_A_ROLE)&0xc0u);wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);wf_r17_m68k_move_b(&rt->m68k_regs,0u,roles);if(roles==0u||bit(a,WF_A_CPU_B5,7)||bit(a,WF_A_FLAGS34,4)||!bit(a,WF_A_STATE,7))continue;
   uint8_t st=wf_u8(a,WF_A_STATE+1);if(st==0u){if(!bit(a,WF_A_CPU_B5,3)){wf_status_t s=tag_role_target_20b44(rt,slot);if(s!=WF_OK)return s;}}
   else if(st==1u&&wf_u8(a,WF_A_CPU_SUBSTATE)==0u){wf_status_t s=tag_role_target_20b44(rt,slot);if(s!=WF_OK)return s;}
  }
 }else{
  for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++,wf_r17_m68k_move_w(&rt->m68k_regs,6u,(uint16_t)(8u-slot))){
   wf_actor_raw_t*a=&rt->actors[slot];if(!bit(a,WF_A_ACTIVE,7)||!bit(a,WF_A_ROLE,7)||bit(a,WF_A_FLAGS34,4))continue;
   uint8_t st=wf_u8(a,WF_A_STATE+1);int run=0;
   if(st==0u){if(!bit(a,WF_A_CPU_B5,3))run=1;}
   else if(st==1u&&wf_u8(a,WF_A_CPU_SUBSTATE)==0u&&bit(a,WF_A_CPU_B6,7))run=1;
   if(!run)continue;
   clrbit(a,WF_A_CPU_B6,7);clrbit(a,WF_A_CPU_B6,6);rt->g.global_168c=0u;
   wf_status_t s=royal_secondary_20bda(rt,slot);if(s!=WF_OK)return s;
   if(rt->g.global_168c!=0u){if(!ptr(rt,rt->g.global_168c))return WF_BLOCKED;wf_set_be32(a,WF_A_TARGET,rt->g.global_168c);}else setbit(a,WF_A_CPU_B6,6);
  }
  for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++)clrbit(&rt->actors[slot],WF_A_TARGET,7);
 }
 return WF_OK;
}
