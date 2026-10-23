#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_core_phases.h"
#include "wf_payment.h"
#include "wf_spatial.h"
#include "wf_target_relation.h"
#include "wf_pair_relation.h"
#include "wf_actor_prepass.h"
#include "wf_r17_referee.h"
#include "wf_r17_contract.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_r17_dispatch_1c03e.h"
#include "wf_physics.h"
#include "wf_r17_ringside_fdee.h"
#include "wf_r17_fixed_four_10120.h"
#include "wf_r17_transient_reset_10e6a.h"
#include "wf_r17_effects_7a00.h"
#include "wf_r17_post7a00.h"

static wf_status_t f4c2_audio_emit(uint16_t command,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;
 if(!c||!c->f4c2_active_rt||!c->audio_command)return WF_BLOCKED;
 return c->audio_command(c->f4c2_active_rt,command,c->r17_owner_user);
}
static void f4c2_sync_raw23_in(wf_runtime_t *rt,wf_gameplay_binding_context_t*c){
 wf_r17_raw23_ctx_t*x=&c->f4c2_raw23_ctx;
 x->mode_0161=rt->g.mode_0161;x->global_15d2=rt->g.global_15d2;x->global_15d3=rt->g.global_15d3;x->shake_1800=rt->g.global_1800;
 x->referee_1150=rt->g.global_1150;x->referee_1180=rt->g.global_1180;x->referee_11b0=rt->g.global_11b0;x->referee_11e0=rt->g.global_11e0;
 x->referee_1151=(uint8_t)rt->g.global_1150;x->referee_11b1=(uint8_t)rt->g.global_11b0;
 x->audio_emit=f4c2_audio_emit;x->special_215b6=0;x->user=c;
}
static void f4c2_sync_raw23_out(wf_runtime_t *rt,wf_gameplay_binding_context_t*c){
 wf_r17_raw23_ctx_t*x=&c->f4c2_raw23_ctx;
 rt->g.global_15d2=x->global_15d2;rt->g.global_15d3=x->global_15d3;rt->g.global_1800=x->shake_1800;
 rt->g.global_1150=x->referee_1150;rt->g.global_1180=x->referee_1180;rt->g.global_11b0=x->referee_11b0;rt->g.global_11e0=x->referee_11e0;
}
static wf_status_t f4c2_dispatch_hook(wf_runtime_t *rt,wf_actor_raw_t *a,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_status_t s;
 if(!rt||!a||!c)return WF_BAD_STATE;
 wf_r17_render_ctx_t action_render={0};
 action_render.scroll_a_x=(uint16_t)c->camera.scroll0_x;action_render.scroll_a_y=(uint16_t)c->camera.scroll0_y;
 action_render.queue=&c->render_queue;action_render.visibility_2188=wf_r17_visibility_2188_exact;
 c->f4c2_actions.render=&action_render;
 c->f4c2_active_rt=rt;f4c2_sync_raw23_in(rt,c);
 c->f4c2_actions.raw23_ctx=&c->f4c2_raw23_ctx;c->f4c2_actions.fixed_object0=&rt->fixed_object0;c->f4c2_actions.fixed_object1=&rt->fixed_object1;c->f4c2_dispatch.actions=&c->f4c2_actions;
 c->f4c2_actions.video_batch=c->post7_video_batch;c->f4c2_actions.video_user=c->r17_owner_user;
 int raw23=wf_u8(a,WF_A_LATCHED+1u)==5u&&wf_u8(a,WF_A_SELECTOR_LO)==0x23u;
 s=wf_r17_dispatch_1c03e_bounded(rt,a,&c->f4c2_dispatch);if(raw23)f4c2_sync_raw23_out(rt,c);c->f4c2_active_rt=0;
 if(s==WF_OK&&c->f4c2_dispatch.extra_render_actor){
  wf_r17_render_ctx_t render={0};
  render.scroll_a_x=(uint16_t)c->camera.scroll0_x;
  render.scroll_a_y=(uint16_t)c->camera.scroll0_y;
  render.queue=&c->render_queue;
  render.visibility_2188=wf_r17_visibility_2188_exact;
  s=wf_r17_world_to_screen_247c(c->f4c2_dispatch.extra_render_actor,
                                 render.scroll_a_x,render.scroll_a_y);
  if(s==WF_OK)s=wf_r17_queue_27b8(&render,c->f4c2_dispatch.extra_render_actor);
  if(s!=WF_OK)c->last_blocked_dependency=0x00012592u;
 }
 if(s!=WF_OK&&c->last_blocked_dependency==0u){c->last_blocked_dependency=c->f4c2_dispatch.blocked_dependency_rom?c->f4c2_dispatch.blocked_dependency_rom:0x0001c03eu;}
 return s;
}
static wf_status_t f4c2_postqueue_2208(wf_runtime_t *rt,wf_actor_raw_t *a,void *user){
 (void)rt;(void)user;if(!a)return WF_BAD_STATE;if(wf_u8(a,WF_A_MOTION_MODE)>2u)return WF_BLOCKED;wf_physics_integrate_common(a);return WF_OK;
}

