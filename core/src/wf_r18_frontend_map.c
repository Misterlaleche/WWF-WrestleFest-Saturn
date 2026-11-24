#include "wf_r18_frontend_map.h"
static uint16_t rw(wf_r18_frontend_map_t*c,unsigned at){return (uint16_t)((c->ram[at]<<8)|c->ram[at+1]);}
static uint32_t rl(wf_r18_frontend_map_t*c,unsigned at){return ((uint32_t)rw(c,at)<<16)|rw(c,at+2);}
static void ww(wf_r18_frontend_map_t*c,unsigned at,uint16_t v){c->ram[at]=(uint8_t)(v>>8);c->ram[at+1]=(uint8_t)v;}
static void wl(wf_r18_frontend_map_t*c,unsigned at,uint32_t v){ww(c,at,(uint16_t)(v>>16));ww(c,at+2,(uint16_t)v);}
static uint16_t d(wf_r18_frontend_map_t*c,unsigned n){return (uint16_t)c->regs->d[n];}
static void dw(wf_r18_frontend_map_t*c,unsigned n,uint16_t v){wf_r17_m68k_move_w(c->regs,n,v);}
static wf_status_t read(wf_r18_frontend_map_t*c,uint32_t at,unsigned n,uint32_t*out){
 uint8_t b[4];if(at>0x80000u||n>0x80000u-at||c->read_rom(c->user,at,b,n)!=WF_OK){c->blocked_dependency=at;return WF_BLOCKED;}
 *out=0;for(unsigned i=0;i<n;i++)*out=(*out<<8)|b[i];return WF_OK;
}
static wf_status_t store(wf_r18_frontend_map_t*c,uint32_t at,uint32_t value,unsigned n){
 wf_status_t st=c->store(c->user,at,value,n);if(st!=WF_OK)c->blocked_dependency=at;return st;
}
static unsigned side(wf_r18_frontend_map_t*c){return !!(c->ram[0x17fe]&0x80u);}
/* $26ADA preserves D0-D2 and returns carry iff no cell boundary changed. */
static int boundary(wf_r18_frontend_map_t*c){
 uint16_t x=rw(c,0x17f6)>>5,y=rw(c,0x17fa)>>5;
 ww(c,0x17d6,x);ww(c,0x17d8,y);
 unsigned direction=x!=rw(c,0x17e2)?(x<rw(c,0x17e2)?3u:1u):y!=rw(c,0x17e4)?(y<rw(c,0x17e4)?2u:0u):255u;
 ww(c,0x16d4,(uint16_t)direction);return direction==255u;
}
static wf_status_t select_side(wf_r18_frontend_map_t*c){
 unsigned s=side(c),mode=rw(c,0x7e);if(mode>=7u)return WF_BLOCKED;
 if(read(c,(s?0x270bcu:0x2709cu)+mode*4u,4,&c->a[2])!=WF_OK)return WF_BLOCKED;
 c->a[6]=s?0x82000u:0x80000u;c->a[5]=s?0x1c1756u:0x1c16d6u;return WF_OK;
}
/* $26CA8 and $26D56 preserve D1-D4/A2, produce A3/A4 only. */
static wf_status_t cell(wf_r18_frontend_map_t*c,uint16_t coords){
 unsigned s=side(c);uint16_t x=(uint16_t)(rw(c,0x17e2)+(int8_t)(coords>>8));
 uint16_t y=(uint16_t)(0x40u-(uint16_t)(rw(c,0x17e4)+(int8_t)coords));
 uint16_t off=(uint16_t)((x/10u)*4u+(y>>3)*32u);uint32_t page;
 if(read(c,c->a[2]+(int16_t)off,4,&page)!=WF_OK)return WF_BLOCKED;
 c->a[3]=page+(x%10u)*(s?4u:8u)+(y&7u)*(s?0x50u:0xa0u);
 c->a[4]=c->a[6]+((x+16u)&15u)*(s?4u:8u)+(y&15u)*(s?0x80u:0x100u);
 return WF_OK;
}
static wf_status_t scroll(wf_r18_frontend_map_t*c){
 if(c->ram[0x6f]&0x10u)return WF_OK;
 for(unsigned s=0;s<2;s++){
  if(store(c,0x100000u+s*4u,rw(c,0x17e6+s*4u),2)!=WF_OK)return WF_BLOCKED;
  if(store(c,0x100002u+s*4u,(uint16_t)(0x800u-rw(c,0x17eeu+s*4u)),2)!=WF_OK)return WF_BLOCKED;
 }
 return WF_OK;
}
static wf_status_t transfer(wf_r18_frontend_map_t*c){
 static const uint8_t source_off[2][4]={{0,4,0x50,0x54},{0,2,0x28,0x2a}};
 static const uint8_t dest_off[2][4]={{0,4,0x80,0x84},{0,2,0x40,0x42}};
 for(unsigned s=0;s<2;s++){
  unsigned mask=s?0x40u:0x80u;int pending=c->ram[0x16d2]&mask;c->ram[0x16d2]&=(uint8_t)~mask;
  if(!pending)continue;
  for(unsigned i=0;i<16;i++){
   unsigned off=(s?0x1756u:0x16d6u)+i*8u;uint32_t from=rl(c,off),to=rl(c,off+4);
   for(unsigned j=0;j<4;j++){uint32_t value;unsigned n=s?2u:4u;
    if(read(c,from+source_off[s][j],n,&value)!=WF_OK||store(c,to+dest_off[s][j],value,n)!=WF_OK)return WF_BLOCKED;
   }
  }
 }
 return WF_OK;
}
static wf_status_t setup(wf_r18_frontend_map_t*c){
 for(unsigned s=0;s<2;s++){
  c->ram[0x17fe]=(uint8_t)((c->ram[0x17fe]&0x7fu)|(s?0x80u:0));
  uint16_t x=rw(c,0x1804),y=rw(c,0x1806);
  ww(c,0x17f6,x);ww(c,0x17fa,y);ww(c,0x17e6,x);ww(c,0x17ea,x);ww(c,0x17ee,y);ww(c,0x17f2,y);
  boundary(c);ww(c,0x17e2,rw(c,0x17d6));ww(c,0x17e4,rw(c,0x17d8));
  if(scroll(c)!=WF_OK)return WF_BLOCKED;
  ww(c,0x17e4,(uint16_t)(rw(c,0x17e4)+4u));ww(c,0x17e2,(uint16_t)(rw(c,0x17e2)-3u));
  if(select_side(c)!=WF_OK)return WF_BLOCKED;
  dw(c,2,15);
  do{
   dw(c,3,0);
   do{
    wf_r17_m68k_clr_l(c->regs,1);
    if(cell(c,0)!=WF_OK)return WF_BLOCKED;
    wl(c,c->a[5]-0x1c0000u,c->a[3]);c->a[5]+=4;
    wl(c,c->a[5]-0x1c0000u,c->a[4]);c->a[5]+=4;
    ww(c,0x17e2,(uint16_t)(rw(c,0x17e2)+1u));dw(c,3,(uint16_t)(d(c,3)+1u));
   }while(d(c,3)<16u);
   ww(c,0x17e2,(uint16_t)(rw(c,0x17e2)-16u));c->ram[0x16d2]|=(uint8_t)(s?0x40u:0x80u);
   if(transfer(c)!=WF_OK)return WF_BLOCKED;
   c->a[5]-=0x80;ww(c,0x17e4,(uint16_t)(rw(c,0x17e4)-1u));dw(c,2,(uint16_t)(d(c,2)-1u));
  }while(d(c,2)!=0xffffu);
  boundary(c);ww(c,0x17da,rw(c,0x17d6));ww(c,0x17dc,rw(c,0x17d8));ww(c,0x17de,rw(c,0x17d6));ww(c,0x17e0,rw(c,0x17d8));
 }
 c->ram[0x185a]=0xff;c->a[0]=0x26e9e;dw(c,0,rw(c,0x7e));uint32_t value;
 if(read(c,c->a[0]+(int16_t)d(c,0),1,&value)!=WF_OK)return WF_BLOCKED;
 wf_r17_m68k_move_b(c->regs,0,(uint8_t)value);return store(c,0x140010,d(c,0),2);
}
static wf_status_t update(wf_r18_frontend_map_t*c){
 if(rw(c,0x16d2))return WF_OK;
 for(unsigned s=0;s<2;s++){
  c->ram[0x17fe]=(uint8_t)((c->ram[0x17fe]&0x7fu)|(s?0x80u:0));
  ww(c,0x17f6,rw(c,0x17e6+s*4u));ww(c,0x17fa,rw(c,0x17eeu+s*4u));
  ww(c,0x17e2,rw(c,0x17da+s*4u));ww(c,0x17e4,rw(c,0x17dc+s*4u));
  if(boundary(c))continue;
  c->ram[0x16d2]|=(uint8_t)(s?0x40u:0x80u);
  if(select_side(c)!=WF_OK)return WF_BLOCKED;
  uint32_t table;if(read(c,0x26f82u+rw(c,0x16d4)*4u,4,&table)!=WF_OK)return WF_BLOCKED;c->a[0]=table;
  /* $26C8A saves D0-D2/A0-A5 around its sixteen queue entries. */
  uint32_t saved_a3=c->a[3],saved_a4=c->a[4];
  for(unsigned i=0;i<16;i++){uint32_t coords;
   if(read(c,table+i*2u,2,&coords)!=WF_OK||cell(c,(uint16_t)coords)!=WF_OK)return WF_BLOCKED;
   unsigned off=(s?0x1756u:0x16d6u)+i*8u;wl(c,off,c->a[3]);wl(c,off+4,c->a[4]);
  }
  c->a[3]=saved_a3;c->a[4]=saved_a4;
  unsigned dir=rw(c,0x16d4),at=(dir&1u)?0x17da+s*4u:0x17dc+s*4u;
  ww(c,at,(uint16_t)(rw(c,at)+(dir<2u?1u:0xffffu)));
 }
 return WF_OK;
}
wf_status_t wf_r18_frontend_map_call(wf_r18_frontend_map_t*c,uint32_t owner){
 if(!c||!c->ram||!c->regs||!c->regs->valid||!c->a||!c->read_rom||!c->store)return WF_BAD_STATE;
 c->blocked_dependency=0;wf_status_t st;
 switch(owner){case 0x26e66:st=setup(c);break;case 0x26a42:st=update(c);break;
 case 0x26dac:st=transfer(c);break;case 0x26e1a:st=scroll(c);break;default:st=WF_BLOCKED;break;}
 if(st!=WF_OK&&!c->blocked_dependency)c->blocked_dependency=owner;
 return st;
}
