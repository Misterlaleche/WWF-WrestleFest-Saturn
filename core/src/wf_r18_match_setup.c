#include "wf_r18_match_setup.h"
#include "wf_r18_royal_setup.h"
#include <string.h>
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static void dw(wf_r18_prematch_t*c,unsigned i,uint16_t v){wf_r17_m68k_move_w(c->regs,i,v);}
static uint16_t d(wf_r18_prematch_t*c,unsigned i){return(uint16_t)c->regs->d[i];}
static wf_status_t block(wf_r18_prematch_t*c,uint32_t pc){c->dependency=pc;return WF_BLOCKED;}
static wf_status_t palette(wf_r18_prematch_t*c){
 c->dependency=0x2aea;if(!c->effect)return WF_BLOCKED;
 wf_status_t st=c->effect(c->user,0x2aea,c);if(st==WF_OK)c->dependency=0;return st;
}
static int active(const uint8_t*p){return!!(p[0]&0x80u);}
static uint8_t*actor(wf_r18_prematch_t*c,unsigned i){return c->ram+0x5b0u+i*0x10cu;}
static const uint16_t xy[4][2]={{0x240,0x150},{0x220,0x160},{0x2c0,0x150},{0x2e0,0x160}};
static void anchor(uint8_t*p,unsigned i){w16(p+6,xy[i][0]);w16(p+0xa,xy[i][1]);w16(p+0xe,0x140);}
static void draw(wf_r18_prematch_t*c,unsigned table){
 static const uint8_t weights[5][8]={{13,13,13,13,12,12,12,12},{17,17,17,17,16,16,0,0},{25,25,25,25,0,0,0,0},{50,50,0,0,0,0,0,0},{13,13,13,13,12,12,12,12}};
 uint32_t saved1=c->regs->d[1],saved2=c->regs->d[2];wf_r17_m68k_moveq(c->regs,1,3);
 for(;;){
  uint32_t seed=r32(c->ram+0x5c);wf_r17_rng21b4_live(&seed,r32(c->ram+0x80),c->regs);w32(c->ram+0x5c,seed);
  c->regs->d[0]&=255u;dw(c,0,(uint16_t)(d(c,0)>>1));if(d(c,0)<100u)break;
  dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu){dw(c,0,50);break;}
 }
 if(!d(c,0))dw(c,0,1);
 unsigned sum=0,index=0;for(;index<8u;index++){sum+=weights[table][index];if(sum>=d(c,0))break;}
 c->regs->d[0]=index;c->regs->d[1]=saved1;c->regs->d[2]=saved2;
}
static wf_status_t health_10782(wf_r18_prematch_t*c,uint8_t*p){
 static const uint16_t human[12]={110,106,98,102,500,500,106,104,100,96,104,96};
 static const uint16_t cpu[10]={115,145,150,155,185,160,165,165,170,200};
 static const int16_t difficulty[4]={0,-15,15,30};uint16_t value;
 if(!(p[0x56]&0x80u)){
  unsigned was=p[0x104]&0x80u;p[0x104]&=0x7fu;if(!was&&c->ram[0x163])return WF_OK;
  unsigned id=r16(p+0x56)&15u;if(id>=12u)return block(c,0x10830);value=human[id];
 }else{
  unsigned progress=r16(c->ram+0x162);if(progress>=10u)return block(c,0x10848);
  dw(c,0,cpu[progress]);if(c->a[0]!=0x1c09e0u)dw(c,0,(uint16_t)(d(c,0)-15u));
  value=(uint16_t)(d(c,0)+difficulty[(r16(c->ram+0x66)>>8)&3u]);
 }
 w16(p+0x66,value);w16(p+0x72,value);w16(p+0x70,0);return WF_OK;
}
wf_status_t wf_r18_match_setup_1086c(wf_r18_prematch_t*c){
 if(!c||!c->ram||!c->regs||!c->regs->valid)return WF_BAD_STATE;
 if(c->ram[0x161]&1u)return wf_r18_royal_setup_1086c(c);
 c->dependency=0;uint32_t saved_d[4],saved_a[3];memcpy(saved_d,c->regs->d,sizeof saved_d);memcpy(saved_a,c->a,sizeof saved_a);
 wf_status_t st;uint8_t*p;
 if(!(active(actor(c,0))&&active(actor(c,2)))){
  if(!r16(c->ram+0x8c)){
   c->a[0]=0x1c0598;dw(c,0,9);
   for(;;){
    if(r16(c->ram+c->a[0]-0x1c0000u)==0x7cu){w32(c->ram+c->a[0]-0x1c0000u,0xffffffffu);break;}
    c->a[0]+=2;dw(c,0,(uint16_t)(d(c,0)-1u));if(d(c,0)==0xffffu)break;
   }
   dw(c,0,0);c->a[0]=0x1c0598;c->a[1]=0x1c0380;
   do{
    dw(c,1,9);int found=0;
    for(;;){
     dw(c,2,d(c,1));dw(c,2,(uint16_t)(d(c,2)<<1));
     if(r16(c->ram+0x598+d(c,2))==d(c,0)||d(c,0)==4u||d(c,0)==5u){found=1;break;}
     dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu)break;
    }
    if(!found){w16(c->ram+c->a[1]-0x1c0000u,d(c,0));c->a[1]+=2;}
    dw(c,0,(uint16_t)(d(c,0)+1u));
   }while(d(c,0)<12u);
   c->a[0]=0x106e0;dw(c,0,r16(c->ram+0x162));if((uint8_t)d(c,0)>=5u)dw(c,0,(uint16_t)(d(c,0)-5u));
   unsigned table=d(c,0);dw(c,0,(uint16_t)(d(c,0)<<2));if(table>=5u)return block(c,0x106e0);
   static const uint32_t tables[5]={0x106f4,0x106fc,0x10702,0x10706,0x106f4};c->a[6]=tables[table];
   draw(c,table);dw(c,1,d(c,0));draw(c,table);
   if(d(c,0)==d(c,1)){dw(c,2,d(c,1)?0xffffu:1u);dw(c,1,(uint16_t)(d(c,1)+d(c,2)));}
   dw(c,0,(uint16_t)(d(c,0)<<1));dw(c,1,(uint16_t)(d(c,1)<<1));c->a[0]=0x1c0380;
   dw(c,0,r16(c->ram+0x380+d(c,0)));dw(c,1,r16(c->ram+0x380+d(c,1)));
   if(d(c,0)==10u||(uint8_t)d(c,0)==11u||(uint8_t)d(c,1)==10u||(uint8_t)d(c,1)==11u){dw(c,0,10);dw(c,1,11);}
   if(c->ram[0x163]==4u||c->ram[0x163]==9u){dw(c,0,4);dw(c,1,5);}
   w16(actor(c,4)+0x56,d(c,0));w16(actor(c,5)+0x56,d(c,1));w16(actor(c,4)+2,d(c,0));w16(actor(c,5)+2,d(c,1));
  }
  w16(actor(c,4),0x8000);w16(actor(c,5),0x8000);dw(c,2,0);c->a[0]=0x1c0598;
  for(;;){
   dw(c,3,r16(c->ram+0x598+d(c,2)));
   if(d(c,3)==0xffffu){w16(c->ram+0x598+d(c,2),d(c,0));w16(c->ram+0x59a+d(c,2),d(c,1));break;}
   dw(c,2,(uint16_t)(d(c,2)+2u));if(d(c,2)>=20u)break;
  }
  dw(c,7,1);c->a[1]=0x1c09e0;
  do{
   p=c->ram+c->a[1]-0x1c0000u;
   if(active(p)){
    w16(p+0x32,0);p[0x56]=0x80;wf_r17_m68k_move_l(c->regs,1,0x1c0aec);
    if(!(uint8_t)d(c,7)){wf_r17_m68k_move_l(c->regs,1,0x1c09e0);p[0x56]|=0x40u;}
    w32(p+0x86,c->regs->d[1]);dw(c,0,r16(p+2));st=palette(c);if(st!=WF_OK)return st;
    /* A0 is still history $0598, so +56/+57 alias actor0 +3E/+3F. */
    if(c->ram[c->a[0]-0x1c0000u+0x57u]>=6u)c->ram[c->a[0]-0x1c0000u+0x56u]|=0x20u;
   }
   c->a[1]+=0x10cu;dw(c,7,(uint16_t)(d(c,7)-1u));
  }while(d(c,7)!=0xffffu);
 }
 c->a[0]=0x1c05b0;dw(c,7,3);
 do{
  p=c->ram+c->a[0]-0x1c0000u;
  if(active(p)){
   if(!(p[0xa1]&1u))p[0x56]=0x40;
   w16(p+2,(uint16_t)(r16(p+0x56)&255u));w16(p+0x32,0);if(!(p[0x56]&0x40u))p[0x33]|=2u;
   dw(c,0,r16(p+2));st=palette(c);if(st!=WF_OK)return st;if(c->a[0]>=0x1c07c8u)p[0x33]|=0x80u;
  }
  c->a[0]+=0x10cu;dw(c,7,(uint16_t)(d(c,7)-1u));
 }while(d(c,7)!=0xffffu);
 dw(c,0,0);c->a[0]=0x1c05b0;
 do{
  p=c->ram+c->a[0]-0x1c0000u;
  if(active(p)){
   st=health_10782(c,p);if(st!=WF_OK)return st;c->a[1]=0x10708;dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)<<2));
   if(d(c,0)>=4u)return block(c,0x10708);
   anchor(p,d(c,0));if(d(c,0)>=2u)p[0x33]|=0x80u;
  }
  c->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)+1u));
 }while(d(c,0)<4u);
 if(active(actor(c,4))){
  dw(c,1,0);if(active(actor(c,0))){dw(c,1,8);actor(c,4)[0x33]|=0x80u;actor(c,5)[0x33]|=0x80u;}
  if(c->a[1]!=0x10708u)return block(c,0x10708);
  anchor(actor(c,4),d(c,1)/4u);anchor(actor(c,5),d(c,1)/4u+1u);
  c->a[0]=0x1c09e0;st=health_10782(c,actor(c,4));if(st!=WF_OK)return st;
  c->a[0]=0x1c0aec;st=health_10782(c,actor(c,5));if(st!=WF_OK)return st;
 }
 c->a[0]=0x1c06bc;dw(c,0,2);
 do{
  p=c->ram+c->a[0]-0x1c0000u;if(active(p))p[0x34]|=2u;c->a[0]+=0x218u;dw(c,0,(uint16_t)(d(c,0)-1u));
 }while(d(c,0)!=0xffffu);
 if(active(actor(c,0))){actor(c,4)[0x33]|=0x80u;actor(c,5)[0x33]|=0x80u;}
 c->a[0]=0x1c05b0;dw(c,0,2);
 do{
  p=c->ram+c->a[0]-0x1c0000u;
  if(active(p)){
   p[0x33]|=1u;if(!(p[0x56]&0x80u)){p[0x33]|=2u;p[0x56]&=0xbfu;}
   c->a[1]=c->a[0]+0x10cu;uint8_t*q=c->ram+c->a[1]-0x1c0000u;q[0x56]|=0x40u;
   if(!(q[0x56]&0x80u)){
    if(!(p[0xa1]&1u))q[0x33]&=0xfdu;
    else if(q[0xa1]&1u){q[0x56]&=0xbfu;q[0x33]|=2u;}
   }
  }
  c->a[0]+=0x218u;dw(c,0,(uint16_t)(d(c,0)-1u));
 }while(d(c,0)!=0xffffu);
 st=wf_r18_setup_referee_10718(c);if(st!=WF_OK)return st;
 memcpy(c->regs->d,saved_d,sizeof saved_d);memcpy(c->a,saved_a,sizeof saved_a);w16(c->ram+0x8c,0);return WF_OK;
}