static uint8_t edge_from_cabinet_word(uint16_t raw,uint8_t *level){
 uint8_t now=(uint8_t)(((uint16_t)(~raw)&0x0080u)>>7);
 uint8_t old=*level;
 uint8_t edge=(uint8_t)((old&now)^now); /* exact AND.B then EOR.B sequence */
 *level=now;
 return edge;
}
static void clear_refill_words(wf_actor_raw_t *a){
 for(size_t o=0xE8u;o<=0xFCu;o+=2u)wf_set_be16(a,o,0u);
}
static void refill_actor(wf_actor_raw_t *a){
 uint16_t m=wf_be16(a,WF_A_STAMINA_MAX);
 wf_set_be16(a,WF_A_STAMINA,m);
 wf_set_be16(a,WF_A_VISIBLE_DELTA,m);
 wf_set_be16(a,WF_A_TIER,0u);
}

wf_status_t wf_phase_input_real(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;wf_gameplay_binding_context_t *c=(wf_gameplay_binding_context_t*)user;if(!rt||!c)return WF_BAD_STATE;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(i+1u));
  wf_actor_raw_t *a=&rt->actors[i];if((wf_u8(a,WF_A_ACTIVE)&0x80u)==0u)continue;
  if((wf_u8(a,WF_A_ROLE)&0x80u)!=0u)continue; /* $5170 */
  if((c->input_valid_mask&(uint16_t)(1u<<i))==0u){c->last_blocked_phase=0x00514eu;c->last_blocked_dependency=0x00514eu;return WF_BLOCKED;}
  uint16_t nibble=(uint16_t)(((uint8_t)~c->input_hw_low[i])>>4);
  uint16_t d2=(uint16_t)((nibble<<5)&0x100u);
  uint16_t d1=(uint16_t)((nibble&7u)|d2);
  wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);
  if((rt->g.pipeline_mode&1u)!=0u){d1=(uint16_t)(d1|wf_be16(a,WF_A_INPUT_ACC));wf_r17_m68k_move_w(&rt->m68k_regs,3u,d1);}
  else d1=wf_be16(a,WF_A_INPUT_ACC);
  wf_r17_m68k_move_w(&rt->m68k_regs,1u,d1);
  wf_input_decode_514e(a,c->input_hw_low[i],rt->g.pipeline_mode&1u);
 }
 return WF_OK;
}

/* Exact gameplay dependency $8B3A/$8C3E.  Payment itself is the audited
 * WP19 $055A routine, not a free-play shortcut.  Only the first four wrestler
 * records are scanned, matching D1=3 + DBF and stride $010C. */
