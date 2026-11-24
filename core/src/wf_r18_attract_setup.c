#include "wf_r18_attract_setup.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void wl(uint8_t*p,uint32_t v){ww(p,(uint16_t)(v>>16));ww(p+2,(uint16_t)v);}
static void dw(wf_r18_prematch_t*c,unsigned i,uint16_t v){wf_r17_m68k_move_w(c->regs,i,v);}
static uint16_t d(wf_r18_prematch_t*c,unsigned i){return(uint16_t)c->regs->d[i];}
static int valid(wf_r18_prematch_t*c){return c&&c->ram&&c->regs&&c->regs->valid;}
static int block(wf_r18_attract_setup_t*s,wf_r18_prematch_t*c,uint32_t pc){s->fatal=1;s->blocked_dependency=c->dependency=pc;return WF_R18_ATTRACT_BLOCKED;}
static int request(wf_r18_attract_setup_t*s,unsigned kind,uint32_t owner,uint32_t ret,uint32_t address){s->request=(wf_r18_attract_request_t){kind,owner,s->pc,ret,address,0};s->pending=1;return WF_R18_ATTRACT_REQUEST;}
static int returned(wf_r18_attract_setup_t*s){if(!s->sub_return)return WF_R18_ATTRACT_RETURN;s->pc=s->sub_return;s->sub_return=0;return WF_R18_ATTRACT_MORE;}
static int actor(uint32_t a){return a>=0x1c05b0u&&a<=0x1c0aecu&&(a-0x1c05b0u)%0x10cu==0u;}
void wf_r18_attract_setup_init(wf_r18_attract_setup_t*s,uint32_t pc){if(s){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}}
int wf_r18_attract_setup_complete(wf_r18_attract_setup_t*s,wf_r18_prematch_t*c,wf_status_t status,uint16_t word){
 if(!s||!valid(c)||!s->pending||s->fatal)return WF_R18_ATTRACT_BAD_CONTEXT;
 if(status!=WF_OK)return block(s,c,c->dependency?c->dependency:s->request.owner);
 s->pending=0;c->dependency=0;
 if(s->request.kind==WF_R18_ATTRACT_READ16&&!!(word&0x400u)!=(s->pc==0x21eau))return WF_R18_ATTRACT_WAIT;
 s->pc=s->request.return_pc;return WF_R18_ATTRACT_MORE;
}
int wf_r18_attract_setup_step(wf_r18_attract_setup_t*s,wf_r18_prematch_t*c){
 static const uint16_t health[14]={0x64,0xc8,0x3c,0x3c,0xc8,0xc8,0xc8,0x64,0x46,0x50,0x64,0x64,0x3c,0x64};
 static const uint8_t identities[14][4]={{0,1,4,5},{3,6,7,8},{0,1,6,7},{1,0,9,8},{2,3,10,11},{3,2,6,9},{6,7,0,1},{7,9,2,0},{8,6,3,2},{9,8,1,3},{10,11,2,1},{11,10,2,1},{4,5,0,1},{5,4,0,1}};
 static const uint8_t royal[6]={3,6,7,8,9,2};
 static const uint32_t links[4]={0x1c06bc,0x1c05b0,0x1c08d4,0x1c07c8};
 if(!s||!valid(c))return WF_R18_ATTRACT_BAD_CONTEXT;
 if(s->fatal)return WF_R18_ATTRACT_BLOCKED;
 if(s->pending)return WF_R18_ATTRACT_REQUEST;
 c->dependency=0;uint8_t*r=c->ram,*p;unsigned index;
 switch(s->pc){
 case 0xb1c:
  dw(c,0,1);if(rw(r+0x166)==1u)s->pc=0xb40;
  else if(rw(r+0x166)==2u){dw(c,0,2);r[0x161]=1;s->pc=0xb40;}
  else s->pc=0xb6a;
  break;
 case 0xb40:return request(s,0,0xae20,0xb46,0);
 case 0xb46:return request(s,0,0x264e2,0xb4c,0);
 case 0xb4c:return request(s,0,0x69c8,0xb52,0);
 case 0xb52:dw(c,0,0x100);s->pc=0xb56;break;
 case 0xb56:s->sub_return=0xb5c;s->pc=0x21e6;break;
 case 0xb5c:return request(s,0,0x264e2,0xb62,0);
 case 0xb62:r[0x161]=0;s->pc=0xb6a;break;
 case 0xb6a:s->pc=rw(r+0x166)==6u||rw(r+0x166)==10u?0xb7eu:0xb84u;break;
 case 0xb7e:return request(s,0,0x8490,0xb84,0);
 case 0xb84:for(unsigned i=0;i<4u;i++)r[0x5b0u+i*0x10cu]|=0x80u;s->pc=0xba4;break;
 case 0xba4:return request(s,0,0xc98,0xbaa,0);
 case 0xbaa:s->sub_return=0xbb0;s->pc=0xe02;break;
 case 0xbb0:return request(s,0,0x8fd6,0xbb6,0);
 case 0xbb6:wl(r+0x80,0);s->pc=0xbbc;break;
 case 0xbbc:return request(s,0,0x1f9e,0xbc2,0);
 case 0xbc2:ww(r+0x15fc,2);s->pc=0xbca;break;
 case 0xbca:return request(s,0,0x2a06,0xbd0,0);
 case 0xbd0:return WF_R18_ATTRACT_RETURN;
 case 0xe02:wl(r+0x80,0);ww(r+0x160,0);wl(r+0x162,0);c->a[0]=0x1c05b0;dw(c,4,0);s->pc=0xe20;break;
 case 0xe20:
  c->a[1]=0xf3e;dw(c,1,rw(r+0x166));if((uint8_t)d(c,1)==1u){s->pc=0xecc;break;}
  dw(c,1,(uint16_t)((d(c,1)<<2)+d(c,4)));index=d(c,1);
  if(index>=sizeof identities||!actor(c->a[0]))return block(s,c,0xf3e);
  p=r+c->a[0]-0x1c0000u;p[3]=identities[index/4u][index%4u];p[0x57]=p[3];p[0x56]=0x80;dw(c,0,rw(p+2));s->pc=0xe4e;break;
 case 0xe4e:return request(s,0,0x2aea,0xe54,0);
 case 0xe54:
  if(!actor(c->a[0])||d(c,4)>=4u)return block(s,c,0xe54);
  p=r+c->a[0]-0x1c0000u;ww(p,0x8000);dw(c,1,(uint16_t)(d(c,4)<<2));c->a[1]=0xf7c;wl(p+0x86,links[d(c,4)]);
  dw(c,1,0x7000);dw(c,2,0x7000);dw(c,3,0);
  if(d(c,4)){c->a[2]=0xf22;dw(c,7,(uint16_t)(rw(r+0x166)<<1));index=d(c,7)>>1;if(index>=14u)return block(s,c,0xf22);dw(c,1,health[index]);dw(c,2,d(c,1));dw(c,3,0);}
  ww(p+0x66,d(c,1));ww(p+0x72,d(c,2));ww(p+0x70,d(c,3));c->a[0]+=0x10c;dw(c,4,(uint16_t)(d(c,4)+1u));s->pc=d(c,4)<4u?0xe20u:0xeb2u;break;
 case 0xeb2:
  c->a[0]=0x1c09e0;c->a[1]=0x1c0aec;dw(c,0,0x10b);memset(r+0x9e0,0,0x218);c->a[0]+=0x10c;c->a[1]+=0x10c;dw(c,0,0xffff);s->pc=0xf20;break;
 case 0xecc:ww(r+0x160,1);s->pc=0xed4;break;
 case 0xed4:return request(s,0,0x2988,0xed8,0);
 case 0xed8:c->a[0]=0x1c05b0;dw(c,0,5);s->pc=0xee2;break;
 case 0xee2:
  s->saved_d0=c->regs->d[0];c->a[1]=0xf76;index=d(c,0);
  if(index>=6u||!actor(c->a[0]))return block(s,c,0xf76);
  p=r+c->a[0]-0x1c0000u;p[0x57]=royal[index];p[0x56]=0x80;p[3]=p[0x57];ww(p,0x8000);dw(c,0,rw(p+2));s->pc=0xf08;break;
 case 0xf08:return request(s,0,0x2aea,0xf0c,0);
 case 0xf0c:c->regs->d[0]=s->saved_d0;c->a[0]+=0x10c;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0xf1au:0xee2u;break;
 case 0xf1a:return request(s,0,0x1086c,0xf20,0);
 case 0xf20:return returned(s);
 case 0x21e6:s->saved_d0=c->regs->d[0];s->pc=0x21ea;break;
 case 0x21ea:return request(s,WF_R18_ATTRACT_READ16,0x21ea,0x21f4,0x140026);
 case 0x21f4:return request(s,WF_R18_ATTRACT_READ16,0x21f4,0x21fe,0x140026);
 case 0x21fe:dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0x2202u:0x21eau;break;
 case 0x2202:c->regs->d[0]=s->saved_d0;s->pc=0x2206;break;
 case 0x2206:return returned(s);
 default:return block(s,c,s->pc);
 }
 return WF_R18_ATTRACT_MORE;
}
