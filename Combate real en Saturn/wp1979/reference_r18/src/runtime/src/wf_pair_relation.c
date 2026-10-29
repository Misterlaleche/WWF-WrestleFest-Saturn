#include "wf_pair_relation.h"
#include "wf_gameplay_bindings.h"
#include "arcade_cpu_final_bridge.h"
#include "wf_r17_m68k_shadow.h"
#include "wf_r17_dependencies.h"

static int bit(const wf_actor_raw_t*a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static void clrbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~(1u<<b)));}
static uint16_t absdiff(uint16_t a,uint16_t b){uint16_t d=(uint16_t)(a-b);return a>=b?d:(uint16_t)(0u-d);}
static wf_status_t block(wf_gameplay_binding_context_t*c,uint32_t dep){if(c){c->last_blocked_phase=0x00f560u;c->last_blocked_dependency=dep;}return WF_BLOCKED;}

static int eligible_f7e8(const wf_runtime_t*rt,const wf_actor_raw_t*a){
 if(bit(a,WF_A_REL_FLAGS,7)||wf_be16(a,WF_A_LOCKOUT)!=0u||bit(a,WF_A_FLAGS32,4)||bit(a,WF_A_CPU_B7,7)||!bit(a,WF_A_STATE,7)||bit(a,WF_A_FLAGS32,0))return 0;
 if((rt->g.mode_0161&2u)!=0u&&!bit(a,WF_A_FLAGS33,2))return 0;
 switch(wf_u8(a,WF_A_STATE+1u)){
  case 0: case 1: break;
  case 4: if(wf_u8(a,WF_A_STATE4_LO)>1u)return 0; break;
  case 5: if(wf_u8(a,WF_A_SELECTOR_LO)!=0x72u)return 0; break;
  default:return 0;
 }
 return wf_be32(a,WF_A_LINK)==0u;
}

static const uint16_t f878[11][4]={
 {0x14,0x0c,0x11,0x00},{0x14,0x00,0x10,0x0c},{0x14,0x0b,0x00,0x0e},{0x0e,0x14,0x0b,0x00},
 {0x14,0x00,0x0d,0x08},{0x00,0x0e,0x14,0x09},{0x14,0x00,0x09,0x0e},{0x08,0x14,0x00,0x0d},
 {0x14,0x08,0x0d,0x00},{0x14,0x00,0x07,0x0c},{0x14,0x0e,0x00,0x08}
};
static const uint16_t f8d0[4]={0x14,0x0e,0x00,0x09};

static void commit_pair(wf_actor_raw_t*a,wf_actor_raw_t*b,uint32_t ap,uint32_t bp,int decision){
 if(decision==0){wf_set_be16(a,WF_A_STATE,0x000bu);wf_set_be16(b,WF_A_STATE,0x000bu);wf_set_be16(a,WF_A_TIMER44,0x2000u);wf_set_be16(b,WF_A_TIMER44,0x8000u);}
 else if(decision<0){wf_set_be16(a,WF_A_STATE,0x00ffu);wf_set_be16(b,WF_A_STATE,0x000cu);wf_set_be16(b,WF_A_CPU_BC,0u);wf_set_be16(a,WF_A_TIMER44,0u);wf_set_be16(b,WF_A_TIMER44,0x2000u);}
 else {wf_set_be16(a,WF_A_STATE,0x000cu);wf_set_be16(a,WF_A_CPU_BC,0u);wf_set_be16(b,WF_A_STATE,0x00ffu);wf_set_be16(a,WF_A_TIMER44,0x2000u);wf_set_be16(b,WF_A_TIMER44,0u);}
 uint16_t v=(uint16_t)(wf_be16(a,WF_A_X)+wf_be16(b,WF_A_X));v>>=1;wf_set_be16(a,WF_A_X,v);wf_set_be16(b,WF_A_X,v);
 v=(uint16_t)(wf_be16(a,WF_A_Y)+wf_be16(b,WF_A_Y));v>>=1;wf_set_be16(a,WF_A_Y,v);wf_set_be16(b,WF_A_Y,v);
 v=(uint16_t)(wf_be16(a,WF_A_PAIR_CYCLE)+1u);if(v>=4u)v=0u;wf_set_be16(a,WF_A_PAIR_CYCLE,v);
 v=(uint16_t)(wf_be16(b,WF_A_PAIR_CYCLE)+1u);if(v>=4u)v=0u;wf_set_be16(b,WF_A_PAIR_CYCLE,v);
 wf_set_be32(b,WF_A_LINK,ap);wf_set_be32(a,WF_A_LINK,bp);wf_set_be32(b,WF_A_TARGET,ap);wf_set_be32(a,WF_A_TARGET,bp);setbit(a,WF_A_FLAGS33,6);setbit(b,WF_A_FLAGS33,6);
}