wf_status_t wf_phase_refill_8b3a(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;
 if(!rt||!c)return WF_BAD_STATE;
 if(rt->g.global_16c4!=0u)return WF_OK;
 if(!c->cabinet_words_valid){c->last_blocked_phase=0x008b3au;c->last_blocked_dependency=0x00140020u;return WF_BLOCKED;}
 rt->g.cabinet_edge_15da=edge_from_cabinet_word(c->cabinet_word_140020,&rt->g.cabinet_level_15db);
 rt->g.cabinet_edge_15dc=edge_from_cabinet_word(c->cabinet_word_140022,&rt->g.cabinet_level_15dd);
 wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
 wf_r17_m68k_moveq(&rt->m68k_regs,1u,3);
 for(unsigned slot=0;slot<4u;slot++,wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(3u-slot))){
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(rt->g.config_0066&0x800u));
  wf_actor_raw_t*a=&rt->actors[slot];
  unsigned d1=3u-slot;
  int gate=0;
  if((rt->g.config_0066&0x0800u)!=0x0800u || (rt->g.mode_0161&0x01u)!=0u || (rt->g.mode_0160&0x80u)==0u){
   gate=wf_u8(a,WF_A_INPUT_FRESH)!=0u; /* TST.B +$A2: high byte of fresh WORD */
  }else if(d1>=2u){
   if(wf_be32(a,WF_A_INPUT_SOURCE)==0x00140020u)gate=wf_u8(a,WF_A_INPUT_FRESH)!=0u;
   else if(rt->g.cabinet_edge_15dc!=0u)gate=1;
  }else{
   if(wf_be32(a,WF_A_INPUT_SOURCE)==0x00140022u){if(rt->g.cabinet_edge_15dc!=0u)gate=1;}
   else if(rt->g.cabinet_edge_15da!=0u)gate=1;
  }
  if(!gate)continue;
  if((wf_u8(a,WF_A_FLAGS32)&0x80u)!=0u)continue;
  if((wf_u8(a,WF_A_FLAGS32)&0x10u)!=0u)continue;
  if((wf_u8(a,WF_A_RESULT_LO)&0x01u)==0u)continue;
  if(wf_be16(a,WF_A_LOCKOUT)!=0u)continue;
  wf_r17_m68k_moveq(&rt->m68k_regs,0u,2);
  int payment_ok=wf_payment_consume_055a(rt,2u);
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,3u); /* $0582 mode-2 payment clobber. */
  if(!payment_ok)continue;
  refill_actor(a);
  if((rt->g.mode_0161&0x01u)==0u){
   wf_actor_raw_t*p=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TEAM_LINK),0);
   if(!p){c->last_blocked_phase=0x008b3au;c->last_blocked_dependency=0x008c58u;return WF_BLOCKED;}
   if((wf_u8(p,WF_A_RESULT_LO)&0x01u)==0u){
    refill_actor(p);wf_set_be16(p,WF_A_MASH,1u);clear_refill_words(p);
   }
  }
  wf_set_be16(a,WF_A_MASH,1u);
  wf_set_be16(a,WF_A_FIELD62,0u);
  rt->g.global_169e=0x8000u;
  rt->g.global_0166=0u;
  clear_refill_words(a);
  return WF_OK;
 }
 return WF_OK;
}


/* $26936/$2983C operate on the same four RAM WORDs later consumed by
 * $26A42 (map streaming), $247C (sprites) and $26E1A (video scrolls).
 * camera is an adapter for the standalone camera API, never a second owner. */
