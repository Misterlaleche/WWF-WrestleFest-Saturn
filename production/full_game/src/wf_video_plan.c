#include "wf_video_plan.h"
#include <string.h>
static unsigned rw(const uint8_t*p){return (unsigned)((p[0]<<8)|p[1]);}
void wf_video_plan_init(wf_video_plan_t*p){memset(p,0,sizeof *p);memset(p->code_slot,255,sizeof p->code_slot);memset(p->slot_code,255,sizeof p->slot_code);}
bool wf_video_plan_map(wf_video_plan_t*p,unsigned layer,const uint8_t*raw,bool(*upload)(void*,unsigned,unsigned,unsigned),void*u){
 if(!p||layer>1||!raw||!upload)return false;
 memset(p->needed,0,sizeof p->needed);
 for(unsigned i=0;i<1024;i++){unsigned code=rw(raw+(layer?i*4+2:i*2))&4095;p->needed[code]=1;}
 unsigned hand=0;
 for(unsigned i=0;i<1024;i++){
  unsigned attr=rw(raw+(layer?i*4:i*2)),code=(layer?rw(raw+i*4+2):attr)&4095;
  unsigned slot=p->code_slot[layer][code];
  if(slot==65535){
   while(hand<1024&&p->slot_code[layer][hand]!=65535&&p->needed[p->slot_code[layer][hand]])hand++;
   if(hand==1024)return false;slot=hand++;
   if(!upload(u,layer,slot,code))return false;
   unsigned old=p->slot_code[layer][slot];if(old!=65535)p->code_slot[layer][old]=65535;
   p->slot_code[layer][slot]=(uint16_t)code;p->code_slot[layer][code]=(uint16_t)slot;
  }
  unsigned bank=layer?(attr&15):(attr>>12);
  uint32_t word=(layer?4096u:0)+slot*4u+((bank+(layer?32u:16u))<<16);
  if(layer)word|=(uint32_t)(attr&0xc0u)<<24;
  p->maps[layer][i]=word;
 }return true;
}
void wf_video_plan_text(wf_video_plan_t*p,const uint8_t*raw){
 for(unsigned i=0;i<2048;i++){
  unsigned a=rw(raw+i*4),b=rw(raw+i*4+2);
  uint32_t word=8192u+(a&255u)+((b&15u)<<8)+(((b>>4)&15u)<<16);
  p->text[i]=p->text[i+2048]=word;
 }
}
void wf_video_plan_palette(uint32_t*out,const uint8_t*raw){
 for(unsigned i=0;i<256;i++){unsigned v=rw(raw+(i/16)*128+(i%16)*2);out[i]=((v&15)*17u)|(((v>>4)&15)*17u<<8)|(((v>>8)&15)*17u<<16);}
}
bool wf_video_plan_sprites(const uint8_t*raw,unsigned records,wf_video_piece_t*out,unsigned capacity,unsigned*count){
 if(!raw||!out||!count||records>512)return false;unsigned n=0;
 for(unsigned i=0;i<records;i++){
  const uint8_t*r=raw+i*16;unsigned attr=rw(r+2);if(!(attr&1))continue;
  int x=(int)((rw(r+10)&255)|((attr&4)<<6));if(x>496)x-=512;
  int y=(int)((256-((rw(r)&255)|((attr&2)<<7)))&511)-16;
  unsigned chain=(attr>>5)&7,code=(rw(r+4)&255)|((rw(r+6)&255)<<8);
  unsigned mask=chain;mask|=mask>>1;mask|=mask>>2;code&=~mask;
  for(unsigned j=0;j<=chain;j++){
   int yy=(attr&8)?y-(int)chain*16+(int)j*16-8:y-(int)j*16-8;
   if(x>=320||x<=-16||yy>=240||yy<=-16)continue;
   if(n==capacity)return false;
   out[n++]=(wf_video_piece_t){(uint16_t)(code+j),(int16_t)x,(int16_t)yy,(uint8_t)(rw(r+8)&15),(uint8_t)(((attr&16)?1:0)|((attr&8)?2:0))};
  }
 }*count=n;return true;
}
