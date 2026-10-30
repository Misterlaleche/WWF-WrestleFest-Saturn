#include "wf_r18_frontend_effect.h"
#include "wf_r18_frontend_fade.h"
#include <string.h>

static uint16_t get16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}

static wf_status_t capture(wf_r18_frontend_effect_t *c){
 uint16_t colors[256];
 for(unsigned role=0;role<4;role++){
  wf_status_t st=c->read_palette(c->user,role,colors);if(st!=WF_OK)return st;
  for(unsigned i=0;i<256;i++)put16(c->ram+0x1cd4u+role*512u+i*2u,colors[i]);
 }
 return WF_OK;
}
static wf_status_t transform(wf_r18_frontend_effect_t *c){
 uint16_t source[256],output[256];
 for(unsigned role=0;role<4;role++){
  for(unsigned i=0;i<256;i++)source[i]=get16(c->ram+0x1cd4u+role*512u+i*2u);
  wf_status_t st=wf_r18_frontend_fade_palette_26642(source,output,c->phase);
  if(st!=WF_OK)return st;
  for(unsigned i=0;i<256;i++)put16(c->ram+0x24d4u+role*512u+i*2u,output[i]);
 }
 /* $26642 does not preserve D6. Reveal's MOVEM excludes it. */
 c->regs->d[6]=((uint32_t)(get16(c->ram+0x24d2u)&15u)<<3)|c->phase;
 return WF_OK;
}
static wf_status_t publish(wf_r18_frontend_effect_t *c,int clear){
 uint16_t colors[256];
 for(unsigned role=0;role<4;role++){
  for(unsigned i=0;i<256;i++)colors[i]=clear?0:get16(c->ram+0x24d4u+role*512u+i*2u);
  wf_status_t st=c->write_palette(c->user,role,colors);if(st!=WF_OK)return st;
 }
 return WF_OK;
}

wf_status_t wf_r18_frontend_effect_begin(wf_r18_frontend_effect_t *c,uint32_t owner){
 if(!c||!c->ram||!c->regs||!c->regs->valid||!c->read_palette||
    !c->write_palette||c->active)return WF_BAD_STATE;
 if(owner!=0x26772u&&owner!=0x26844u&&owner!=0x264e2u)return WF_BAD_STATE;
 c->owner=owner==0x26772u?0u:owner==0x26844u?1u:2u;
 c->phase=c->owner==2u?7u:0u;c->step=1u;c->active=1u;
 memcpy(c->saved_d,c->regs->d,sizeof c->saved_d);
 c->saved_flags=get16(c->ram+0x76u);
 if(c->owner!=1u)memset(c->ram+0x1cd4u,0,4096);
 if(c->owner)c->ram[0x76u]&=0x7fu;
 if(c->owner==1u){
  if(transform(c)!=WF_OK){c->active=0;return WF_BLOCKED;}
  c->step=3u;
 }
 return WF_OK;
}
wf_r18_frontend_edge_t wf_r18_frontend_effect_resume(wf_r18_frontend_effect_t*c,unsigned high){
 wf_status_t st=WF_OK;
 if(!c||!c->active)return WF_R18_FRONTEND_EFFECT_ERROR;
 unsigned want=(c->step&1u)?1u:0u;
 if(!!high!=want)return want?WF_R18_FRONTEND_WAIT_HIGH:WF_R18_FRONTEND_WAIT_LOW;
 switch(c->step){
 case 1:
  st=capture(c);c->step=2u;break;
 case 2:
  if(c->owner==2u)st=transform(c);
  c->step=3u;break;
 case 3:
  st=publish(c,c->owner==0u);c->step=4u;break;
 case 4:
  if(c->owner==0u){
   wf_r17_m68k_move_w(c->regs,0,0xffffu);
   wf_r17_m68k_move_w(c->regs,1,0xffffu);
   c->active=0;return WF_R18_FRONTEND_EFFECT_DONE;
  }
  c->step=5u;break;
 case 5:case 6:case 7:c->step++;break;
 case 8:
  if((c->owner==1u&&c->phase==7u)||(c->owner==2u&&c->phase==0u)){
   if(c->owner==1u){
    memcpy(c->regs->d,c->saved_d,6u*sizeof(uint32_t));c->ram[0x76u]|=0x80u;
   }else{
    memcpy(c->regs->d,c->saved_d,sizeof c->saved_d);put16(c->ram+0x76u,c->saved_flags);
   }
   c->active=0;return WF_R18_FRONTEND_EFFECT_DONE;
  }
  c->phase=(uint8_t)(c->phase+(c->owner==1u?1u:255u));
  st=transform(c);c->step=3u;break;
 default:st=WF_BAD_STATE;break;
 }
 if(st!=WF_OK){c->active=0;return WF_R18_FRONTEND_EFFECT_ERROR;}
 return (c->step&1u)?WF_R18_FRONTEND_WAIT_HIGH:WF_R18_FRONTEND_WAIT_LOW;
}
