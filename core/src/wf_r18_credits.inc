/* Native continuation of ROM $1B4E..$1DCF, shared by the Royal winner and
 * the last TAG victory. Included by the post cursor to reuse its checked
 * memory/ROM access and asynchronous request contract. No renderer, input,
 * IRQ, wait, sound, fade, palette or ranking dependency is synthesized. */
static uint32_t credits_rl(const uint8_t *p){return ((uint32_t)rw(p)<<16)|rw(p+2);}
static void credits_wl(uint8_t *p,uint32_t v){ww(p,(uint16_t)(v>>16));ww(p+2,(uint16_t)v);}
static int credits_level(wf_r18_post_tag_t*s,unsigned high,uint32_t ret){
 return request(s,WF_R18_POST_LEVEL,s->pc,ret,0x140026u,high);
}
static int credits_read(wf_r18_post_tag_t*s,uint32_t address,uint32_t ret){
 return request(s,WF_R18_POST_READ16,s->pc,ret,address,0);
}
static void credits_input(wf_r18_prematch_t*c,unsigned reg,uint16_t value){
 dw(c,reg,value);dw(c,reg,(uint16_t)~d(c,reg));dw(c,reg,d(c,reg)&0x30u);
}
static int credits_step(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x){
 wf_r18_prematch_t*c=x->machine;uint8_t*r=c->ram,*p,*q,t[8];uint32_t value;
 switch(s->pc){
 case 0x1b4e:
  memcpy(s->saved_d,c->regs->d,2*sizeof(uint32_t));
  memcpy(s->saved_a,c->a,3*sizeof(uint32_t));s->pc=0x1b52;break;
 case 0x1b52:return call(s,0x264e2,0x1b58);
 case 0x1b58:return call(s,0x1f6c,0x1b5c);
 case 0x1b5c:return call(s,0x1fde,0x1b60);
 case 0x1b60:return call(s,0x1f9e,0x1b64);
 case 0x1b64:return call(s,0x2988,0x1b68);
 case 0x1b68:r[0x76]&=0x7fu;dw(c,0,0x11);s->pc=0x1b74;break;
 case 0x1b74:return call(s,0x2052,0x1b7a);
 case 0x1b7a:ww(r+0x7e,4);ww(r+0x15f4,4);ww(r+0x15f8,4);s->pc=0x1b92;break;
 case 0x1b92:return call(s,0x2a06,0x1b96);
 case 0x1b96:ww(r+0x1804,0x280);ww(r+0x1806,0x300);s->pc=0x1ba6;break;
 case 0x1ba6:return call(s,0x26e66,0x1bac);
 case 0x1bac:return credits_level(s,1,0x1bb6);
 case 0x1bb6:return credits_level(s,0,0x1bc0);
 case 0x1bc0:return request(s,WF_R18_POST_WRITE16,s->pc,0x1bc8,0x140010,0x78);
 case 0x1bc8:
  c->a[0]=0x1c05b0;c->a[1]=0x1de8;p=object(s,c,0,6);if(!p)return WF_R18_POST_BLOCKED;
  ww(p,0x8000);ww(p+2,0x4d);dw(c,1,8);dw(c,3,0x58);s->pc=0x1be8;break;
 case 0x1be8:
  p=object(s,c,0,0x18);if(!p)return WF_R18_POST_BLOCKED;
  if(!readrom(s,x,c->a[1],t,6))return WF_R18_POST_BLOCKED;
  ww(p+0x14,rw(t));c->a[1]+=2;ww(p+0x16,rw(t+2));c->a[1]+=2;
  ww(p+4,rw(t+4));dw(c,0,rw(t+4));c->a[1]+=2;
  dw(c,0,(uint16_t)(d(c,0)+0x40u));s->pc=0x1bfa;break;
 case 0x1bfa:return call(s,0x2aea,0x1bfe);
 case 0x1bfe:
  value=c->regs->d[0];c->regs->d[0]=c->regs->d[3];c->regs->d[3]=value;s->pc=0x1c00;break;
 case 0x1c00:return call(s,0x2503c,0x1c06);
 case 0x1c06:
  value=c->regs->d[0];c->regs->d[0]=c->regs->d[3];c->regs->d[3]=value;
  dw(c,3,(uint16_t)(d(c,3)+1u));s->pc=0x1c0a;break;
 case 0x1c0a:return call(s,0x27b8,0x1c0e);
 case 0x1c0e:return call(s,0x2836,0x1c12);
 case 0x1c12:return credits_level(s,1,0x1c1c);
 case 0x1c1c:return request(s,WF_R18_POST_WRITE16,s->pc,0x1c22,0x140008,0);
 case 0x1c22:return credits_level(s,0,0x1c2c);
 case 0x1c2c:return call(s,0x2b58,0x1c30);
 case 0x1c30:dw(c,0,0x80);s->pc=0x1c34;break;
 case 0x1c34:return credits_read(s,0x140020,0x1c3a);
 case 0x1c3a:credits_input(c,4,(uint16_t)s->request.value);s->pc=0x1c40;break;
 case 0x1c40:return credits_read(s,0x140022,0x1c46);
 case 0x1c46:credits_input(c,5,(uint16_t)s->request.value);s->pc=0x1c4c;break;
 case 0x1c4c:return credits_read(s,0x140024,0x1c52);
 case 0x1c52:credits_input(c,6,(uint16_t)s->request.value);s->pc=0x1c58;break;
 case 0x1c58:return credits_read(s,0x140026,0x1c5e);
 case 0x1c5e:
  credits_input(c,7,(uint16_t)s->request.value);
  dw(c,4,d(c,4)|d(c,5));dw(c,4,d(c,4)|d(c,6));dw(c,4,d(c,4)|d(c,7));
  s->pc=d(c,4)?0x1d72u:0x1c70u;break;
 case 0x1c70:return credits_level(s,1,0x1c7a);
 case 0x1c7a:return credits_level(s,0,0x1c84);
 case 0x1c84:dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0x1c34u:0x1c88u;break;
 case 0x1c88:return call(s,0x1f9e,0x1c8c);
 case 0x1c8c:dw(c,1,(uint16_t)(d(c,1)-1u));s->pc=d(c,1)!=0xffffu?0x1be8u:0x1c90u;break;
 case 0x1c90:return call(s,0x1fde,0x1c94);
 case 0x1c94:dw(c,0,0x3b);s->pc=0x1c98;break;
 case 0x1c98:return call(s,0x2aea,0x1c9e);
 case 0x1c9e:c->a[1]=0x5122;dw(c,0,0);s->pc=0x1ca6;break;
 case 0x1ca6:
  dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)<<2));
  if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,1),t,4))return WF_R18_POST_BLOCKED;
  c->a[0]=credits_rl(t);p=object(s,c,0,0x2e);if(!p)return WF_R18_POST_BLOCKED;
  ww(p,0x8001);ww(p+2,0x50);ww(p+4,d(c,0));ww(p+0x2c,0);ww(p+0x2a,12);
  c->a[2]=0x1dd0;
  if(!readrom(s,x,c->a[2]+(int32_t)(int16_t)d(c,1),t,4))return WF_R18_POST_BLOCKED;
  ww(p+6,rw(t));ww(p+10,rw(t+2));ww(p+14,0x100);credits_wl(p+0x18,0);
  dw(c,0,(uint16_t)(d(c,0)+1u));s->pc=(uint8_t)d(c,0)<6u?0x1ca6u:0x1cecu;break;
 case 0x1cec:dw(c,1,0x500);s->pc=0x1cf0;break;
 case 0x1cf0:dw(c,0,0);s->pc=0x1cf2;break;
 case 0x1cf2:
  /* $250E returns each active object from the original nine-entry table;
   * D0.W advances through inactive entries too, and A0 is caller-visible. */
  s->carry=1;
  while(d(c,0)<9u){
   c->a[0]=0x5122;dw(c,0,(uint16_t)(d(c,0)<<2));
   if(!readrom(s,x,c->a[0]+(int32_t)(int16_t)d(c,0),t,4))return WF_R18_POST_BLOCKED;
   c->a[0]=credits_rl(t);dw(c,0,d(c,0)>>2);dw(c,0,(uint16_t)(d(c,0)+1u));
   p=object(s,c,0,1);if(!p)return WF_R18_POST_BLOCKED;
   if(p[0]&128u){s->carry=0;break;}
  }
  s->pc=s->carry?0x1d16u:0x1cf8u;break;
 case 0x1cf8:return call(s,0x2208,0x1cfc);
 case 0x1cfc:
  p=object(s,c,0,12);if(!p)return WF_R18_POST_BLOCKED;
  s->pc=rw(p+10)<0x180u||rw(p+10)>=0x380u?0x1cf2u:0x1d0cu;break;
 case 0x1d0c:return call(s,0x247c,0x1d10);
 case 0x1d10:return call(s,0x27b8,0x1d14);
 case 0x1d14:s->pc=0x1cf2;break;
 case 0x1d16:return call(s,0x2836,0x1d1a);
 case 0x1d1a:return credits_read(s,0x140020,0x1d20);
 case 0x1d20:credits_input(c,4,(uint16_t)s->request.value);s->pc=0x1d26;break;
 case 0x1d26:return credits_read(s,0x140022,0x1d2c);
 case 0x1d2c:credits_input(c,5,(uint16_t)s->request.value);s->pc=0x1d32;break;
 case 0x1d32:return credits_read(s,0x140024,0x1d38);
 case 0x1d38:credits_input(c,6,(uint16_t)s->request.value);s->pc=0x1d3e;break;
 case 0x1d3e:return credits_read(s,0x140026,0x1d44);
 case 0x1d44:
  credits_input(c,7,(uint16_t)s->request.value);
  dw(c,4,d(c,4)|d(c,5));dw(c,4,d(c,4)|d(c,6));dw(c,4,d(c,4)|d(c,7));
  s->pc=d(c,4)?0x1d72u:0x1d54u;break;
 case 0x1d54:return credits_level(s,1,0x1d5e);
 case 0x1d5e:return request(s,WF_R18_POST_WRITE16,s->pc,0x1d64,0x140008,0);
 case 0x1d64:return credits_level(s,0,0x1d6e);
 case 0x1d6e:dw(c,1,(uint16_t)(d(c,1)-1u));s->pc=d(c,1)!=0xffffu?0x1cf0u:0x1d72u;break;
 case 0x1d72:dw(c,0,0x1f);s->pc=0x1d76;break;
 case 0x1d76:return call(s,0x2052,0x1d7c);
 case 0x1d7c:return call(s,0x264e2,0x1d82);
 case 0x1d82:
  r[0x76]|=128u;memcpy(c->regs->d,s->saved_d,2*sizeof(uint32_t));
  memcpy(c->a,s->saved_a,3*sizeof(uint32_t));dw(c,0,0x3b);s->pc=0x1d92;break;
 case 0x1d92:return call(s,0x2b58,0x1d98);
 case 0x1d98:c->a[0]=0x1c0092;c->a[1]=0x1c05b0;dw(c,0,3);s->pc=0x1da8;break;
 case 0x1da8:
  p=object(s,c,1,0xa2);if(!p)return WF_R18_POST_BLOCKED;
  if((p[0]&128u)&&(p[0xa1]&1u)){
   q=object(s,c,0,4);if(!q)return WF_R18_POST_BLOCKED;credits_wl(q,c->a[1]);c->a[0]+=4;
  }
  c->a[1]+=0x10c;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0x1da8u:0x1dc4u;break;
 case 0x1dc4:return call(s,0x65c0,0x1dca);
 case 0x1dca:s->pc=0x6fc;break;
 default:return block(s,c,s->pc);
 }
 return WF_R18_POST_MORE;
}
