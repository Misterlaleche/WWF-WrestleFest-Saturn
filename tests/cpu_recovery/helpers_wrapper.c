#include "wf_actor.h"
#include "wf_runtime.h"
#include "wf_r17_actions_r11.h"
#include <stddef.h>
#ifndef WF_TEST_PREFIX
#define WF_TEST_PREFIX candidate_
#endif
#define WF_CAT_(a,b) a##b
#define WF_CAT(a,b) WF_CAT_(a,b)
#define F(n) WF_CAT(WF_TEST_PREFIX,n)
uint8_t F(get8)(const wf_actor_raw_t *a,size_t o){return wf_u8(a,o);}
uint16_t F(get16)(const wf_actor_raw_t *a,size_t o){return wf_be16(a,o);}
uint32_t F(get32)(const wf_actor_raw_t *a,size_t o){return wf_be32(a,o);}
void F(set8)(wf_actor_raw_t *a,size_t o,uint8_t v){wf_set_u8(a,o,v);}
void F(set16)(wf_actor_raw_t *a,size_t o,uint16_t v){wf_set_be16(a,o,v);}
void F(set32)(wf_actor_raw_t *a,size_t o,uint32_t v){wf_set_be32(a,o,v);}
uint32_t F(get16_6)(const wf_actor_raw_t *a){return wf_be16(a,6);}
uint32_t F(get32_6)(const wf_actor_raw_t *a){return wf_be32(a,6);}
void F(move)(wf_r17_m68k_shadow_t *r,unsigned op,unsigned reg,uint32_t v){
 switch(op){case 0:wf_r17_m68k_move_b(r,reg,(uint8_t)v);break;
 case 1:wf_r17_m68k_move_w(r,reg,(uint16_t)v);break;
 case 2:wf_r17_m68k_move_l(r,reg,v);break;
 case 3:wf_r17_m68k_moveq(r,reg,(int8_t)v);break;
 case 4:wf_r17_m68k_clr_l(r,reg);break;
 default:wf_r17_m68k_software_reset(r);break;}
}
#define R(x) offsetof(wf_runtime_t,x)
#define B(x) offsetof(wf_r17_actions_r11_bridge_t,x)
size_t F(layout)(size_t *out){
 const size_t v[]={sizeof(wf_actor_raw_t),sizeof(wf_runtime_t),sizeof(wf_r17_actions_r11_bridge_t),
 R(g),R(actors),R(fixed_object0),R(fixed_object1),R(fixed_four_1134),R(referee_aux),R(fixed_four_1194),R(fixed_four_11c4),R(referee),R(transient_1258),R(branchb_list_182a),R(branchb_prev_185a),R(branchb_objs_188a),R(effect_1450),R(compact_14ce),R(m68k_regs),R(bindings),R(binding_count),R(trace),R(trace_count),R(frame_ready),R(vblank_ack),R(frame_stage),R(audio_command),R(audio_user),R(audio_status),
 B(raw23_ctx),B(fixed_object0),B(fixed_object1),B(video_batch),B(video_user),B(render),B(transient_views),B(trace),B(raw23_trace),B(resource_2b58),B(resource_2aea)};
 size_t i;for(i=0;i<sizeof v/sizeof v[0];i++)out[i]=v[i];return i;
}

typedef void (*word_move_fn)(wf_r17_m68k_shadow_t *,unsigned,uint16_t);
word_move_fn F(word_move_address)(void){return wf_r17_m68k_move_w;}
