#include "wf_r18_video.h"
#include <string.h>
static uint8_t *mapped(wf_r18_video_t*v,uint32_t a,unsigned n){
 if(!n||n>4)return NULL;
 if(a>=0xc0000&&a<=0xc4000-n)return v->fg0+(a-0xc0000);
 if(a>=0x80000&&a<=0x84000-n)return v->background+(a-0x80000);
 if(a>=0x188000&&a<=0x188800-n)return v->fg_palette+(a-0x188000);
 if(a>=0x186000&&a<=0x186800-n)return v->bg_palette+(a-0x186000);
 if(a>=0x180000&&a<=0x184000-n)return v->palettes+(a-0x180000);
 v->failed_address=a;return NULL;
}
static int valid(wf_r18_video_t*v,const wf_r17_fg0_write_t*w,size_t n){for(size_t i=0;i<n;i++)if(!mapped(v,w[i].addr,w[i].width))return 0;return 1;}
static void apply(wf_r18_video_t*v,const wf_r17_fg0_write_t*w,size_t n){
 for(size_t i=0;i<n;i++){
  uint8_t*p=mapped(v,w[i].addr,w[i].width);
  for(unsigned k=0;k<w[i].width;k++){
   uint8_t value=(uint8_t)(w[i].value>>(8*(w[i].width-1-k)));
   if(p[k]==value)continue;p[k]=value;
   uint32_t a=w[i].addr+k;
   if(a>=0xc0000u&&a<0xc2000u&&(a&1u)){
    unsigned cell=(a-0xc0000u)>>2;v->hud_dirty[cell>>3]|=(uint8_t)(1u<<(cell&7u));
   }
  }
 }
}
wf_status_t wf_r18_video_commit(wf_runtime_t*r,const wf_r17_fg0_write_t*a,size_t na,const wf_r17_fg0_write_t*b,size_t nb,void*u){wf_r18_video_t*v=u;(void)r;if(!v||(!a&&na)||(!b&&nb))return WF_BAD_STATE;if(!valid(v,a,na)||!valid(v,b,nb))return WF_BLOCKED;apply(v,a,na);apply(v,b,nb);return WF_OK;}
wf_status_t wf_r18_video_batch(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){return wf_r18_video_commit(r,w,n,NULL,0,u);}
wf_status_t wf_r18_video_peek(wf_runtime_t*r,uint32_t a,uint32_t*out,void*u){wf_r18_video_t*v=u;uint8_t*p;(void)r;if(!v||!out)return WF_BAD_STATE;p=mapped(v,a,4);if(!p)return WF_BLOCKED;*out=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];return WF_OK;}
static wf_status_t rd(wf_r18_video_t*v,uint32_t a,unsigned n,uint32_t*out){uint8_t b[4];if(!v||!v->read_rom||v->read_rom(v->read_user,a,b,n)!=WF_OK){if(v)v->failed_address=a;return WF_BLOCKED;}*out=0;for(unsigned i=0;i<n;i++)*out=(*out<<8)|b[i];return WF_OK;}
wf_status_t wf_r18_palette_2aea(wf_runtime_t*r,uint16_t id,void*u){wf_r18_video_t*v=u;uint8_t bytes[32];unsigned bank;(void)r;if(!v||id>255)return WF_BAD_STATE;if(v->palette_map[id]&128)return WF_OK;for(bank=0;bank<16&&v->palette_used[bank];bank++){}if(bank==16)return WF_OK; /* ROM $2B12 returns without allocation. */
 if(!v->read_rom||v->read_rom(v->read_user,0x2f22u+id*32u,bytes,32)!=WF_OK)return WF_BLOCKED;
 bytes[0]=bytes[1]=0;memcpy(v->palettes+0x2000+bank*128,bytes,32);v->palette_used[bank]=255;v->palette_map[id]=(uint8_t)(128|bank);return WF_OK;}
