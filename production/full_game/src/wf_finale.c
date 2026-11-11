#include "wf_finale.h"
#include <string.h>

/* ROM sha256 265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306.
 * PCs are original instruction addresses, not a replacement scene script. */
static uint16_t rw(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint16_t d(const wf_r18_prematch_t*m,unsigned n){return(uint16_t)m->regs->d[n];}
static void dw(wf_r18_prematch_t*m,unsigned n,uint16_t value){wf_r17_m68k_move_w(m->regs,n,value);}
static int valid(const wf_finale_context_t*c){return c&&c->machine&&c->machine->ram&&c->machine->regs&&c->machine->regs->valid;}
static int block(wf_finale_t*s,wf_r18_prematch_t*m,uint32_t pc){s->fatal=1;s->blocked_dependency=m->dependency=pc;return WF_FINALE_BLOCKED;}
static uint8_t*object(wf_finale_t*s,wf_r18_prematch_t*m,unsigned n,unsigned bytes){
 uint32_t at=m->a[n]&0xffffffu;
 if(at<0x1c0000u||at>0x1c4000u-bytes){block(s,m,s->pc);return NULL;}
 return m->ram+at-0x1c0000u;
}
static int rom(wf_finale_t*s,wf_finale_context_t*c,uint32_t at,uint8_t*out,size_t n){
 if(!c->read_rom||c->read_rom(c->user,at&0xffffffu,out,n)!=WF_OK){block(s,c->machine,s->pc);return 0;}
 return 1;
}
static int request(wf_finale_t*s,unsigned kind,uint32_t owner,uint32_t ret,uint32_t at,uint32_t value){
 s->request=(wf_finale_request_t){kind,owner,s->pc,ret,at,value};s->pending=1;return WF_FINALE_REQUEST;
}
static int call(wf_finale_t*s,uint32_t owner,uint32_t ret){return request(s,WF_FINALE_CALL,owner,ret,0,0);}
static int level(wf_finale_t*s,unsigned high,uint32_t ret){return request(s,WF_FINALE_LEVEL,s->pc,ret,0x140026u,high);}
void wf_finale_init(wf_finale_t*s){if(s){memset(s,0,sizeof *s);s->pc=0x1b4eu;}}
int wf_finale_complete(wf_finale_t*s,wf_finale_context_t*c,wf_status_t status,uint16_t value){
 if(!s||!valid(c)||!s->pending||s->fatal)return WF_FINALE_BAD_CONTEXT;
 wf_r18_prematch_t*m=c->machine;
 if(status!=WF_OK)return block(s,m,m->dependency?m->dependency:s->request.owner);
 if(s->request.kind==WF_FINALE_LEVEL&&!!(value&4u)!=s->request.value)return WF_FINALE_WAIT;
 if(s->request.kind==WF_FINALE_READ16||s->request.kind==WF_FINALE_READ8)s->request.value=value;
 s->pending=0;m->dependency=0;s->pc=s->request.return_pc;return WF_FINALE_MORE;
}
int wf_finale_step(wf_finale_t*s,wf_finale_context_t*c){
 if(!s||!valid(c))return WF_FINALE_BAD_CONTEXT;
 wf_r18_prematch_t*m=c->machine;
 if(s->fatal)return WF_FINALE_BLOCKED;
 if(s->pending)return WF_FINALE_REQUEST;
 m->dependency=0;uint8_t*r=m->ram,*p,*q,t[4];uint32_t swap;
 switch(s->pc){
 case 0x1b4e:memcpy(s->saved_d,m->regs->d,sizeof s->saved_d);memcpy(s->saved_a,m->a,sizeof s->saved_a);s->pc=0x1b52;break;
 case 0x1b52:return call(s,0x264e2,0x1b58);
 case 0x1b58:return call(s,0x1f6c,0x1b5c);
 case 0x1b5c:return call(s,0x1fde,0x1b60);
 case 0x1b60:return call(s,0x1f9e,0x1b64);
 case 0x1b64:return call(s,0x2988,0x1b68);
 case 0x1b68:r[0x76]&=0x7fu;dw(m,0,0x11);s->pc=0x1b74;break;
 case 0x1b74:return call(s,0x2052,0x1b7a);
 case 0x1b7a:ww(r+0x7e,4);ww(r+0x15f4,4);ww(r+0x15f8,4);s->pc=0x1b92;break;
 case 0x1b92:return call(s,0x2a06,0x1b96);
 case 0x1b96:ww(r+0x1804,0x280);ww(r+0x1806,0x300);s->pc=0x1ba6;break;
 case 0x1ba6:return call(s,0x26e66,0x1bac);
 case 0x1bac:return level(s,1,0x1bb6);
 case 0x1bb6:return level(s,0,0x1bc0);
 case 0x1bc0:return request(s,WF_FINALE_WRITE16,s->pc,0x1bc8,0x140010,0x78);
 case 0x1bc8:m->a[0]=0x1c05b0;m->a[1]=0x1de8;s->pc=0x1bd4;break;
 case 0x1bd4:p=object(s,m,0,4);if(!p)return WF_FINALE_BLOCKED;ww(p,0x8000);ww(p+2,0x4d);dw(m,1,8);dw(m,3,0x58);s->pc=0x1be8;break;
 case 0x1be8:case 0x1bec:
  p=object(s,m,0,0x18);if(!p||!rom(s,c,m->a[1],t,2))return WF_FINALE_BLOCKED;
  ww(p+(s->pc==0x1be8u?0x14u:0x16u),rw(t));m->a[1]+=2;s->pc=s->pc==0x1be8u?0x1becu:0x1bf0u;break;
 case 0x1bf0:
  p=object(s,m,0,6);if(!p||!rom(s,c,m->a[1],t,2))return WF_FINALE_BLOCKED;
  ww(p+4,rw(t));s->pc=0x1bf4;break;
 case 0x1bf4:
  if(!rom(s,c,m->a[1],t,2))return WF_FINALE_BLOCKED;
  dw(m,0,rw(t));m->a[1]+=2;dw(m,0,(uint16_t)(d(m,0)+0x40u));s->pc=0x1bfa;break;
 case 0x1bfa:return call(s,0x2aea,0x1bfe);
 case 0x1bfe:swap=m->regs->d[0];m->regs->d[0]=m->regs->d[3];m->regs->d[3]=swap;s->pc=0x1c00;break;
 case 0x1c00:return call(s,0x2503c,0x1c06);
 case 0x1c06:swap=m->regs->d[0];m->regs->d[0]=m->regs->d[3];m->regs->d[3]=swap;dw(m,3,(uint16_t)(d(m,3)+1u));s->pc=0x1c0a;break;
 case 0x1c0a:return call(s,0x27b8,0x1c0e);
 case 0x1c0e:return call(s,0x2836,0x1c12);
 case 0x1c12:return level(s,1,0x1c1c);
 case 0x1c1c:return request(s,WF_FINALE_WRITE16,s->pc,0x1c22,0x140008,0);
 case 0x1c22:return level(s,0,0x1c2c);
 case 0x1c2c:return call(s,0x2b58,0x1c30);
 case 0x1c30:dw(m,0,0x80);s->pc=0x1c34;break;
 case 0x1c34:case 0x1c40:case 0x1c4c:case 0x1c58:
  return request(s,WF_FINALE_READ16,s->pc,s->pc+6u,0x140020u+((s->pc-0x1c34u)/12u)*2u,0);
 case 0x1c3a:case 0x1c46:case 0x1c52:case 0x1c5e:
  dw(m,4u+(s->pc-0x1c3au)/12u,(uint16_t)(~s->request.value&0x30u));s->pc+=6u;break;
 case 0x1c64:dw(m,4,(uint16_t)(d(m,4)|d(m,5)|d(m,6)|d(m,7)));s->pc=d(m,4)?0x1d72u:0x1c70u;break;
 case 0x1c70:return level(s,1,0x1c7a);
 case 0x1c7a:return level(s,0,0x1c84);
 case 0x1c84:dw(m,0,(uint16_t)(d(m,0)-1u));s->pc=d(m,0)!=0xffffu?0x1c34u:0x1c88u;break;
 case 0x1c88:return call(s,0x1f9e,0x1c8c);
 case 0x1c8c:dw(m,1,(uint16_t)(d(m,1)-1u));s->pc=d(m,1)!=0xffffu?0x1be8u:0x1c90u;break;
 case 0x1c90:return call(s,0x1fde,0x1c94);
 case 0x1c94:dw(m,0,0x3b);s->pc=0x1c98;break;
 case 0x1c98:return call(s,0x2aea,0x1c9e);
 case 0x1c9e:m->a[1]=0x5122;dw(m,0,0);s->pc=0x1ca6;break;
 case 0x1ca6:dw(m,1,d(m,0));dw(m,1,(uint16_t)(d(m,1)<<2));s->pc=0x1caa;break;
 case 0x1caa:
  if(!rom(s,c,m->a[1]+(int32_t)(int16_t)d(m,1),t,4))return WF_FINALE_BLOCKED;
  m->a[0]=((uint32_t)rw(t)<<16)|rw(t+2);s->pc=0x1cae;break;
 case 0x1cae:
  p=object(s,m,0,0x2e);if(!p)return WF_FINALE_BLOCKED;
  ww(p,0x8001);ww(p+2,0x50);ww(p+4,d(m,0));ww(p+0x2c,0);ww(p+0x2a,12);m->a[2]=0x1dd0;s->pc=0x1cce;break;
 case 0x1cce:case 0x1cd4:
  p=object(s,m,0,0x1c);if(!p)return WF_FINALE_BLOCKED;
  if(!rom(s,c,m->a[2]+(int32_t)(int16_t)d(m,1)+(s->pc==0x1cd4u?2u:0u),t,2))return WF_FINALE_BLOCKED;
  ww(p+(s->pc==0x1cceu?6u:10u),rw(t));s->pc=s->pc==0x1cceu?0x1cd4u:0x1cdau;break;
 case 0x1cda:
  p=object(s,m,0,0x1c);if(!p)return WF_FINALE_BLOCKED;
  ww(p+14,0x100);memset(p+0x18,0,4);dw(m,0,(uint16_t)(d(m,0)+1u));s->pc=(uint8_t)d(m,0)<6u?0x1ca6u:0x1cecu;break;
 case 0x1cec:dw(m,1,0x500);s->pc=0x1cf0;break;
 case 0x1cf0:dw(m,0,0);s->pc=0x1cf2;break;
 case 0x1cf2:s->pc=0x250e;break;
 /* Original $250E: WORD arithmetic preserves D0's upper half. It uses
  * the actual ROM pointers; do not substitute six sequential objects. */
 case 0x250e:s->pc=d(m,0)>=9u?0x2532u:0x2516u;break;
 case 0x2516:m->a[0]=0x5122;dw(m,0,(uint16_t)(d(m,0)<<2));s->pc=0x251e;break;
 case 0x251e:
  if(!rom(s,c,m->a[0]+(int32_t)(int16_t)d(m,0),t,4))return WF_FINALE_BLOCKED;
  m->a[0]=((uint32_t)rw(t)<<16)|rw(t+2);s->pc=0x2522;break;
 case 0x2522:dw(m,0,d(m,0)>>2);dw(m,0,(uint16_t)(d(m,0)+1u));s->pc=0x2526;break;
 case 0x2526:p=object(s,m,0,1);if(!p)return WF_FINALE_BLOCKED;s->pc=p[0]&0x80u?0x253au:0x250eu;break;
 case 0x2532:s->carry=1;s->pc=0x253e;break;
 case 0x253a:s->carry=0;s->pc=0x253e;break;
 case 0x253e:s->pc=0x1cf6;break;
 case 0x1cf6:s->pc=s->carry?0x1d16u:0x1cf8u;break;
 case 0x1cf8:return call(s,0x2208,0x1cfc);
 case 0x1cfc:p=object(s,m,0,12);if(!p)return WF_FINALE_BLOCKED;s->pc=rw(p+10)<0x180u?0x1cf2u:0x1d04u;break;
 case 0x1d04:p=object(s,m,0,12);if(!p)return WF_FINALE_BLOCKED;s->pc=rw(p+10)>=0x380u?0x1cf2u:0x1d0cu;break;
 case 0x1d0c:return call(s,0x247c,0x1d10);
 case 0x1d10:return call(s,0x27b8,0x1d14);
 case 0x1d14:s->pc=0x1cf2;break;
 case 0x1d16:return call(s,0x2836,0x1d1a);
 case 0x1d1a:case 0x1d26:case 0x1d32:case 0x1d3e:
  return request(s,WF_FINALE_READ16,s->pc,s->pc+6u,0x140020u+((s->pc-0x1d1au)/12u)*2u,0);
 case 0x1d20:case 0x1d2c:case 0x1d38:case 0x1d44:
  dw(m,4u+(s->pc-0x1d20u)/12u,(uint16_t)(~s->request.value&0x30u));s->pc+=6u;break;
 case 0x1d4a:dw(m,4,(uint16_t)(d(m,4)|d(m,5)|d(m,6)|d(m,7)));s->pc=d(m,4)?0x1d72u:0x1d54u;break;
 case 0x1d54:return level(s,1,0x1d5e);
 case 0x1d5e:return request(s,WF_FINALE_WRITE16,s->pc,0x1d64,0x140008,0);
 case 0x1d64:return level(s,0,0x1d6e);
 case 0x1d6e:dw(m,1,(uint16_t)(d(m,1)-1u));s->pc=d(m,1)!=0xffffu?0x1cf0u:0x1d72u;break;
 case 0x1d72:dw(m,0,0x1f);s->pc=0x1d76;break;
 case 0x1d76:return call(s,0x2052,0x1d7c);
 case 0x1d7c:return call(s,0x264e2,0x1d82);
 case 0x1d82:r[0x76]|=0x80u;memcpy(m->regs->d,s->saved_d,sizeof s->saved_d);memcpy(m->a,s->saved_a,sizeof s->saved_a);s->pc=0x1d8e;break;
 case 0x1d8e:dw(m,0,0x3b);s->pc=0x1d92;break;
 case 0x1d92:return call(s,0x2b58,0x1d98);
 case 0x1d98:m->a[0]=0x1c0092;m->a[1]=0x1c05b0;dw(m,0,3);s->pc=0x1da8;break;
 case 0x1da8:
  p=object(s,m,1,0xa2);if(!p)return WF_FINALE_BLOCKED;
  if((p[0]&0x80u)&&(p[0xa1]&1u)){
   q=object(s,m,0,4);if(!q)return WF_FINALE_BLOCKED;
   ww(q,(uint16_t)(m->a[1]>>16));ww(q+2,(uint16_t)m->a[1]);m->a[0]+=4;
  }
  m->a[1]+=0x10c;dw(m,0,(uint16_t)(d(m,0)-1u));if(d(m,0)==0xffffu)s->pc=0x1dc4;break;
 case 0x1dc4:return call(s,0x65c0,0x1dca);
 case 0x1dca:s->pc=0x6fc;break;
 case 0x6fc:return WF_FINALE_BOOT;
 default:return block(s,m,s->pc);
 }
 return WF_FINALE_MORE;
}
