#include "wf_r18_frontend_render.h"
#include <string.h>
static uint16_t rw(wf_r18_frontend_render_t*c,unsigned off){return (uint16_t)((c->ram[off]<<8)|c->ram[off+1]);}
static uint32_t rl(wf_r18_frontend_render_t*c,unsigned off){return ((uint32_t)rw(c,off)<<16)|rw(c,off+2);}
static void ww(wf_r18_frontend_render_t*c,unsigned off,uint16_t v){c->ram[off]=(uint8_t)(v>>8);c->ram[off+1]=(uint8_t)v;}
static void wl(wf_r18_frontend_render_t*c,unsigned off,uint32_t v){ww(c,off,(uint16_t)(v>>16));ww(c,off+2,(uint16_t)v);}
static int object(wf_r18_frontend_render_t*c,uint32_t address,unsigned bytes,unsigned*out){
 uint32_t physical=address&0x00ffffffu;
 if(physical<0x1c0000u||physical>0x1c4000u-bytes){c->blocked_dependency=address;return 0;}
 *out=physical-0x1c0000u;return 1;
}
static wf_status_t project(wf_r18_frontend_render_t*c){
 unsigned at;if(!object(c,c->a[0],0x2f,&at))return WF_BLOCKED;
 int16_t dx=(int8_t)(uint8_t)rw(c,at+0x18),dy=(int8_t)(uint8_t)rw(c,at+0x1a);
 if(c->ram[at+0x2e]&0x80u)dx=(int16_t)-dx;
 ww(c,at+0x14,(uint16_t)(rw(c,at+6)-rw(c,0x17e6)+dx));
 ww(c,at+0x16,(uint16_t)(rw(c,at+10)+rw(c,at+14)-rw(c,0x17ee)+dy));return WF_OK;
}
static wf_status_t queue(wf_r18_frontend_render_t*c){
 unsigned at;if(!object(c,c->a[0],0x18,&at))return WF_BLOCKED;
 if(!(c->ram[at]&0x80u)||(rw(c,at+4)&0x7fffu)==0x7fffu)return WF_OK;
 if(c->ram[at+3]<0x4eu){
  int16_t x=(int16_t)rw(c,at+0x14),y=(int16_t)rw(c,at+0x16);
  if(x>=383||x< -63||y>=319||y< -63)return WF_OK;
 }
 unsigned list=rw(c,at+0x12);if(list>=6u)return WF_BLOCKED;
 unsigned count=rw(c,0x19b8u+list*4u);if(count>=32u)return WF_BLOCKED;
 wl(c,0x19d0u+list*128u+count*4u,c->a[0]);ww(c,0x19b8u+list*4u,(uint16_t)(count+1u));return WF_OK;
}
static wf_status_t sort(wf_r18_frontend_render_t*c,unsigned base,unsigned count){
 for(unsigned n=count;n>1;n--){int changed=0;
  for(unsigned i=0;i+1<n;i++){unsigned p,q;uint32_t pa=rl(c,base+i*4u),qa=rl(c,base+i*4u+4u);
   if(!object(c,pa,0xc,&p)||!object(c,qa,0xc,&q))return WF_BLOCKED;
   if((int16_t)rw(c,p+10)<(int16_t)rw(c,q+10)){wl(c,base+i*4u,qa);wl(c,base+i*4u+4u,pa);changed=1;}
  }
  if(!changed)break;
 }
 return WF_OK;
}
static wf_status_t draw(wf_r18_frontend_render_t*c){
 if(!c->emit_d1fc||!c->clear_attribute)return WF_BLOCKED;
 uint32_t saved_d[6],saved_a[6];memcpy(saved_d,c->regs->d,sizeof saved_d);memcpy(saved_a,c->a,sizeof saved_a);
 ww(c,0x19b2,0);c->a[2]=0xc2000;
 for(unsigned order=0;order<6;order++){
  unsigned list=5u-order,base=0x19d0u+list*128u,count_at=0x19b8u+list*4u,count=rw(c,count_at);
  if(count>32u)return WF_BLOCKED;
  c->a[1]=0x1c0000u+base;c->a[6]=0x1c0000u+count_at;
  if((list==0u||list==2u||list>=4u)&&sort(c,base,count)!=WF_OK)return WF_BLOCKED;
  if(!count)continue;
  ww(c,count_at,0);
  for(unsigned i=0;i<count;i++){
   c->a[0]=rl(c,base+i*4u);c->a[1]+=4u;unsigned at;
   if(!object(c,c->a[0],0x2a,&at))return WF_BLOCKED;
   if(c->ram[at+0x21]==255u||(rw(c,at+4)&0x7fffu)==0x7fffu)continue;
   uint32_t a0=c->a[0],a1=c->a[1];
   if(c->emit_d1fc(c->user,c)!=WF_OK){if(!c->blocked_dependency)c->blocked_dependency=0xd1fc;return WF_BLOCKED;}
   c->a[0]=a0;c->a[1]=a1;
   if(c->ram[at+0x26]&0x80u){if(!object(c,a0,0x100,&at))return WF_BLOCKED;if(!rw(c,at+0xfe))wl(c,at+0x26,0);}
  }
 }
 if(c->a[2]<0xc2000u||c->a[2]>0xc4000u||(c->a[2]&15u))return WF_BLOCKED;
 uint16_t count=(uint16_t)((c->a[2]-0xc2000u)>>4),previous=rw(c,0x19b4);ww(c,0x19b2,count);
 for(unsigned i=count;i<previous;i++){
  if(i>=512u||c->clear_attribute(c->user,0xc2003u+i*16u)!=WF_OK){c->blocked_dependency=0x28de;return WF_BLOCKED;}
 }
 ww(c,0x19b4,count);memcpy(c->regs->d,saved_d,sizeof saved_d);memcpy(c->a,saved_a,sizeof saved_a);return WF_OK;
}
wf_status_t wf_r18_frontend_render_call(wf_r18_frontend_render_t*c,uint32_t owner){
 if(!c||!c->ram||!c->regs||!c->regs->valid||!c->a)return WF_BAD_STATE;
 c->blocked_dependency=0;wf_status_t st;
 switch(owner){case 0x247c:st=project(c);break;case 0x27b8:st=queue(c);break;case 0x2836:st=draw(c);break;default:st=WF_BLOCKED;break;}
 if(st!=WF_OK&&!c->blocked_dependency)c->blocked_dependency=owner;
 return st;
}
