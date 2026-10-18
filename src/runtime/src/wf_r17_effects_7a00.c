#include <string.h>
#include "wf_r17_effects_7a00.h"

static wf_status_t block(wf_r17_effects_7a00_trace_t*t,uint32_t dep){if(t)t->blocked_dependency_rom=dep;return WF_BLOCKED;}
static uint16_t low16(uint32_t v){return (uint16_t)v;}
static uint32_t swap32(uint32_t v){return (v<<16)|(v>>16);}
static wf_actor_raw_t *effect(wf_runtime_t*rt,unsigned i){return &rt->effect_1450[i];}

static wf_status_t rng21b4(wf_runtime_t*rt,wf_r17_effects_7a00_trace_t*t,uint32_t*out){
 if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(t,0x000021b4u);
 *out=wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);
 if(t)t->rng_calls++;
 return WF_OK;
}

/* $7B5C/$7B60. The helper's MOVEQ writes the full D2 before $21B4. */
static wf_status_t random_shift(wf_runtime_t*rt,unsigned shift,wf_r17_effects_7a00_trace_t*t,uint16_t*out){
 uint32_t v;uint16_t w;wf_status_t s;
 wf_r17_m68k_moveq(&rt->m68k_regs,2,(int8_t)shift);
 s=rng21b4(rt,t,&v);if(s!=WF_OK)return s;
 v=swap32(v);wf_r17_m68k_move_l(&rt->m68k_regs,0,v);
 w=(uint16_t)(low16(v)&3u);w=(uint16_t)(w<<shift);wf_r17_m68k_move_w(&rt->m68k_regs,0,w);*out=w;return WF_OK;
}

static wf_status_t state0(wf_actor_raw_t*a){
 uint16_t v=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)-1u);wf_set_be16(a,WF_A_ANIM_INDEX,v);
 if((int16_t)v<0)wf_set_be16(a,WF_A_LATCHED,(uint16_t)(wf_be16(a,WF_A_LATCHED)+1u));
 return WF_OK;
}

static wf_status_t state1_position_nonroyal(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_effects_7a00_trace_t*t){
 uint32_t v;uint16_t w;wf_status_t s;
 s=rng21b4(rt,t,&v);if(s!=WF_OK)return s;
 v=swap32(v);wf_r17_m68k_move_l(&rt->m68k_regs,0,v);w=(uint16_t)(low16(v)&0x003fu);w=(uint16_t)(w<<3);w=(uint16_t)(w+0x0190u);wf_r17_m68k_move_w(&rt->m68k_regs,0,w);wf_set_be16(a,WF_A_X,w);
 s=rng21b4(rt,t,&v);if(s!=WF_OK)return s;
 v=swap32(v);wf_r17_m68k_move_l(&rt->m68k_regs,0,v);w=(uint16_t)(low16(v)&0x003fu);w=(uint16_t)(w+0x01e0u);wf_r17_m68k_move_w(&rt->m68k_regs,0,w);wf_set_be16(a,WF_A_Y,w);
 return WF_OK;
}

static wf_status_t state1_position_royal(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_render_ctx_t*render,wf_r17_effects_7a00_trace_t*t){
 uint32_t v;uint16_t x,r=0u,tmp;wf_status_t s;uint16_t scroll=render->scroll_a_x;
 s=rng21b4(rt,t,&v);if(s!=WF_OK)return s;
 x=(uint16_t)(low16(v)&0x007fu);x=(uint16_t)(x<<1);x=(uint16_t)(x+scroll);wf_r17_m68k_move_w(&rt->m68k_regs,0,x);wf_set_be16(a,WF_A_X,x);
 if(scroll<0x01e0u){
  if(x<0x01a0u){s=random_shift(rt,4u,t,&tmp);if(s!=WF_OK)return s;wf_r17_m68k_move_w(&rt->m68k_regs,2,tmp);s=random_shift(rt,5u,t,&r);if(s!=WF_OK)return s;r=(uint16_t)(r+low16(rt->m68k_regs.d[2]));wf_r17_m68k_move_w(&rt->m68k_regs,0,r);}
  else if(x<0x01e0u){s=random_shift(rt,5u,t,&r);if(s!=WF_OK)return s;}
  else{s=random_shift(rt,4u,t,&r);if(s!=WF_OK)return s;}
 }else if(scroll>=0x03c0u){
  if(x<0x0460u){s=random_shift(rt,4u,t,&r);if(s!=WF_OK)return s;}
  else if(x<0x04a0u){s=random_shift(rt,5u,t,&r);if(s!=WF_OK)return s;}
  else{s=random_shift(rt,4u,t,&tmp);if(s!=WF_OK)return s;wf_r17_m68k_move_w(&rt->m68k_regs,2,tmp);s=random_shift(rt,5u,t,&r);if(s!=WF_OK)return s;r=(uint16_t)(r+low16(rt->m68k_regs.d[2]));wf_r17_m68k_move_w(&rt->m68k_regs,0,r);}
 }else{s=random_shift(rt,4u,t,&r);if(s!=WF_OK)return s;}
 /* $7AF0 MOVE.W #$0200,D2 / SUB.W D0,D2. */
 wf_r17_m68k_move_w(&rt->m68k_regs,2,0x0200u);r=(uint16_t)(0x0200u-low16(rt->m68k_regs.d[0]));wf_r17_m68k_move_w(&rt->m68k_regs,2,r);wf_set_be16(a,WF_A_Y,r);
 return WF_OK;
}