static void camera_import(const wf_runtime_t *rt,wf_camera_state_t *c){
 c->scroll0_x=(int16_t)rt->g.global_17e6;c->scroll1_x=(int16_t)rt->g.global_17ea;
 c->scroll0_y=(int16_t)rt->g.global_17ee;c->scroll1_y=(int16_t)rt->g.global_17f2_word;
 c->target_x=(int16_t)rt->g.global_1804;c->target_y=(int16_t)rt->g.global_1806;c->scene_index=rt->g.global_007e;
}
static void camera_publish(wf_runtime_t *rt,const wf_camera_state_t *c){
 rt->g.global_17e6=(uint16_t)c->scroll0_x;rt->g.global_17ea=(uint16_t)c->scroll1_x;
 rt->g.global_17ee=(uint16_t)c->scroll0_y;rt->g.global_17f2_word=(uint16_t)c->scroll1_y;
}
wf_status_t wf_phase_camera_26936(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;if(!rt||!c)return WF_BAD_STATE;
 if(!c->camera_valid){c->last_blocked_phase=0x026936u;c->last_blocked_dependency=0x026936u;return WF_BLOCKED;}
 camera_import(rt,&c->camera);
 wf_status_t s=wf_camera_track(rt,&c->camera);if(s==WF_OK)camera_publish(rt,&c->camera);return s;
}
wf_status_t wf_phase_camera_2983c(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;if(!rt||!c)return WF_BAD_STATE;
 if(!c->camera_valid){c->last_blocked_phase=0x02983cu;c->last_blocked_dependency=0x02983cu;return WF_BLOCKED;}
 camera_import(rt,&c->camera);
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(c->camera.scene_index<<3));
 wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)c->camera.scroll0_y);
 wf_status_t s=wf_camera_clamp(&c->camera);if(s==WF_OK)camera_publish(rt,&c->camera);return s;
}


wf_status_t wf_phase_referee_1f914(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_referee_ctx_t rc;wf_r17_referee_hooks_t h;wf_r17_render_ctx_t render;wf_status_t s;(void)addr;
 if(!rt||!c||!c->camera_valid)return WF_BLOCKED;
 memset(&rc,0,sizeof(rc));rc.display_2067c=c->referee_display_2067c;rc.clear_206fe=c->referee_clear_206fe;rc.direction_20c8=c->referee_direction_20c8;rc.resolver_280dc=c->referee_resolver_280dc?c->referee_resolver_280dc:wf_r17_resolver_280dc_hook;rc.royal_phase6_21358=c->referee_royal_phase6_21358;rc.tag_phase6_90d6=c->referee_tag_phase6_90d6;rc.result_20156=c->referee_result_20156;rc.audio_2052=(wf_r17_ref_audio_fn)c->audio_command;rc.user=c->r17_owner_user;
 render.scroll_a_x=(uint16_t)c->camera.scroll0_x;render.scroll_a_y=(uint16_t)c->camera.scroll0_y;render.queue=&c->render_queue;render.visibility_2188=wf_r17_visibility_2188_exact;render.visibility_user=0;
 h.owner_1f914_logic=wf_r17_referee_logic_1f914;h.royal_f8e4=0;h.user=&rc;
 s=h.owner_1f914_logic(rt,&rt->referee,h.user);if(s==WF_OK)s=wf_r17_world_to_screen_247c(&rt->referee,render.scroll_a_x,render.scroll_a_y);if(s==WF_OK)s=wf_r17_queue_27b8(&render,&rt->referee);
 if(s!=WF_OK){c->last_blocked_phase=0x01f914u;c->last_blocked_dependency=rc.blocked_dependency_rom?rc.blocked_dependency_rom:0x01f914u;return s;}
 if((rt->g.mode_0161&2u)!=0u){s=wf_r17_royal_f8e4(rt,&rt->referee,&render);if(s!=WF_OK){c->last_blocked_phase=0x01f914u;c->last_blocked_dependency=0x0000f8e4u;return s;}}
 return WF_OK;
}

wf_status_t wf_phase_wrestlers_f4c2(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_render_ctx_t render;wf_r17_f4c2_hooks_t h;wf_status_t s;(void)addr;
 if(!rt||!c||!c->camera_valid)return WF_BLOCKED;
 render.scroll_a_x=(uint16_t)c->camera.scroll0_x;render.scroll_a_y=(uint16_t)c->camera.scroll0_y;render.queue=&c->render_queue;render.visibility_2188=wf_r17_visibility_2188_exact;render.visibility_user=0;
 h.resolver_280dc=wf_r17_resolver_280dc_hook;h.dispatcher_1c03e=f4c2_dispatch_hook;h.postqueue_2208=f4c2_postqueue_2208;h.user=c;
 c->last_blocked_phase=0u;c->last_blocked_dependency=0u;s=wf_r17_wrestlers_f4c2(rt,&render,&h);
 if(s!=WF_OK){c->last_blocked_phase=0x00f4c2u;if(c->last_blocked_dependency==0u)c->last_blocked_dependency=(s==WF_BAD_STATE)?0x000280dcu:0x00f4c2u;return s;}return WF_OK;
}


