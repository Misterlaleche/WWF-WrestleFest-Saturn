#include "wf_collision_dispatch.h"
#include "wf_collision.h"
#include "wf_collision_runtime.h"
#include "wf_runtime.h"
#include "wf_r17_actions_r11.h"
static int hi_profile_zero(const wf_actor_raw_t*a){return wf_u8(a,WF_A_PROFILE)==0;}
static int orient_equal(const wf_actor_raw_t*a,const wf_actor_raw_t*b){return wf_u8(a,WF_A_FACING)==wf_u8(b,WF_A_FACING);}
static int incoming_zero_or_0a(const wf_actor_raw_t*a){uint16_t w=wf_be16(a,WF_A_STATE4);return w==0u||w==0x000Au;}
static void to_state4_word(wf_actor_raw_t*a,uint16_t w){wf_set_be16(a,WF_A_STATE,4);wf_state4_write_word(a,w);}
static void norm(wf_collision_pair_ctx_t*c){wf_collision_normalize_receiver(c->src,c->dst);}
static uint8_t retained_source_profile(const wf_collision_pair_ctx_t*c){
 return c->source_profile_id<34u?c->source_profile_id:wf_collision_profile_id(c->src);
}
/* $212D4 can dereference the partner and, depending on flags, the linked
 * wrestler's partner. Check every consumed pointer before mutating the pair. */
