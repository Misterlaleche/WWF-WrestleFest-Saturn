#include "wf_spatial.h"
typedef struct { uint16_t x_limit,y_floor,value; } wf_spatial_row_t;
static const wf_spatial_row_t rows[18]={
{0x0238u,0x016Cu,0x0000u},{0x0240u,0x0142u,0x0001u},{0x0258u,0x0110u,0x0002u},{0x0280u,0x016Cu,0x0003u},{0x0280u,0x0142u,0x0004u},{0x0280u,0x0110u,0x0005u},{0x02B0u,0x016Cu,0x0006u},{0x02B8u,0x0142u,0x0007u},{0x02C0u,0x0110u,0x0008u},{0x0330u,0x016Cu,0x0009u},{0x0330u,0x0142u,0x000Au},{0x0340u,0x0110u,0x000Bu},{0x026Cu,0x0080u,0x0001u},{0x0318u,0x0080u,0x0004u},{0x03C4u,0x0080u,0x0007u},{0x0480u,0x0080u,0x000Au},{0x0280u,0x0110u,0x0000u},{0x0400u,0x0110u,0x0001u}};
wf_status_t wf_phase_spatial_2088e(wf_runtime_t *rt,uint32_t addr,void *user){
 (void)addr;(void)user;if(!rt)return WF_BAD_STATE;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
 for(unsigned slot=0;slot<WF_ACTOR_SLOTS;++slot){
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(slot+1u));
  wf_actor_raw_t*a=&rt->actors[slot];if((wf_u8(a,WF_A_ACTIVE)&0x80u)==0u)continue;
  unsigned i=(wf_u8(a,WF_A_FLAGS33)&0x04u)?0x0Cu:((wf_u8(a,WF_A_FLAGS32)&0x01u)?0x10u:0u);
  const uint16_t x=wf_be16(a,WF_A_X),y=wf_be16(a,WF_A_Y);
  wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)i);
  wf_r17_m68k_move_w(&rt->m68k_regs,2u,x);wf_r17_m68k_move_w(&rt->m68k_regs,3u,y);
  for(;i<0x12u;++i,wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)i)){
   /* $208CC MOVE.W then MULU.W writes all of D4. */
   wf_r17_m68k_move_l(&rt->m68k_regs,4u,i*6u);if(x>=rows[i].x_limit)continue;if(y<rows[i].y_floor)continue;wf_set_be16(a,WF_A_SPATIAL_CLASS,rows[i].value);break;}
 }return WF_OK;
}
