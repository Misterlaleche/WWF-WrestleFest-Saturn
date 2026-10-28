#include "wf_r18_frontend_effect.h"
#include <string.h>
typedef struct bridge {uint8_t *mem;unsigned *events;unsigned count;} bridge_t;
static const uint32_t palette[4]={0x180000u,0x182000u,0x186000u,0x188000u};
static wf_status_t read_palette(void*u,unsigned role,uint16_t out[256]){
 bridge_t*b=u;if(role>=4)return WF_BAD_STATE;
 for(unsigned i=0;i<256;i++){unsigned at=palette[role]+i/16u*128u+i%16u*2u;out[i]=(uint16_t)((b->mem[at]<<8)|b->mem[at+1]);}return WF_OK;
}
static wf_status_t write_palette(void*u,unsigned role,uint16_t in[256]){
 bridge_t*b=u;if(role>=4)return WF_BAD_STATE;
 for(unsigned i=0;i<256;i++){unsigned at=palette[role]+i/16u*128u+i%16u*2u;b->mem[at]=(uint8_t)(in[i]>>8);b->mem[at+1]=(uint8_t)in[i];}return WF_OK;
}
int frontend_effect_oracle(uint8_t *mem,uint32_t d[8],unsigned owner,unsigned events[128],uint8_t *polls){
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 bridge_t b={mem,events,0};wf_r18_frontend_effect_t c={.ram=mem+0x1c0000,.regs=&regs,.read_palette=read_palette,.write_palette=write_palette,.user=&b};
 if(wf_r18_frontend_effect_begin(&c,owner)!=WF_OK)return -1;
 if(wf_r18_frontend_effect_begin(&c,owner)!=WF_BAD_STATE)return -3;
 wf_r18_frontend_edge_t edge=wf_r18_frontend_effect_resume(&c,0);
 while(edge!=WF_R18_FRONTEND_EFFECT_DONE){
  if(edge>WF_R18_FRONTEND_WAIT_LOW||b.count==128)return -2;
  /* A held wrong status level must neither advance nor republish data. */
  uint8_t before[0xd000];wf_r18_frontend_effect_t old=c;
  wf_r17_m68k_shadow_t old_regs=regs;
  memcpy(before,mem+0x1c0000,0x4000);memcpy(before+0x4000,mem+0x180000,0x9000);
  for(unsigned repeat=0;repeat<3;repeat++)
   if(wf_r18_frontend_effect_resume(&c,edge!=WF_R18_FRONTEND_WAIT_HIGH)!=edge)return -4;
  if(memcmp(&old,&c,sizeof c)||memcmp(&old_regs,&regs,sizeof regs)||
     memcmp(before,mem+0x1c0000,0x4000)||memcmp(before+0x4000,mem+0x180000,0x9000))return -5;
  memcpy(polls+b.count*0xd000u,mem+0x1c0000,0x4000);
  memcpy(polls+b.count*0xd000u+0x4000u,mem+0x180000,0x9000);
  events[b.count++]=edge;
  edge=wf_r18_frontend_effect_resume(&c,edge==WF_R18_FRONTEND_WAIT_HIGH);
 }
 if(wf_r18_frontend_effect_resume(&c,0)!=WF_R18_FRONTEND_EFFECT_ERROR)return -6;
 memcpy(d,regs.d,32);return (int)b.count;
}
