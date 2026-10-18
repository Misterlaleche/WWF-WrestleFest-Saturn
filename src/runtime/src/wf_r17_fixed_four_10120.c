#include <string.h>
#include "wf_r17_fixed_four_10120.h"

static int bit(const wf_actor_raw_t*a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static wf_status_t block(wf_r17_fixed_four_trace_t*t,uint32_t dep){if(t)t->blocked_dependency_rom=dep;return WF_BLOCKED;}

static wf_actor_raw_t *obj(wf_runtime_t *rt,unsigned i){
 if(i==0u)return &rt->fixed_four_1134;
 if(i==1u)return &rt->referee_aux;
 if(i==2u)return &rt->fixed_four_1194;
 return &rt->fixed_four_11c4; /* callers are bounded to 0..3 */
}

static void init_one(wf_actor_raw_t*a,uint16_t x,uint16_t list,uint16_t facing){
 wf_set_be16(a,WF_A_ACTIVE,0x8000u);
 wf_set_be16(a,WF_A_LATCHED,0u);
 wf_set_be16(a,WF_A_X,x);
 wf_set_be16(a,WF_A_Y,0x0178u);
 wf_set_be16(a,WF_A_Z,0x0140u);
 wf_set_be16(a,WF_A_CHARACTER,0x000eu);
 wf_set_be16(a,WF_A_RENDER_LIST,list);
 wf_set_be16(a,WF_A_FACING,facing);
}
void wf_r17_fixed_four_init_1004c(wf_runtime_t *rt){
 if(!rt)return;
 init_one(&rt->fixed_four_1134,0x01c0u,1u,0x0000u);
 init_one(&rt->referee_aux,     0x01c0u,3u,0x0000u);
 init_one(&rt->fixed_four_1194,0x0330u,1u,0x8000u);
 init_one(&rt->fixed_four_11c4,0x0330u,3u,0x8000u);
}

static wf_status_t state0_10186(wf_actor_raw_t*a){
 if(bit(a,WF_A_LATCHED,7u))return WF_OK;
 setbit(a,WF_A_LATCHED,7u);
 wf_set_be16(a,WF_A_MIRROR,bit(a,WF_A_RENDER_LIST+1u,1u)?0x000au:0x0002u);
 wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
 return WF_OK;
}

static const uint8_t s1a[12]={0x03,0x06,0x04,0x06,0x05,0x06,0x06,0x06,0x02,0x06,0x04,0x06};
static const uint8_t s1b[12]={0x07,0x06,0x08,0x06,0x0a,0x06,0x09,0x06,0x0a,0x06,0x08,0x06};
static const uint8_t s2a[12]={0x03,0x10,0x04,0x06,0x02,0x06,0x06,0x06,0x02,0x06,0x04,0x06};
static const uint8_t s2b[12]={0x07,0x10,0x08,0x06,0x0a,0x06,0x09,0x06,0x0a,0x06,0x08,0x06};
static const uint8_t s3 [ 6]={0x04,0x0a,0x05,0x0a,0x06,0x06};
static const uint8_t s4 [ 4]={0x06,0x0e,0x02,0x0c};

static wf_status_t animated(wf_runtime_t*rt,wf_actor_raw_t*a,const uint8_t*seq0,const uint8_t*seq1,
                            unsigned frames,wf_r17_fixed_four_audio_fn audio,void*audio_user,
                            wf_r17_fixed_four_trace_t*t){
 uint16_t idx,timer;const uint8_t*seq=seq0;
 if(!bit(a,WF_A_LATCHED,7u)){
  setbit(a,WF_A_LATCHED,7u);
  if(!audio)return block(t,0x00002052u);
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,0x002cu);
  {wf_r17_m68k_shadow_t saved=rt->m68k_regs;wf_status_t st=audio(rt,0x002cu,audio_user);rt->m68k_regs=saved;if(st!=WF_OK)return block(t,0x00002052u);}
  wf_set_be16(a,WF_A_ANIM_INDEX,0u);
 }else{
  timer=(uint16_t)(wf_be16(a,WF_A_ANIM_COUNTER)-1u);wf_set_be16(a,WF_A_ANIM_COUNTER,timer);
  if(timer!=0u)return WF_OK;
  idx=(uint16_t)(wf_be16(a,WF_A_ANIM_INDEX)+1u);wf_set_be16(a,WF_A_ANIM_INDEX,idx);
  if(idx>=frames){wf_set_be16(a,WF_A_LATCHED,0u);return WF_OK;}
 }
 idx=wf_be16(a,WF_A_ANIM_INDEX);
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(idx*2u));
 if(seq1&&bit(a,WF_A_RENDER_LIST+1u,1u))seq=seq1;
 wf_set_u8(a,WF_A_MIRROR+1u,seq[(unsigned)idx*2u]);
 wf_set_u8(a,WF_A_ANIM_COUNTER+1u,seq[(unsigned)idx*2u+1u]);
 wf_set_u8(a,WF_A_MIRROR,wf_u8(a,WF_A_FACING));
 return WF_OK;
}

