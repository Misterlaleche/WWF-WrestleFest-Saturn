#include <string.h>
#include "wf_r17_ringside_fdee.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_physics.h"

static int bit(const wf_actor_raw_t *a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t *a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static wf_status_t block(wf_r17_fdee_trace_t *t,uint32_t dep){if(t)t->blocked_dependency_rom=dep;return WF_BLOCKED;}
static uint16_t addw(uint16_t a,uint16_t b){return (uint16_t)(a+b);}

/* $258E, restricted to the two ROM-initialized fixed objects consumed by
 * $FDEE.  $FFD2 writes +$74={0,1}; $FE8E multiplies it by three before
 * entering $258E, selecting table rows 0 and 3 at $25CA/$25DC. */
static wf_status_t velocity_258e_fixed(wf_actor_raw_t *a,wf_r17_fdee_trace_t *t){
 static const uint16_t row[2][3]={{0x00c0u,0x0300u,0x0038u},{0x0000u,0x0300u,0x0038u}};
 uint16_t type=wf_be16(a,WF_A_REL_FLAGS); /* +$74 */
 uint16_t vx;
 if(type>1u)return block(t,0x0000258eu);
 vx=row[type][0];
 wf_set_be16(a,WF_A_VX,vx);
 wf_set_be16(a,WF_A_VY,0u);
 wf_set_be16(a,WF_A_VZ,row[type][1]);
 wf_set_be16(a,WF_A_VZ_DEC,row[type][2]);
 wf_set_u8(a,WF_A_MOTION_MODE,2u);
 if(bit(a,WF_A_FACING,7u))wf_set_be16(a,WF_A_VX,(uint16_t)(0u-wf_be16(a,WF_A_VX)));
 return WF_OK;
}

static wf_status_t set_visual_from_type(wf_actor_raw_t *a,wf_r17_fdee_trace_t *t){
 uint16_t type=wf_be16(a,WF_A_REL_FLAGS);
 uint16_t v;
 if(type>1u)return block(t,0x0000fe9au);
 v=(uint16_t)(type*3u);
 wf_set_be16(a,WF_A_MIRROR,v);
 wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
 return WF_OK;
}

static wf_status_t render_object(wf_r17_render_ctx_t *render,wf_actor_raw_t *a){
 wf_status_t s=wf_r17_world_to_screen_247c(a,render->scroll_a_x,render->scroll_a_y);
 if(s!=WF_OK)return s;
 return wf_r17_queue_27b8(render,a);
}

static wf_status_t state0_fe36(wf_actor_raw_t *a){wf_set_be16(a,WF_A_MIRROR,0xffffu);return WF_OK;}

static wf_status_t state1_fe3e(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_fdee_trace_t *t){
 wf_actor_raw_t *linked;uint16_t dx;
 if(!bit(a,WF_A_LATCHED,7u))wf_set_be16(a,WF_A_MIRROR,0xffffu);
 linked=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_REL_LINK),0); /* +$76 */
 if(!linked)return block(t,0x0000fe4cu);
 wf_set_u8(a,WF_A_FACING,wf_u8(linked,WF_A_FACING));
 wf_set_be16(a,WF_A_X,wf_be16(linked,WF_A_X));
 wf_set_be16(a,WF_A_Y,wf_be16(linked,WF_A_Y));
 wf_set_be16(a,WF_A_Z,wf_be16(linked,WF_A_Z));
 wf_set_be16(a,WF_A_Z,addw(wf_be16(a,WF_A_Z),0x0050u));
 dx=0x0020u;if(!bit(a,WF_A_FACING,7u))dx=(uint16_t)(0u-dx);
 wf_set_be16(a,WF_A_X,addw(wf_be16(a,WF_A_X),dx));
 return WF_OK;
}

