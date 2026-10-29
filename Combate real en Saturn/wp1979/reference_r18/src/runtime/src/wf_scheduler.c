#include <string.h>
#include "wf_runtime.h"
#include "wf_r17_m68k_shadow.h"

typedef struct { uint32_t addr; } phase_t;
static const phase_t branch_a[] = {
 {0x00514E},{0x008B3A},{0x02088E},{0x020958},{0x026936},{0x02983C},{0x00F560},{0x00F18A},{0x01C150},{0x01F914},{0x00F4C2},{0x024062},{0x024E58},{0x00FDEE},{0x010120},{0x010E6A},{0x007A00},{0x0088A2},{0x008710},{0x0262D2},{0x00BB14},{0x007366},{0x008CF8},{0x009052},{0x00899C},{0x008ECC},{0x002836},{0x0018C4},{0x0298EC}
};
static const phase_t branch_b[] = {
 {0x0285DA},{0x026A42},{0x026936},{0x02983C},{0x00514E},{0x008B3A},{0x02088E},{0x020958},{0x00F560},{0x00F18A},{0x01C150},{0x01F914},{0x00F4C2},{0x00FDEE},{0x010120},{0x010E6A},{0x007548},{0x008710},{0x007A00},{0x007366},{0x008CF8},{0x00BCFE},{0x00C04A},{0x00C710},{0x002836},{0x0298EC}
};

static uint16_t low16(uint32_t v){return (uint16_t)(v&0xffffu);}

void wf_runtime_init(wf_runtime_t *rt, wf_phase_binding_t *b,size_t n){
 memset(rt,0,sizeof(*rt));rt->bindings=b;rt->binding_count=n;wf_r17_m68k_software_reset(&rt->m68k_regs);
}
void wf_runtime_vblank_ack(wf_runtime_t *rt){
 if(!rt)return;
 rt->g.frame_handshake[1]=(uint8_t)(rt->g.frame_handshake[1]|0x02u);
 rt->vblank_ack=1u;
}
wf_status_t wf_dispatch_phase(wf_runtime_t *rt,uint32_t addr){
 if(rt->trace_count<64) rt->trace[rt->trace_count++]=addr;
 for(size_t i=0;i<rt->binding_count;i++) if(rt->bindings[i].rom_addr==addr) return rt->bindings[i].fn(rt,addr,rt->bindings[i].user);
 return WF_BLOCKED; /* Never silently skip an unknown ROM phase. */
}

/* $10F2.. normal routes that genuinely return to $F9A. Scene-transition paths
 * remain fail-closed until their complete owner contracts are
 * present; no synthetic register restore is allowed here. */
static wf_status_t post_ack_to_f9a(wf_runtime_t *rt){
 if(rt->g.credited_route==0u){
  static const uint16_t threshold_119a[14]={
   0x0350u,0x02e0u,0x0410u,0x03a0u,0x0290u,0x03a0u,0x04b0u,
   0x03b0u,0x0650u,0x0480u,0x0320u,0x0330u,0x0340u,0x0360u
  };
  uint16_t idx=rt->g.global_0166;
  uint32_t d0;
  if(idx>=14u)return WF_BLOCKED;
  /* $1108 CLR.L D0; $110A MOVE.W progression,D0; $1110 LSL.W #1,D0;
   * $1112 table WORD -> D0. The table load is WORD and upper 16 remain zero. */
  d0=(uint32_t)threshold_119a[idx];
  wf_r17_m68k_move_l(&rt->m68k_regs,0u,d0);
  if(d0>=rt->g.frame_counter)return WF_OK; /* BCC $F9A */
  return WF_BLOCKED; /* $8F50/$21E6/scene jump owner, outside normal next-frame route. */
 }
 if((rt->g.mode_0161&0x01u)!=0u){
  wf_status_t s=wf_dispatch_phase(rt,0x2017au);
  if(s!=WF_OK)return s;
  if((rt->g.global_16c4>>8)!=0u)return WF_BLOCKED; /* $1B4E */
  if((uint8_t)rt->g.global_16c4==3u)return WF_BLOCKED; /* $1AFC */
  return WF_OK; /* $1172 JMP $F9A */
 }
 /* $1178/$1186 are WORD writes into D0 followed by ANDI.W #$C000. */
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(&rt->actors[0],WF_A_RESULT));
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])&0xc000u));
 if(low16(rt->m68k_regs.d[0])!=0u)return WF_BLOCKED; /* $11B6 path */
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(&rt->actors[2],WF_A_RESULT));
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(low16(rt->m68k_regs.d[0])&0xc000u));
 if(low16(rt->m68k_regs.d[0])!=0u)return WF_BLOCKED; /* $11B6 path */
 return WF_OK;
}

wf_status_t wf_runtime_frame(wf_runtime_t *rt){
 const phase_t *p; size_t n;
 if(!rt||!rt->m68k_regs.valid)return WF_BAD_STATE;

 /* Resume only the real VBlank wait/post-ACK boundary; never rerun the frame
  * owners while $1C006F.bit1 is still clear. */
 if(rt->frame_stage==1u){
  if(!rt->vblank_ack || (rt->g.frame_handshake[1]&0x02u)==0u)return WF_BLOCKED;
  /* $10F2 ADDQ.L #1,$1C0080 executes exactly once after ACK. */
  rt->g.frame_counter+=1u;
  /* $0082/$0083 are the low WORD of the same LONG, not independent RAM.
   * External bootstrap must seed these legacy aliases coherently too. */
  rt->g.global_0082=(uint8_t)(rt->g.frame_counter>>8);
  rt->g.pipeline_mode=(uint8_t)rt->g.frame_counter;
  rt->frame_stage=2u;
 }
 if(rt->frame_stage==2u){
  if(post_ack_to_f9a(rt)!=WF_OK)return WF_BLOCKED;
  rt->frame_stage=0u;
  rt->frame_ready=0u;
  return WF_OK;
 }

 /* $F9A CLR.W $1C006E. This does not alter D0..D7. */
 rt->trace_count=0u;rt->frame_ready=0u;rt->vblank_ack=0u;
 if(rt->frame_stage==3u){
  /* The direct boot owner has already executed $F9A..$FA0. */
  rt->frame_stage=0u;
 }else{
  rt->g.frame_handshake[0]=0u;rt->g.frame_handshake[1]=0u;
  if(!rt->g.rng_state_valid)return WF_BLOCKED;
  wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);
 }

 if((rt->g.pipeline_mode&1u)==0u){p=branch_a;n=sizeof(branch_a)/sizeof(branch_a[0]);}
 else {p=branch_b;n=sizeof(branch_b)/sizeof(branch_b[0]);}
 for(size_t i=0u;i<n;i++){
  wf_status_t s=wf_dispatch_phase(rt,p[i].addr);
  if(s!=WF_OK)return s;
 }
 /* $1048/$10E0 BSET #0,$1C006F, then wait for VBlank bit1. */
 rt->g.frame_handshake[1]=(uint8_t)(rt->g.frame_handshake[1]|0x01u);
 rt->frame_ready=1u;rt->frame_stage=1u;
 return WF_BLOCKED;
}
