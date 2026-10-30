#include "wf_r18_match_aux.h"
#include "wf_physics.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t rl(const uint8_t*p){return ((uint32_t)rw(p)<<16)|rw(p+2);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void wl(uint8_t*p,uint32_t v){ww(p,(uint16_t)(v>>16));ww(p+2,(uint16_t)v);}
wf_status_t wf_r18_match_aux_call(wf_r18_prematch_t*m,wf_r18_video_t*v,uint32_t owner){
 if(!m||!m->ram||!m->regs||!m->regs->valid)return WF_BAD_STATE;
 m->dependency=0;
 if(owner==0x2208u){
  uint32_t at=m->a[0]&0xffffffu;
  if(at<0x1c0000u||at>0x1c3fa0u){m->dependency=owner;return WF_BLOCKED;}
  uint8_t*p=m->ram+at-0x1c0000u;unsigned mode=p[1];
  if(mode>2u){m->dependency=0x2224u;return WF_BLOCKED;}
  if(mode==1u){
   unsigned angle=p[0x2d],speed=p[0x2b],q=angle>>6,i=q&1u?64u-(angle&63u):angle&63u;
   uint32_t x=(uint32_t)wf_qx[i]*speed,y=(uint32_t)wf_qy[i]*speed;
   if(q>=2u)x=0u-x;
   if(q==1u||q==2u)y=0u-y;
   wl(p+6,rl(p+6)+x);wl(p+10,rl(p+10)+y);
  }else if(mode==2u){
   for(unsigned i=0;i<3;i++)wl(p+6u+i*4u,rl(p+6u+i*4u)+(uint32_t)((int32_t)(int16_t)rw(p+0x58u+i*2u)*256));
   uint16_t vz=(uint16_t)(rw(p+0x5c)-rw(p+0x5e));
   /* Preserve the original BPL/BCC unsigned sequence at $22AA. */
   if((vz&0x8000u)&&vz<0x2000u)vz=0x2000u;
   ww(p+0x5c,vz);
  }
  return WF_OK;
 }
 if(owner==0xb2e4u){
  if(!v){m->dependency=owner;return WF_BLOCKED;}
  /* Original B2E4: five rows of36 LONGs, with a0x100-byte pitch.
   * MOVEM restores D0/D1/A0; all caller registers remain unchanged. */
  for(unsigned row=0;row<5u;row++)memset(v->fg0+0x1908u+row*0x100u,0,36u*4u);
  memset(v->hud_dirty,0xff,sizeof v->hud_dirty);
  return WF_OK;
 }
 if(owner==0xb29eu){
  if(!v||!m->effect){m->dependency=owner;return WF_BLOCKED;}
  uint32_t saved_d[5],saved_a[4];memcpy(saved_d,m->regs->d,sizeof saved_d);memcpy(saved_a,m->a,sizeof saved_a);
  wl(v->fg0+0x1d08,0xfffaff14u);
  for(unsigned i=1;i<=28;i++)wl(v->fg0+0x1d08+i*4u,0xfffbff14u);
  memset(v->hud_dirty,0xff,sizeof v->hud_dirty);
  m->a[0]=0xc1d78u;m->a[2]=0xc1a7cu;wf_r17_m68k_move_w(m->regs,0,0xffffu);
  wf_r17_m68k_move_w(m->regs,2,0xdu);wf_r17_m68k_move_w(m->regs,7,0x10u);
  wf_status_t st=m->effect(m->user,0x9acau,m);if(st!=WF_OK){m->dependency=0x9acau;return st;}
  memcpy(m->regs->d,saved_d,sizeof saved_d);memcpy(m->a,saved_a,sizeof saved_a);return WF_OK;
 }
 m->dependency=owner;return WF_BLOCKED;
}