wf_status_t wf_r18_palette_2b58(wf_runtime_t*r,uint16_t id,void*u){wf_r18_video_t*v=u;(void)r;if(!v||id>255)return WF_BAD_STATE;if(v->palette_map[id]){v->palette_used[v->palette_map[id]&15]=0;v->palette_map[id]=0;}return WF_OK;}
/* All text owners preserve caller D registers in ROM. Stage transfers before commit. */
static wf_r17_fg0_write_t stage[4096];static size_t used;static unsigned busy;
static int put(uint32_t a,uint32_t val,unsigned width){if(used==4096)return 0;stage[used++]=(wf_r17_fg0_write_t){a,val,(uint8_t)width};return 1;}
static int glyph(uint32_t a,uint16_t value){return put(a+1,value&255,1)&&put(a+3,value>>8,1);}
#define READ(A,N,V) do {if(rd(v,(A),(N),&(V))!=WF_OK)goto fail;}while(0)
wf_status_t wf_r18_text_2503c(wf_runtime_t*r,uint16_t id,void*u){
 wf_r18_video_t*v=u;uint32_t p,type,bank,base,code,param,x;unsigned guard=0;uint16_t d4=0;wf_status_t result;
 if(!v||busy)return WF_BAD_STATE;
 busy=1;used=0;
 READ(0x25204u+((id<<2)&65535u),4,p);READ(p,1,type);READ(p+1,1,bank);READ(p+2,2,base);p+=4;bank=(bank<<12)&65535;x=base;
 if(type>3)goto fail;
 while(guard++<4096){READ(p++,1,code);if(!code)break;
  if(code<8){if(code==1){base+=type==1||type==2?0x200:0x100;x=base;}else if(code==2){READ(p++,1,param);d4=(uint16_t)(((d4&0xff00)|param)<<8);}else if(code==3){READ(p,2,base);p+=2;x=base;}else goto fail;continue;}
  uint32_t a=0xc0000+x;uint16_t value=(uint16_t)(code-16);int erase=(id&0x8000)!=0;
  if(type==0){if(!glyph(a,erase?0:(uint16_t)(value|bank)))goto fail;x+=4;}
  else if(type==1){value=(uint16_t)((value*2+0x30)|bank);if(erase){if(!put(a,0,4)||!put(a+256,0,4))goto fail;}else if(!glyph(a,value)||!glyph(a+256,(uint16_t)(value+1)))goto fail;x+=4;}
  else if(type==2){value=(uint16_t)((value*4+0x94)|bank);if(erase||code==0x20){if(!put(a,0,4)||!put(a+4,0,4)||!put(a+256,0,4)||!put(a+260,0,4))goto fail;}else if(!glyph(a,value)||!glyph(a+256,(uint16_t)(value+1))||!glyph(a+4,(uint16_t)(value+2))||!glyph(a+260,(uint16_t)(value+3)))goto fail;x+=8;}
  else { /* $251A0 MOVEP.W D1,1(A2), not D2: high then low byte. */
   (void)d4;if(!put(a+1,erase?0:bank>>8,1)||!put(a+3,erase?0:bank&255,1))goto fail;x+=4;}
 }
 if(guard>=4096)goto fail;
 result=wf_r18_video_batch(r,stage,used,v);busy=0;return result;
fail:busy=0;return WF_BLOCKED;
}
wf_status_t wf_r18_number_26122(wf_runtime_t*r,uint16_t id,void*u){wf_r18_video_t*v=u;uint32_t p,type,bank,base,src;uint16_t raw;unsigned started=0;wf_status_t result;
 if(!v||!v->read_ram||busy)return WF_BAD_STATE;
 busy=1;used=0;
 READ(0x262a0u+((id<<2)&65535u),4,p);READ(p,1,type);READ(p+1,1,bank);READ(p+2,2,base);READ(p+4,4,src);if(type>2||v->read_ram(r,src,&raw,v->read_user)!=WF_OK)goto fail;
 for(unsigned i=0;i<2;i++){unsigned digit=(raw>>(12-4*i))&15;uint32_t a=0xc0000+base;if(digit)started=1;if(digit>=10)digit+=7;
  uint16_t value=(uint16_t)(((bank&15)<<12)+(type==0?digit+0x20:type==1?digit*2+0x70:digit*4+0x114));
  if((id&0x8000)||!started){if(!put(a,0,4))goto fail;if(type&&!put(a+256,0,4))goto fail;if(type==2&&(!put(a+4,0,4)||!put(a+260,0,4)))goto fail;}
  else {if(!glyph(a,value))goto fail;if(type&&!glyph(a+256,(uint16_t)(value+1)))goto fail;if(type==2&&(!glyph(a+4,(uint16_t)(value+2))||!glyph(a+260,(uint16_t)(value+3))))goto fail;}
  started=1;base+=type==2?8:4;
 }
 result=wf_r18_video_batch(r,stage,used,v);busy=0;return result;
fail:busy=0;return WF_BLOCKED;
}
wf_status_t wf_r18_strip_9aca(wf_runtime_t*r,uint16_t idx,uint16_t palette,uint32_t dest,void*u){wf_r18_video_t*v=u;uint32_t p,tile,count,off;unsigned row=0;wf_status_t result;if(!v||busy)return WF_BAD_STATE;
 busy=1;used=0;READ(0x9b26u+(idx&255)*4u,4,p);READ(p,2,tile);
 do{READ(p+row*4+2,2,count);for(unsigned i=0;i<=count;i++){if(!put(dest+i*4+1,tile&255,1)||!put(dest+i*4+3,(tile>>8)|palette,1))goto fail;tile=(tile+1)&65535;}row++;READ(p+row*4,2,off);dest+=off;}while(off&&row<128);
 if(row==128)goto fail;
 result=wf_r18_video_batch(r,stage,used,v);busy=0;return result;
fail:busy=0;return WF_BLOCKED;}
