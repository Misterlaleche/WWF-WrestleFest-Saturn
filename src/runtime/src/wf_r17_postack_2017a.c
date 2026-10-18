#include <string.h>
#include "wf_r17_postack_2017a.h"
/* Raw $1C0156 indexed writes deliberately cross named global fields. */
static void status_write(wf_globals_raw_t *g,unsigned i){
 if(i<4u)g->global_0156[i]=0x3001u;
 else switch(i){
 case 4:g->global_015e=0x3001u;break;
 case 5:g->mode_0160=0x30u;g->mode_0161=1u;break;
 case 6:g->progression_0162=0x3001u;break;
 case 7:g->global_0164=0x3001u;break;
 case 8:g->global_0166=0x3001u;break;
 }
}
wf_status_t wf_r17_postack_2017a(wf_runtime_t *rt,wf_r17_ref_simple_fn clear_206fe,void *user){
 wf_r17_m68k_shadow_t *d;unsigned selected=0u;
 if(!rt||!rt->m68k_regs.valid)return WF_BAD_STATE;
 d=&rt->m68k_regs;
 if(!rt->g.credited_route||!(rt->g.mode_0161&1u))return WF_OK;
 if(rt->g.global_16c4&1u){
  wf_r17_m68k_move_l(d,0,0);wf_r17_m68k_move_l(d,3,3);
  for(int i=3;i>=0;i--){
   wf_r17_m68k_move_w(d,0,(uint16_t)(i*2));
   if(rt->g.global_0156[i])return WF_OK;
   wf_r17_m68k_move_w(d,3,(uint16_t)(i-1));
  }
  rt->g.global_16c4|=2u;return WF_OK;
 }
 wf_r17_m68k_move_l(d,1,3);wf_r17_m68k_move_l(d,3,0);
 for(int i=3;i>=0;i--){
  wf_r17_m68k_move_b(d,3,(uint8_t)i);wf_r17_m68k_move_w(d,3,(uint16_t)(d->d[3]<<1));
  if(rt->g.royal_slots_1586[i])return WF_OK;
  wf_r17_m68k_move_w(d,1,(uint16_t)(i-1));
 }
 wf_r17_m68k_move_l(d,2,8);wf_r17_m68k_move_l(d,3,0);wf_r17_m68k_move_l(d,4,0);wf_r17_m68k_move_l(d,0,0);wf_r17_m68k_move_l(d,6,0);
 for(unsigned i=0;i<9u;i++){
  wf_actor_raw_t *a=&rt->actors[i];
  if(wf_u8(a,0)&0x80u){
   if(!(wf_u8(a,0x32)&0x10u)){
    if(!(wf_u8(a,0x56)&0x80u)){selected=i;wf_r17_m68k_move_l(d,5,wf_actor_rom_base(i));wf_r17_m68k_move_w(d,3,(uint16_t)(d->d[3]+1u));}
    else wf_r17_m68k_move_w(d,4,(uint16_t)(d->d[4]+1u));
   }else if(!(wf_u8(a,0x56)&0x80u)){wf_r17_m68k_move_w(d,6,(uint16_t)(i*2u));status_write(&rt->g,i);}
  }
  wf_r17_m68k_move_w(d,0,(uint16_t)(i+1u));wf_r17_m68k_move_w(d,2,(uint16_t)(7u-i));
 }
 if(d->d[3]==0u){rt->g.global_16c4|=1u;rt->g.global_169e=0;return WF_OK;}
 if((uint8_t)d->d[3]!=1u||d->d[4])return WF_OK;
 /* $20254 BSET precedes the external transfer. Missing transfer is blocked. */
 rt->g.global_16c4|=4u;
 if(!clear_206fe)return WF_BLOCKED;
 {uint32_t saved[8];wf_status_t s;memcpy(saved,d->d,sizeof(saved));s=clear_206fe(rt,user);memcpy(d->d,saved,sizeof(saved));if(s!=WF_OK)return s;}
 rt->g.global_169e=0;wf_set_be16(&rt->referee,0x20,0x8009u);rt->g.global_0092=d->d[5];
 wf_set_be16(&rt->actors[selected],0x20,1);wf_set_be16(&rt->actors[selected],0xae,12);
 wf_r17_m68k_move_l(d,0,3);
 for(unsigned i=4;i<8;i++){
  wf_actor_raw_t *a=&rt->actors[i];
  if(!(wf_u8(a,0)&0x80u)){wf_set_u8(a,0,(uint8_t)(wf_u8(a,0)|0x80u));wf_set_be16(a,0x44,0);wf_set_be16(a,0x32,0);wf_set_be16(a,0x20,5);wf_set_be16(a,0x60,0x8e);return WF_OK;}
  wf_r17_m68k_move_w(d,0,(uint16_t)(6u-i));
 }
 return WF_OK;
}