static wf_status_t relation_f5e4(wf_runtime_t*rt,wf_actor_raw_t*a,wf_actor_raw_t*b,uint32_t ap,uint32_t bp,wf_gameplay_binding_context_t*c){
 int decision=0;
 if(rt->g.credited_route==0u){decision=wf_be16(a,WF_A_STAMINA)>=wf_be16(b,WF_A_STAMINA)?1:-1;commit_pair(a,b,ap,bp,decision);return WF_OK;}
 wf_set_be16(a,WF_A_TIMER52,0u);wf_set_be16(b,WF_A_TIMER52,0u);clrbit(a,WF_A_FLAGS34,2);clrbit(b,WF_A_FLAGS34,2);
 if(bit(a,WF_A_FLAGS33,1))clrbit(a,WF_A_ROLE,6);
 if(bit(b,WF_A_FLAGS33,1))clrbit(b,WF_A_ROLE,6);
 clrbit(a,WF_A_FLAGS34,7);clrbit(a,WF_A_FLAGS34,6);clrbit(b,WF_A_FLAGS34,7);clrbit(b,WF_A_FLAGS34,6);
 if(!bit(a,WF_A_ROLE,7)&&bit(a,WF_A_FLAGS33,5))decision=1;
 else if(!bit(b,WF_A_ROLE,7)&&bit(b,WF_A_FLAGS33,5))decision=-1;
 else if(bit(a,WF_A_ROLE,7)&&bit(a,WF_A_FLAGS33,5))decision=1;
 else if(bit(b,WF_A_ROLE,7)&&bit(b,WF_A_FLAGS33,5))decision=-1;
 else if(wf_u8(a,WF_A_STATE+1u)==4u&&wf_u8(a,WF_A_STATE4_LO)==1u)decision=-1;
 else if(wf_u8(b,WF_A_STATE+1u)==4u&&wf_u8(b,WF_A_STATE4_LO)==1u)decision=1;
 else if(!bit(a,WF_A_ROLE,7)&&bit(b,WF_A_ROLE,7)){
  unsigned idx=wf_be16(a,WF_A_PAIR_CYCLE);uint16_t base,sample,d2;uint32_t out;
  if(idx>=4u)return block(c,0x00f6e8u);
  if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(c,0x000021b4u);
  if((rt->g.mode_0161&1u)!=0u)base=f8d0[idx];
  else {if(rt->g.progression_0162>=11u)return block(c,0x00f878u);base=f878[rt->g.progression_0162][idx];}
  /* $F6D?-$F6F2: D2.W is table-derived and keeps its high WORD. R03 closes D4 as locally rebuilt. */
  wf_r17_m68k_move_w(&rt->m68k_regs,2u,base);
  wf_r17_m68k_clr_l(&rt->m68k_regs,4u);
  if((rt->g.mode_0161&1u)==0u)wf_r17_m68k_move_w(&rt->m68k_regs,4u,(uint16_t)(rt->g.progression_0162<<3));
  out=wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);
  sample=(uint16_t)(out&0x000fu);wf_r17_m68k_move_w(&rt->m68k_regs,0u,sample);
  d2=(uint16_t)((uint16_t)rt->m68k_regs.d[2]+sample);wf_r17_m68k_move_w(&rt->m68k_regs,2u,d2);
  decision=d2>=0x14u?1:(d2<0x0fu?-1:0);
 }else{
  uint8_t d4=(uint8_t)((rt->g.pipeline_mode>>1)&7u);decision=d4>=5u?1:(d4<2u?-1:0);
 }
 commit_pair(a,b,ap,bp,decision);return WF_OK;
}