static int cleanup_tag_safe(wf_runtime_t*rt,wf_actor_raw_t*a){
 if(!(wf_u8(a,WF_A_FLAGS33)&1u))return 1;
 wf_actor_raw_t*p=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TEAM_LINK),NULL);
 if(!p)return 0;
 if(!(wf_u8(p,WF_A_FLAGS32)&1u)){
  wf_actor_raw_t*t=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),NULL);
  if(!t||!wf_actor_from_rom_ptr(rt,wf_be32(t,WF_A_TEAM_LINK),NULL))return 0;
 }
 return 1;
}
static int cleanup_212a0_safe(wf_runtime_t*rt,wf_actor_raw_t*a){
 if(!rt->g.credited_route)return 1;
 if(rt->g.mode_0161&1u){
  uint32_t ptr=wf_be32(a,WF_A_RESERVE);
  if(!ptr)return 1;
  if(!wf_actor_from_rom_ptr(rt,ptr,NULL))return 0;
  if(wf_u8(a,WF_A_SELECTOR_LO)==0x53u){
   wf_actor_raw_t*t=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),NULL);
   if(!t)return 0;
   ptr=wf_be32(t,WF_A_RESERVE);
   if(!ptr||!wf_actor_from_rom_ptr(rt,ptr,NULL))return 0;
  }
  return 1;
 }
 wf_actor_raw_t*t=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),NULL);
 return t&&cleanup_tag_safe(rt,a)&&cleanup_tag_safe(rt,t);
}
/* $24900 emits a TAG-only notification unless A1's partner is A0. */
static int collision_24900(wf_collision_pair_ctx_t*c){
 if(!c->runtime)return 0;
 if(c->runtime->g.mode_0161&1u)return 1;
 unsigned slot=0;while(slot<WF_ACTOR_SLOTS&&c->src!=&c->runtime->actors[slot])slot++;
 if(slot>=WF_ACTOR_SLOTS)return 0;
 if(wf_be32(c->dst,WF_A_TEAM_LINK)!=wf_actor_rom_base(slot)){
  c->runtime->g.global_15d2=15u;c->runtime->g.global_15d3=25u;
 }
 return 1;
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
  wf_runtime_emit_audio(c->runtime,0x2au);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);to_state4_word(d,0x0000);
  if((hp&0x80u)==0u){if((f33&0x10u)||wf_be16(d,WF_A_TIMER52)>=3u){wf_state4_write_word(d,0x0002);}norm(c);pending_plus1_24ddc(c);}
  return WF_COLL_ACCEPT;
 case 0x02:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  wf_runtime_emit_audio(c->runtime,0x2bu);
  /* $2447A tests the receiver's state BYTE before either actor is written. */
  if(wf_u8(d,WF_A_STATE+1u)==2u){to_state4_word(a,0x0002u);wf_set_be16(a,0x9au,80u);}
  else to_state4_word(a,0x0018u);
  to_state4_word(d,0x0002u);wf_set_be16(d,0x9au,80u);norm(c);
  wf_set_u8(a,WF_A_LINK,(uint8_t)(wf_u8(a,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;
 case 0x03:
  if(!hi_profile_zero(d)||(f33&0x10u)==0u||wf_u8(a,WF_A_FACING)==wf_u8(d,WF_A_FACING))return WF_COLL_REJECT;
  copy_profile_damage(c);to_state4_word(d,0x000E);return WF_COLL_ACCEPT;
 case 0x04:
  if(!hi_profile_zero(d))return WF_COLL_REJECT;
  wf_runtime_emit_audio(c->runtime,0x2bu);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);wf_set_be16(a,WF_A_PROFILE,0u);
  to_state4_word(d,(((wf_u8(a,WF_A_ANGLE)&0x80u)!=wf_u8(d,WF_A_FACING))?0x000Bu:0x0010u));
  pending_plus1_24ddc(c);if(wf_u8(a,0x03u)==0u||wf_u8(a,0x03u)==5u)wf_set_be16(d,WF_A_PENDING_DAMAGE,(uint16_t)(wf_be16(d,WF_A_PENDING_DAMAGE)+3u));return WF_COLL_ACCEPT;
 case 0x05:case 0x0C:case 0x0D:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  wf_runtime_emit_audio(c->runtime,0x2bu);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);to_state4_word(d,(f33&0x10u)?0x0003u:0x0002u);norm(c);pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x06:
  wf_runtime_emit_audio(c->runtime,0x2eu);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);to_state4_word(d,(f33&0x10u)?0x0004u:0x0003u);norm(c);pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x07:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  wf_runtime_emit_audio(c->runtime,0x2bu);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);to_state4_word(d,wf_u8(a,WF_A_SELECTOR_LO)==0x41u?0x001Du:0x0014u);norm(c);pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 case 0x08:
  wf_runtime_emit_audio(c->runtime,0x2bu);
  copy_profile_damage(c);to_state4_word(d,0x000A);
  if((hp&0x80u)==0u){if((f33&0x10u)||wf_be16(d,WF_A_TIMER52)>=3u){wf_state4_write_word(d,0x0002);}norm(c);pending_plus1_24ddc(c);}
  return WF_COLL_ACCEPT;
 case 0x09:
  if(!hi_profile_zero(d)){return WF_COLL_REJECT;}
  wf_runtime_emit_audio(c->runtime,0x2bu);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);wf_set_be16(a,WF_A_PROFILE,0u);to_state4_word(d,0x0011);norm(c);return WF_COLL_ACCEPT;
 case 0x0A:case 0x0E:
  if(!hi_profile_zero(d)||(f33&0x10u)==0u||c->target_cursor_d1==wf_be16(d,WF_A_FACING))return WF_COLL_REJECT;
  wf_runtime_emit_audio(c->runtime,0x29u);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);to_state4_word(d,retained_source_profile(c)==0x1Bu?0x0028u:0x001Fu);wf_set_be16(a,WF_A_PROFILE,0u);return WF_COLL_ACCEPT;
 case 0x0B:
  if(hp&0x80u){return WF_COLL_REJECT;}
  wf_runtime_emit_audio(c->runtime,0x2bu);wf_runtime_emit_audio(c->runtime,0x32u);
  copy_profile_damage(c);wf_set_be16(a,WF_A_PROFILE,0u);to_state4_word(d,0x0012);
  if(c->target_profile_id!=0x1Au){norm(c);}
  pending_plus1_24ddc(c);return WF_COLL_ACCEPT;
 default:return WF_COLL_BLOCKED;
 }
}
wf_coll_result_t wf_collision_second_apply(uint8_t s,wf_collision_pair_ctx_t*c){
 wf_actor_raw_t*d=c->dst,*l=c->linked;
 switch(s){
 case 0x00:
  /* ROM $24852/$24858 tag both links before the composed-sprite cleanup. */
  wf_set_u8(c->src,WF_A_LINK,(uint8_t)(wf_u8(c->src,WF_A_LINK)|0x80u));
  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));
  wf_set_be16(d,WF_A_TIMER44,0u);norm(c);return WF_COLL_ACCEPT;
 case 0x01:
  /* $24874 BEQ $248F8: equal facing resumes state5 directly. The linked
   * actor is loaded into A2 but is never dereferenced on this branch. */
  if(orient_equal(d,c->src)){wf_set_be16(d,WF_A_STATE,0x8005u);return WF_COLL_ACCEPT;}
  /* $24878..$248F6: interrupt the current linked action, or reverse it
   * for the two original low-timer subtypes. Check consumed pointers first. */
  if(!l||!c->runtime)return WF_COLL_BLOCKED;
  {int trigger=incoming_zero_or_0a(d);
   int low=trigger&&wf_be16(d,WF_A_TIMER52)<3u;
   if(!low&&!cleanup_212a0_safe(c->runtime,d))return WF_COLL_BLOCKED;
   if(trigger&&!collision_24900(c))return WF_COLL_BLOCKED;
   wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x40u));
   wf_set_u8(l,WF_A_FLAGS33,(uint8_t)(wf_u8(l,WF_A_FLAGS33)&~0x40u));
   wf_set_u8(d,WF_A_PINFLAGS,(uint8_t)(wf_u8(d,WF_A_PINFLAGS)&~0x04u));
   wf_set_u8(l,WF_A_PINFLAGS,(uint8_t)(wf_u8(l,WF_A_PINFLAGS)&~0x02u));
   if(low){
    wf_set_be16(d,WF_A_STATE,5u);wf_set_be16(d,WF_A_SELECTOR,0xc052u);
    wf_set_be16(d,WF_A_TIMER44,1u);wf_set_be16(l,WF_A_STATE,5u);
    wf_set_be16(l,WF_A_SELECTOR,0xc065u);return WF_COLL_ACCEPT;
   }
   if(trigger)to_state4_word(d,0x0002u);
   wf_set_be16(l,WF_A_STATE,5u);wf_set_be16(l,WF_A_SELECTOR,0x006fu);
   {uint32_t blocked_rom=0;
    if(wf_r17_action_cleanup_212a0(c->runtime,d,&blocked_rom)!=WF_OK)return WF_COLL_BLOCKED;}
   wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;
  }
 case 0x02:
  /* $24930->$2497C: unequal facing BYTEs resume state5, retaining subtype. */
  if(!orient_equal(d,c->src)){wf_set_be16(d,WF_A_STATE,0x8005u);return WF_COLL_ACCEPT;}
  /* $24932..$2497A interrupts the linked actor and clears hold flags.
   * $24D7A's optional subtype normalization is overwritten by the $000B
   * write before any subsequent consumer; it preserves D0/D1. */
  if(!l||!c->runtime)return WF_COLL_BLOCKED;
  if(!cleanup_212a0_safe(c->runtime,l)||!collision_24900(c))return WF_COLL_BLOCKED;
  wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x40u));
  wf_set_u8(l,WF_A_FLAGS33,(uint8_t)(wf_u8(l,WF_A_FLAGS33)&~0x40u));
  wf_set_u8(l,WF_A_PINFLAGS,(uint8_t)(wf_u8(l,WF_A_PINFLAGS)&~0x04u));
  wf_set_u8(d,WF_A_PINFLAGS,(uint8_t)(wf_u8(d,WF_A_PINFLAGS)&~0x02u));
  wf_set_be16(l,WF_A_STATE,0u);to_state4_word(d,0x000bu);
  {uint32_t blocked_rom=0;
   if(wf_r17_action_cleanup_212a0(c->runtime,l,&blocked_rom)!=WF_OK)return WF_COLL_BLOCKED;}
  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;
 case 0x03:
  /* $2498C->$24A30: equal facing BYTEs resume state5, retaining subtype. */
  if(orient_equal(d,c->src)){wf_set_be16(d,WF_A_STATE,0x8005u);return WF_COLL_ACCEPT;}
  if(!l||!c->runtime)return WF_COLL_BLOCKED;
  if(incoming_zero_or_0a(d)&&wf_be16(d,WF_A_TIMER52)<3u){
   /* $24990..$24A04: opposite-facing low-timer reversal. The first
    * callback has already selected subtype 0 or $0A. */
   if(!collision_24900(c))return WF_COLL_BLOCKED;
   wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x40u));
   wf_set_u8(l,WF_A_FLAGS33,(uint8_t)(wf_u8(l,WF_A_FLAGS33)&~0x40u));
   uint16_t type=wf_be16(d,WF_A_STATE4);
   wf_set_be16(d,WF_A_STATE,5u);wf_set_be16(d,WF_A_SELECTOR,0xc053u);
   wf_set_be16(d,WF_A_TIMER44,type==0u?1u:2u);
   wf_set_be16(l,WF_A_STATE,5u);wf_set_be16(l,WF_A_SELECTOR,type==0u?0xc073u:0xc074u);
   return WF_COLL_ACCEPT;
  }
  if(!cleanup_212a0_safe(c->runtime,d)||!collision_24900(c))return WF_COLL_BLOCKED;
  wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x40u));
  wf_set_u8(l,WF_A_FLAGS33,(uint8_t)(wf_u8(l,WF_A_FLAGS33)&~0x40u));
  to_state4_word(d,0x0002);wf_set_be16(l,WF_A_STATE,5u);wf_set_be16(l,WF_A_SELECTOR,0x006fu);
  {uint32_t blocked_rom=0;
   if(wf_r17_action_cleanup_212a0(c->runtime,d,&blocked_rom)!=WF_OK)return WF_COLL_BLOCKED;}
  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;
 case 0x04:
  /* $24A40 BNE $24A88: opposite-facing targets resume state 5.
   * This path never dereferences the link at +$26. */
  if(!orient_equal(d,c->src)){wf_set_be16(d,WF_A_STATE,0x8005u);return WF_COLL_ACCEPT;}
  /* $24A42..$24A86 clears the hold, notifies and cleans the linked actor. */
  if(!l||!c->runtime)return WF_COLL_BLOCKED;
  if(!cleanup_212a0_safe(c->runtime,l))return WF_COLL_BLOCKED;
  if(!collision_24900(c))return WF_COLL_BLOCKED;
  wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x40u));
  wf_set_u8(l,WF_A_FLAGS33,(uint8_t)(wf_u8(l,WF_A_FLAGS33)&~0x40u));
  to_state4_word(l,0x0001);to_state4_word(d,0x0002);
  wf_set_be16(d,WF_A_TIMER44,0u);norm(c);
  {uint32_t blocked_rom=0;
   if(wf_r17_action_cleanup_212a0(c->runtime,l,&blocked_rom)!=WF_OK)return WF_COLL_BLOCKED;}
  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;
 case 0x05:
  /* $24A90: CLR.W +$74(A1); A2=+$76(A1); MOVE.W #2,+$1C(A2);
   * CLR.L +$76(A1); CLR.W +$44(A1).  No subtype write. */
  /* The relation object at +$76 is distinct from the grapple link at +$26.
   * Resolve it before changing either participant; direct callback callers
   * without a runtime may still supply the already-resolved object. */
  if(c->runtime){
   uint32_t relation=wf_be32(d,WF_A_REL_LINK)&0x00ffffffu;
   /* Ringside relations can point to either fixed object, not just a
    * wrestler. $24A98 writes the same latched word through the 24-bit bus. */
   if(relation==0x001c0f1cu)l=&c->runtime->fixed_object0;
   else if(relation==0x001c1028u)l=&c->runtime->fixed_object1;
   else l=wf_actor_from_rom_ptr(c->runtime,relation,0);
  }
  if(!l)return WF_COLL_BLOCKED;
  wf_set_be16(d,WF_A_REL_FLAGS,0u);
  wf_set_be16(l,WF_A_LATCHED,0x0002u);
  wf_set_be32(d,WF_A_REL_LINK,0u);
  wf_set_be16(d,WF_A_TIMER44,0u);
  return WF_COLL_ACCEPT;
 case 0x06:
  /* $24AA8: a normal hit interrupts the linked action; a different
   * incoming subtype restores the attacker's latched state instead. */
  if(!c->runtime)return WF_COLL_BLOCKED;
  if(!incoming_zero_or_0a(d)){
   wf_set_be16(c->src,WF_A_STATE,(uint16_t)(wf_be16(c->src,WF_A_LATCHED)&0x80ffu));
   wf_set_be16(d,WF_A_STATE,0x8005u);
   c->runtime->g.global_15d2=0u;c->runtime->g.global_15d3=0u;
   wf_set_be16(d,WF_A_TIMER44,0u);return WF_COLL_ACCEPT;
  }
  if(!l||!cleanup_212a0_safe(c->runtime,l)||!collision_24900(c))return WF_COLL_BLOCKED;
  to_state4_word(d,0x0002u);norm(c);wf_set_be16(l,WF_A_STATE,7u);
  wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x40u));
  wf_set_u8(l,WF_A_FLAGS33,(uint8_t)(wf_u8(l,WF_A_FLAGS33)&~0x40u));
  wf_set_u8(d,WF_A_PINFLAGS,(uint8_t)(wf_u8(d,WF_A_PINFLAGS)&~0x02u));
  wf_set_u8(l,WF_A_PINFLAGS,(uint8_t)(wf_u8(l,WF_A_PINFLAGS)&~0x04u));
  {uint32_t blocked_rom=0;
   if(wf_r17_action_cleanup_212a0(c->runtime,l,&blocked_rom)!=WF_OK)return WF_COLL_BLOCKED;}
  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;
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
  /* $24D64->$24D78 is an accepted no-op unless the subtype BYTE is $0A. */
  if(wf_state4_subtype(d)!=0x0Au){return WF_COLL_ACCEPT;}
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
