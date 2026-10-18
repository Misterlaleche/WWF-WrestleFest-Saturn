#include "wf_r17_dependencies.h"
#include "wf_gameplay_bindings.h"

static int bit(const wf_actor_raw_t*a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static void clrbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~(1u<<b)));}
static wf_status_t block(wf_gameplay_binding_context_t*c,uint32_t dep){if(c){c->last_blocked_phase=0x00f560u;c->last_blocked_dependency=dep;}return WF_BLOCKED;}
static void clear_common(wf_actor_raw_t*a){
 wf_set_be16(a,0x18u,0u);wf_set_be16(a,0x1au,0u);wf_set_be16(a,0x12u,0u);wf_set_be16(a,0xaeu,0u);wf_set_be16(a,0x34u,0u);wf_set_be16(a,0x74u,0u);
 wf_set_be32(a,0x26u,0u);wf_set_be32(a,0x9cu,0u);wf_set_be16(a,0x4cu,0u);wf_set_be16(a,0x4au,0u);if(bit(a,0x56u,7))wf_set_be16(a,0xb4u,0u);
}
static void mask_flags32(wf_actor_raw_t*a){wf_set_be16(a,0x32u,(uint16_t)(wf_be16(a,0x32u)&0x00c3u));}
static wf_status_t emit_audio(wf_runtime_t*rt,wf_gameplay_binding_context_t*c,uint16_t cmd){if(!c||!c->audio_command)return block(c,0x00002052u);return c->audio_command(rt,cmd,c->r17_owner_user);}

