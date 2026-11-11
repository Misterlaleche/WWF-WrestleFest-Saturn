/* Literal actor-memory control flow of original 68000 $20CCC..$20DFE.
 * Included only in the derived wf_target_relation.c; Royal and role-target
 * helpers stay byte-for-byte in the immutable input translation unit. */
static uint16_t tag_word_distance(uint16_t candidate,uint16_t source){
 /* SUB.W / BCC / NEG.W: borrow is unsigned, not signed coordinate distance. */
 uint16_t d=(uint16_t)(candidate-source);
 if(candidate<source)d=(uint16_t)(0u-d);
 return d;
}

static wf_status_t tag_pair_20ccc(wf_runtime_t *rt,unsigned source_slot){
 wf_actor_raw_t *src=&rt->actors[source_slot];
 uint32_t sp=ptr_of(source_slot),partner=wf_be32(src,WF_A_TEAM_LINK);
 for(unsigned slot=0;slot<WF_ACTOR_SLOTS;slot++){
  wf_actor_raw_t *a3=&rt->actors[slot];uint32_t p3=ptr_of(slot);
  if(!bit(a3,WF_A_ACTIVE,7)||p3==sp||p3==partner)continue; /* $20CD4 */
  uint32_t p4=wf_be32(a3,WF_A_TEAM_LINK);
  wf_actor_raw_t *a4=ptr(rt,p4);if(!a4)return WF_BLOCKED;
  /* $20CF6/$20D88 normalize the opposing pair independently in both paths. */
  if(!bit(a3,WF_A_FLAGS33,0)){
   wf_actor_raw_t *t=a3;a3=a4;a4=t;uint32_t q=p3;p3=p4;p4=q;
  }
  if(!bit(src,WF_A_FLAGS33,0))goto source_bit0_clear; /* $20CF2 */
  if(bit(src,WF_A_FLAGS33,2)&&!bit(a3,WF_A_FLAGS33,2))goto primary_a4; /* $20D1A */
  if(bit(a4,WF_A_FLAGS32,0))goto primary_a3; /* $20D24 */
  if(!bit(a3,WF_A_FLAGS33,6))goto primary_a3; /* $20D2E */
  if(wf_u8(a3,WF_A_STATE+1)==5u&&wf_u8(a3,WF_A_SELECTOR_LO)==0x61u)
   goto primary_a3; /* $20D40 */
  if(!bit(a3,WF_A_PINFLAGS,1))goto primary_a4; /* $20D4A */
  {
   uint16_t sx=wf_be16(src,WF_A_X),sy=wf_be16(src,WF_A_Y);
   uint16_t m3=(uint16_t)(tag_word_distance(wf_be16(a3,WF_A_X),sx)+
                          tag_word_distance(wf_be16(a3,WF_A_Y),sy));
   uint16_t m4=(uint16_t)(tag_word_distance(wf_be16(a4,WF_A_X),sx)+
                          tag_word_distance(wf_be16(a4,WF_A_Y),sy));
   if(m4<m3)goto primary_a4; /* $20D84: ties retain A3 */
   goto primary_a3;
  }
source_bit0_clear: /* $20D9C */
  if(bit(src,WF_A_FLAGS32,0)||bit(a4,WF_A_FLAGS32,0))goto primary_a3;
  if(!bit(a3,WF_A_FLAGS33,6)){
   if(bit(a4,WF_A_FLAGS33,6))goto primary_a3; /* $20DDC */
   goto primary_a4;
  }
  if(bit(a3,WF_A_PINFLAGS,0)||bit(a3,WF_A_PINFLAGS,1))goto primary_a3;
  if(wf_u8(a3,WF_A_STATE+1)!=5u)goto primary_a4; /* $20DCA */
  if(wf_u8(a3,WF_A_SELECTOR_LO)==0x1du)goto primary_a3;
  goto primary_a4;
primary_a3: /* $20DE0 */
  wf_set_be32(src,WF_A_TARGET,p3);wf_set_be32(src,WF_A_TARGET2,p4);return WF_OK;
primary_a4: /* $20DEA */
  wf_set_be32(src,WF_A_TARGET,p4);wf_set_be32(src,WF_A_TARGET2,p3);return WF_OK;
 }
 return WF_OK; /* $20DFE: no candidate preserves both existing targets. */
}