static wf_status_t pair_scan_f574(wf_runtime_t*rt,wf_gameplay_binding_context_t*c){
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(i+1u));
  wf_actor_raw_t*a=&rt->actors[i];if(!bit(a,WF_A_ACTIVE,7)||!eligible_f7e8(rt,a))continue;
  uint32_t bp=wf_be32(a,WF_A_TARGET);unsigned j=0;wf_actor_raw_t*b=wf_actor_from_rom_ptr(rt,bp,&j);if(!b)return block(c,0x00f586u);
  if(!eligible_f7e8(rt,b))continue;
  wf_r17_m68k_move_b(&rt->m68k_regs,2u,wf_u8(a,WF_A_FACING));
  wf_r17_m68k_move_b(&rt->m68k_regs,3u,(uint8_t)(wf_u8(a,WF_A_FACING)^wf_u8(b,WF_A_FACING)));
  if((uint8_t)rt->m68k_regs.d[3]==0u)continue;
  wf_r17_m68k_move_b(&rt->m68k_regs,2u,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&4u));wf_r17_m68k_move_b(&rt->m68k_regs,3u,(uint8_t)(wf_u8(b,WF_A_FLAGS33)&4u));
  if((wf_u8(a,WF_A_FLAGS33)&4u)!=(wf_u8(b,WF_A_FLAGS33)&4u))continue;
  wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,WF_A_X));wf_r17_m68k_move_w(&rt->m68k_regs,3u,absdiff(wf_be16(a,WF_A_X),wf_be16(b,WF_A_X)));
  if((uint16_t)rt->m68k_regs.d[3]>=0x48u)continue;
  wf_r17_m68k_move_w(&rt->m68k_regs,2u,wf_be16(a,WF_A_Y));wf_r17_m68k_move_w(&rt->m68k_regs,3u,absdiff(wf_be16(a,WF_A_Y),wf_be16(b,WF_A_Y)));
  if((uint16_t)rt->m68k_regs.d[3]>=0x10u)continue;
  uint32_t saved[5];for(unsigned k=0;k<5;k++)saved[k]=rt->m68k_regs.d[k]; /* $F5E4 MOVEM */
  wf_status_t s=relation_f5e4(rt,a,b,wf_actor_rom_base(i),bp,c);
  for(unsigned k=0;k<5;k++)rt->m68k_regs.d[k]=saved[k];
  if(s!=WF_OK)return s;
 }
 return WF_OK;
}

static const uint16_t fdb4[12]={0x0930,0x0846,0x0b7e,0x0a1c,0x0b08,0x08bc,0x0a90,0x0a1c,0x09a6,0x0b08,0x0930,0x0b08};
static const uint8_t fdcc[12]={0x13,0x11,0x18,0x15,0x13,0x10,0x15,0x15,0x14,0x18,0x12,0x17};
static const uint16_t fdd8[10]={0x01d6,0x024c,0x024c,0x02c2,0x0372,0x0372,0x0372,0x0372,0x0372,0x0372};
static wf_status_t timed_flags_fd00(wf_runtime_t*rt,wf_gameplay_binding_context_t*c){
 wf_r17_m68k_moveq(&rt->m68k_regs,4u,5);
 for(unsigned i=0;i<6u;i++,wf_r17_m68k_move_w(&rt->m68k_regs,4u,(uint16_t)(5u-i))){
  wf_actor_raw_t*a=&rt->actors[i];
  if(bit(a,WF_A_FLAGS33,5)){
   uint16_t t=wf_be16(a,WF_A_C8);if(t!=0u){t=(uint16_t)(t-1u);wf_set_be16(a,WF_A_C8,t);if(t!=0u)continue;}
   wf_set_be16(a,WF_A_C8,0u);wf_set_be16(a,WF_A_C6,0u);clrbit(a,WF_A_FLAGS33,5); /* $8962 is ROM RTS. */
   continue;
  }
  uint8_t id=wf_u8(a,WF_A_INTERNAL_ID);if(id>=12u)return block(c,0x00fdb4u);
  if(!bit(a,WF_A_ROLE,7)){
   if(!bit(a,WF_A_FLAGS32,0)){wf_set_be16(a,WF_A_C8,0u);continue;}
   wf_r17_m68k_move_l(&rt->m68k_regs,1u,(uint32_t)id*2u);wf_r17_m68k_move_w(&rt->m68k_regs,2u,fdb4[id]);
   if(fdb4[id]<wf_be16(a,WF_A_C8)){setbit(a,WF_A_C6,6);wf_set_be16(a,WF_A_C8,0x03a9u);}
  }else{
   wf_r17_m68k_move_l(&rt->m68k_regs,1u,id);wf_r17_m68k_move_b(&rt->m68k_regs,2u,fdcc[id]);
   if(fdcc[id]<(uint8_t)wf_u8(a,WF_A_C7)){
    if(rt->g.progression_0162>=10u)return block(c,0x00fdd8u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(rt->g.progression_0162*2u));wf_r17_m68k_move_w(&rt->m68k_regs,2u,fdd8[rt->g.progression_0162]);
    wf_set_be16(a,WF_A_C8,fdd8[rt->g.progression_0162]);setbit(a,WF_A_FLAGS33,5);
   }
  }
 }
 return WF_OK;
}

wf_status_t wf_phase_pair_relation_f560(wf_runtime_t*rt,uint32_t addr,void*user){
 (void)addr;wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;if(!rt)return WF_BAD_STATE;
 wf_status_t s=pair_scan_f574(rt,c);if(s!=WF_OK)return s;
 s=wf_r17_f98c(rt,c);if(s!=WF_OK)return s;
 s=timed_flags_fd00(rt,c);if(s!=WF_OK)return s;
 s=wf_r17_a0e0(rt,c);if(s!=WF_OK)return s;
 return WF_OK;
}