wf_status_t wf_phase_collision_24062(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;if(!rt||!c)return WF_BAD_STATE;
 wf_status_t s=wf_r17_collision_24062(rt,&c->collision_24062);
 if(s!=WF_OK){c->last_blocked_phase=0x024062u;c->last_blocked_dependency=c->collision_24062.blocked_dependency_rom?c->collision_24062.blocked_dependency_rom:0x024062u;}
 return s;
}


wf_status_t wf_phase_ringside_fdee(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_render_ctx_t render;wf_status_t s;(void)addr;
 if(!rt||!c||!c->camera_valid)return WF_BLOCKED;
 render.scroll_a_x=(uint16_t)c->camera.scroll0_x;render.scroll_a_y=(uint16_t)c->camera.scroll0_y;render.queue=&c->render_queue;render.visibility_2188=wf_r17_visibility_2188_exact;render.visibility_user=0;
 s=wf_r17_ringside_fdee(rt,&render,&c->ringside_fdee);
 if(s!=WF_OK){c->last_blocked_phase=0x00fdeeu;c->last_blocked_dependency=c->ringside_fdee.blocked_dependency_rom?c->ringside_fdee.blocked_dependency_rom:0x00fdeeu;}
 return s;
}

wf_status_t wf_phase_fixed_four_10120(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_render_ctx_t render;wf_status_t s;(void)addr;
 if(!rt||!c||!c->camera_valid)return WF_BLOCKED;
 render.scroll_a_x=(uint16_t)c->camera.scroll0_x;render.scroll_a_y=(uint16_t)c->camera.scroll0_y;render.queue=&c->render_queue;render.visibility_2188=wf_r17_visibility_2188_exact;render.visibility_user=0;
 s=wf_r17_fixed_four_10120(rt,&render,(wf_r17_fixed_four_audio_fn)c->audio_command,c->r17_owner_user,&c->fixed_four_10120);
 if(s!=WF_OK){c->last_blocked_phase=0x010120u;c->last_blocked_dependency=c->fixed_four_10120.blocked_dependency_rom?c->fixed_four_10120.blocked_dependency_rom:0x010120u;}
 return s;
}


wf_status_t wf_phase_transient_reset_10e6a(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_status_t s;(void)addr;
 if(!rt||!c)return WF_BAD_STATE;
 s=wf_r17_transient_reset_10e6a(rt);
 if(s!=WF_OK){c->last_blocked_phase=0x010e6au;c->last_blocked_dependency=0x010e6au;}
 return s;
}


wf_status_t wf_phase_effects_7a00(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_render_ctx_t render;wf_status_t s;(void)addr;
 if(!rt||!c||!c->camera_valid)return WF_BLOCKED;
 render.scroll_a_x=(uint16_t)c->camera.scroll0_x;render.scroll_a_y=(uint16_t)c->camera.scroll0_y;render.queue=&c->render_queue;render.visibility_2188=wf_r17_visibility_2188_exact;render.visibility_user=0;
 s=wf_r17_effects_7a00(rt,&render,&c->effects_7a00);
 if(s!=WF_OK){c->last_blocked_phase=0x007a00u;c->last_blocked_dependency=c->effects_7a00.blocked_dependency_rom?c->effects_7a00.blocked_dependency_rom:0x007a00u;}
 return s;
}


