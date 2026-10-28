#include "wf_r18_royal_setup.h"
#include <string.h>

static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static void dw(wf_r18_prematch_t*c,unsigned i,uint16_t v){wf_r17_m68k_move_w(c->regs,i,v);}
static void dq(wf_r18_prematch_t*c,unsigned i,int8_t v){wf_r17_m68k_moveq(c->regs,i,v);}
static uint16_t d(wf_r18_prematch_t*c,unsigned i){return(uint16_t)c->regs->d[i];}
static wf_status_t block(wf_r18_prematch_t*c,uint32_t pc){c->dependency=pc;return WF_BLOCKED;}
static wf_status_t palette(wf_r18_prematch_t*c){
 c->dependency=0x2aea;if(!c->effect)return WF_BLOCKED;
 wf_status_t st=c->effect(c->user,0x2aea,c);if(st==WF_OK)c->dependency=0;return st;
}
static const uint16_t xy[6][2]={{0x240,0x120},{0x2d0,0x120},{0x230,0x150},{0x2c0,0x150},{0x240,0x180},{0x2b0,0x180}};
static const uint16_t orientation[6]={0x8000,0,0x8000,0,0x8000,0};
static void anchor(uint8_t*p,unsigned slot){
 w16(p+0x2e,orientation[slot]);w16(p+4,orientation[slot]);
 w16(p+6,xy[slot][0]);w16(p+0xa,xy[slot][1]);w16(p+0xe,0x140);
}
/* Original $24CC, preserving D1/D2 around live $21B4 calls. */
static void draw(wf_r18_prematch_t*c){
 static const uint8_t weights[6]={17,17,17,17,16,16};
 uint32_t saved1=c->regs->d[1],saved2=c->regs->d[2];dq(c,1,3);
 for(;;){
  uint32_t seed=r32(c->ram+0x5c);wf_r17_rng21b4_live(&seed,r32(c->ram+0x80),c->regs);w32(c->ram+0x5c,seed);
  c->regs->d[0]&=255u;dw(c,0,(uint16_t)(d(c,0)>>1));
  if(d(c,0)<100u)break;
  dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu){dw(c,0,50);break;}
 }
 if(!d(c,0))dw(c,0,1);
 unsigned sum=0,index=0;for(;index<6u;index++){sum+=weights[index];if(sum>=d(c,0))break;}
 c->regs->d[0]=index;c->regs->d[1]=saved1;c->regs->d[2]=saved2;
}
/* $10AF0/$10B26 return the first free index; the caller does not use CCR. */
static void first_free(wf_r18_prematch_t*c,unsigned base){
 unsigned i=0;for(;i<6u;i++)if(!r16(c->ram+base+i*2u))break;dq(c,0,(int8_t)i);
}
static wf_status_t health_10782(wf_r18_prematch_t*c,uint8_t*p){
 static const uint16_t human[12]={110,106,98,102,500,500,106,104,100,96,104,96};
 uint16_t value;
 if(!(p[0x56]&0x80u)){
  unsigned was=p[0x104]&0x80u;p[0x104]&=0x7fu;
  if(!was&&c->ram[0x163])return WF_OK;
  unsigned identity=r16(p+0x56)&15u;if(identity>=12u)return block(c,0x10830);
  value=human[identity];
 }else value=0x87;
 w16(p+0x66,value);w16(p+0x72,value);w16(p+0x70,0);return WF_OK;
}
static wf_status_t referee_10718(wf_r18_prematch_t*c){
 c->a[0]=0x1c11f4;uint8_t*p=c->ram+0x11f4;
 w16(p,0x8000);w16(p+2,12);w16(p+6,0x280);w16(p+0xa,0x198);w16(p+0xe,0x140);
 dw(c,0,r16(p+2));wf_status_t st=palette(c);if(st!=WF_OK)return st;
 w16(p+0x20,0x8000);w16(p+0x34,0);
 static const uint16_t ids[4]={7,14,29,1};
 for(unsigned i=0;i<4u;i++){dw(c,0,ids[i]);st=palette(c);if(st!=WF_OK)return st;}
 c->ram[0x1697]=1;return WF_OK;
}
wf_status_t wf_r18_royal_setup_1086c(wf_r18_prematch_t*c){
 if(!c||!c->ram||!c->regs||!c->regs->valid)return WF_BAD_STATE;
 c->dependency=0;if(!(c->ram[0x161]&1u))return block(c,0x1034a);
 uint32_t saved_d[4],saved_a[3];memcpy(saved_d,c->regs->d,sizeof saved_d);memcpy(saved_a,c->a,sizeof saved_a);
 wf_status_t st;uint8_t*p;
 if(!r16(c->ram+0x7c)){
  c->a[0]=0x1c05b0;c->a[1]=0x10acc;c->a[2]=0x10ae4;dq(c,1,0);dq(c,0,5);
  for(unsigned i=0;i<6u;i++){
   p=c->ram+c->a[0]-0x1c0000u;dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)<<1));
   anchor(p,d(c,0));dw(c,1,(uint16_t)(d(c,1)<<1));
   w16(p+0x32,1);w16(p+0x34,0);w16(p+0x20,0);w16(p+0xb6,0x8000);
   c->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)-1u));
  }
 }else{
  c->a[0]=0x1c05b0;dq(c,3,3);dq(c,7,0);dq(c,4,0);dq(c,5,0);dq(c,6,0);
  c->a[1]=0x10acc;c->a[2]=0x10ae4;c->a[3]=0x1c16a8;
  for(unsigned i=0;i<4u;i++){
   p=c->ram+c->a[0]-0x1c0000u;
   if(p[0]&0x80u){
    unsigned id=p[0x57];if(id>=12u)return block(c,0x1090a);
    wf_r17_m68k_move_b(c->regs,4,(uint8_t)id);dw(c,4,(uint16_t)(d(c,4)<<1));w16(c->ram+0x16a8+d(c,4),1);
    w16(p+2,r16(p+0x56));
    if(id>=6u){dw(c,6,(uint16_t)(d(c,6)+1u));p[0x56]|=0x20u;}else dw(c,5,(uint16_t)(d(c,5)+1u));
    w16(p+0x32,3);w16(p+0x34,16);dw(c,4,d(c,7));dw(c,4,(uint16_t)(d(c,4)<<1));
    if(d(c,7)>=6u)return block(c,0x10acc);
    anchor(p,d(c,7));dw(c,4,(uint16_t)(d(c,4)<<1));dw(c,0,r16(p+2));
    st=palette(c);if(st!=WF_OK)return st;
    w16(p+0x20,0);w16(p+0xae,0);st=health_10782(c,p);if(st!=WF_OK)return st;
    dw(c,7,(uint16_t)(d(c,7)+1u));
   }
   c->a[0]+=0x10cu;dw(c,3,(uint16_t)(d(c,3)-1u));
  }
  c->a[5]=0x1c0380;dq(c,3,0);dq(c,4,0);
  if(d(c,5)>3u||d(c,6)>3u)return block(c,d(c,5)>3u?0x1098c:0x109dc);
  if(d(c,5)<3u){
   dq(c,1,3);c->regs->d[1]-=c->regs->d[5];
   do{
    dq(c,2,3);c->a[0]=0x1c16a8;
    for(;;){
     c->a[6]=0x10b5c;draw(c);dw(c,3,d(c,0));dw(c,3,(uint16_t)(d(c,3)<<1));
     if(!r16(c->ram+0x16a8+d(c,3))){w16(c->ram+0x16a8+d(c,3),1);break;}
     dw(c,2,(uint16_t)(d(c,2)-1u));if(d(c,2)==0xffffu){
      first_free(c,0x16a8);if(d(c,0)>=6u)return block(c,0x10b26);
      dw(c,3,d(c,0));dw(c,3,(uint16_t)(d(c,3)<<1));w16(c->ram+0x16a8+d(c,3),1);break;
     }
    }
    dw(c,5,d(c,4));dw(c,5,(uint16_t)(d(c,5)<<1));w16(c->ram+0x380+d(c,5),d(c,0));
    dw(c,4,(uint16_t)(d(c,4)+1u));dw(c,1,(uint16_t)(d(c,1)-1u));
   }while(d(c,1));
  }
  if(d(c,6)<3u){
   dq(c,1,3);c->regs->d[1]-=c->regs->d[6];
   do{
    dq(c,2,3);
    for(;;){
     c->a[6]=0x10b5c;draw(c);dw(c,3,d(c,0));dw(c,3,(uint16_t)(d(c,3)<<1));c->a[0]=0x1c16b4;
     if(!r16(c->ram+0x16b4+d(c,3))){w16(c->ram+0x16b4+d(c,3),1);break;}
     dw(c,2,(uint16_t)(d(c,2)-1u));if(d(c,2)==0xffffu){
      first_free(c,0x16b4);if(d(c,0)>=6u)return block(c,0x10af0);
      dw(c,3,d(c,0));dw(c,3,(uint16_t)(d(c,3)<<1));w16(c->ram+0x16b4+d(c,3),1);break;
     }
    }
    dw(c,5,d(c,4));dw(c,5,(uint16_t)(d(c,5)<<1));dw(c,0,(uint16_t)(d(c,0)+6u));w16(c->ram+0x380+d(c,5),d(c,0));
    dw(c,4,(uint16_t)(d(c,4)+1u));dw(c,1,(uint16_t)(d(c,1)-1u));
   }while(d(c,1));
  }
  dq(c,2,6);dw(c,2,(uint16_t)(d(c,2)-d(c,7)));c->regs->d[2]-=1u;dq(c,3,0);c->a[0]=0x1c09e0;c->a[3]=0x1c0380;
  do{
   p=c->ram+c->a[0]-0x1c0000u;p[0]|=0x80u;w16(p+0x32,1);dw(c,4,d(c,7));dw(c,4,(uint16_t)(d(c,4)<<1));
   if(d(c,7)>=6u)return block(c,0x10acc);
   anchor(p,d(c,7));dw(c,4,(uint16_t)(d(c,4)<<1));dw(c,4,d(c,3));dw(c,4,(uint16_t)(d(c,4)<<1));
   w16(p+0x56,r16(c->ram+0x380+d(c,4)));w16(p+2,r16(p+0x56));
   if(p[0x57]>=6u)p[0x56]|=0x20u;
   p[0x56]|=0x80u;p[0xb6]|=0x80u;dw(c,0,r16(p+2));st=palette(c);if(st!=WF_OK)return st;
   w16(p+0x20,0);w16(p+0xae,0);st=health_10782(c,p);if(st!=WF_OK)return st;
   dw(c,7,(uint16_t)(d(c,7)+1u));dw(c,3,(uint16_t)(d(c,3)+1u));c->a[0]+=0x10cu;dw(c,2,(uint16_t)(d(c,2)-1u));
  }while(d(c,2)!=0xffffu);
 }
 st=referee_10718(c);if(st!=WF_OK)return st;
 memcpy(c->regs->d,saved_d,sizeof saved_d);memcpy(c->a,saved_a,sizeof saved_a);return WF_OK;
}

wf_status_t wf_r18_setup_referee_10718(wf_r18_prematch_t*c){
 if(!c||!c->ram||!c->regs||!c->regs->valid)return WF_BAD_STATE;
 return referee_10718(c);
}