static wf_status_t state1(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_render_ctx_t*render,wf_r17_effects_7a00_trace_t*t){
 uint8_t hi=wf_u8(a,WF_A_LATCHED);wf_status_t s;uint16_t timer,phase;
 if((hi&0x80u)==0u){
  wf_set_u8(a,WF_A_LATCHED,(uint8_t)(hi|0x80u));
  wf_set_be16(a,WF_A_ANIM_INDEX,0u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);wf_set_be16(a,WF_A_CHARACTER,0x001du);wf_set_be16(a,WF_A_Z,0x0140u);
  if((rt->g.mode_0161&0x02u)!=0u){wf_set_be16(a,WF_A_Z,0x0100u);s=state1_position_royal(rt,a,render,t);}else s=state1_position_nonroyal(rt,a,t);
  if(s!=WF_OK)return s;
 }
 timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
 if((int16_t)timer>=0)return WF_OK;
 wf_set_be16(a,WF_A_ANIM_COUNTER,0u);phase=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);wf_set_be16(a,WF_A_ANIM_INDEX,phase);
 if(phase>=6u){wf_set_be16(a,WF_A_LATCHED,0u);wf_set_u8(a,WF_A_ACTIVE,(uint8_t)(wf_u8(a,WF_A_ACTIVE)&0x7fu));return WF_OK;}
 s=wf_r17_world_to_screen_247c(a,render->scroll_a_x,render->scroll_a_y);if(s!=WF_OK)return s;
 wf_set_be16(a,WF_A_MIRROR,phase);s=wf_r17_queue_27b8(render,a);if(s==WF_OK&&t)t->rendered_objects++;return s;
}

wf_status_t wf_r17_effects_7a00(wf_runtime_t*rt,wf_r17_render_ctx_t*render,wf_r17_effects_7a00_trace_t*t){
 wf_actor_raw_t snap[WF_R17_EFFECT_1450_COUNT];wf_r17_render_queue_t qsnap;wf_r17_m68k_shadow_t rsnap;uint32_t rngsnap;wf_status_t s=WF_OK;unsigned i;
 if(t)memset(t,0,sizeof(*t));
 if(!rt||!render||!render->queue)return WF_BAD_STATE;
 for(i=0u;i<WF_R17_EFFECT_1450_COUNT;i++)snap[i]=*effect(rt,i);
 qsnap=*render->queue;
 rsnap=rt->m68k_regs;
 rngsnap=rt->g.rng_state;
 wf_r17_m68k_moveq(&rt->m68k_regs,1,2);
 for(i=0u;i<WF_R17_EFFECT_1450_COUNT;i++){
  wf_actor_raw_t*a=effect(rt,i);uint8_t active=wf_u8(a,WF_A_ACTIVE);uint16_t delay;uint32_t rv;
  wf_set_u8(a,WF_A_ACTIVE,(uint8_t)(active|0x80u));
  if((active&0x80u)==0u){s=rng21b4(rt,t,&rv);if(s!=WF_OK)goto rollback;delay=(uint16_t)(low16(rv)&0x007fu);wf_r17_m68k_move_w(&rt->m68k_regs,0,delay);wf_set_be16(a,WF_A_ANIM_INDEX,delay);wf_set_be16(a,WF_A_LATCHED,0u);}
  /* $7A28..$7A30 dispatch index is a WORD write, after delay storage. */
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)((wf_be16(a,WF_A_LATCHED)&1u)<<2));
  if((wf_be16(a,WF_A_LATCHED)&1u)==0u)s=state0(a);else s=state1(rt,a,render,t);
  if(s!=WF_OK)goto rollback;
  if(t)t->processed_objects=(uint8_t)(i+1u);
  wf_r17_m68k_move_w(&rt->m68k_regs,1,(uint16_t)(low16(rt->m68k_regs.d[1])-1u));
 }
 return WF_OK;
rollback:
 for(i=0u;i<WF_R17_EFFECT_1450_COUNT;i++)*effect(rt,i)=snap[i];
 *render->queue=qsnap;
 rt->m68k_regs=rsnap;
 rt->g.rng_state=rngsnap;
 return s;
}