static void post7_build_ctx(wf_gameplay_binding_context_t *c,wf_r17_post7_ctx_t *p,wf_r17_render_ctx_t *render){
 memset(p,0,sizeof(*p));
 p->audio_2052=(wf_r17_post7_audio_fn)c->audio_command;
 p->fg0_batch=c->post7_fg0_batch;
 p->video_batch=c->post7_video_batch;
 p->resource_2aea=c->post7_resource_2aea;
 p->resource_2b58=c->post7_resource_2b58;
 p->raw_2503c=c->post7_raw_2503c;
 p->wait_21e6=c->post7_wait_21e6;
 p->raw_9aca=c->post7_raw_9aca;
 p->fg0_peek_long=c->post7_fg0_peek_long;
 p->branchb_288f0=c->branchb_288f0;
 p->branchb_26b68=c->branchb_26b68;
 p->render_2836_batch=c->post7_render_2836_batch;
 p->tag_6e34=c->post7_tag_6e34;
 p->royal_9132=c->post7_royal_9132;
 p->user=c->r17_owner_user;
 if(c->camera_valid){
  render->scroll_a_x=(uint16_t)c->camera.scroll0_x;render->scroll_a_y=(uint16_t)c->camera.scroll0_y;
  render->queue=&c->render_queue;render->visibility_2188=wf_r17_visibility_2188_exact;render->visibility_user=0;
  p->render=render;
 }
}
static wf_status_t post7_finish(wf_gameplay_binding_context_t *c,wf_r17_post7_ctx_t *p,uint32_t phase,wf_status_t s){
 if(s!=WF_OK){c->last_blocked_phase=phase;c->last_blocked_dependency=p->blocked_dependency_rom?p->blocked_dependency_rom:phase;}
 return s;
}
wf_status_t wf_phase_branchb_285da(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x000285dau,wf_r17_owner_285da(rt,&p));
}
wf_status_t wf_phase_branchb_26a42(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00026a42u,wf_r17_owner_26a42(rt,&p));
}
wf_status_t wf_phase_branchb_7548(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00007548u,wf_r17_owner_7548(rt,&p));
}
wf_status_t wf_phase_branchb_bcfe(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x0000bcfeu,wf_r17_owner_bcfe(rt,&p));
}
wf_status_t wf_phase_branchb_c04a(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x0000c04au,wf_r17_owner_c04a(rt,&p));
}
wf_status_t wf_phase_branchb_c710(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x0000c710u,wf_r17_owner_c710(rt,&p));
}
wf_status_t wf_phase_post7_88a2(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x000088a2u,wf_r17_owner_88a2(rt,&p));
}
wf_status_t wf_phase_post7_8710(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00008710u,wf_r17_owner_8710(rt,&p));
}
wf_status_t wf_phase_post7_262d2(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x000262d2u,wf_r17_owner_262d2(rt,&p));
}
wf_status_t wf_phase_post7_bb14(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x0000bb14u,wf_r17_owner_bb14(rt,&p));
}
wf_status_t wf_phase_post7_7366(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00007366u,wf_r17_owner_7366(rt,&p));
}
wf_status_t wf_phase_post7_8cf8(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00008cf8u,wf_r17_owner_8cf8(rt,&p));
}
wf_status_t wf_phase_post7_9052(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00009052u,wf_r17_owner_9052(rt,&p));
}
wf_status_t wf_phase_post7_899c(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x0000899cu,wf_r17_owner_899c(rt,&p));
}
wf_status_t wf_phase_post7_8ecc(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_status_t s;(void)addr;if(!rt||!c)return WF_BAD_STATE;s=wf_r17_owner_8ecc(rt);if(s!=WF_OK){c->last_blocked_phase=0x00008eccu;c->last_blocked_dependency=0x00008eccu;}return s;
}


