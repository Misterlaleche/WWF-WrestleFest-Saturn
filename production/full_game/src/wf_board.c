#include "wf_board.h"
#include <string.h>
void wf_board_init(wf_board_t*b,uint8_t d1,uint8_t d2){
 memset(b,0,sizeof *b);b->line=271;
 b->ports[0]=(uint16_t)(0xcfffu|((d2&0xc0u)<<6));
 b->ports[1]=(uint16_t)(0xc0ffu|((d2&0x3fu)<<8));
 b->ports[2]=(uint16_t)(0xc0ffu|((d1&0x3fu)<<8));
 b->ports[3]=(uint16_t)(0xfcffu|((d1&0xc0u)<<2));
}
void wf_board_input(wf_board_t*b,unsigned port,uint8_t pressed,bool coin){
 if(port>=4)return;
 b->ports[port]=(uint16_t)((b->ports[port]&0xff00u)|(uint8_t)~pressed);
 if(port==0)b->ports[0]=(uint16_t)((b->ports[0]|0x100u)&(coin?0xfeffu:0xffffu));
}
bool wf_board_high(const wf_board_t*b){return b->line>=8&&b->line<248;}
static bool contains(uint32_t at,unsigned n,uint32_t base,unsigned size){return (n==1||n==2||n==4)&&at>=base&&at-base<size&&n<=size-(at-base);}
bool wf_board_read(const wf_board_t*b,uint32_t at,uint32_t*out,unsigned n){
 if(!b||!out)return false;
 const uint8_t*p=NULL;
 if(contains(at,n,0x100000,0x20))p=b->scroll+(at-0x100000);
 else if(contains(at,n,0x140000,0x20))p=b->control+(at-0x140000);
 else if(!contains(at,n,0x140020,8)&&!contains(at,n,0x184000,2))return false;
 uint32_t value=0;
 for(unsigned i=0;i<n;i++){
  unsigned a=at+i;uint8_t byte;
  if(p)byte=p[i];else{
   uint16_t w=a>=0x184000?b->backdrop:b->ports[(a-0x140020)/2];
   if(a>=0x140026&&a<0x140028)w=(uint16_t)((w&~0x400u)|(wf_board_high(b)?0x400u:0));
   byte=(uint8_t)(w>>((a&1)?0:8));
  }
  value=(value<<8)|byte;
 }*out=value;return true;
}
bool wf_board_write(wf_board_t*b,uint32_t at,uint32_t v,unsigned n){
 if(!b)return false;uint8_t*p=NULL;
 if(contains(at,n,0x100000,0x20))p=b->scroll+(at-0x100000);
 else if(contains(at,n,0x140000,0x20))p=b->control+(at-0x140000);
 else if(at==0x184000&&n==2){b->backdrop=(uint16_t)v;return true;}
 else return false;
 for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-i-1)));
 if(at<0x140002&&at+n>0x140000)b->irq3=false;
 if(at<0x140004&&at+n>0x140002)b->irq2=false;
 if(at<=0x14000c&&at+n>0x14000d){b->last_sound=(uint16_t)((b->control[12]<<8)|b->control[13]);b->sound_count++;}
 return true;
}
unsigned wf_board_advance(wf_board_t*b){
 if(b->line>=256){b->line=0;b->frames++;}
 else if(b->line==0)b->line=8;
 else if(b->line==8)b->line=16;
 else if(b->line==240)b->line=248;
 else if(b->line==248)b->line=256;
 else b->line+=16;
 if(!(b->line%16))b->irq2=true;
 if(b->line==248)b->irq3=true;
 return b->line;
}
bool wf_board_dispatch(wf_board_t*b,uint16_t sr,bool(*irq)(void*,unsigned),void*u){
 if(!b||!irq)return false;
 unsigned mask=(sr>>8)&7u;
 if(b->irq3&&mask<3)return irq(u,3);
 if(b->irq2&&mask<2)return irq(u,2);
 return true;
}
