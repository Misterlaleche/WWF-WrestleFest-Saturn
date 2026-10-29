#include "wf_r18_card_scene.h"
#include <string.h>
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static void dw(wf_r18_prematch_t*c,unsigned i,uint16_t v){wf_r17_m68k_move_w(c->regs,i,v);}
static uint16_t d(wf_r18_prematch_t*c,unsigned i){return(uint16_t)c->regs->d[i];}
static int valid(wf_r18_prematch_t*c){return c&&c->ram&&c->regs&&c->regs->valid;}
static int actor_address(uint32_t a){return a>=0x1c05b0u&&a<=0x1c0f1cu&&(a-0x1c05b0u)%0x10cu==0u;}
static int partner(uint32_t a){return a==0x1c06bcu||a==0x1c08d4u||a==0x1c0aecu;}
static int primary(uint32_t a){return a==0x1c05b0u||a==0x1c07c8u||a==0x1c09e0u;}
static int special(const uint8_t*r){return r[0x163]==4u||r[0x163]==9u;}
static int block(wf_r18_card_scene_t*s,wf_r18_prematch_t*c,uint32_t pc){
 s->fatal=1;s->blocked_dependency=c->dependency=pc;return WF_R18_CARD_BLOCKED;
}
static int request(wf_r18_card_scene_t*s,unsigned kind,uint32_t owner,uint32_t ret,uint32_t address,uint32_t value){
 s->request=(wf_r18_card_request_t){kind,owner,s->pc,ret,address,value};s->pending=1;return WF_R18_CARD_REQUEST;
}
static void local(wf_r18_card_scene_t*s,uint32_t owner,uint32_t ret){s->sub_return=ret;s->pc=owner;}
static int returned(wf_r18_card_scene_t*s){
 if(!s->sub_return)return WF_R18_CARD_RETURN;
 s->pc=s->sub_return;s->sub_return=0;return WF_R18_CARD_MORE;
}
static void swap(wf_r18_prematch_t*c,uint8_t*p,uint8_t*q){dw(c,0,r16(p+2));w16(p+2,r16(q+2));w16(q+2,d(c,0));}
void wf_r18_card_scene_init(wf_r18_card_scene_t*s,uint32_t pc){if(s){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}}
int wf_r18_card_scene_complete(wf_r18_card_scene_t*s,wf_r18_prematch_t*c,wf_status_t status){
 if(!s||!valid(c)||!s->pending||s->fatal)return WF_R18_CARD_BAD_CONTEXT;
 if(status!=WF_OK)return block(s,c,c->dependency?c->dependency:s->request.owner);
 s->pending=0;c->dependency=0;
 if(s->request.kind==WF_R18_CARD_SYNC&&!(c->ram[0x6f]&2u))return WF_R18_CARD_WAIT;
 s->pc=s->request.return_pc;return WF_R18_CARD_MORE;
}
int wf_r18_card_scene_step(wf_r18_card_scene_t*s,wf_r18_prematch_t*c){
 static const uint16_t palette[12][2]={{0,0},{1,1},{2,0x30},{3,3},{4,0x30},{4,0x30},{6,6},{7,7},{8,8},{9,9},{10,10},{10,10}};
 static const uint16_t royal_text[12]={0x2c,0x2e,0x30,0x32,0,0,0x36,0x38,0x3a,0x3c,0x3e,0x3f};
 static const uint16_t tag_text[12][2]={{0x2b,0x2c},{0x2d,0x2e},{0x2f,0x30},{0x31,0x32},{0x33,0x34},{0x33,0x34},{0x35,0x36},{0x37,0x38},{0x39,0x3a},{0x3b,0x3c},{0x3d,0x3d},{0x3d,0x3d}};
 if(!s||!valid(c))return WF_R18_CARD_BAD_CONTEXT;
 if(s->fatal)return WF_R18_CARD_BLOCKED;
 if(s->pending)return WF_R18_CARD_REQUEST;
 c->dependency=0;uint8_t*r=c->ram,*p;unsigned index;
 switch(s->pc){
 case 0x7b70:s->pc=r16(r+0x8c)||(!special(r)&&r[0x163])?0x7e0eu:0x7b9au;break;
 case 0x7b9a:dw(c,0,special(r)?0x310d:0x310e);s->pc=0x7bb6;break;
 case 0x7bb6:return request(s,0,0x2052,0x7bbc,0,0);
 case 0x7bbc:w16(r+0x80,0);s->pc=0x7bc2;break;
 case 0x7bc2:return request(s,0,0x1fc0,0x7bc8,0,0);
 case 0x7bc8:w16(r+0x15fc,0);w16(r+0x7e,4);w16(r+0x15f4,4);w16(r+0x15f8,4);w16(r+0x1804,0x140);w16(r+0x1806,0x200);s->pc=0x7bf8;break;
 case 0x7bf8:return request(s,0,0x26e66,0x7bfe,0,0);
 case 0x7bfe:return request(s,WF_R18_CARD_WRITE8,0x7bfe,0x7c06,0x140011,0x7b);
 case 0x7c06:return request(s,0,0x2a06,0x7c0c,0,0);
 case 0x7c0c:return request(s,0,0x285da,0x7c12,0,0);
 case 0x7c12:dw(c,0,0x34);s->pc=0x7c16;break;
 case 0x7c16:return request(s,0,0x2aea,0x7c1c,0,0);
 case 0x7c1c:s->pc=r[0x161]&1u?0x7e16u:0x7c28u;break;
 case 0x7c28:if(!r[0x5b3])swap(c,r+0x5b0,r+0x6bc);s->pc=0x7c48;break;
 case 0x7c48:if(r[0x6bf]==9u)swap(c,r+0x6bc,r+0x5b0);s->pc=0x7c68;break;
 case 0x7c68:s->pc=special(r)?0x7cf0u:0x7c80u;break;
 case 0x7c80:c->a[0]=0x1c05b0;s->pc=r[0x5b0]&0x80u?0x7c90u:0x7cf0u;break;
 case 0x7c90:c->a[0]=0x1c05b0;dw(c,0,1);s->pc=0x7c9a;break;
 case 0x7c9a:s->saved_d0=c->regs->d[0];w16(r+0x6e,0);local(s,0x7f0e,0x7caa);break;
 case 0x7caa:return request(s,0,0x247c,0x7cb0,0,0);
 case 0x7cb0:return request(s,0,0x27b8,0x7cb6,0,0);
 case 0x7cb6:c->regs->d[0]=s->saved_d0;c->a[0]+=0x10c;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0x7cc4u:0x7c9au;break;
 case 0x7cc4:local(s,0x7e86,0x7cca);break;
 case 0x7cca:return request(s,0,0x285da,0x7cd0,0,0);
 case 0x7cd0:return request(s,0,0x2836,0x7cd6,0,0);
 case 0x7cd6:r[0x6f]|=1u;s->pc=0x7cde;break;
 case 0x7cde:case 0x7dfc:case 0x7e5e:{
  uint32_t ret=s->pc==0x7cdeu?0x7ce8u:s->pc==0x7dfcu?0x7e06u:0x7e68u;
  if(r[0x6f]&2u){s->pc=ret;break;}
  return request(s,WF_R18_CARD_SYNC,s->pc,ret,0x1c006f,2);
 }
 case 0x7ce8:s->pc=r16(r+0x80)?0x7cf0u:0x7c90u;break;
 case 0x7cf0:w16(r+0x80,0);if(!r[0x7cb])swap(c,r+0x7c8,r+0x8d4);s->pc=0x7d18;break;
 case 0x7d18:if(r[0x8d7]==9u)swap(c,r+0x8d4,r+0x7c8);s->pc=0x7d38;break;
 case 0x7d38:
  if(special(r)){c->a[0]=0x1c09e0;w16(r+0xa36,4);w16(r+0x9e2,4);w16(r+0xb42,5);w16(r+0xaee,5);w16(r+0x9e0,0x8000);w16(r+0xaec,0x8000);}
  else c->a[0]=0x1c07c8;
  s->pc=0x7d8a;break;
 case 0x7d8a:
  if(!actor_address(c->a[0]))return block(s,c,0x7d8a);
  s->pc=r[c->a[0]-0x1c0000u]&0x80u?0x7d94u:0x7e0eu;break;
 case 0x7d94:c->a[0]=special(r)?0x1c09e0u:0x1c07c8u;dw(c,0,1);s->pc=0x7db8;break;
 case 0x7db8:s->saved_d0=c->regs->d[0];w16(r+0x6e,0);local(s,0x7f0e,0x7dc8);break;
 case 0x7dc8:return request(s,0,0x247c,0x7dce,0,0);
 case 0x7dce:return request(s,0,0x27b8,0x7dd4,0,0);
 case 0x7dd4:c->regs->d[0]=s->saved_d0;c->a[0]+=0x10c;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0x7de2u:0x7db8u;break;
 case 0x7de2:local(s,0x7e86,0x7de8);break;
 case 0x7de8:return request(s,0,0x285da,0x7dee,0,0);
 case 0x7dee:return request(s,0,0x2836,0x7df4,0,0);
 case 0x7df4:r[0x6f]|=1u;s->pc=0x7dfc;break;
 case 0x7e06:s->pc=r16(r+0x80)?0x7e0eu:0x7d94u;break;
 case 0x7e0e:return request(s,0,0x1fc0,0x7e14,0,0);
 case 0x7e14:case 0x7e84:return WF_R18_CARD_RETURN;
 case 0x7e16:c->a[0]=0x1c05b0;dw(c,0,3);s->pc=0x7e20;break;
 case 0x7e20:
  if(!actor_address(c->a[0]))return block(s,c,0x7e20);
  s->saved_d0=c->regs->d[0];s->pc=r[c->a[0]-0x1c0000u]&0x80u?0x7e2cu:0x7e70u;break;
 case 0x7e2c:w16(r+0x6e,0);local(s,0x7f0e,0x7e38);break;
 case 0x7e38:return request(s,0,0x247c,0x7e3e,0,0);
 case 0x7e3e:return request(s,0,0x27b8,0x7e44,0,0);
 case 0x7e44:local(s,0x7e86,0x7e4a);break;
 case 0x7e4a:return request(s,0,0x285da,0x7e50,0,0);
 case 0x7e50:return request(s,0,0x2836,0x7e56,0,0);
 case 0x7e56:r[0x6f]|=1u;s->pc=0x7e5e;break;
 case 0x7e68:s->pc=r16(r+0x80)?0x7e70u:0x7e2cu;break;
 case 0x7e70:c->regs->d[0]=s->saved_d0;c->a[0]+=0x10c;w16(r+0x80,0);dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0x7e84u:0x7e20u;break;
 case 0x7e86:
  s->saved_a0=c->a[0];if(!(r[0x161]&1u))c->a[0]-=0x10cu;
  if(!actor_address(c->a[0]))return block(s,c,0x7eb8);
  c->a[1]=0x1c14ce;p=r+0x14ce;w16(p,0x8000);w16(p+2,0x4c);w16(p+6,0x1d8);w16(p+0xe,0x100);w16(p+0xa,(uint16_t)(r16(r+c->a[0]-0x1c0000u+0xa)+1u));
  dw(c,0,r16(p+0x24)?0xffffu:0);w16(p+4,d(c,0));w16(p+0x22,(uint16_t)(r16(p+0x22)+1u));dw(c,0,r16(p+0x24)?1u:2u);
  if(d(c,0)<r16(p+0x22)){w16(p+0x22,0);w16(p+0x24,(uint16_t)(r16(p+0x24)^1u));}
  c->a[0]=c->a[1];s->pc=0x7efc;break;
 case 0x7efc:return request(s,0,0x247c,0x7f02,0,0);
 case 0x7f02:return request(s,0,0x27b8,0x7f08,0,0);
 case 0x7f08:c->a[0]=s->saved_a0;s->pc=0x7f0c;break;
 case 0x7f0c:return returned(s);
 case 0x7f0e:
  if(!actor_address(c->a[0]))return block(s,c,0x7f0e);
  p=r+c->a[0]-0x1c0000u;if(!(p[0]&0x80u)){s->pc=0x809e;break;}
  if(p[0x1c]&0x80u){s->pc=0x8068;break;}
  p[0x1c]|=0x80u;s->pc=r[0x161]&1u?0x7f9au:partner(c->a[0])?0x7fc6u:0x7f4cu;break;
 case 0x7f4c:return request(s,0,0x8fd6,0x7f52,0,0);
 case 0x7f52:return request(s,0,0x1f9e,0x7f58,0,0);
 case 0x7f58:
  if(!actor_address(c->a[0]))return block(s,c,0x7f5e);
  c->a[2]=0x8186;dw(c,1,(uint16_t)((r16(r+c->a[0]-0x1c0000u+2)&255u)<<2));index=d(c,1)>>2;if(index>=12u)return block(s,c,0x8186);
  dw(c,0,tag_text[index][0]);s->pc=0x7f6c;break;
 case 0x7f6c:return request(s,0,0x2503c,0x7f72,0,0);
 case 0x7f72:
  c->a[1]=c->a[0]+0x10cu;if(!actor_address(c->a[1]))return block(s,c,0x7f7a);
  dw(c,1,(uint16_t)(((r16(r+c->a[1]-0x1c0000u+2)&255u)<<2)+2u));index=d(c,1)>>2;if(index>=12u||c->a[2]!=0x8186u)return block(s,c,0x8186);
  dw(c,0,tag_text[index][1]);s->pc=0x7f8c;break;
 case 0x7f8c:return request(s,0,0x2503c,0x7f92,0,0);
 case 0x7f92:return request(s,0,0xb29e,0x7f98,0,0);
 case 0x7f98:s->pc=0x7fc6;break;
 case 0x7f9a:return request(s,0,0x8fd6,0x7fa0,0,0);
 case 0x7fa0:return request(s,0,0x1f9e,0x7fa6,0,0);
 case 0x7fa6:return request(s,0,0xb29e,0x7fac,0,0);
 case 0x7fac:
  if(!actor_address(c->a[0]))return block(s,c,0x7fac);
  dw(c,0,(uint16_t)((r16(r+c->a[0]-0x1c0000u+2)&255u)<<1));c->a[1]=0x816e;index=d(c,0)>>1;if(index>=12u)return block(s,c,0x816e);
  dw(c,0,royal_text[index]);s->pc=0x7fc0;break;
 case 0x7fc0:return request(s,0,0x2503c,0x7fc6,0,0);
 case 0x7fc6:dw(c,0,0x3180);s->pc=0x7fca;break;
 case 0x7fca:return request(s,0,0x2052,0x7fd0,0,0);
 case 0x7fd0:
  if(!actor_address(c->a[0]))return block(s,c,0x7fd6);
  c->a[1]=0x813e;dw(c,1,(uint16_t)((r16(r+c->a[0]-0x1c0000u+2)&255u)<<2));index=d(c,1)>>2;if(index>=12u)return block(s,c,0x813e);
  dw(c,0,palette[index][0]);s->pc=0x7fe4;break;
 case 0x7fe4:return request(s,0,0x2aea,0x7fea,0,0);
 case 0x7fea:index=d(c,1)>>2;if(index>=12u||c->a[1]!=0x813eu)return block(s,c,0x813e);dw(c,0,palette[index][1]);s->pc=0x7fee;break;
 case 0x7fee:return request(s,0,0x2aea,0x7ff4,0,0);
 case 0x7ff4:
  if(!actor_address(c->a[0]))return block(s,c,0x7ff4);
  p=r+c->a[0]-0x1c0000u;w16(p+0x22,0);w16(p+0x24,0);w16(p+2,(uint16_t)(r16(p+2)+0x40u));if(r16(p+2)==0x4bu)w16(p+2,0x4a);
  w16(p,0x8001);w16(p+0x2a,8);w16(p+0x2c,0x80);w16(p+0xa,0x130);w16(p+0xe,0x100);w16(p+6,r[0x161]&1u?0x1d8u:primary(c->a[0])?0x1c0u:0x1f0u);s->pc=0x8068;break;
 case 0x8068:return request(s,0,0x2208,0x806e,0,0);
 case 0x806e:
  if(!actor_address(c->a[0]))return block(s,c,0x806e);
  p=r+c->a[0]-0x1c0000u;s->pc=r16(p+0xa)>=0x90u?0x80aau:!(r[0x161]&1u)&&partner(c->a[0])?0x809eu:0x8098u;break;
 case 0x8098:return request(s,0,0x8f50,0x809e,0,0);
 case 0x809e:w16(r+0x80,0x8000);s->pc=0x8134;break;
 case 0x80aa:
  if(!actor_address(c->a[0]))return block(s,c,0x80aa);
  p=r+c->a[0]-0x1c0000u;c->a[1]=0x8136;dw(c,0,r16(p+0x24)&255u);index=d(c,0);if(index>=8u)return block(s,c,0x8136);p[5]=(uint8_t)index;
  if(p[3]==0x41u)p[0x1b]=p[5]>=4u?0x30u:0;
  w16(p+0x22,(uint16_t)(r16(p+0x22)+1u));if(r16(p+0x22)<12u){s->pc=0x8134;break;}
  w16(p+0x22,0);w16(p+0x24,(uint16_t)(r16(p+0x24)+0x101u));dw(c,0,r16(p+0x24)&0xff00u);if(d(c,0)<0x400u){s->pc=0x8134;break;}
  w16(p+0x24,0);if(p[3]!=0x40u&&p[3]!=0x41u&&p[3]!=0x47u&&p[3]!=0x49u){s->pc=0x8134;break;}
  {uint32_t seed=r32(r+0x5c);wf_r17_rng21b4_live(&seed,r32(r+0x80),c->regs);w32(r+0x5c,seed);dw(c,0,d(c,0)&1u);if(!d(c,0))w16(p+0x24,4);}
  s->pc=0x8134;break;
 case 0x8134:return returned(s);
 default:return block(s,c,s->pc);
 }
 return WF_R18_CARD_MORE;
}