wf_status_t wf_r17_f98c(wf_runtime_t*rt,wf_gameplay_binding_context_t*c){
 static const uint8_t t_clear[10]={2,6,0,6,2,6,2,2,6,2};
 static const uint8_t t_set[10]={0,5,0,5,0,5,0,0,5,0};
 unsigned i;int royal;wf_status_t s;
 if(!rt)return WF_BAD_STATE;
 if((rt->g.global_1678&0x80u)==0u)return WF_OK;
 if(rt->g.progression_0162>=10u)return block(c,0x0000fa00u);
 royal=(rt->g.global_1678&0x40u)!=0u;
 rt->g.global_1670=0u;rt->g.global_1674=0u;rt->g.gameplay_vblank_enable=(uint8_t)(rt->g.gameplay_vblank_enable&0x7fu);
 if(!c||!c->transition_reset)return block(c,0x00001f6cu);
 s=c->transition_reset(rt,(uint8_t)royal,c->r17_owner_user);if(s!=WF_OK)return s;
 if(!royal){
  rt->g.global_007f=t_clear[rt->g.progression_0162];rt->g.mode_0161=(uint8_t)(rt->g.mode_0161|2u);
  wf_set_be16(&rt->fixed_object0,0x0eu,0x0100u);wf_set_be16(&rt->fixed_object0,0x1cu,3u);
  wf_set_be16(&rt->fixed_object1,0x0eu,0x0100u);wf_set_be16(&rt->fixed_object1,0x1cu,3u);
  for(i=0;i<WF_ACTOR_SLOTS;i++){
   wf_actor_raw_t*a=&rt->actors[i];if(!bit(a,0x00u,7))continue;clear_common(a);
   if(bit(a,0x33u,2)&&wf_u8(a,0x61u)==0x6au){
    rt->g.global_1806=0x0200u;rt->g.global_1804=bit(a,0x2eu,7)?0x0350u:0x0190u;
    wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x006au);setbit(a,0x32u,5);wf_set_be16(a,0x68u,8u);
   }else if(!bit(a,0x33u,0)){
    mask_flags32(a);clrbit(a,0xc6u,6);setbit(a,0x33u,2);setbit(a,0x32u,5);wf_set_be16(a,0x20u,5u);wf_set_be16(a,0x60u,0x006bu);wf_set_be16(a,0x0au,0x0164u);wf_set_be16(a,0x0eu,0x0100u);
    if(!bit(a,0x33u,7)){wf_set_be16(a,0x06u,0x0200u);setbit(a,0x2eu,7);}else{wf_set_be16(a,0x06u,0x0428u);clrbit(a,0x2eu,7);}
   }else{mask_flags32(a);wf_set_be16(a,0x20u,0u);wf_set_be16(a,0x0au,0x0160u);wf_set_be16(a,0x06u,bit(a,0x2eu,7)?0x03a0u:0x02a0u);}
  }
  if(!c->transition_barrier)return block(c,0x00026e66u);
  s=c->transition_barrier(rt,0u,c->r17_owner_user);if(s!=WF_OK)return s;
  rt->g.global_15f4=rt->g.global_007e;rt->g.global_15f8=rt->g.global_007e;
  wf_set_be16(&rt->referee,0x06u,0x0340u);wf_set_be16(&rt->referee,0x0au,0x0160u);wf_set_be16(&rt->referee,0x20u,0x8003u);
  rt->g.gameplay_vblank_enable=(uint8_t)(rt->g.gameplay_vblank_enable|0x80u);rt->g.global_1678=0u;return emit_audio(rt,c,0x0080u);
 }
 rt->g.global_007f=t_set[rt->g.progression_0162];rt->g.mode_0161=(uint8_t)(rt->g.mode_0161&0xfdu);rt->g.global_1678=0u;rt->g.global_1804=0x01e0u;rt->g.global_1806=0x0230u;
 for(i=0;i<WF_ACTOR_SLOTS;i++){
  wf_actor_raw_t*a=&rt->actors[i];if(!bit(a,0x00u,7))continue;clear_common(a);mask_flags32(a);clrbit(a,0xc6u,6);
  if(bit(a,0x33u,0)){wf_set_be16(a,0x20u,0u);wf_set_be16(a,0xaeu,0u);wf_set_be16(a,0x0au,0x0150u);wf_set_be16(a,0x0eu,0x0140u);wf_set_be16(a,0x06u,bit(a,0x33u,7)?0x02c0u:0x0240u);}
  else{wf_set_be16(a,0x20u,1u);wf_set_be16(a,0xaeu,1u);wf_set_be16(a,0x0au,0x0160u);wf_set_be16(a,0x0eu,0x0140u);}
 }
 if(!c||!c->transition_barrier)return block(c,0x00026e66u);
 s=c->transition_barrier(rt,1u,c->r17_owner_user);if(s!=WF_OK)return s;
 wf_set_be16(&rt->referee,0x06u,0x0280u);wf_set_be16(&rt->referee,0x0au,0x0198u);wf_set_be16(&rt->referee,0x20u,0x8000u);wf_set_be16(&rt->referee,0x34u,0u);wf_set_be16(&rt->referee,0x12u,0u);
 s=emit_audio(rt,c,0x0020u);if(s!=WF_OK)return s;rt->g.global_15d4=0u;rt->g.global_15d5=0u;rt->g.gameplay_vblank_enable=(uint8_t)(rt->g.gameplay_vblank_enable|0x80u);return WF_OK;
}