wf_status_t wf_phase_post7_2836(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x00002836u,wf_r17_owner_2836(rt,&p));
}
wf_status_t wf_phase_post7_18c4(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;
 wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;wf_r17_participant_io_t io;
 (void)addr;if(!rt||!c)return WF_BAD_STATE;
 post7_build_ctx(c,&p,&r);memset(&io,0,sizeof(io));
 io.input_word=c->participant_input_word;io.fg0_batch=c->post7_fg0_batch;
 io.video_batch=c->post7_video_batch;io.commit_batch=c->participant_commit_batch;
 io.user=c->r17_owner_user;p.participants=&io;
 return post7_finish(c,&p,0x000018c4u,wf_r17_owner_18c4(rt,&p,c->input_hw_low,c->input_valid_mask));
}
wf_status_t wf_phase_post7_298ec(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;wf_r17_post7_ctx_t p;wf_r17_render_ctx_t r;(void)addr;if(!rt||!c)return WF_BAD_STATE;post7_build_ctx(c,&p,&r);return post7_finish(c,&p,0x000298ecu,wf_r17_owner_298ec(rt,&p));
}

wf_status_t wf_phase_cpu_real(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;wf_gameplay_binding_context_t *c=(wf_gameplay_binding_context_t*)user;if(!rt||!c)return WF_BAD_STATE;
 if((rt->g.global_16c4&1u)!=0u)return WF_OK;
 wf_r17_m68k_moveq(&rt->m68k_regs,6u,8);
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++,wf_r17_m68k_move_w(&rt->m68k_regs,6u,(uint16_t)(8u-i))){
  wf_actor_raw_t*a=&rt->actors[i];
  if((wf_u8(a,WF_A_ACTIVE)&0x80u)==0u)continue;
  /* $1C16E-$1C19A dispatches only CPU/partner roles. A selected human
   * with +$56.bit6/bit7 clear never enters the CPU state table. */
  uint8_t role=wf_u8(a,WF_A_ROLE);
  if(!(role&0x40u)&&(!(role&0x80u)||(wf_u8(a,WF_A_FLAGS32)&0x80u)))continue;
  if(wf_u8(a,WF_A_STATE+1u)==0xffu||wf_be16(a,WF_A_LOCKOUT)!=0u||
     (wf_u8(a,WF_A_FLAGS32)&0x10u))continue;
  /* $1C19C refreshes this actor's target relation immediately before its
   * decision; a relation computed for another phase/actor is not reusable. */
  wf_status_t s=wf_cpu_relation_1c228(rt,i);
  if(s!=WF_OK){c->last_blocked_phase=0x01c150u;c->last_blocked_dependency=0x01c228u;return s;}
  if(!rt->g.credited_route){
   wf_set_be16(a,0xd2u,3u);wf_set_be16(a,0xd4u,2u);wf_set_be16(a,0xd6u,3u);
   wf_set_be16(a,0xd8u,2u);wf_set_be16(a,0xdau,3u);wf_set_be16(a,0xdeu,3u);wf_set_be16(a,0xe4u,4u);
  }
  /* $1C1D2..$1C1DA leave the WORD table index live in D0. */
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((wf_be16(a,WF_A_STATE)&0xffu)<<2));
  s=wf_cpu_actor_step(rt,i,&c->cpu_trace[i]);
  if(s!=WF_OK){c->last_blocked_phase=0x01c150u;c->last_blocked_dependency=c->cpu_trace[i].blocked_dependency_rom?c->cpu_trace[i].blocked_dependency_rom:c->cpu_trace[i].state5.blocked_dependency_rom;return s;}
 }
 return WF_OK;
}