static wf_status_t state2_fe82(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_render_ctx_t *render,wf_r17_fdee_trace_t *t){
 wf_status_t s;
 if(!bit(a,WF_A_LATCHED,7u)){
  setbit(a,WF_A_LATCHED,7u);
  s=velocity_258e_fixed(a,t);if(s!=WF_OK)return s;
  s=set_visual_from_type(a,t);if(s!=WF_OK)return s;
  wf_set_be16(a,WF_A_RENDER_LIST,0u);
  wf_set_be32(a,WF_A_REL_LINK,0u);
 }
 /* $FEB6 calls $2208 on every state-2 invocation, not only first entry. */
 if(wf_u8(a,WF_A_MOTION_MODE)>2u)return block(t,0x00002208u);
 wf_physics_integrate_common(a);
 wf_set_be16(a,WF_A_FIELD3E,0x0020u);
 s=wf_r17_resolver_280dc(rt,a);if(s!=WF_OK)return block(t,0x000280dcu);
 if(wf_be16(a,WF_A_CONTACT)==0u){
  wf_set_be16(a,WF_A_FIELD3E,0xffe0u);
  s=wf_r17_resolver_280dc(rt,a);if(s!=WF_OK)return block(t,0x000280dcu);
 }
 if(wf_be16(a,WF_A_CONTACT)!=0u){
  wf_set_be16(a,WF_A_FIELD3E,0u);
  wf_set_be16(a,WF_A_X,addw(wf_be16(a,WF_A_X),wf_be16(a,WF_A_CORR_X)));
  wf_set_be16(a,WF_A_Y,addw(wf_be16(a,WF_A_Y),wf_be16(a,WF_A_CORR_Y)));
  wf_set_be16(a,WF_A_Z,addw(wf_be16(a,WF_A_Z),wf_be16(a,WF_A_CORR_Z)));
  if(bit(a,WF_A_CONTACT+1u,4u)){wf_set_be16(a,WF_A_LATCHED,4u);wf_set_be16(a,WF_A_CONTACT,0u);}
 }
 return render_object(render,a);
}

static wf_status_t state3_ff1e(wf_actor_raw_t *a,wf_r17_render_ctx_t *render,wf_r17_fdee_trace_t *t){
 wf_status_t s;
 if(!bit(a,WF_A_LATCHED,7u)){
  wf_set_u8(a,WF_A_MOTION_MODE,0u);
  s=set_visual_from_type(a,t);if(s!=WF_OK)return s;
  wf_set_be16(a,WF_A_RENDER_LIST,2u);
  wf_set_be32(a,WF_A_REL_LINK,0u);
 }
 return render_object(render,a);
}

static wf_status_t state4_ff54(wf_actor_raw_t *a,wf_r17_render_ctx_t *render,wf_r17_fdee_trace_t *t){
 uint16_t timer,phase,type;uint8_t lo;
 if(!bit(a,WF_A_LATCHED,7u)){
  setbit(a,WF_A_LATCHED,7u);
  wf_set_u8(a,WF_A_MOTION_MODE,0u);
  wf_set_be16(a,WF_A_ANIM_INDEX,0u); /* CLR.W +$24 */
  wf_set_be16(a,WF_A_RENDER_LIST,2u);
 }else{
  timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
  if(timer!=0u)return render_object(render,a);
 }
 wf_set_be16(a,WF_A_ANIM_COUNTER,0x0010u);
 phase=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);wf_set_be16(a,WF_A_ANIM_INDEX,phase);
 if(phase==4u){wf_set_be16(a,WF_A_LATCHED,3u);return render_object(render,a);}
 type=wf_be16(a,WF_A_REL_FLAGS);if(type>1u)return block(t,0x0000ff8eu);
 wf_set_be16(a,WF_A_MIRROR,(uint16_t)(type*3u));
 lo=wf_u8(a,WF_A_MIRROR+1u);
 lo=(uint8_t)(lo+((wf_u8(a,WF_A_ACTION_PHASE)&1u)?1u:2u));
 wf_set_u8(a,WF_A_MIRROR+1u,lo);
 wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
 return render_object(render,a);
}

static wf_status_t one_object(wf_runtime_t *rt,wf_actor_raw_t *a,wf_r17_render_ctx_t *render,wf_r17_fdee_trace_t *t){
 uint16_t state=(uint16_t)(wf_be16(a,WF_A_LATCHED)&0x000fu);
 switch(state){
  case 0u:return state0_fe36(a);
  case 1u:return state1_fe3e(rt,a,t);
  case 2u:return state2_fe82(rt,a,render,t);
  case 3u:return state3_ff1e(a,render,t);
  case 4u:return state4_ff54(a,render,t);
  default:return block(t,0x0000fe22u);
 }
}

wf_status_t wf_r17_ringside_fdee(wf_runtime_t *rt,wf_r17_render_ctx_t *render,wf_r17_fdee_trace_t *trace){
 wf_status_t s;
 if(trace)memset(trace,0,sizeof(*trace));
 if(!rt||!render)return WF_BAD_STATE;
 /* $FDEE: BTST #1,$1C0161 / BEQ $FE08. */
 if((rt->g.mode_0161&0x02u)==0u)return WF_OK;
 s=one_object(rt,&rt->fixed_object0,render,trace);if(s!=WF_OK)return s;if(trace)trace->processed_objects=1u;
 s=one_object(rt,&rt->fixed_object1,render,trace);if(s!=WF_OK)return s;if(trace)trace->processed_objects=2u;
 return WF_OK;
}