static int valid(wf_r18_prematch_t*c){return c&&c->ram&&c->regs&&c->regs->valid;}
wf_status_t wf_r18_match_setup_d1c(wf_r18_prematch_t*c){
 if(!valid(c))return WF_BAD_STATE;
 c->dependency=0;
 c->a[0]=0x1c05b0;c->a[1]=0x1c0168;wf_r17_m68k_moveq(c->regs,0,8);
 do{
  uint8_t*p=c->ram+c->a[0]-0x1c0000u;uint8_t*q=c->ram+c->a[1]-0x1c0000u;
  w16(q,r16(p+0x68));w16(q+2,r16(p+0x66));w16(q+4,r16(p+0xa0));w16(q+6,r16(p+2));
  w16(q+8,r16(p+0x56));w16(q+0xa,r16(p));w32(q+0xc,r32(p+0x8a));w32(q+0x10,r32(p+0x86));
  w16(q+0x14,r16(p+0x72));w16(q+0x16,r16(p+0x70));w16(q+0x18,r16(p+0x104));
  c->a[0]+=0x10cu;c->a[1]+=0x20u;dw(c,0,(uint16_t)(d(c,0)-1u));
 }while(d(c,0)!=0xffffu);
 /* Exact $1F38 range; globals, history and palette scratch outside it survive. */
 memset(c->ram+0x5ac,0,0x1cd4u-0x5acu);
 c->a[0]=0x1c05b0;c->a[1]=0x1c0168;dw(c,0,8);
 do{
  uint8_t*p=c->ram+c->a[0]-0x1c0000u;uint8_t*q=c->ram+c->a[1]-0x1c0000u;
  w16(p+0x68,r16(q));w16(p+0x66,r16(q+2));w16(p+0xa0,(uint16_t)(r16(q+4)&15u));w16(p+2,r16(q+6));
  w16(p+0x56,r16(q+8));p[0x56]=0;w16(p,r16(q+0xa));w32(p+0x8a,r32(q+0xc));w32(p+0x86,r32(q+0x10));
  w16(p+0x72,r16(q+0x14));w16(p+0x70,r16(q+0x16));w16(p+0x104,r16(q+0x18));
  c->a[0]+=0x10cu;c->a[1]+=0x20u;dw(c,0,(uint16_t)(d(c,0)-1u));
 }while(d(c,0)!=0xffffu);
 return WF_OK;
}
wf_status_t wf_r18_match_setup_ffd2(wf_r18_prematch_t*c){
 if(!valid(c))return WF_BAD_STATE;
 c->dependency=0;
 static const uint16_t coords[2][2]={{0x40d,0x140},{0x330,0x119}};
 for(unsigned i=0;i<2u;i++){
  c->a[0]=0x1c0f1cu+i*0x10cu;uint8_t*p=c->ram+c->a[0]-0x1c0000u;
  w16(p,0x8000);w16(p+0x74,(uint16_t)i);if(!i)p[0x2e]=0;
  w16(p+2,15);w16(p+6,coords[i][0]);w16(p+0xa,coords[i][1]);w16(p+0xe,0x100);w16(p+0x32,4);w16(p+0x1c,0);
 }
 dw(c,0,15);return palette(c);
}
wf_status_t wf_r18_match_setup_1004a(wf_r18_prematch_t*c){
 if(!valid(c))return WF_BAD_STATE;
 c->dependency=0;
 for(unsigned i=0;i<4u;i++){
  c->a[0]=0x1c1134u+i*0x30u;uint8_t*p=c->ram+c->a[0]-0x1c0000u;
  w16(p,0x8000);w16(p+0x1c,0);w16(p+2,14);w16(p+6,i<2u?0x1c0u:0x330u);w16(p+0xa,0x178);w16(p+0xe,0x140);
  w16(p+0x12,(i&1u)?3u:1u);w16(p+0x2e,i<2u?0u:0x8000u);
 }
 return WF_OK;
}
void wf_r18_match_join_init(wf_r18_match_join_t*s){if(s){s->pc=0xc98;s->blocked_dependency=0;}}
static int join_call(wf_r18_match_join_t*s,wf_r18_prematch_t*c,uint32_t owner){
 c->dependency=owner;
 if(!c->effect||c->effect(c->user,owner,c)!=WF_OK){s->blocked_dependency=c->dependency;return WF_R18_PREMATCH_BLOCKED;}
 c->dependency=0;return WF_R18_PREMATCH_MORE;
}
static int join_local(wf_r18_match_join_t*s,wf_r18_prematch_t*c,wf_status_t status){
 if(status!=WF_OK){s->blocked_dependency=c->dependency;return WF_R18_PREMATCH_BLOCKED;}return WF_R18_PREMATCH_MORE;
}
int wf_r18_match_join_step(wf_r18_match_join_t*s,wf_r18_prematch_t*c){
 if(!s||!valid(c))return WF_R18_PREMATCH_BAD_CONTEXT;
 c->dependency=0;s->blocked_dependency=0;
 switch(s->pc){
 case 0xc98:
  if(join_local(s,c,wf_r18_match_setup_d1c(c)))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xc9e;break;
 case 0xc9e:{
  static const uint8_t worlds[10]={0,5,1,5,0,5,0,1,5,0};
  dw(c,0,(uint16_t)(r16(c->ram+0x162)&255u));c->a[0]=0xd12;
  if(d(c,0)>=10u){c->dependency=0xd12;s->blocked_dependency=0xd12;return WF_R18_PREMATCH_BLOCKED;}
  c->ram[0x7f]=worlds[d(c,0)];s->pc=0xcb6;break;
 }
 case 0xcb6:if(join_call(s,c,0x2988))return WF_R18_PREMATCH_BLOCKED;s->pc=0xcba;break;
 case 0xcba:if(join_call(s,c,0x1fc0))return WF_R18_PREMATCH_BLOCKED;s->pc=0xcbe;break;
 case 0xcbe:if(join_call(s,c,0x1f6c))return WF_R18_PREMATCH_BLOCKED;s->pc=0xcc4;break;
 case 0xcc4:w16(c->ram+0x1804,0x1e0);w16(c->ram+0x1806,0x230);s->pc=0xcd4;break;
 case 0xcd4:if(join_call(s,c,0x26e66))return WF_R18_PREMATCH_BLOCKED;s->pc=0xcda;break;
 case 0xcda:c->ram[0x15f5]=c->ram[0x7f];c->ram[0x15f9]=c->ram[0x7f];s->pc=0xcee;break;
 case 0xcee:
  if(join_local(s,c,wf_r18_match_setup_1086c(c)))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xcf4;break;
 case 0xcf4:
  if(join_local(s,c,wf_r18_match_setup_ffd2(c)))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xcfa;break;
 case 0xcfa:
  if(join_local(s,c,wf_r18_match_setup_1004a(c)))return WF_R18_PREMATCH_BLOCKED;
  s->pc=0xd00;break;
 case 0xd00:if(join_call(s,c,0x2a06))return WF_R18_PREMATCH_BLOCKED;s->pc=0xd04;break;
 case 0xd04:
  s->pc=r16(c->ram+0x7c)?0xd0cu:0xd10u;if(s->pc==0xd10u)return WF_R18_PREMATCH_DONE;break;
 case 0xd0c:if(join_call(s,c,0x1f9e))return WF_R18_PREMATCH_BLOCKED;s->pc=0xd10;return WF_R18_PREMATCH_DONE;
 case 0xd10:return WF_R18_PREMATCH_DONE;
 default:s->blocked_dependency=s->pc;c->dependency=s->pc;return WF_R18_PREMATCH_BLOCKED;
 }
 return WF_R18_PREMATCH_MORE;
}