size_t wf_gameplay_bindings_build(wf_gameplay_binding_context_t *ctx,wf_phase_binding_t *out,size_t cap){
 if(!ctx||!out||cap<36u) return 0u;
 memset(ctx,0,sizeof(*ctx));
 out[0]=(wf_phase_binding_t){0x00514eu,wf_phase_input_real,ctx};
 out[1]=(wf_phase_binding_t){0x008b3au,wf_phase_refill_8b3a,ctx};
 out[2]=(wf_phase_binding_t){0x02088eu,wf_phase_spatial_2088e,ctx};
 out[3]=(wf_phase_binding_t){0x020958u,wf_phase_target_relation_20958,ctx};
 out[4]=(wf_phase_binding_t){0x026936u,wf_phase_camera_26936,ctx};
 out[5]=(wf_phase_binding_t){0x02983cu,wf_phase_camera_2983c,ctx};
 out[6]=(wf_phase_binding_t){0x00f560u,wf_phase_pair_relation_f560,ctx};
 out[7]=(wf_phase_binding_t){0x00f18au,wf_phase_actor_prepass_f18a,ctx};
 out[8]=(wf_phase_binding_t){0x01c150u,wf_phase_cpu_real,ctx};
 out[9]=(wf_phase_binding_t){0x01f914u,wf_phase_referee_1f914,ctx};
 out[10]=(wf_phase_binding_t){0x00f4c2u,wf_phase_wrestlers_f4c2,ctx};
 out[11]=(wf_phase_binding_t){0x024062u,wf_phase_collision_24062,ctx};
 out[12]=(wf_phase_binding_t){0x024e58u,wf_phase_damage,ctx};
 out[13]=(wf_phase_binding_t){0x00fdeeu,wf_phase_ringside_fdee,ctx};
 out[14]=(wf_phase_binding_t){0x010120u,wf_phase_fixed_four_10120,ctx};
 out[15]=(wf_phase_binding_t){0x010e6au,wf_phase_transient_reset_10e6a,ctx};
 out[16]=(wf_phase_binding_t){0x007a00u,wf_phase_effects_7a00,ctx};
 out[17]=(wf_phase_binding_t){0x0088a2u,wf_phase_post7_88a2,ctx};
 out[18]=(wf_phase_binding_t){0x008710u,wf_phase_post7_8710,ctx};
 out[19]=(wf_phase_binding_t){0x0262d2u,wf_phase_post7_262d2,ctx};
 out[20]=(wf_phase_binding_t){0x00bb14u,wf_phase_post7_bb14,ctx};
 out[21]=(wf_phase_binding_t){0x007366u,wf_phase_post7_7366,ctx};
 out[22]=(wf_phase_binding_t){0x008cf8u,wf_phase_post7_8cf8,ctx};
 out[23]=(wf_phase_binding_t){0x009052u,wf_phase_post7_9052,ctx};
 out[24]=(wf_phase_binding_t){0x00899cu,wf_phase_post7_899c,ctx};
 out[25]=(wf_phase_binding_t){0x008eccu,wf_phase_post7_8ecc,ctx};
 out[26]=(wf_phase_binding_t){0x002836u,wf_phase_post7_2836,ctx};
 out[27]=(wf_phase_binding_t){0x0018c4u,wf_phase_post7_18c4,ctx};
 out[28]=(wf_phase_binding_t){0x0298ecu,wf_phase_post7_298ec,ctx};
 out[29]=(wf_phase_binding_t){0x0285dau,wf_phase_branchb_285da,ctx};
 out[30]=(wf_phase_binding_t){0x026a42u,wf_phase_branchb_26a42,ctx};
 out[31]=(wf_phase_binding_t){0x007548u,wf_phase_branchb_7548,ctx};
 out[32]=(wf_phase_binding_t){0x00bcfeu,wf_phase_branchb_bcfe,ctx};
 out[33]=(wf_phase_binding_t){0x00c04au,wf_phase_branchb_c04a,ctx};
 out[34]=(wf_phase_binding_t){0x00c710u,wf_phase_branchb_c710,ctx};
 ctx->f4c2_actions.raw23_ctx=&ctx->f4c2_raw23_ctx;ctx->f4c2_dispatch.actions=&ctx->f4c2_actions;
 out[35]=(wf_phase_binding_t){0x02017au,wf_phase_postack_2017a,ctx};
 return 36u;
}

wf_status_t wf_phase_postack_2017a(wf_runtime_t *rt,uint32_t addr,void *user){
 wf_gameplay_binding_context_t *c=(wf_gameplay_binding_context_t*)user;wf_status_t s;
 (void)addr;if(!c)return WF_BAD_STATE;
 s=wf_r17_postack_2017a(rt,c->referee_clear_206fe,c->r17_owner_user);
 if(s!=WF_OK){c->last_blocked_phase=0x2017au;c->last_blocked_dependency=0x206feu;}
 return s;
}
