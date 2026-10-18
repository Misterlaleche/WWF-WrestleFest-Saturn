#include "wf_collision_dispatch.h"
#include "wf_collision.h"
#include "wf_collision_runtime.h"
static int hi_profile_zero(const wf_actor_raw_t*a){return wf_u8(a,WF_A_PROFILE)==0;}
static int orient_equal(const wf_actor_raw_t*a,const wf_actor_raw_t*b){return wf_u8(a,WF_A_FACING)==wf_u8(b,WF_A_FACING);}
static int incoming_zero_or_0a(const wf_actor_raw_t*a){uint16_t w=wf_be16(a,WF_A_STATE4);return w==0u||w==0x000Au;}
static void to_state4_word(wf_actor_raw_t*a,uint16_t w){wf_set_be16(a,WF_A_STATE,4);wf_state4_write_word(a,w);}
static void norm(wf_collision_pair_ctx_t*c){wf_collision_normalize_receiver(c->src,c->dst);}
static uint8_t retained_source_profile(const wf_collision_pair_ctx_t*c){
 return c->source_profile_id<34u?c->source_profile_id:wf_collision_profile_id(c->src);
}
static void copy_profile_damage(wf_collision_pair_ctx_t*c){
 uint8_t p=retained_source_profile(c);if(p<34u)wf_collision_write_profile_damage(c->dst,wf_collision_profiles[p].pending_damage);
}
/* ROM $24DDC: BTST #4,+$33(A1); BEQ skip; ADDI.W #1,+$68(A1). */
static void pending_plus1_24ddc(wf_collision_pair_ctx_t*c){
 if((wf_u8(c->dst,WF_A_FLAGS33)&0x10u)!=0u)wf_set_be16(c->dst,WF_A_PENDING_DAMAGE,(uint16_t)(wf_be16(c->dst,WF_A_PENDING_DAMAGE)+1u));
}
wf_coll_result_t wf_collision_first_apply(uint8_t s,wf_collision_pair_ctx_t*c){
 wf_actor_raw_t*d=c->dst,*a=c->src; uint8_t hp=wf_u8(d,WF_A_PROFILE), f33=wf_u8(d,WF_A_FLAGS33);
 switch(s){
 case 0x00:return WF_COLL_ACCEPT;
 case 0x01:
  if(hp&0x40u){return WF_COLL_REJECT;}
  copy_profile_damage(c);to_state4_word(d,0x0000);
  if((hp&0x80u)==0u){if((f33&0x10u)||wf_be16(d,WF_A_TIMER52)>=3u){wf_state4_write_word(d,0x0002);}norm(c);}
  pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x02:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  to_state4_word(d,0x0002);norm(c);return WF_COLL_ACCEPT;
 case 0x03:
  if(!hi_profile_zero(d)||(f33&0x10u)==0u||wf_u8(a,WF_A_FACING)==wf_u8(d,WF_A_FACING))return WF_COLL_REJECT;
  copy_profile_damage(c);to_state4_word(d,0x000E);return WF_COLL_ACCEPT;
 case 0x04:
  if(!hi_profile_zero(d))return WF_COLL_REJECT;
  copy_profile_damage(c);wf_set_be16(a,WF_A_PROFILE,0u);
  to_state4_word(d,(((wf_u8(a,WF_A_ANGLE)&0x80u)!=wf_u8(d,WF_A_FACING))?0x000Bu:0x0010u));
  pending_plus1_24ddc(c);if(wf_u8(a,0x03u)==0u||wf_u8(a,0x03u)==5u)wf_set_be16(d,WF_A_PENDING_DAMAGE,(uint16_t)(wf_be16(d,WF_A_PENDING_DAMAGE)+3u));return WF_COLL_ACCEPT;
 case 0x05:case 0x0C:case 0x0D:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  copy_profile_damage(c);to_state4_word(d,(f33&0x10u)?0x0003u:0x0002u);norm(c);pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x06:
  copy_profile_damage(c);to_state4_word(d,(f33&0x10u)?0x0004u:0x0003u);norm(c);pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x07:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  copy_profile_damage(c);to_state4_word(d,wf_u8(a,WF_A_SELECTOR_LO)==0x41u?0x001Du:0x0014u);norm(c);pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x08:
  copy_profile_damage(c);to_state4_word(d,0x000A);
  if((hp&0x80u)==0u){if((f33&0x10u)||wf_be16(d,WF_A_TIMER52)>=3u){wf_state4_write_word(d,0x0002);}norm(c);}
  pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x09:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  copy_profile_damage(c);wf_set_be16(a,WF_A_PROFILE,0u);to_state4_word(d,0x0011);norm(c);return WF_COLL_ACCEPT;
 case 0x0A:case 0x0E:
  if(!hi_profile_zero(d)||(f33&0x10u)==0u||c->target_cursor_d1==wf_be16(d,WF_A_FACING))return WF_COLL_REJECT;
  copy_profile_damage(c);to_state4_word(d,retained_source_profile(c)==0x1Bu?0x0028u:0x001Fu);wf_set_be16(a,WF_A_PROFILE,0u);return WF_COLL_ACCEPT;
 case 0x0B:
  if(hp&0x80u){return WF_COLL_REJECT;}
  copy_profile_damage(c);wf_set_be16(a,WF_A_PROFILE,0u);to_state4_word(d,0x0012);
  if(c->target_profile_id!=0x1Au){norm(c);}
  pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 default:return WF_COLL_BLOCKED;
 }
}
wf_coll_result_t wf_collision_second_apply(uint8_t s,wf_collision_pair_ctx_t*c){
 wf_actor_raw_t*d=c->dst,*l=c->linked;
 switch(s){
 case 0x00:norm(c);wf_set_be16(d,WF_A_TIMER44,0);return WF_COLL_ACCEPT;
 case 0x01:
  if(orient_equal(d,c->src))return WF_COLL_BLOCKED;
  if(incoming_zero_or_0a(d)&&wf_be16(d,WF_A_TIMER52)<3u)return WF_COLL_BLOCKED;
  to_state4_word(d,0x0002);return WF_COLL_ACCEPT;
 case 0x02:
  if(!orient_equal(d,c->src)){return WF_COLL_BLOCKED;}
  if(!l){return WF_COLL_BLOCKED;}
  wf_set_be16(l,WF_A_STATE,0);to_state4_word(d,0x000B);return WF_COLL_ACCEPT;
 case 0x03:
  if(orient_equal(d,c->src))return WF_COLL_BLOCKED;
  if(incoming_zero_or_0a(d)&&wf_be16(d,WF_A_TIMER52)<3u)return WF_COLL_BLOCKED;
  to_state4_word(d,0x0002);return WF_COLL_ACCEPT;
 case 0x04:
  if(!orient_equal(d,c->src)||!l){return WF_COLL_BLOCKED;}
  to_state4_word(l,0x0001);to_state4_word(d,0x0002);norm(c);return WF_COLL_ACCEPT;
 case 0x05:
  /* $24A90: CLR.W +$74(A1); A2=+$76(A1); MOVE.W #2,+$1C(A2);
   * CLR.L +$76(A1); CLR.W +$44(A1).  No subtype write. */
  if(!l)return WF_COLL_BLOCKED;
  wf_set_be16(d,WF_A_REL_FLAGS,0u);
  wf_set_be16(l,WF_A_LATCHED,0x0002u);
  wf_set_be32(d,WF_A_REL_LINK,0u);
  wf_set_be16(d,WF_A_TIMER44,0u);
  return WF_COLL_ACCEPT;
 case 0x06:
  if(!incoming_zero_or_0a(d)||!l){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0002);norm(c);wf_set_be16(l,WF_A_STATE,7);return WF_COLL_ACCEPT;
 case 0x07:
  if(!incoming_zero_or_0a(d)||!l){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0002);norm(c);wf_set_be16(l,WF_A_STATE,0);wf_set_be16(c->src,WF_A_COLL_CACHE,0xFFFF);return WF_COLL_ACCEPT;
 case 0x08:
  if(wf_state4_subtype(d)!=0x12u){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0012);return WF_COLL_ACCEPT;
 case 0x09:
  if(!incoming_zero_or_0a(d)||!l){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0002);norm(c);to_state4_word(l,0x0009);return WF_COLL_ACCEPT;
 case 0x0A:
  if(!incoming_zero_or_0a(d)||!l){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0002);norm(c);to_state4_word(l,0x000B);wf_set_be16(l,0x9A,8);return WF_COLL_ACCEPT;
 case 0x0B:
  if(!incoming_zero_or_0a(d)||!l){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0002);norm(c);wf_set_be16(l,WF_A_STATE,7);return WF_COLL_ACCEPT;
 case 0x0C:
  if(wf_state4_subtype(d)!=0x0Au){return WF_COLL_BLOCKED;}
  to_state4_word(d,0x0002);norm(c);return WF_COLL_ACCEPT;
 default:return WF_COLL_BLOCKED;
 }
}
wf_coll_result_t wf_collision_profile_pair_apply_rows(uint8_t ps,uint8_t pt,wf_collision_pair_ctx_t*c){
 if(!c||ps>=34u||pt>=34u){return WF_COLL_BLOCKED;}
 wf_actor_raw_t src_before=*c->src, dst_before=*c->dst, linked_before;
 int have_linked=(c->linked!=0); if(have_linked){linked_before=*c->linked;}
 c->source_profile_id=ps;c->target_profile_id=pt;
 uint8_t f=wf_collision_profiles[ps].first_selector,s=wf_collision_profiles[pt].second_selector;
 wf_coll_result_t r=wf_collision_first_apply(f,c);
 if(r!=WF_COLL_ACCEPT){return r;}
 r=wf_collision_second_apply(s,c);
 if(r==WF_COLL_BLOCKED){*c->src=src_before;*c->dst=dst_before;if(have_linked){*c->linked=linked_before;}}
 return r;
}
wf_coll_result_t wf_collision_profile_pair_apply(wf_collision_pair_ctx_t*c){
 if(!c)return WF_COLL_BLOCKED;
 return wf_collision_profile_pair_apply_rows(wf_collision_profile_id(c->src),wf_collision_profile_id(c->dst),c);
}
