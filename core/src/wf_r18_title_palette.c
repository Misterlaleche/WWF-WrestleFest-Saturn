#include "wf_r18_title_palette.h"
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint16_t d(wf_r18_title_palette_context_t*c,unsigned n){return (uint16_t)c->regs->d[n];}
static void dw(wf_r18_title_palette_context_t*c,unsigned n,uint16_t v){wf_r17_m68k_move_w(c->regs,n,v);}
static void dq(wf_r18_title_palette_context_t*c,unsigned n,int8_t v){wf_r17_m68k_moveq(c->regs,n,v);}

/* Original B476, including partial register writes and DBRA-inclusive
 * rotation count. No arithmetic interpolation replaces nibble rotation. */
static void target(wf_r18_title_palette_context_t*c,const uint8_t table[32],unsigned bank){
 unsigned color=d(c,0);uint16_t value=r16(table+color*2u);
 dq(c,2,0);dw(c,2,(uint16_t)(color*2u));
 dw(c,3,value);dw(c,2,r16(c->ram+0xf2cu+bank*32u+color*2u));
 dw(c,5,(uint16_t)(r16(c->ram+0x16c0u+bank*2u)&15u));
 unsigned rotations=d(c,5)+1u;
 while(rotations--){
  dw(c,4,(uint16_t)((d(c,3)<<8)&0xf00u));
  dw(c,3,(uint16_t)(((d(c,3)>>4)&255u)|d(c,4)));
  dw(c,5,(uint16_t)(d(c,5)-1u));
 }
}
static void approach(wf_r18_title_palette_context_t*c,const uint8_t table[32],unsigned bank){
 uint32_t saved[8];memcpy(saved,c->regs->d,sizeof saved);
 uint8_t *flag=c->ram+0x16c0u+bank*2u;
 unsigned fresh=!(*flag&0x80u);*flag|=0x80u;
 if(fresh)memcpy(c->ram+0xf2cu+bank*32u,table,32);
 target(c,table,bank);
 uint16_t current=d(c,2),wanted=d(c,3);
 unsigned a=wanted&15u,b=(wanted>>4)&15u,e=(wanted>>8)&15u;
 if(a!=b||b!=e){
  static const uint16_t step[3]={1u,16u,256u},mask[3]={15u,240u,3840u};
  uint8_t *out=c->ram+0xf2cu+bank*32u+d(c,0)*2u;
  uint16_t value=r16(out);
  for(unsigned j=3;j-->0;){
   if((current&mask[j])>(wanted&mask[j]))value=(uint16_t)(value-step[j]);
   else if((current&mask[j])<(wanted&mask[j]))value=(uint16_t)(value+step[j]);
  }
  w16(out,value);
 }
 memcpy(c->regs->d,saved,sizeof saved);
}
wf_status_t wf_r18_title_palette_b312(wf_r18_title_palette_context_t *c){
 uint8_t tables[2][32];uint32_t saved[2];
 if(!c||!c->ram||!c->regs||!c->regs->valid||!c->read_rom)return WF_BAD_STATE;
 wf_status_t st=c->read_rom(c->user,0x2e82u,tables,sizeof tables);if(st!=WF_OK)return st;
 memcpy(saved,c->regs->d,sizeof saved);
 if(!(c->ram[0x16c2u]&0x40u)){
  dq(c,1,0);
  for(unsigned bank=0;bank<2;bank++){
   dq(c,0,7);
   for(unsigned color=7;color<16;color++){
    approach(c,tables[bank],bank);dw(c,0,(uint16_t)(d(c,0)+1u));
   }
   dw(c,1,(uint16_t)(d(c,1)+1u));
  }
  c->ram[0x16c0u]|=0x40u;c->ram[0x16c2u]|=0x40u;
 }
 dq(c,1,0);
 for(unsigned bank=0;bank<2;bank++){
  dq(c,0,7);
  for(unsigned color=7;color<16;color++){
   target(c,tables[bank],bank);
   if(d(c,2)!=d(c,3))goto done;
   dw(c,0,(uint16_t)(d(c,0)+1u));
  }
  dw(c,1,(uint16_t)(d(c,1)+1u));
 }
 c->ram[0x16c1u]++;c->ram[0x16c3u]++;
 wf_r17_m68k_move_b(c->regs,2,c->ram[0x16c3u]);
 if(c->ram[0x16c3u]==3u)c->ram[0x16c1u]=c->ram[0x16c3u]=0;
done:
 memcpy(c->regs->d,saved,sizeof saved);return WF_OK;
}
wf_status_t wf_r18_title_palette_irq_b4b8(wf_r18_title_palette_context_t *c){
 wf_r17_fg0_write_t writes[18];unsigned n=0;
 if(!c||!c->ram||!c->regs||!c->regs->valid||!c->write)return WF_BAD_STATE;
 if(!(c->ram[0x16c2u]&0x40u))return WF_OK;
 uint8_t flag=(uint8_t)(c->ram[0x16c2u]+1u);c->ram[0x16c2u]=flag;
 if((flag&15u)!=3u)return WF_OK;
 c->ram[0x16c2u]=(uint8_t)(flag-3u);
 for(unsigned bank=0;bank<2;bank++)for(unsigned color=7;color<16;color++)
  writes[n++]=(wf_r17_fg0_write_t){0x180300u+bank*128u+color*2u,
            r16(c->ram+0xf2cu+bank*32u+color*2u),2};
 wf_status_t st=c->write(c->runtime,writes,n,c->user);if(st!=WF_OK)return st;
 c->ram[0x16c0u]&=0xbfu;c->ram[0x16c2u]&=0xbfu;
 return WF_OK;
}
