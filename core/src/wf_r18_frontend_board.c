#include "wf_r18_frontend_board.h"
#include <string.h>
static uint16_t r16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t *p){return ((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static wf_status_t blocked(wf_r18_frontend_board_t *c,uint32_t pc,uint32_t at){
 c->blocked_dependency=pc;c->blocked_address=at;return WF_BLOCKED;
}
static wf_status_t rom(wf_r18_frontend_board_t *c,uint32_t pc,uint32_t at,void *p,size_t n){
 uint32_t physical=at&0x00ffffffu;
 if(!c->read_rom)return blocked(c,pc,physical);
 wf_status_t st=c->read_rom(c->user,physical,p,n);
 if(st!=WF_OK){c->blocked_dependency=pc;c->blocked_address=physical;}
 return st;
}
static wf_status_t store(wf_r18_frontend_board_t *c,uint32_t pc,uint32_t at,uint32_t value,unsigned width){
 uint32_t physical=at&0x00ffffffu;
 if(!c->store_bus)return blocked(c,pc,physical);
 wf_status_t st=c->store_bus(c->user,physical,value,width);
 if(st!=WF_OK){c->blocked_dependency=pc;c->blocked_address=physical;}
 return st;
}
static wf_status_t load(wf_r18_frontend_board_t *c,uint32_t pc,uint32_t at,uint32_t *value,unsigned width){
 uint32_t physical=at&0x00ffffffu;
 if(!c->read_bus)return blocked(c,pc,physical);
 wf_status_t st=c->read_bus(c->user,physical,value,width);
 if(st!=WF_OK){c->blocked_dependency=pc;c->blocked_address=physical;}
 return st;
}
static wf_status_t setup_2006(wf_r18_frontend_board_t *c){
 static const uint32_t addresses[6]={0x100008,0x10000c,0x140006,0x140010,0x140012,0x140014};
 static const uint16_t values[6]={0x81,0x50,0xab,0x78,0xf7,1};
 wf_status_t st;uint32_t value;
 for(unsigned i=0;i<6;i++){
  st=store(c,0x2006u+i*8u,addresses[i],values[i],2);if(st!=WF_OK)return st;
 }
 st=load(c,0x2036,0x10000b,&value,1);if(st!=WF_OK)return st;
 st=store(c,0x2036,0x10000b,(value&255u)&~1u,1);if(st!=WF_OK)return st;
 if(c->ram[0x67]&64u){
  st=load(c,0x2048,0x10000b,&value,1);if(st!=WF_OK)return st;
  st=store(c,0x2048,0x10000b,(value&255u)|1u,1);if(st!=WF_OK)return st;
 }
 return WF_OK;
}
static wf_status_t palette_b53a(wf_r18_frontend_board_t *c){
 uint8_t *r=c->ram;uint8_t bytes[4];wf_status_t st;
 if(!(r[0x6f]&16u)||!(r[0xf1d]&128u))return WF_OK;
 unsigned initialized=r[0xf1d]&64u;r[0xf1d]|=64u;
 if(!initialized){r[0xf1d]=0xc0;r[0xf1c]=0;}
 r[0xf1c]++;if(r[0xf1c]!=0x60)return WF_OK;
 r[0xf1c]=0;
 st=rom(c,0xb59a,0xb5e0u+(r[0xf1d]&1u)*4u,bytes,4);if(st!=WF_OK)return st;
 uint32_t source=r32(bytes);
 for(unsigned i=1;i<16;i++){
  st=rom(c,0xb5aa,source+i*2u,bytes,2);if(st!=WF_OK)return st;
  st=store(c,0xb5aa,0x180080u+i*2u,r16(bytes),2);if(st!=WF_OK)return st;
 }
 r[0xf1c]=0x5d;r[0xf1d]++;
 if(r[0xf1d]==0xc5)w16(r+0xf1c,0x0080);
 return WF_OK;
}
static wf_status_t shake(wf_r18_frontend_board_t *c,unsigned horizontal){
 uint8_t *r=c->ram;unsigned count_at=horizontal?0x1802u:0x1800u;
 uint16_t count=r16(r+count_at);uint8_t bytes[2];
 if(!count)return WF_OK;
 /* LSL.W then signed D0.W indexed addressing; count$8000 wraps to0. */
 uint32_t at=0x27048u+(int32_t)(int16_t)(uint16_t)(count<<1);
 wf_status_t st=rom(c,horizontal?0x27080u:0x2702cu,at,bytes,2);if(st!=WF_OK)return st;
 unsigned first=horizontal?0x17e6u:0x17eeu,second=horizontal?0x17eau:0x17f2u;
 uint16_t delta=r16(bytes);w16(r+first,(uint16_t)(r16(r+first)+delta));
 w16(r+second,(uint16_t)(r16(r+second)+delta));w16(r+count_at,(uint16_t)(count-1));
 return WF_OK;
}
static wf_status_t animated_layer(wf_r18_frontend_board_t *c,unsigned layer){
 uint8_t *r=c->ram;unsigned count_at=layer?0x19aau:0x19acu;
 unsigned count=r16(r+count_at);uint8_t bytes[4];wf_status_t st;
 if(!count)return WF_OK;
 uint32_t table=(layer?0x295dcu:0x295a4u)+(int32_t)(int16_t)(uint16_t)(r16(r+0x7e)<<3);
 st=rom(c,layer?0x29536u:0x29486u,table,bytes,4);if(st!=WF_OK)return st;
 uint32_t destinations=r32(bytes);
 st=rom(c,layer?0x2953au:0x2948au,table+4u,bytes,4);if(st!=WF_OK)return st;
 uint32_t source=r32(bytes);unsigned queue=layer?0x197au:0x194au;
 for(unsigned i=0;i<count;i++){
  unsigned frame=r[queue+i];
  st=rom(c,layer?0x29554u:0x294a4u,destinations+frame*2u,bytes,2);if(st!=WF_OK)return st;
  uint32_t destination=(layer?0x82000u:0x80000u)+r16(bytes);
  uint32_t pixels=source+(frame<<(layer?5:6));
  unsigned words=layer?8u:16u,row_bytes=layer?0x40u:0x80u,per_row=layer?2u:4u;
  for(unsigned j=0;j<words;j++){
   uint32_t pc=(layer?0x29564u:0x294b4u)+j*6u;
   st=rom(c,pc,pixels+j*4u,bytes,4);if(st!=WF_OK)return st;
   st=store(c,pc,destination+(j/per_row)*row_bytes+(j%per_row)*4u,r32(bytes),4);if(st!=WF_OK)return st;
  }
 }
 w16(r+count_at,0);return WF_OK;
}
static wf_status_t animations_2946a(wf_r18_frontend_board_t *c){
 /* The two producer allocations are48 bytes each. Check both before any
  * transport so an invalid second queue cannot leave half a frame copied. */
 if(r16(c->ram+0x19ac)>48u)return blocked(c,0x2946e,0x1c19ac);
 if(r16(c->ram+0x19aa)>48u)return blocked(c,0x2951e,0x1c19aa);
 wf_status_t st=animated_layer(c,0);if(st!=WF_OK)return st;
 return animated_layer(c,1);
}
static wf_status_t clock_264aa(wf_r18_frontend_board_t *c){
 if(!(c->ram[0x169e]&32u))return WF_OK;
 unsigned flash=c->ram[0x16a5]&1u;
 wf_status_t st=store(c,flash?0x264beu:0x264d0u,0x18071c,flash?0x000fu:0x00ffu,2);
 if(st!=WF_OK)return st;
 return store(c,flash?0x264c6u:0x264d8u,0x18071e,flash?0x0003u:0x000fu,2);
}
static wf_status_t palette_2988_payload(wf_r18_frontend_board_t *c){
 static const uint32_t bases[4]={0x182000,0x188000,0x186000,0x180000};
 memset(c->ram+0x1600,0,64);
 for(unsigned bank=0;bank<16;bank++)for(unsigned word=0;word<8;word++)for(unsigned port=0;port<4;port++){
  wf_status_t st=store(c,0x29c6u+port*2u,bases[port]+bank*128u+word*4u,0,4);if(st!=WF_OK)return st;
 }
 c->regs->d[1]=(c->regs->d[1]&0xffff0000u)|0xffffu;c->a[3]=0x180800;
 return store(c,0x29ee,0x184000,0,2);
}
wf_status_t wf_r18_frontend_board_call(wf_r18_frontend_board_t *c,uint32_t owner){
 if(!c||!c->ram||c->ram_bytes<0x4000u||!c->regs||!c->regs->valid||!c->a)return WF_BAD_STATE;
 c->blocked_dependency=c->blocked_address=0;
 switch(owner){
 case 0x2006:return setup_2006(c);
 case 0xb53a:return palette_b53a(c);
 case 0x27012:return shake(c,0);
 case 0x27066:return shake(c,1);
 case 0x2946a:return animations_2946a(c);
 case 0x264aa:return clock_264aa(c);
 case 0x2988:return palette_2988_payload(c);
 default:return blocked(c,owner,0);
 }
}
