#include "wf_r18_post_tag.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint16_t d(wf_r18_prematch_t*c,unsigned n){return(uint16_t)c->regs->d[n];}
static void dw(wf_r18_prematch_t*c,unsigned n,uint16_t v){wf_r17_m68k_move_w(c->regs,n,v);}
static void db(wf_r18_prematch_t*c,unsigned n,uint8_t v){c->regs->d[n]=(c->regs->d[n]&0xffffff00u)|v;}
static int valid(wf_r18_post_tag_context_t*c){return c&&c->machine&&c->machine->ram&&c->machine->regs&&c->machine->regs->valid;}
static int block(wf_r18_post_tag_t*s,wf_r18_prematch_t*c,uint32_t pc){s->fatal=1;s->blocked_dependency=c->dependency=pc;return WF_R18_POST_BLOCKED;}
static uint8_t* object(wf_r18_post_tag_t*s,wf_r18_prematch_t*c,unsigned n,unsigned bytes){uint32_t at=c->a[n]&0xffffffu;if(at<0x1c0000u||at>0x1c4000u-bytes){block(s,c,s->pc);return NULL;}return c->ram+at-0x1c0000u;}
static int readrom(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x,uint32_t at,uint8_t*p,size_t n){if(!x->read_rom||x->read_rom(x->user,at&0xffffffu,p,n)!=WF_OK){block(s,x->machine,s->pc);return 0;}return 1;}
static int request(wf_r18_post_tag_t*s,unsigned kind,uint32_t owner,uint32_t ret,uint32_t at,uint32_t value){s->request=(wf_r18_post_tag_request_t){kind,owner,s->pc,ret,at,value};s->pending=1;return WF_R18_POST_REQUEST;}
static int call(wf_r18_post_tag_t*s,uint32_t owner,uint32_t ret){return request(s,0,owner,ret,0,0);}
static void local(wf_r18_post_tag_t*s,uint32_t owner,uint32_t ret){s->sub_return=ret;s->pc=owner;}
static int returned(wf_r18_post_tag_t*s){if(s->sub_return){s->pc=s->sub_return;s->sub_return=0;return WF_R18_POST_MORE;}return WF_R18_POST_RETURN;}
static void clear_b886(uint8_t*p){ww(p,0);ww(p+4,0);ww(p+0x22,0);ww(p+0x24,0);ww(p+2,0);ww(p+0x14,0);ww(p+0x16,0);}
static int choice_b9a8(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x){
 wf_r18_prematch_t*c=x->machine;uint8_t*p=object(s,c,0,0x18),*q,t[2];if(!p)return 0;
 dw(c,2,0x22);if(d(c,1)!=12u){dw(c,2,0x23);if(d(c,1)!=16u){
  ww(p+0x16,0xf);dw(c,0,d(c,1));c->a[1]=0x1c1134;dw(c,0,(uint16_t)(d(c,0)-9u));
  if(d(c,1)>=13u){c->a[1]=0x1c1164;dw(c,0,(uint16_t)(d(c,0)-4u));}
  q=object(s,c,1,6);if(!q)return 0;dw(c,2,rw(q+4));c->regs->d[2]=(uint32_t)d(c,2)*3u;dw(c,2,(uint16_t)(d(c,2)+d(c,0)));c->a[2]=0xba24;
  if(!readrom(s,x,c->a[2]+(int32_t)(int16_t)d(c,2),t,1))return 0;
  db(c,2,t[0]);
  dw(c,2,d(c,2)&255u);
  if(!d(c,2))dw(c,2,0xffff);
 }}
 if(rw(c->ram+0x142a)==0x1eu){c->a[2]=0x1c1450;c->regs->d[0]=4;do{dw(c,2,0xffff);c->a[2]+=0x2a;dw(c,0,(uint16_t)(d(c,0)-1u));}while(d(c,0)!=0xffffu);}
 return 1;
}
static int width_baac(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x,uint8_t*p){
 wf_r18_prematch_t*c=x->machine;uint8_t t[2];dw(c,2,rw(p+0x14));dw(c,3,rw(p+4));
 if(d(c,3)==0xffffu)dw(c,3,0xff80);else{dw(c,3,(uint16_t)((d(c,3)-0xeu)<<1));if(!readrom(s,x,c->a[2]+(int32_t)(int16_t)d(c,3),t,2))return 0;dw(c,3,rw(t));}
 dw(c,2,(uint16_t)(d(c,2)+d(c,3)));return 1;
}
static int scroll_ba48(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x){
 wf_r18_prematch_t*c=x->machine;uint8_t*p=object(s,c,0,0x26),*q;if(!p)return 0;
 if(p[0]&64u)return 1;
 ww(p+0x14,(uint16_t)(rw(p+0x14)-2u));
 if(d(c,0)!=16u){c->a[2]=0xbaac;c->a[1]=c->a[0]+0x2a;q=object(s,c,1,0x16);if(!q)return 0;
  if(!(q[0]&128u)){if(!width_baac(s,x,p))return 0;if(d(c,2)<0x140u){q[0]|=128u;ww(q+0x14,0x1bf);}}
 }
 if(p[0]&128u){c->a[2]=0xbaac;if(!width_baac(s,x,p))return 0;if(d(c,2)&0x8000u)ww(p,0x4000);}
 return 1;
}
/* Original C404/055A: tests use D0.B except the first full-WORD zero test.
 * Carry is explicit because prematch's data/address ABI has no CCR slot. */