typedef struct {uint16_t cmd,dur;} pair_t;
typedef struct {uint8_t count;pair_t p[4];} script_t;
static const pair_t character[24]={
 {0x34,0x30},{0x35,0x22},{0x36,0x1c},{0x37,0x32},{0x3c,0x1e},{0x3d,0x1c},{0x3a,0x2a},{0x38,0x45},{0x3b,0x2c},{0x39,0x32},{0x77,0x26},{0x76,0x28},
 {0x6d,0x40},{0x6e,0x80},{0x6f,0x80},{0x74,0x80},{0x6d,0x80},{0x6d,0x80},{0x71,0x80},{0x70,0x80},{0x72,0x40},{0x75,0x100},{0x73,0x40},{0x73,0x40}
};
static const script_t scripts[46]={
 {1,{{0x3f,0x30}}},{1,{{0x3f,0x30}}},{1,{{0x44,0x30}}},{1,{{0x40,0x38}}},{1,{{0x41,0x3e}}},{1,{{0x42,0x30}}},{1,{{0x46,0x46}}},{1,{{0x52,0x14}}},{1,{{0x53,0x28}}},{1,{{0x4f,0x50}}},{1,{{0x4d,0x3c}}},{1,{{0x4e,0x3a}}},
 {1,{{0x5d,0x2e}}},{1,{{0x5e,0x3a}}},{1,{{0x58,0x30}}},{1,{{0x5b,0x4c}}},{1,{{0x55,0x2c}}},{1,{{0x5c,0x3e}}},{1,{{0x59,0x2a}}},{1,{{0x43,0x3e}}},{1,{{0x3e,0x34}}},{1,{{0x60,0x36}}},{2,{{0x56,0x30},{0x42,0x36}}},{1,{{0x7e,0x58}}},
 {1,{{0x69,0x44}}},{1,{{0x7f,0x48}}},{1,{{0x45,0x28}}},{1,{{0x4c,0x42}}},{1,{{0x48,0x38}}},{1,{{0x4b,0x3a}}},{1,{{0x51,0x40}}},{1,{{0x54,0x2a}}},{2,{{0x81,0x1c},{0x60,0x36}}},{1,{{0x5a,0x32}}},{2,{{0x81,0x1c},{0x43,0x3e}}},{1,{{0x5f,0x4a}}},
 {1,{{0x61,0x44}}},{2,{{0x4a,0x20},{0x60,0x36}}},{1,{{0x47,0x3c}}},{2,{{0x81,0x1c},{0x3f,0x36}}},{1,{{0x57,0x46}}},{1,{{0x50,0x58}}},{4,{{0x2f,0x0a},{0x2f,0x0a},{0x2f,0x0a},{0x7a,0xec}}},{1,{{0x6c,0xb4}}},{3,{{0x2f,0x0a},{0x2f,0x0a},{0x2f,0x0a}}},{1,{{0x49,0x3a}}}
};
static void mailbox_reset(wf_runtime_t*rt){rt->g.global_15d2=rt->g.global_15d3=rt->g.global_15d4=rt->g.global_15d5=0u;rt->g.global_15d6=rt->g.global_15d8=0u;}
static int character_valid(wf_runtime_t*rt,uint8_t event){unsigned i;uint8_t id=event>=0x0cu?(uint8_t)(event-0x0cu):event;for(i=0;i<WF_ACTOR_SLOTS;i++){wf_actor_raw_t*a=&rt->actors[i];if(bit(a,0,7)&&wf_u8(a,0x57u)==id)return bit(a,0x33u,0);}return 0;}
wf_status_t wf_r17_a0e0(wf_runtime_t*rt,wf_gameplay_binding_context_t*c){
 uint8_t active,event;uint16_t step;pair_t p;wf_status_t s;
 if(!rt)return WF_BAD_STATE;
 if(rt->g.global_15d5==0u){if(rt->g.global_15d3==0u)return WF_OK;rt->g.global_15d4=rt->g.global_15d2;rt->g.global_15d5=rt->g.global_15d3;}
 else{if(rt->g.global_15d3!=0u)rt->g.global_15d3=0u;rt->g.global_15d8=(uint16_t)(rt->g.global_15d8-1u);if(rt->g.global_15d8!=0u)return WF_OK;}
 active=rt->g.global_15d4;event=rt->g.global_15d5;
 if(active!=0x0fu){
  if(active>=24u||!character_valid(rt,active)){mailbox_reset(rt);return WF_OK;}p=character[active];rt->g.global_15d8=p.dur;
  s=emit_audio(rt,c,(uint16_t)(p.cmd|0x3100u));if(s!=WF_OK)return s;rt->g.global_15d4=0x0fu;return WF_OK;
 }
 if(event>=46u){mailbox_reset(rt);return WF_BAD_STATE;}step=rt->g.global_15d6;if(step>=scripts[event].count){mailbox_reset(rt);return WF_OK;}
 p=scripts[event].p[step];rt->g.global_15d8=p.dur;s=emit_audio(rt,c,(uint16_t)(p.cmd|0x3100u));if(s!=WF_OK)return s;rt->g.global_15d6=(uint16_t)(step+1u);return WF_OK;
}