static wf_status_t one(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_fixed_four_audio_fn audio,void*audio_user,wf_r17_fixed_four_trace_t*t){
 switch((unsigned)(wf_be16(a,WF_A_LATCHED)&0x000fu)){
  case 0u:return state0_10186(a);
  case 1u:return animated(rt,a,s1a,s1b,6u,audio,audio_user,t);
  case 2u:return animated(rt,a,s2a,s2b,6u,audio,audio_user,t);
  case 3u:return animated(rt,a,s3,0,3u,audio,audio_user,t);
  case 4u:return animated(rt,a,s4,0,2u,audio,audio_user,t);
  default:return block(t,0x00010172u);
 }
}

static wf_status_t preflight(wf_runtime_t*rt,wf_r17_render_ctx_t*render,wf_r17_fixed_four_audio_fn audio,wf_r17_fixed_four_trace_t*t){
 unsigned i,k,need[WF_R17_RENDER_LISTS]={0};
 if(!rt||!render||!render->queue)return WF_BAD_STATE;
 for(i=0;i<4u;i++){
  wf_actor_raw_t*a=obj(rt,i);unsigned st=(unsigned)(wf_be16(a,WF_A_LATCHED)&0x000fu);
  uint16_t klass=wf_be16(a,WF_A_RENDER_LIST);
  if(st>4u)return block(t,0x00010172u);
  if(st!=0u&&!bit(a,WF_A_LATCHED,7u)&&!audio)return block(t,0x00002052u);
  if(klass>=WF_R17_RENDER_LISTS)return WF_BAD_STATE;
  if((wf_u8(a,WF_A_ACTIVE)&0x80u)!=0u){need[klass]++;if(wf_u8(a,WF_A_CHARACTER+1u)<0x4eu&&!render->visibility_2188)return block(t,0x00002188u);}
 }
 for(k=0;k<WF_R17_RENDER_LISTS;k++)if((unsigned)render->queue->count[k]+need[k]>WF_R17_RENDER_LIST_CAP)return WF_BAD_STATE;
 return WF_OK;
}

wf_status_t wf_r17_fixed_four_10120(wf_runtime_t*rt,wf_r17_render_ctx_t*render,
                                    wf_r17_fixed_four_audio_fn audio,void*audio_user,
                                    wf_r17_fixed_four_trace_t*t){
 wf_actor_raw_t snap[4];wf_r17_render_queue_t qsnap;wf_r17_m68k_shadow_t rsnap;wf_status_t s;unsigned i;
 if(t)memset(t,0,sizeof(*t));
 if(!rt||!render||!render->queue)return WF_BAD_STATE;
 /* $10120: BTST #1,$1C0161 / BNE $10160. */
 if((rt->g.mode_0161&0x02u)!=0u)return WF_OK;
 s=preflight(rt,render,audio,t);if(s!=WF_OK)return s;
 for(i=0;i<4u;i++)snap[i]=*obj(rt,i);
 qsnap=*render->queue;rsnap=rt->m68k_regs;
 /* ROM $10130/$1013C..$10144, WORD writes preserve upper halves. */
 wf_r17_m68k_move_w(&rt->m68k_regs,1u,0u);
 for(i=0;i<4u;i++){
  wf_actor_raw_t*a=obj(rt,i);
  wf_r17_m68k_move_w(&rt->m68k_regs,2u,(uint16_t)((wf_be16(a,WF_A_LATCHED)&15u)*4u));
  s=one(rt,a,audio,audio_user,t);
  if(s==WF_OK)s=wf_r17_world_to_screen_247c(a,render->scroll_a_x,render->scroll_a_y);
  if(s==WF_OK)s=wf_r17_queue_27b8(render,a);
  if(s!=WF_OK){unsigned j;for(j=0;j<4u;j++)*obj(rt,j)=snap[j];*render->queue=qsnap;rt->m68k_regs=rsnap;return s;}
  wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)((i+1u)*4u));
  if(t)t->processed_objects=(uint8_t)(i+1u);
 }
 return WF_OK;
}