static int credit(wf_r18_prematch_t*c,int spend){
 uint8_t*r=c->ram;uint16_t arg=d(c,0);unsigned low=(uint8_t)arg;
 if(!arg){if(!r[0x4f])return 0;if(spend)r[0x4f]--;return 1;}
 if(low==1u||low==2u||low==5u){
  dw(c,0,low==1u?4u:low==2u?3u:2u);
  if(r[0x67]&(1u<<d(c,0))){if(!r[0x4f])return 0;if(spend)r[0x4f]--;return 1;}
  if(!spend)return r[0x4e]||r[0x4f];
  if(r[0x4e]){r[0x4e]--;return 1;}if(!r[0x4f])return 0;r[0x4f]--;r[0x4e]=(uint8_t)(r[0x4b]-1u);return 1;
 }
 if(low==3u){
  if(!(r[0x67]&16u)){dw(c,0,rw(r+0x66));dw(c,0,d(c,0)&3u);
   if(d(c,0)){
    if(!spend)return r[0x4e]>=2u||r[0x4f];
    if(!r[0x4f]&&r[0x4e]<2u)return 0;
    if(r[0x4e]>=2u){r[0x4e]-=2;return 1;}
    r[0x4f]--;db(c,0,r[0x4e]);db(c,0,(uint8_t)((uint8_t)d(c,0)+r[0x4b]));r[0x4e]=(uint8_t)((uint8_t)d(c,0)-2u);return 1;
   }
  }
  if(r[0x4f]<2u)return 0;
  if(spend)r[0x4f]-=2;
  return 1;
 }
 if(spend&&low==4u){if(r[0x4f]<2u)return 0;r[0x4f]-=2;return 1;}
 return 0;
}
void wf_r18_post_tag_init(wf_r18_post_tag_t*s,uint32_t pc){if(s){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}}
int wf_r18_post_tag_complete(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x,wf_status_t status,uint16_t value){
 if(!s||!valid(x)||!s->pending||s->fatal)return WF_R18_POST_BAD_CONTEXT;
 wf_r18_prematch_t*c=x->machine;if(status!=WF_OK)return block(s,c,c->dependency?c->dependency:s->request.owner);
 s->pending=0;c->dependency=0;if(s->request.kind==WF_R18_POST_SYNC&&!(c->ram[0x6f]&2u))return WF_R18_POST_WAIT;
 if(s->request.kind==WF_R18_POST_READ16||s->request.kind==WF_R18_POST_READ8)s->request.value=value;
 s->pc=s->request.return_pc;return WF_R18_POST_MORE;
}
int wf_r18_post_tag_step(wf_r18_post_tag_t*s,wf_r18_post_tag_context_t*x){
 if(!s||!valid(x))return WF_R18_POST_BAD_CONTEXT;
 wf_r18_prematch_t*c=x->machine;if(s->fatal)return WF_R18_POST_BLOCKED;if(s->pending)return WF_R18_POST_REQUEST;
 c->dependency=0;uint8_t*r=c->ram,*p,*q,t[8];unsigned i;uint16_t v;
 switch(s->pc){
 case 0x55a:s->carry=(uint8_t)credit(c,1);s->pc=0x652;break;
 case 0xc404:s->carry=(uint8_t)credit(c,0);s->pc=0xc496;break;
 case 0x652:case 0xc496:return WF_R18_POST_RETURN;
 case 0x11b6:memset(r+0x92,0,16);s->pc=0x11ce;break;
 case 0x11ce:dw(c,0,0x20);s->pc=0x11d2;break;
 case 0x11d2:return call(s,0x21e6,0x11d8);
 case 0x11d8:dw(c,0,0x8050);s->pc=0x11dc;break;
 case 0x11dc:return call(s,0x2503c,0x11e2);
 case 0x11e2:dw(c,0,0x20);s->pc=0x11e6;break;
 case 0x11e6:return call(s,0x21e6,0x11ec);
 case 0x11ec:dw(c,0,0x51);c->a[0]=0x1c05b0;dw(c,1,3);s->pc=0x11fa;break;
 case 0x11fa:p=object(s,c,0,0xa1);if(!p)return WF_R18_POST_BLOCKED;
  if(p[0xa0]&64u)s->pc=r[0x163]==9u?0x1222u:0x121cu;
  else{c->a[0]+=0x10cu;dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu){dw(c,0,0x50);s->pc=0x121c;}}
  break;
 case 0x121c:return call(s,0x2503c,0x1222);
 case 0x1222:dw(c,0,0x40);s->pc=0x1226;break;
 case 0x1226:return call(s,0x21e6,0x122c);
 case 0x122c:ww(r+0x16c,0);ww(r+0x8c,0);dw(c,0,0x40);s->pc=0x123c;break;
 case 0x123c:return call(s,0x21e6,0x1240);
 case 0x1240:s->pc=(r[0x650]&128u)||(r[0x868]&128u)?0x1256u:0x1396u;break;
 case 0x1256:return call(s,0x206fe,0x125c);
 case 0x125c:return call(s,0x1f6c,0x1260);
 case 0x1260:return call(s,0x1fc0,0x1264);
 case 0x1264:dw(c,0,0x3107);s->pc=0x1268;break;
 case 0x1268:return call(s,0x2052,0x126e);
 case 0x126e:ww(r+0x1804,0x140);ww(r+0x1806,0x600);ww(r+0x7e,3);s->pc=0x1286;break;
 case 0x1286:return call(s,0x26e66,0x128c);
 case 0x128c:ww(r+0x15f4,rw(r+0x7e));ww(r+0x15f8,rw(r+0x7e));ww(r+0x15fc,2);s->pc=0x12a8;break;
 case 0x12a8:return call(s,0x2a06,0x12ac);
 case 0x12ac:return call(s,0x1fde,0x12b0);
 case 0x12b0:return call(s,0x1f9e,0x12b4);
 case 0x12b4:
  if(!(r[0x66]&8u)){r[0x160]=0;if((r[0x5b0]&128u)&&(r[0x7c8]&128u))r[0x160]|=64u;}
  ww(r+0x168,0);ww(r+0x16a,0);c->a[0]=0x1c05b0;c->a[1]=0x1c06bc;dw(c,0,1);s->pc=0x12f6;break;
 case 0x12f6:p=object(s,c,0,0xa2);q=object(s,c,1,0xa2);if(!p||!q)return WF_R18_POST_BLOCKED;
  dw(c,1,(uint16_t)(rw(p+0xa0)&1u));dw(c,2,(uint16_t)((rw(q+0xa0)&1u)<<1));dw(c,1,(uint16_t)(d(c,1)+d(c,2)));
  s->saved_d[d(c,0)==1u?0u:1u]=c->regs->d[1];c->a[0]+=0x218u;c->a[1]+=0x218u;dw(c,0,(uint16_t)(d(c,0)-1u));
  if(d(c,0)==0xffffu){c->regs->d[1]=s->saved_d[1];ww(r+0x16a,d(c,1));c->regs->d[1]=s->saved_d[0];ww(r+0x168,d(c,1));s->pc=0x1332;}break;
 case 0x1332:s->pc=rw(r+0x168)&&(r[0x650]&128u)?0x1344u:0x135cu;break;
 case 0x1344:return call(s,0x15f4,0x134a);
 case 0x134a:c->a[1]=0x1c05b0;dw(c,7,rw(r+0x168));s->pc=0x1356;break;
 case 0x1356:return call(s,0x139c,0x135c);
 case 0x135c:s->pc=rw(r+0x16a)&&(r[0x868]&128u)?0x136eu:0x1382u;break;
 case 0x136e:return call(s,0x15f4,0x1372);
 case 0x1372:c->a[1]=0x1c07c8;dw(c,7,rw(r+0x16a));s->pc=0x137e;break;
 case 0x137e:return call(s,0x139c,0x1382);
 case 0x1382:return call(s,0x1fc0,0x1388);
 case 0x1388:ww(r+0x15fc,1);s->pc=0x1390;break;
 case 0x1390:return call(s,0x2a06,0x1396);
 case 0x1396:s->pc=0x19ba;break;
 case 0x139c:dw(c,0,0x8018);s->pc=0x13a0;break;
 case 0x13a0:return call(s,0x2503c,0x13a6);
 case 0x13a6:dw(c,0,0x801d);s->pc=0x13aa;break;
 case 0x13aa:return call(s,0x2503c,0x13b0);
 case 0x13b0:dw(c,0,0x801e);s->pc=0x13b4;break;
 case 0x13b4:return call(s,0x2503c,0x13ba);
 case 0x13ba:
  if(r[0x66]&8u){dw(c,0,c->a[1]==0x1c05b0u?0x16u:0x17u);if(r[0x160]&128u)dw(c,0,0x18);}
  else{dw(c,0,(uint16_t)(((d(c,7)&15u)-1u)+0x16u));if(c->a[1]==0x1c07c8u)dw(c,0,(uint16_t)(d(c,0)+3u));}
  s->pc=0x13fe;break;
 case 0x13fe:return call(s,0x2503c,0x1404);
 case 0x1404:ww(r+0x80,9);ww(r+0x82,0);dw(c,2,0x14);dw(c,7,0xf0);c->a[2]=0xc1118;s->pc=0x141c;break;
 case 0x141c:return call(s,0x9aca,0x1422);
 case 0x1422:dw(c,0,(uint16_t)(rw(r+0x80)|0x8000u));s->pc=0x142c;break;
 case 0x142c:return call(s,0x2067c,0x1432);
 case 0x1432:ww(r+0x82,(uint16_t)(rw(r+0x82)+1u));if(rw(r+0x82)>=0x40u){ww(r+0x82,0);ww(r+0x80,(uint16_t)(rw(r+0x80)-1u));}dw(c,0,0x1e);s->pc=0x1456;break;
 case 0x1456:return call(s,0x2aea,0x145c);
 case 0x145c:c->a[0]=0x1c14ce;dw(c,0,1);s->saved_a[0]=c->a[1];s->pc=0x146a;break;
 case 0x146a:p=object(s,c,0,0x26);q=object(s,c,1,4);if(!p||!q)return WF_R18_POST_BLOCKED;s->saved_d0=c->regs->d[0];v=p[1]&1u;p[1]|=1u;
  if(!v){p[0]=128u;ww(p+0x14,c->a[0]==0x1c14f8u?0xe8u:0x58u);ww(p+0x16,0x88);ww(p+0x22,8);ww(p+0x24,0);ww(p+2,(uint16_t)(rw(q+2)+0x30u));dw(c,1,(uint16_t)(rw(p+2)-0x20u));dw(c,0,d(c,1));s->pc=0x14b6;}
  else s->pc=0x14ba;
  break;
 case 0x14b6:return call(s,0x2aea,0x14ba);
 case 0x14ba:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;ww(p+4,rw(p+0x24));ww(p+0x22,(uint16_t)(rw(p+0x22)-1u));s->pc=rw(p+0x22)?0x14e4u:0x14ceu;break;
 case 0x14ce:{uint32_t seed=((uint32_t)rw(r+0x5c)<<16)|rw(r+0x5e),frame=((uint32_t)rw(r+0x80)<<16)|rw(r+0x82);wf_r17_rng21b4_live(&seed,frame,c->regs);ww(r+0x5c,(uint16_t)(seed>>16));ww(r+0x5e,(uint16_t)seed);s->pc=0x14d2;break;}
 case 0x14d2:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;dw(c,0,(uint16_t)((d(c,0)&15u)+8u));ww(p+0x22,d(c,0));ww(p+0x24,(uint16_t)(rw(p+0x24)^1u));s->pc=0x14e4;break;
 case 0x14e4:return call(s,0x27b8,0x14e8);
 case 0x14e8:c->regs->d[0]=s->saved_d0;c->a[0]+=0x2a;c->a[1]+=0x10c;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0x146au:0x14fcu;break;
 case 0x14fc:return call(s,0x2836,0x1500);
 case 0x1500:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0x150e;break;
 case 0x150e:case 0x1742:case 0x1864:
  v=s->pc==0x150eu?0x1518u:s->pc==0x1742u?0x174cu:0x186eu;
  if(r[0x6f]&2u){s->pc=v;break;}return request(s,3,s->pc,v,0x1c006f,2);
 case 0x1518:c->a[1]=s->saved_a[0];dw(c,1,0x1c);dw(c,0,1);dw(c,7,rw(r+(c->a[1]==0x1c05b0u?0x168u:0x16au)));if(d(c,7)==3u)dw(c,0,3);s->carry=(uint8_t)credit(c,0);s->pc=0x1548;break;
 case 0x1548:if(s->carry)dw(c,1,d(c,1)|0x8000u);dw(c,0,d(c,1));s->pc=0x1550;break;
 case 0x1550:return call(s,0x2503c,0x1556);
 case 0x1556:local(s,0x160e,0x155c);break;
 case 0x155c:if(s->carry){s->pc=0x15ce;break;}if(!(r[0x80]&128u)){s->pc=0x1422;break;}c->a[2]=c->a[1]+0x10c;c->a[3]=0x1c0092;dw(c,0,1);s->pc=0x157e;break;
 case 0x157e:p=object(s,c,3,8);if(!p)return WF_R18_POST_BLOCKED;
  if(!rw(p)){q=object(s,c,1,0xa2);if(!q)return WF_R18_POST_BLOCKED;if(q[0xa1]&1u){ww(p,(uint16_t)(c->a[1]>>16));ww(p+2,(uint16_t)c->a[1]);}q=object(s,c,2,0xa2);if(!q)return WF_R18_POST_BLOCKED;if(q[0xa1]&1u){ww(p+4,(uint16_t)(c->a[2]>>16));ww(p+6,(uint16_t)c->a[2]);}s->pc=0x15a8;}
  else{c->a[3]+=8;dw(c,0,(uint16_t)(d(c,0)-1u));if(d(c,0)==0xffffu)s->pc=0x15a8;}break;
 case 0x15a8:p=object(s,c,1,0x10c);q=object(s,c,2,0x10c);if(!p||!q)return WF_R18_POST_BLOCKED;dw(c,0,0x10b);dw(c,1,rw(p+0x56));dw(c,2,rw(q+0x56));memset(p,0,0x10c);memset(q,0,0x10c);dw(c,0,0xffff);ww(p+0x56,d(c,1));ww(q+0x56,d(c,2));s->pc=0x15cc;break;
 case 0x15cc:return WF_R18_POST_RETURN;
 case 0x15ce:ww(r+0x16c,0x8000);ww(r+0x8c,0x8000);c->a[2]=c->a[1]+0x10c;p=object(s,c,1,0x106);q=object(s,c,2,0x106);if(!p||!q)return WF_R18_POST_BLOCKED;ww(p+0x104,0x8000);ww(q+0x104,0x8000);s->pc=0x15f2;break;
 case 0x15f2:return WF_R18_POST_RETURN;
 case 0x160e:if(r[0x66]&8u){s->pc=0x175c;break;}dw(c,5,c->a[1]==0x1c05b0u?0u:2u);dw(c,7,rw(r+(c->a[1]==0x1c05b0u?0x168u:0x16au)));dw(c,0,(uint16_t)(0x1du+d(c,5)));dw(c,6,d(c,7)&1u);s->pc=d(c,6)?0x1642u:0x1676u;break;
 case 0x1642:s->saved_d0=c->regs->d[0];dw(c,0,d(c,7)==3u?3u:1u);s->carry=(uint8_t)credit(c,0);c->regs->d[0]=s->saved_d0;if(!s->carry)dw(c,0,d(c,0)|0x8000u);s->pc=s->carry?0x1670u:0x1664u;break;
 case 0x1664:return call(s,0x2503c,0x166a);
 case 0x166a:s->pc=0x1676;break;
 case 0x1670:return call(s,0x2503c,0x1676);
 case 0x1676:dw(c,6,d(c,7)&2u);if(!d(c,6)){s->pc=0x16b8;break;}dw(c,0,(uint16_t)(0x1eu+d(c,5)));s->saved_d0=c->regs->d[0];dw(c,0,d(c,7)==3u?3u:1u);s->carry=(uint8_t)credit(c,0);c->regs->d[0]=s->saved_d0;if(!s->carry)dw(c,0,d(c,0)|0x8000u);s->pc=s->carry?0x16b2u:0x16a6u;break;
 case 0x16a6:return call(s,0x2503c,0x16ac);
 case 0x16ac:s->pc=0x16b8;break;
 case 0x16b2:return call(s,0x2503c,0x16b8);
 case 0x16b8:dw(c,6,(uint16_t)(d(c,7)<<3));c->a[5]=0x1884+(c->a[1]==0x1c05b0u?0u:0x20u);
  if(!readrom(s,x,c->a[5]+(int32_t)(int16_t)d(c,6),t,4))return WF_R18_POST_BLOCKED;
  c->a[4]=((uint32_t)rw(t)<<16)|rw(t+2);
  s->pc=0x16d4;
  break;
 case 0x16d4:return request(s,1,0x16d4,0x16d8,c->a[4],0);
 case 0x16d8:dw(c,5,s->request.value&128u);if(!d(c,5)){s->pc=0x16f2;break;}
  if(!readrom(s,x,c->a[5]+4u+(int32_t)(int16_t)d(c,6),t,4))return WF_R18_POST_BLOCKED;
  c->a[4]=((uint32_t)rw(t)<<16)|rw(t+2);
  s->pc=0x16e2;
  break;
 case 0x16e2:return request(s,1,0x16e2,0x16e6,c->a[4],0);
 case 0x16e6:dw(c,5,s->request.value&128u);if(d(c,5)){s->carry=0;s->pc=0x175a;}else s->pc=0x16f2;break;
 case 0x16f2:dw(c,0,(uint8_t)d(c,7)==3u?3u:1u);s->carry=(uint8_t)credit(c,1);s->pc=s->carry?0x1708u:0x175au;break;
 case 0x1708:ww(r+0x14d2,2);ww(r+0x14fc,2);c->a[0]=0x1c14ce;s->pc=0x171e;break;
 case 0x171e:return call(s,0x27b8,0x1724);
 case 0x1724:c->a[0]=0x1c14f8;s->pc=0x172a;break;
 case 0x172a:return call(s,0x27b8,0x1730);
 case 0x1730:return call(s,0x2836,0x1734);
 case 0x1734:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0x1742;break;
 case 0x174c:dw(c,0,0x40);s->pc=0x1750;break;
 case 0x1750:return call(s,0x21e6,0x1756);
 case 0x1756:s->carry=1;s->pc=0x175a;break;
 case 0x175a:return returned(s);
 case 0x175c:dw(c,1,0x66);if(!(r[0x160]&128u))dw(c,1,c->a[1]==0x1c05b0u?0x61u:0x62u);dw(c,0,r[0x160]&128u?3u:1u);s->carry=(uint8_t)credit(c,0);if(!s->carry)dw(c,1,d(c,1)|0x8000u);dw(c,0,d(c,1));s->pc=0x17a6;break;
 case 0x17a6:return call(s,0x2503c,0x17ac);
 case 0x17ac:s->pc=r[0x160]&128u?(c->a[1]==0x1c05b0u?0x17beu:0x17d6u):(c->a[1]==0x1c05b0u?0x1804u:0x1812u);break;
 case 0x17be:case 0x17d6:case 0x1804:return request(s,4,s->pc,s->pc==0x17beu?0x17c6u:s->pc==0x17d6u?0x17deu:0x180cu,0x140021,0);
 case 0x17c8:case 0x17e0:case 0x1812:return request(s,4,s->pc,s->pc==0x17c8u?0x17d0u:s->pc==0x17e0u?0x17e8u:0x181au,0x140023,0);
 case 0x17c6:s->pc=s->request.value&128u?0x17c8u:0x17ecu;break;
 case 0x17de:s->pc=s->request.value&64u?0x17e0u:0x17ecu;break;
 case 0x17d0:case 0x17e8:s->pc=s->request.value&128u?0x187eu:0x17ecu;break;
 case 0x180c:case 0x181a:s->pc=s->request.value&128u?0x187eu:0x181eu;break;
 case 0x17ec:dw(c,0,3);s->carry=(uint8_t)credit(c,1);s->pc=s->carry?0x182au:0x187eu;break;
 case 0x181e:dw(c,0,1);s->carry=(uint8_t)credit(c,1);s->pc=s->carry?0x182au:0x187eu;break;
 case 0x182a:ww(r+0x14d2,2);ww(r+0x14fc,2);c->a[0]=0x1c14ce;s->pc=0x1840;break;
 case 0x1840:return call(s,0x27b8,0x1846);
 case 0x1846:c->a[0]=0x1c14f8;s->pc=0x184c;break;
 case 0x184c:return call(s,0x27b8,0x1852);
 case 0x1852:return call(s,0x2836,0x1856);
 case 0x1856:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0x1864;break;
 case 0x186e:dw(c,0,0x40);s->pc=0x1872;break;
 case 0x1872:return call(s,0x21e6,0x1878);
 case 0x1878:s->carry=1;s->pc=0x187c;break;
 case 0x187c:case 0x1882:return returned(s);
 case 0x187e:s->carry=0;s->pc=0x1882;break;
 case 0x15f4:c->a[4]=0x1c14ce;c->a[5]=0x1c14f8;dw(c,0,0x29);memset(r+0x14ce,0,0x2a);memset(r+0x14f8,0,0x2a);c->a[4]+=0x2a;c->a[5]+=0x2a;dw(c,0,0xffff);s->pc=0x160c;break;
 case 0x160c:return WF_R18_POST_RETURN;
 case 0x19ba:return call(s,0x8eb6,0x19c0);
 case 0x19c0:c->a[0]=0x1c05b0;dw(c,0,3);s->pc=0x19ca;break;
 case 0x19ca:p=object(s,c,0,0x106);if(!p)return WF_R18_POST_BLOCKED;
  if((p[0xa0]&64u)||rw(p+0x104))s->pc=0x19ee;
  else{c->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)-1u));if(d(c,0)==0xffffu)s->pc=0x19e2;}break;
 case 0x19e2:return call(s,0x65c0,0x19e8);
 case 0x19e8:s->pc=0x6fc;break;
 /* Royal GAME OVER continues at $1B1E after the result display. The ROM
  * appends enabled participants without clearing the unused LONG slots. */
 case 0x1b1e:c->a[0]=0x1c0092u;c->a[1]=0x1c05b0u;dw(c,0,3u);s->pc=0x1b2e;break;
 case 0x1b2e:
  p=object(s,c,1,1);if(!p)return WF_R18_POST_BLOCKED;
  if(p[0]&0x80u){
   q=object(s,c,0,4);if(!q)return WF_R18_POST_BLOCKED;
   ww(q,(uint16_t)(c->a[1]>>16));ww(q+2,(uint16_t)c->a[1]);c->a[0]+=4u;
  }
  c->a[1]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)-1u));
  if(d(c,0)==0xffffu)s->pc=0x1b42;
  break;
 case 0x1b42:return call(s,0x65c0,0x1b48);
 case 0x1b48:s->pc=0x6fc;break;
 case 0x19ee:
  if(r[0x160]&64u){
   if(rw(r+0x6b4)&&rw(r+0x8cc)){s->pc=0x1af0;break;}
   if(r[0x650]&64u){if(rw(r+0x8cc)){s->pc=0x1af0;break;}}
   else if(r[0x868]&64u){if(rw(r+0x6b4)){s->pc=0x1af0;break;}}
   ww(r+0x8c,0);s->pc=0x1a5e;
  }else s->pc=rw(r+0x6b4)||rw(r+0x8cc)?0x1af0u:0x1a5eu;
  break;
 case 0x1a5e:s->pc=r[0x163]==9u?0x1b4eu:0x1a6au;break;
 case 0x1a6a:return call(s,0xb608,0x1a70);
 case 0x1a70:return call(s,0xbda6,0x1a76);
 case 0x1a76:r[0x163]++;r[0x165]++;if((r[0x66]&128u)&&(r[0x163]==3u||r[0x163]==8u))r[0x163]++;
  if(r[0x163]!=5u){s->pc=0x1af0;break;}dw(c,0,9);c->a[4]=0x1c0598;
  for(i=0;i<10;i++){ww(r+0x598+i*2u,0xffff);c->a[4]+=2;dw(c,0,(uint16_t)(d(c,0)-1u));}
  c->a[4]=0x1c0598;c->a[0]=0x1c05b0;dw(c,0,3);s->pc=0x1ad8;break;
 case 0x1ad8:p=object(s,c,0,0x58);q=object(s,c,4,2);if(!p||!q)return WF_R18_POST_BLOCKED;
  if(p[0]&128u){q[0]=0;q[1]=p[0x57];c->a[4]+=2;}c->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)-1u));if(d(c,0)==0xffffu)s->pc=0x1af0;break;
 case 0x1af0:s->pc=0xac0;break;
 case 0xac0:return WF_R18_POST_AC0;
 case 0x6fc:case 0x1b4e:return WF_R18_POST_BOUNDARY;

 /* Original AE20: title-match presenter0 and interview setup3. */
 case 0xae20:s->pc=d(c,0)==3u?0xae30u:0xae26u;break;
 case 0xae26:s->pc=rw(r+0x8c)?0xb0c0u:0xae30u;break;
 case 0xae30:if(d(c,0)>3u)return block(s,c,0xae20);memcpy(s->saved_d,c->regs->d,sizeof s->saved_d);memcpy(s->saved_a,c->a,sizeof s->saved_a);s->pc=0xae34;break;
 case 0xae34:ww(r+0x1278,d(c,0));s->pc=0xae3a;break;
 case 0xae3a:return call(s,0x2988,0xae40);
 case 0xae40:return call(s,0x1fc0,0xae46);
 case 0xae46:ww(r+0x1804,0x280);ww(r+0x1806,0x200);ww(r+0x7e,4);s->pc=0xae5e;break;
 case 0xae5e:return call(s,0x26e66,0xae64);
 case 0xae64:return request(s,2,0xae64,0xae6c,0x140010,0x7b);
 case 0xae6c:ww(r+0x15f4,rw(r+0x7e));ww(r+0x15f8,rw(r+0x7e));ww(r+0x15fc,0);s->pc=0xae88;break;
 case 0xae88:return call(s,0x2a06,0xae8e);
 case 0xae8e:c->a[0]=0x1c1258;c->a[1]=0xb0c2;dw(c,1,rw(r+0x1278));if(d(c,1)>=3u)c->regs->d[1]=0;
  if(!readrom(s,x,c->a[1]+3u+d(c,1),t,1))return WF_R18_POST_BLOCKED;
  r[0x1287]=t[0];
  if(rw(r+0x1278)==3u)ww(r+0x1286,0xffff);
  c->regs->d[1]=0;
  s->pc=0xaec4;
  break;
 case 0xaec4:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;
  p[0]|=128u;ww(p+0x24,0);ww(p+0x22,0);ww(p+0x1c,0);ww(p+0x1e,0);
  if(!readrom(s,x,c->a[1]+d(c,1),t,1))return WF_R18_POST_BLOCKED;
  p[0x15]=t[0];
  ww(p+0x14,(uint16_t)(rw(p+0x14)&255u));
  ww(p+0x16,0x77);
  p[3]=0x2c;
  ww(r+0x125c,0x16);ww(r+0x12b0,3);s->pc=0xaf02;break;
 case 0xaf02:return call(s,0x27b8,0xaf08);
 case 0xaf08:dw(c,0,d(c,1));dw(c,0,(uint16_t)(d(c,0)+0x31));s->pc=0xaf0e;break;
 case 0xaf0e:return call(s,0x2aea,0xaf14);
 case 0xaf14:c->a[0]+=0x2a;dw(c,1,(uint16_t)(d(c,1)+1u));s->pc=d(c,1)!=3u?0xaec4u:0xaf22u;break;
 case 0xaf22:s->pc=rw(r+0x1278)==3u?0xaf7cu:0xaf2cu;break;
 case 0xaf2c:return call(s,0x2836,0xaf32);
 case 0xaf32:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0xaf40;break;
 case 0xaf40:case 0xb068:
  if(r[0x6f]&2u){s->pc=s->pc==0xaf40u?0xaf4au:0xb072u;break;}
  return request(s,3,s->pc,s->pc==0xaf40u?0xaf4au:0xb072u,0x1c006f,2);
 case 0xaf4a:return call(s,0x26772,0xaf50);
 case 0xaf50:return call(s,0x1f9e,0xaf56);
 case 0xaf56:s->pc=rw(r+0x1278)?0xaf76u:0xaf5eu;break;
 case 0xaf5e:return call(s,0xb29e,0xaf62);
 case 0xaf62:dw(c,0,0x27);s->pc=0xaf66;break;
 case 0xaf66:return call(s,0x2503c,0xaf6c);
 case 0xaf6c:dw(c,0,0x28);s->pc=0xaf70;break;
 case 0xaf70:return call(s,0x2503c,0xaf76);
 case 0xaf76:return call(s,0x26844,0xaf7c);
 case 0xaf7c:s->pc=rw(r+0x1278)>=3u?0xb0b6u:0xaf88u;break;
 case 0xaf88:if(rw(r+0x1278)){s->pc=0xafa4;break;}dw(c,0,0x3100);s->pc=0xaf94;break;
 case 0xaf94:return call(s,0x2052,0xaf9a);
 case 0xaf9a:dw(c,0,0x3110);s->pc=0xaf9e;break;
 case 0xaf9e:return call(s,0x2052,0xafa4);
 case 0xafa4:s->pc=rw(r+0x1278)==2u?0xafd4u:rw(r+0x1278)==1u?0xafc2u:0xafb8u;break;
 case 0xafb8:r[0x129f]|=1u;s->pc=0xafe4;break;
 case 0xafc2:ww(r+0x1276,2);r[0x12c9]|=1u;s->pc=0xafe4;break;
 case 0xafd4:ww(r+0x1276,4);r[0x1275]|=1u;s->pc=0xafe4;break;
 case 0xafe4:c->a[0]=0x1c1258;c->regs->d[0]=0;s->pc=0xafec;break;
 case 0xafec:local(s,0xb0c8,0xaff0);break;
 case 0xaff0:return call(s,0x27b8,0xaff6);
 case 0xaff6:s->pc=rw(r+0x1278)==1u?0xb034u:0xb000u;break;
 case 0xb000:s->pc=rw(r+0x1278)==2u?0xb046u:0xb00au;break;
 case 0xb00a:s->pc=rw(r+0x12a6)==0x1bu?0xb014u:0xb020u;break;
 case 0xb014:r[0x12c9]|=1u;s->pc=0xb01c;break;
 case 0xb01c:return call(s,0xb2e4,0xb020);
 case 0xb020:s->pc=rw(r+0x12a6)==0x2eu?0xb02au:0xb046u;break;
 case 0xb02a:r[0x1275]|=1u;s->pc=0xb046;break;
 case 0xb034:if(rw(r+0x12d0)==0x1cu)ww(r+0x12d0,0x1e);s->pc=0xb046;break;
 case 0xb046:c->a[0]+=0x2a;dw(c,0,(uint16_t)(d(c,0)+1u));s->pc=d(c,0)!=3u?0xafecu:0xb054u;break;
 case 0xb054:return call(s,0x2836,0xb05a);
 case 0xb05a:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0xb068;break;
 case 0xb072:s->pc=rw(r+0x1278)==1u?0xb0a0u:0xb07cu;break;
 case 0xb07c:s->pc=rw(r+0x1278)==2u?0xb094u:0xb086u;break;
 case 0xb086:s->pc=rw(r+0x127c)!=0x1cu?0xafe4u:0xb0acu;break;
 case 0xb094:s->pc=rw(r+0x1274)?0xafe4u:0xb0acu;break;
 case 0xb0a0:s->pc=rw(r+0x12d0)!=0x1fu?0xafe4u:0xb0acu;break;
 case 0xb0ac:return call(s,0x264e2,0xb0b2);
 case 0xb0b2:local(s,0xb152,0xb0b6);break;
 case 0xb0b6:ww(r+0x1278,0);memcpy(c->regs->d,s->saved_d,sizeof s->saved_d);memcpy(c->a,s->saved_a,sizeof s->saved_a);s->pc=0xb0c0;break;
 case 0xb0c0:return WF_R18_POST_RETURN;

 /* B0C8 is the original compact presenter callback; data remains in ROM. */
 case 0xb0c8:p=object(s,c,0,0x1e);if(!p)return WF_R18_POST_BLOCKED;
  if(!(p[0x1d]&1u)){s->pc=0xb148;break;}s->pc=0xb0d2;break;
 case 0xb0d2:if(c->a[0]&1u)return block(s,c,0xb0d2);p=object(s,c,0,0x24);if(!p)return WF_R18_POST_BLOCKED;
  ww(p+0x22,(uint16_t)(rw(p+0x22)-1u));s->pc=(rw(p+0x22)&0x8000u)?0xb0dcu:0xb148u;break;
 case 0xb0dc:c->a[1]=0xb19c;dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)<<2));s->pc=0xb0e6;break;
 case 0xb0e6:if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,1),t,4))return WF_R18_POST_BLOCKED;
  c->a[1]=((uint32_t)rw(t)<<16)|rw(t+2);s->pc=0xb0ea;break;
 case 0xb0ea:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;dw(c,1,rw(p+0x24));dw(c,1,(uint16_t)(d(c,1)<<1));s->pc=0xb0f0;break;
 case 0xb0f0:p=object(s,c,0,6);if(!p)return WF_R18_POST_BLOCKED;
  if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,1),t,1))return WF_R18_POST_BLOCKED;
  p[5]=t[0];s->pc=(p[5]&128u)?0xb0feu:0xb124u;break;
 case 0xb0fe:s->saved_d0=c->regs->d[0];s->callback_a0=c->a[0];c->a[0]=0xb14a;dw(c,0,rw(r+0x1276));s->pc=0xb10e;break;
 case 0xb10e:if(!readrom(s,x,c->a[0]+(int32_t)(int16_t)d(c,0),t,2))return WF_R18_POST_BLOCKED;
  dw(c,0,rw(t));ww(r+0x1276,(uint16_t)(rw(r+0x1276)+2u));s->pc=0xb11a;break;
 case 0xb11a:return call(s,0x2052,0xb120);
 case 0xb120:c->regs->d[0]=s->saved_d0;c->a[0]=s->callback_a0;s->pc=0xb124;break;
 case 0xb124:p=object(s,c,0,6);if(!p)return WF_R18_POST_BLOCKED;ww(p+4,rw(p+4)&0x7fu);s->pc=0xb12a;break;
 case 0xb12a:p=object(s,c,0,0x24);if(!p)return WF_R18_POST_BLOCKED;
  if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,1)+1u,t,1))return WF_R18_POST_BLOCKED;
  p[0x23]=t[0];ww(p+0x22,rw(p+0x22)&255u);s->pc=0xb136;break;
 case 0xb136:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;
  ww(p+0x24,(uint16_t)(rw(p+0x24)+1u));if(p[0x23]==0xfeu)ww(p+0x1c,0);s->pc=0xb148;break;
 case 0xb148:return returned(s);
 case 0xb152:c->a[0]=0x1c1258;c->regs->d[0]=2;s->pc=0xb15a;break;
 case 0xb15a:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;
  ww(p,0);ww(p+0x1c,0);ww(p+0x1e,0);ww(p+4,0);ww(p+0x22,0);ww(p+0x24,0);ww(p+2,0);ww(p+0x14,0);ww(p+0x16,0);
  s->saved_d0=c->regs->d[0];dw(c,0,(uint16_t)(d(c,0)+0x31u));s->pc=0xb186;break;
 case 0xb186:return call(s,0x2b58,0xb18c);
 case 0xb18c:c->regs->d[0]=s->saved_d0;c->a[0]+=0x2a;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0xb15au:0xb19au;break;
 case 0xb19a:return returned(s);

 case 0xb608:memcpy(s->saved_d,c->regs->d,sizeof s->saved_d);memcpy(s->saved_a,c->a,3*sizeof(uint32_t));s->pc=!(r[0x161]&1u)&&rw(r+0x162)==4u?0xb624u:0xb80eu;break;
 case 0xb624:return call(s,0x2988,0xb62a);
 case 0xb62a:return call(s,0x1f6c,0xb630);
 case 0xb630:return call(s,0x1fc0,0xb636);
 case 0xb636:ww(r+0x1804,0x140);ww(r+0x1806,0x300);ww(r+0x7e,4);s->pc=0xb64e;break;
 case 0xb64e:return call(s,0x26e66,0xb654);
 case 0xb654:return request(s,2,0xb654,0xb65c,0x140010,0x7b);
 case 0xb65c:ww(r+0x15f4,rw(r+0x7e));ww(r+0x15f8,rw(r+0x7e));ww(r+0x15fc,0);s->pc=0xb678;break;
 case 0xb678:return call(s,0x2a06,0xb67e);
 case 0xb67e:c->a[0]=0x1c1134;c->a[1]=0x1c05b0;c->a[2]=0xb814;c->regs->d[1]=3;c->regs->d[4]=0;s->pc=0xb694;break;
 case 0xb694:p=object(s,c,1,0x58);if(!p)return WF_R18_POST_BLOCKED;
  if(!(p[0]&128u)){s->pc=0xb6fa;break;}q=object(s,c,0,0x18);if(!q)return WF_R18_POST_BLOCKED;ww(q,0);
  dw(c,2,p[0x57]==0xbu?10u:(uint16_t)(rw(p+0x56)&15u));ww(q+4,d(c,2));c->regs->d[2]=(uint32_t)d(c,2)*3u;dw(c,3,d(c,2));dw(c,3,(uint16_t)(d(c,3)+d(c,4)));
  if(!readrom(s,x,c->a[2]+(int32_t)(int16_t)d(c,3),t,1))return WF_R18_POST_BLOCKED;
  db(c,3,t[0]);
  dw(c,3,d(c,3)&255u);
  ww(q+0x14,d(c,3));
  ww(q+0x16,0x7e);
  ww(q+2,0x4d);
  ww(q+0x12,(uint16_t)(d(c,4)+1u));
  if(!readrom(s,x,c->a[2]+2u+(int32_t)(int16_t)d(c,2),t,1))return WF_R18_POST_BLOCKED;
  db(c,0,t[0]);
  dw(c,0,d(c,0)&255u);
  s->pc=0xb6ec;
  break;
 case 0xb6ec:return call(s,0x2aea,0xb6f2);
 case 0xb6f2:dw(c,4,(uint16_t)(d(c,4)+1u));c->a[0]+=0x30;s->pc=0xb6fa;break;
 case 0xb6fa:c->a[1]+=0x10c;dw(c,1,(uint16_t)(d(c,1)-1u));s->pc=d(c,1)!=0xffffu?0xb694u:0xb704u;break;
 case 0xb704:local(s,0xb94c,0xb708);break;
 case 0xb708:c->a[0]=0x1c1194;p=r+0x1194;p[0]|=128u;ww(p+2,0x4e);ww(p+0x14,0xf0);ww(p+0x16,0x6e);ww(p+0x22,0x40);ww(p+0x12,0);ww(p+4,0);ww(p+0x24,0);dw(c,0,0x37);s->pc=0xb73c;break;
 case 0xb73c:return call(s,0x2aea,0xb742);
 case 0xb742:return call(s,0x27b8,0xb748);
 case 0xb748:return call(s,0x2836,0xb74e);
 case 0xb74e:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0xb75c;break;
 case 0xb75c:case 0xb7f0:if(r[0x6f]&2u){s->pc=s->pc==0xb75cu?0xb766u:0xb7fau;break;}return request(s,3,s->pc,s->pc==0xb75cu?0xb766u:0xb7fau,0x1c006f,2);
 case 0xb766:return call(s,0x26772,0xb76c);
 case 0xb76c:return call(s,0x1f9e,0xb772);
 case 0xb772:return call(s,0x26844,0xb778);
 case 0xb778:dw(c,0,0x3100);s->pc=0xb77c;break;
 case 0xb77c:return call(s,0x2052,0xb782);
 case 0xb782:dw(c,0,0x3107);s->pc=0xb786;break;
 case 0xb786:return call(s,0x2052,0xb78c);
 case 0xb78c:c->regs->d[0]=0;c->a[0]=0x1c1134;s->pc=0xb794;break;
 case 0xb794:local(s,0xb8a4,0xb798);break;
 case 0xb798:p=object(s,c,0,1);if(!p)return WF_R18_POST_BLOCKED;s->pc=p[0]&128u?0xb7a0u:0xb7a6u;break;
 case 0xb7a0:return call(s,0x27b8,0xb7a6);
 case 0xb7a6:c->a[0]+=0x30;dw(c,0,(uint16_t)(d(c,0)+1u));s->pc=d(c,0)!=3u?0xb794u:0xb7b4u;break;
 case 0xb7b4:c->a[0]=0x1c1258;c->regs->d[0]=0;s->pc=0xb7bc;break;
 case 0xb7bc:p=object(s,c,0,1);if(!p)return WF_R18_POST_BLOCKED;if(p[0]&128u){if(!scroll_ba48(s,x))return WF_R18_POST_BLOCKED;s->pc=0xb7c8;}else s->pc=0xb7ce;break;
 case 0xb7c8:return call(s,0x27b8,0xb7ce);
 case 0xb7ce:c->a[0]+=0x2a;dw(c,0,(uint16_t)(d(c,0)+1u));s->pc=d(c,0)!=0x11u?0xb7bcu:0xb7dcu;break;
 case 0xb7dc:return call(s,0x2836,0xb7e2);
 case 0xb7e2:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0xb7f0;break;
 case 0xb7fa:s->pc=r[0x14f8]&64u?0xb804u:0xb78cu;break;
 case 0xb804:return call(s,0x264e2,0xb80a);
 case 0xb80a:local(s,0xb836,0xb80e);break;
 case 0xb80e:memcpy(c->regs->d,s->saved_d,sizeof s->saved_d);memcpy(c->a,s->saved_a,3*sizeof(uint32_t));s->pc=0xb812;break;
 case 0xb812:return WF_R18_POST_RETURN;
 case 0xb836:c->regs->d[1]=2;c->a[0]=0x1c1134;c->a[1]=0xb814;s->pc=0xb844;break;
 case 0xb844:p=object(s,c,0,6);if(!p)return WF_R18_POST_BLOCKED;dw(c,0,rw(p+4));c->regs->d[0]=(uint32_t)d(c,0)*3u;
  if(!readrom(s,x,c->a[1]+2u+(int32_t)(int16_t)d(c,0),t,1))return WF_R18_POST_BLOCKED;
  db(c,0,t[0]);
  s->pc=0xb850;
  break;
 case 0xb850:return call(s,0x2b58,0xb856);
 case 0xb856:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;clear_b886(p);c->a[0]+=0x30;dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu){dw(c,0,0x37);s->pc=0xb866;}else s->pc=0xb844;break;
 case 0xb866:return call(s,0x2b58,0xb86c);
 case 0xb86c:c->regs->d[1]=0x10;c->a[0]=0x1c1258;s->pc=0xb874;break;
 case 0xb874:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;clear_b886(p);dw(c,1,(uint16_t)(d(c,1)-1u));if(d(c,1)==0xffffu){dw(c,0,0x3c);s->pc=0xb87e;}break;
 case 0xb87e:return call(s,0x2b58,0xb884);
 case 0xb884:return returned(s);
 case 0xb8a4:if(d(c,0)!=2u){s->pc=0xb934;break;}p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;c->a[1]=0xb936;
  if(rw(p+0x22)){s->pc=0xb92e;break;}dw(c,1,rw(p+0x24));if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,1),t,1))return WF_R18_POST_BLOCKED;db(c,2,t[0]);dw(c,2,d(c,2)&255u);ww(p+4,d(c,2));ww(p+0x22,8);p[0x25]++;
  if(!(r[0x127a]&128u)){if(rw(p+0x24)==1u){s->pc=0xb8e8;break;}if(rw(p+0x24)==5u){r[0x1258]|=128u;r[0x127a]|=128u;}}
  s->pc=0xb918;break;
 case 0xb8e8:return request(s,2,0xb8e8,0xb8f0,0x140010,0x78);
 case 0xb8f0:r[0x1134]|=128u;r[0x1164]|=128u;s->pc=0xb918;break;
 case 0xb918:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;if(rw(p+4)==255u){ww(p+0x22,0);ww(p+0x24,3);s->pc=0xb8a4;}else s->pc=0xb92e;break;
 case 0xb92e:p=object(s,c,0,0x24);if(!p)return WF_R18_POST_BLOCKED;ww(p+0x22,(uint16_t)(rw(p+0x22)-1u));s->pc=0xb934;break;
 case 0xb934:return returned(s);
 case 0xb94c:dw(c,0,0x3c);s->pc=0xb950;break;
 case 0xb950:return call(s,0x2aea,0xb956);
 case 0xb956:c->regs->d[1]=0;c->a[0]=0x1c1258;s->pc=0xb95e;break;
 case 0xb95e:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;ww(p+2,0x4e);ww(p+0x16,0xc);ww(p+0x14,0x1bf);ww(p,0);ww(p+0x24,0);
  if(d(c,1)<9u){dw(c,2,d(c,1)<6u?0xeu:0x19u);dw(c,2,(uint16_t)(d(c,2)+d(c,1)));}else if(!choice_b9a8(s,x))return WF_R18_POST_BLOCKED;
  ww(p+4,d(c,2));c->a[0]+=0x2a;dw(c,1,(uint16_t)(d(c,1)+1u));s->pc=d(c,1)!=0x11u?0xb95eu:0xb9a6u;break;
 case 0xb9a6:return returned(s);
 case 0xba48:if(!scroll_ba48(s,x))return WF_R18_POST_BLOCKED;s->pc=0xbaaa;break;
 case 0xbaaa:return WF_R18_POST_RETURN;
 case 0xbda6:memcpy(s->saved_d,c->regs->d,3*sizeof(uint32_t));memcpy(s->saved_a,c->a,3*sizeof(uint32_t));s->pc=(r[0x161]&1u)?0xbf52u:0xbdb6u;break;
 case 0xbdb6:c->regs->d[0]=3;s->pc=0xbdb8;break;
 case 0xbdb8:return call(s,0xae20,0xbdbc);
 case 0xbdbc:ww(r+0x126c,0x60);ww(r+0x12c0,0xe0);dw(c,0,rw(r+0x162));if(r[0x66]&128u)dw(c,0,(uint16_t)(d(c,0)+1u));
  if(d(c,0)>=3u){if(d(c,0)>=8u||d(c,0)<5u){s->pc=0xbf52;break;}dw(c,0,(uint16_t)(d(c,0)-5u));}
  c->regs->d[0]=(uint32_t)d(c,0)*7u;dw(c,0,(uint16_t)(d(c,0)+4u));ww(r+0x12da,d(c,0));ww(r+0x1304,3);c->a[0]=0x1c12d6;c->a[1]=0xbf58;c->regs->d[0]=1;s->pc=0xbe1e;break;
 case 0xbe1e:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;p[0]|=128u;ww(p+2,0x4f);ww(p+0x24,0);ww(p+0x1c,0);
  if(!readrom(s,x,c->a[1]+d(c,0),t,1))return WF_R18_POST_BLOCKED;
  db(c,1,t[0]);
  p[0x15]=t[0];
  if(!readrom(s,x,c->a[1]+2u+d(c,0),t,1))return WF_R18_POST_BLOCKED;
  db(c,1,t[0]);
  p[0x17]=t[0];
  if(!readrom(s,x,c->a[1]+4u+d(c,0),t,1))return WF_R18_POST_BLOCKED;
  db(c,1,t[0]);
  dw(c,1,d(c,1)&255u);
  dw(c,2,d(c,0));
  dw(c,0,d(c,1));
  s->pc=0xbe4e;
  break;
 case 0xbe4e:return call(s,0x2aea,0xbe54);
 case 0xbe54:dw(c,0,d(c,2));s->pc=0xbe56;break;
 case 0xbe56:return call(s,0x27b8,0xbe5c);
 case 0xbe5c:c->a[0]+=0x2a;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0xbe1eu:0xbe66u;break;
 case 0xbe66:return call(s,0x2836,0xbe6c);
 case 0xbe6c:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0xbe7a;break;
 case 0xbe7a:case 0xbf34:if(r[0x6f]&2u){s->pc=s->pc==0xbe7au?0xbe84u:0xbf3eu;break;}return request(s,3,s->pc,s->pc==0xbe7au?0xbe84u:0xbf3eu,0x1c006f,2);
 case 0xbe84:ww(r+0x15fc,1);s->pc=0xbe8c;break;
 case 0xbe8c:return call(s,0x2a06,0xbe92);
 case 0xbe92:dw(c,0,15);c->a[0]=0x188280;s->pc=0xbe9c;break;
 case 0xbe9c:dw(c,1,d(c,0));dw(c,2,(uint16_t)(d(c,0)<<8));dw(c,1,(uint16_t)(d(c,1)<<1));s->pc=0xbea4;break;
 case 0xbea4:return request(s,2,0xbea4,0xbea8,c->a[0]+(int32_t)(int16_t)d(c,1),d(c,2));
 case 0xbea8:dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0xbe9cu:0xbeacu;break;
 case 0xbeac:return call(s,0x26772,0xbeb2);
 case 0xbeb2:return call(s,0x1f9e,0xbeb8);
 case 0xbeb8:c->a[2]=0xc1540;dw(c,2,0x10);dw(c,7,0x10);s->pc=0xbec6;break;
 case 0xbec6:return call(s,0x9aca,0xbeca);
 case 0xbeca:s->pc=rw(r+0x12da)==0x12u?0xbed4u:0xbee6u;break;
 case 0xbed4:c->a[2]=0xc1578;dw(c,2,0x17);dw(c,7,0x10);s->pc=0xbee2;break;
 case 0xbee2:return call(s,0x9aca,0xbee6);
 case 0xbee6:return call(s,0x26844,0xbeec);
 case 0xbeec:ww(r+0x1322,0x40);r[0x131c]|=128u;s->pc=0xbefc;break;
 case 0xbefc:c->regs->d[0]=4;c->a[0]=0x1c1258;s->pc=0xbf04;break;
 case 0xbf04:local(s,0xbf5e,0xbf08);break;
 case 0xbf08:p=object(s,c,0,1);if(!p)return WF_R18_POST_BLOCKED;s->pc=p[0]&128u?0xbf10u:0xbf16u;break;
 case 0xbf10:return call(s,0x27b8,0xbf16);
 case 0xbf16:c->a[0]+=0x2a;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)!=0xffffu?0xbf04u:0xbf20u;break;
 case 0xbf20:return call(s,0x2836,0xbf26);
 case 0xbf26:ww(r+0x6e,0);r[0x6f]|=1u;s->pc=0xbf34;break;
 case 0xbf3e:s->pc=rw(r+0x12fa)!=0x10u?0xbefcu:0xbf48u;break;
 case 0xbf48:return call(s,0x264e2,0xbf4e);
 case 0xbf4e:local(s,0xc004,0xbf52);break;
 case 0xbf52:memcpy(c->regs->d,s->saved_d,3*sizeof(uint32_t));memcpy(c->a,s->saved_a,3*sizeof(uint32_t));s->pc=0xbf56;break;
 case 0xbf56:return WF_R18_POST_RETURN;
 case 0xbf5e:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;
  if(d(c,0)==0u){
   if(!(p[0x1c]&128u)){s->pc=0xbffc;break;}if(rw(p+0x22)){s->pc=0xbff6;break;}
   ww(p+0x22,10);dw(c,1,rw(p+0x24));c->a[1]=0xbffe;
   if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,1),t,1))return WF_R18_POST_BLOCKED;
   db(c,1,t[0]);
   dw(c,1,d(c,1)&15u);
   ww(p+4,d(c,1));
   ww(p+0x24,(uint16_t)(rw(p+0x24)+1u));
   if(rw(p+0x24)>=5u)ww(p+0x1c,0);
   if(rw(p+4)==2u){r[0x12f2]|=128u;
   s->saved_d0=c->regs->d[0];
   dw(c,0,0x312a);
   s->pc=0xbfbc;
   }else s->pc=0xbff6;
  }else if(d(c,0)==1u){if(!(p[0x1c]&128u)){s->pc=0xbffc;break;}if(!rw(p+0x22)){ww(p+0x22,8);ww(p+0x24,(uint16_t)(rw(p+0x24)+1u));if(rw(p+0x24)<7u)ww(p+4,(uint16_t)(rw(p+4)+1u));}s->pc=0xbff6;}
  else s->pc=0xbffc;
  break;
 case 0xbfbc:return call(s,0x2052,0xbfc2);
 case 0xbfc2:c->regs->d[0]=s->saved_d0;s->pc=0xbff6;break;
 case 0xbff6:p=object(s,c,0,0x24);if(!p)return WF_R18_POST_BLOCKED;ww(p+0x22,(uint16_t)(rw(p+0x22)-1u));s->pc=0xbffc;break;
 case 0xbffc:return returned(s);
 case 0xc004:c->a[0]=0x1c1300;c->a[1]=0xc044;c->regs->d[2]=4;s->pc=0xc012;break;
 case 0xc012:p=object(s,c,0,0x26);if(!p)return WF_R18_POST_BLOCKED;ww(p,0);ww(p+4,0);ww(p+0x22,0);ww(p+0x1c,0);ww(p+0x24,0);ww(p+2,0);
  if(!readrom(s,x,c->a[1]+(int32_t)(int16_t)d(c,2),t,1))return WF_R18_POST_BLOCKED;
  db(c,0,t[0]);
  dw(c,0,d(c,0)&255u);
  s->pc=0xc032;
  break;
 case 0xc032:return call(s,0x2b58,0xc038);
 case 0xc038:c->a[0]-=0x2a;dw(c,2,(uint16_t)(d(c,2)-1u));s->pc=d(c,2)!=0xffffu?0xc012u:0xc042u;break;
 case 0xc042:return returned(s);
 default:return block(s,c,s->pc);
 }
 (void)v;return WF_R18_POST_MORE;
}
