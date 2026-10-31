#include "wf_r18_ranking.h"
#include <string.h>
static int valid(wf_r18_ranking_context_t*c){return c&&c->machine&&c->machine->ram&&c->machine->regs&&c->machine->regs->valid;}
static uint16_t d(wf_r18_prematch_t*c,unsigned n){return (uint16_t)c->regs->d[n];}
static void dw(wf_r18_prematch_t*c,unsigned n,uint16_t v){wf_r17_m68k_move_w(c->regs,n,v);}
static void db(wf_r18_prematch_t*c,unsigned n,uint8_t v){wf_r17_m68k_move_b(c->regs,n,v);}
static void q(wf_r18_prematch_t*c,unsigned n,uint32_t v){wf_r17_m68k_move_l(c->regs,n,v);}
static uint32_t indexed(uint32_t a,uint16_t index){return a+(uint32_t)(int32_t)(int16_t)index;}
static int block(wf_r18_ranking_t*s,wf_r18_prematch_t*c,uint32_t pc){s->fatal=1;s->blocked_dependency=c->dependency=pc;return WF_R18_RANK_BLOCKED;}
static int read_value(wf_r18_ranking_t*s,wf_r18_ranking_context_t*x,uint32_t at,unsigned n,uint32_t*v){
 uint8_t b[4];at&=0xffffffu;
 if(at>=0x1c0000u&&at<=0x1c4000u-n)memcpy(b,x->machine->ram+at-0x1c0000u,n);
 else if(at<0x80000u&&n<=0x80000u-at&&x->read_rom&&x->read_rom(x->user,at,b,n)==WF_OK){}
 else{block(s,x->machine,s->pc);return 0;}
 *v=0;for(unsigned i=0;i<n;i++)*v=(*v<<8)|b[i];return 1;
}
static int write_value(wf_r18_ranking_t*s,wf_r18_prematch_t*c,uint32_t at,unsigned n,uint32_t v){
 at&=0xffffffu;if(at<0x1c0000u||at>0x1c4000u-n){block(s,c,s->pc);return 0;}
 for(unsigned i=0;i<n;i++)c->ram[at-0x1c0000u+i]=(uint8_t)(v>>(8u*(n-1u-i)));
 return 1;
}
#define RD(v,at,n) do{if(!read_value(s,x,(at),(n),&(v)))return WF_R18_RANK_BLOCKED;}while(0)
#define WR(at,n,v) do{if(!write_value(s,c,(at),(n),(v)))return WF_R18_RANK_BLOCKED;}while(0)
#define RDP(atpc,v,at,n) do{s->pc=(atpc);RD(v,at,n);}while(0)
#define WRP(atpc,at,n,v) do{s->pc=(atpc);WR(at,n,v);}while(0)
static int request(wf_r18_ranking_t*s,unsigned kind,uint32_t owner,uint32_t ret,uint32_t at,uint32_t value,unsigned width){
 s->request=(wf_r18_ranking_request_t){kind,owner,s->pc,ret,at,value,width};s->pending=1;return WF_R18_RANK_REQUEST;
}
static int call(wf_r18_ranking_t*s,uint32_t owner,uint32_t ret){return request(s,WF_R18_RANK_CALL,owner,ret,0,0,0);}
static int local(wf_r18_ranking_t*s,wf_r18_prematch_t*c,uint32_t owner,uint32_t ret){
 if(s->depth>=8u)return block(s,c,s->pc);
 s->return_stack[s->depth++]=ret;s->pc=owner;return WF_R18_RANK_MORE;
}
static int returned(wf_r18_ranking_t*s){if(s->depth){s->pc=s->return_stack[--s->depth];return WF_R18_RANK_MORE;}return WF_R18_RANK_RETURN;}
void wf_r18_ranking_init(wf_r18_ranking_t*s,uint32_t pc){if(s){memset(s,0,sizeof *s);s->pc=s->entry_pc=pc;}}
int wf_r18_ranking_complete(wf_r18_ranking_t*s,wf_r18_ranking_context_t*x,wf_status_t status,uint32_t value){
 if(!s||!valid(x)||!s->pending||s->fatal)return WF_R18_RANK_BAD_CONTEXT;
 if(status!=WF_OK)return block(s,x->machine,x->machine->dependency?x->machine->dependency:(s->request.owner?s->request.owner:s->request.call_pc));
 s->io_value=value;s->pending=0;s->pc=s->request.return_pc;
 if(s->request.kind==WF_R18_RANK_READ){
  if(s->request.call_pc==0x6cf2u)dw(x->machine,0,(uint16_t)value);
  else if(s->request.call_pc==0x698cu){s->pc=0x698c;if(!write_value(s,x->machine,x->machine->a[1],4,value))return WF_R18_RANK_BLOCKED;s->pc=s->request.return_pc;}
 }
 if(s->request.kind==WF_R18_RANK_SYNC&&!(x->machine->ram[0x6f]&2u))return WF_R18_RANK_WAIT;
 return WF_R18_RANK_MORE;
}
int wf_r18_ranking_step(wf_r18_ranking_t*s,wf_r18_ranking_context_t*x){
 if(!s||!valid(x))return WF_R18_RANK_BAD_CONTEXT;
 if(s->fatal)return WF_R18_RANK_BLOCKED;
 if(s->pending)return WF_R18_RANK_REQUEST;
 wf_r18_prematch_t*c=x->machine;uint8_t*r=c->ram;uint32_t*a=c->a,v,w,z;unsigned i;
 switch(s->pc){
 case 0x65c0:
  if((r[0x66]&8u)&&!(r[0x161]&1u)){
   a[0]=0x1c0092;q(c,0,0);q(c,2,0);
   do{
    dw(c,1,(uint16_t)(d(c,0)<<2));RD(v,indexed(a[0],d(c,1)),4);a[1]=v;
    if(a[1]){
     a[2]=0x1c07c8;
     if(a[1]==a[2]){a[1]-=0x10c;WR(indexed(a[0],d(c,1)),4,a[1]);dw(c,2,(uint16_t)(d(c,2)+1u));}
     else{a[2]+=0x10c;if(a[1]==a[2]){a[1]-=0x324;WR(indexed(a[0],d(c,1)),4,a[1]);dw(c,2,(uint16_t)(d(c,2)+1u));}}
    }
    dw(c,0,(uint16_t)(d(c,0)+1u));
   }while(d(c,0)!=4u);
   if(d(c,2)){a[1]=0x1c05b0;a[2]=0x1c07c8;RD(v,a[2]+0x56,2);WR(a[1]+0x56,2,v);a[1]+=0x10c;a[2]+=0x10c;RD(v,a[2]+0x56,2);WR(a[1]+0x56,2,v);}
  }
  s->pc=0x664e;break;
 case 0x664e:WR(0x1c00ba,4,0xffffffffu);WR(0x1c00be,4,0xffffffffu);q(c,1,0);s->pc=0x6664;break;
 case 0x6664:dw(c,2,(uint16_t)(d(c,1)<<2));a[0]=0x1c0092;RD(v,indexed(a[0],d(c,2)),4);a[0]=v;return local(s,c,0x7720,0x6676);
 case 0x6676:if(!s->carry){s->pc=0x6682;break;}return local(s,c,0x67b2,0x667c);
 case 0x667c:if(!s->carry){s->pc=0x6682;break;}return local(s,c,0x683e,0x6682);
 case 0x6682:dw(c,1,(uint16_t)(d(c,1)+1u));if(d(c,1)!=4u){s->pc=0x6664;break;}return local(s,c,0x68ac,0x668e);
 case 0x668e:
  q(c,0,0);q(c,1,0);
  do{dw(c,2,(uint16_t)(d(c,1)<<1));a[0]=0x1c00ba;RD(v,indexed(a[0],d(c,2)),2);if(v!=0xffffu)dw(c,0,(uint16_t)(d(c,0)+1u));dw(c,1,(uint16_t)(d(c,1)+1u));}while(d(c,1)!=4u);
  if(!d(c,0)){s->pc=0x6792;break;}return local(s,c,0x69c8,0x66b8);
 case 0x66b8:dw(c,0,0x310e);s->pc=0x66bc;return call(s,0x2052,0x66c2);
 case 0x66c2:
  q(c,0,0);do{a[0]=0x1c00a2;WR(indexed(a[0],d(c,0)),2,0x31);a[0]=0x1c00b2;WR(indexed(a[0],d(c,0)),2,0);a[0]=0x1c00aa;WR(indexed(a[0],d(c,0)),2,0);dw(c,0,(uint16_t)(d(c,0)+2u));}while(d(c,0)!=8u);
  WR(0x1c169c,2,0x700);WR(0x1c1cd4,4,0);s->pc=0x66fe;break;
 case 0x66fe:q(c,1,3);s->pc=0x6700;break;
 case 0x6700:
  dw(c,0,(uint16_t)(d(c,1)<<1));a[0]=0x1c00ba;RD(v,indexed(a[0],d(c,0)),2);dw(c,0,(uint16_t)v);
  if(d(c,0)==0xffffu){s->pc=0x672c;break;}dw(c,5,d(c,0));a[2]=0x1c1cd4;RD(v,indexed(a[2],d(c,1)),1);
  if(v&8u){s->pc=0x6728;break;}return local(s,c,0x6b6e,0x6728);
 case 0x6728:return local(s,c,0x68f6,0x672c);
 case 0x672c:dw(c,1,(uint16_t)(d(c,1)-1u));s->pc=(d(c,1)&0x8000u)?0x6730u:0x6700u;break;
 case 0x6730:WR(0x1c006e,2,0);r[0x6f]|=1u;s->pc=0x673e;break;
 case 0x673e:if(!(r[0x6f]&2u))return request(s,WF_R18_RANK_SYNC,0,0x673e,0x1c006f,2,1);s->pc=0x6748;break;
 case 0x6748:RD(v,0x1c169c,2);v=(uint16_t)(v-1u);WR(0x1c169c,2,v);if(!v){WR(0x1c00ba,4,0xffffffffu);WR(0x1c00be,4,0xffffffffu);}s->pc=0x6766;break;
 case 0x6766:return local(s,c,0x6794,0x6768);
 case 0x6768:if(s->carry){s->pc=0x66fe;break;}return local(s,c,0x6a26,0x676e);
 case 0x676e:r[0x6f]|=1u;s->pc=0x6776;break;
 case 0x6776:if(!(r[0x6f]&2u))return request(s,WF_R18_RANK_SYNC,0,0x6776,0x1c006f,2,1);s->pc=0x6780;break;
 case 0x6780:WR(0x1c0092,4,0);WR(0x1c0096,4,0);s->pc=0x678c;return call(s,0x264e2,0x6792);
 case 0x6792:return returned(s);
 case 0x6794:q(c,6,0);s->pc=0x6796;break;
 case 0x6796:dw(c,7,(uint16_t)(d(c,6)<<1));s->pc=0x679a;break;
 case 0x679a:
  v=indexed(a[0],d(c,7));if((v&0xffffffu)>=0x1c0000u&&(v&0xffffffu)<=0x1c3ffeu){RD(s->io_value,v,2);s->pc=0x67a0;break;}
  return request(s,WF_R18_RANK_READ,0,0x67a0,v,0,2);
 case 0x67a0:
  if((uint16_t)s->io_value!=0xffffu){s->carry=1;return returned(s);}dw(c,6,(uint16_t)(d(c,6)+1u));
  if(d(c,6)<4u){s->pc=0x6796;break;}s->carry=0;return returned(s);
 case 0x67b2:
  a[1]=0x1c00fe;RD(v,0x1c0164,2);dw(c,2,(uint16_t)v);
  if(r[0x161]&1u){a[1]=0x1c0140;RDP(0x67ce,v,a[0]+0xc4,2);dw(c,2,(uint16_t)v);if(d(c,2)>=100u)dw(c,2,0);}
  RD(v,a[1],1);if((uint8_t)d(c,2)<v){s->carry=0;return returned(s);}
  if(!(r[0x161]&1u)){
   if(!(d(c,0)&1u)){a[0]+=0x10c;RDP(0x67f8,v,a[0]+0x57,1);WR(a[1]+5,1,v);a[0]-=0x10c;}
   else{a[0]-=0x10c;RDP(0x680c,v,a[0]+0x57,1);WR(a[1]+5,1,v);a[0]+=0x10c;}
  }
  WR(a[1],1,d(c,2));WR(a[1]+1,1,0x5b);WR(a[1]+2,1,0x5c);WR(a[1]+3,1,d(c,0));RDP(0x682c,v,a[0]+0x57,1);WR(a[1]+4,1,v);s->carry=1;return returned(s);
 case 0x683e:
  a[1]=(r[0x161]&1u)?0x1c0104u:0x1c00c2u;
  do{
   dw(c,0,0);dw(c,7,54);
   do{
    RD(v,indexed(a[1],d(c,7)),1);db(c,2,(uint8_t)v);RD(w,indexed(a[1],d(c,7))+6u,1);db(c,3,(uint8_t)w);
    unsigned swap=v<w;
    if(v==w){RD(v,indexed(a[1],d(c,7))+8u,1);RD(w,indexed(a[1],d(c,7))+2u,1);swap=v==0x5cu&&w!=0x5cu;}
    if(swap){q(c,0,1);RD(v,indexed(a[1],d(c,7))+6u,4);q(c,2,v);RD(v,indexed(a[1],d(c,7))+10u,2);dw(c,3,(uint16_t)v);RD(v,indexed(a[1],d(c,7)),4);WR(indexed(a[1],d(c,7))+6u,4,v);RD(v,indexed(a[1],d(c,7))+4u,2);WR(indexed(a[1],d(c,7))+10u,2,v);WR(indexed(a[1],d(c,7)),4,c->regs->d[2]);WR(indexed(a[1],d(c,7))+4u,2,d(c,3));}
    dw(c,7,(uint16_t)(d(c,7)-6u));
   }while(!(d(c,7)&0x8000u));
  }while(d(c,0));return returned(s);
 case 0x68ac:
  a[1]=(r[0x161]&1u)?0x1c0104u:0x1c00c2u;q(c,0,0);
  do{RD(v,a[1]+2,1);if(v==0x5cu){RD(v,a[1]+3,1);db(c,2,(uint8_t)v);dw(c,2,d(c,2)&255u);a[2]=0x1c00ba;dw(c,2,(uint16_t)(d(c,2)<<1));WR(indexed(a[2],d(c,2)),2,d(c,0));WR(a[1]+2,2,0x5b5b);}a[1]+=6;dw(c,0,(uint16_t)(d(c,0)+1u));}while(d(c,0)<11u);return returned(s);
 case 0x68f6:
  a[1]=0x1c1cd8;dw(c,7,d(c,1)&15u);q(c,7,(uint32_t)d(c,7)*3u);dw(c,7,(uint16_t)(d(c,7)<<8));q(c,7,c->regs->d[7]&0xfffu);a[1]+=c->regs->d[7];
  a[2]=0x1c00ba;dw(c,7,(uint16_t)(d(c,1)<<1));RDP(0x691a,v,indexed(a[2],d(c,7)),2);dw(c,0,(uint16_t)v);if(d(c,0)==0xffffu)dw(c,0,d(c,5));
  q(c,7,0);a[0]=0xc0f0c;dw(c,6,(uint16_t)((d(c,0)-3u)<<9));
  if(d(c,0)<3u){dw(c,7,2);a[0]=0x69c2;dw(c,6,(uint16_t)(d(c,0)<<1));s->pc=0x694c;break;}
  s->pc=0x6956;break;
 case 0x694c:RD(v,indexed(a[0],d(c,6)),2);dw(c,6,(uint16_t)v);a[0]=0xc0000;s->pc=0x6956;break;
 case 0x6956:q(c,6,c->regs->d[6]&0xfffu);a[0]+=c->regs->d[6];s->pc=0x695e;break;
 case 0x695e:dw(c,6,0x23);s->pc=0x6962;break;
 case 0x6962:
  a[2]=0x1c1cd4;RDP(0x6968,v,indexed(a[2],d(c,1)),1);db(c,5,(uint8_t)v);dw(c,5,d(c,5)&15u);
  if(d(c,5)&8u)s->pc=d(c,5)==15u?0x6984u:0x6992u;
  else s->pc=d(c,5)==7u?0x698cu:0x69b4u;
  break;
 case 0x6984:RD(v,a[1],4);return request(s,WF_R18_RANK_WRITE,0,0x699a,a[0],v,4);
 case 0x698c:return request(s,WF_R18_RANK_READ,0,0x6992,a[0],0,4);
 case 0x6992:return request(s,WF_R18_RANK_WRITE,0,0x699a,a[0],0,4);
 case 0x699a:
  a[0]+=4;a[1]+=4;dw(c,6,(uint16_t)(d(c,6)-1u));if(d(c,6)!=0xffffu){s->pc=0x6962;break;}
  a[0]+=0x6c;dw(c,7,(uint16_t)(d(c,7)-1u));s->pc=d(c,7)!=0xffffu?0x695eu:0x69b4u;break;
 case 0x69b4:a[2]=0x1c1cd4;RDP(0x69ba,v,indexed(a[2],d(c,1)),1);WR(indexed(a[2],d(c,1)),1,v+1u);return returned(s);
 case 0x69c8:return call(s,0x1f6c,0x69cc);
 case 0x69cc:return call(s,0x1fc0,0x69d0);
 case 0x69d0:
  WR(0x1c1804,2,0x140);WR(0x1c1806,2,0x600);WR(0x1c007e,2,3);WR(0x1c15f4,2,3);WR(0x1c15f8,2,3);WR(0x1c15fc,2,0);s->pc=0x6a04;break;
 case 0x6a04:return call(s,0x2a06,0x6a08);
 case 0x6a08:return call(s,0x26772,0x6a0e);
 case 0x6a0e:return call(s,0x26e66,0x6a14);
 case 0x6a14:return call(s,0x1f9e,0x6a18);
 case 0x6a18:return call(s,0x1fde,0x6a1c);
 case 0x6a1c:return local(s,c,0x6a26,0x6a1e);
 case 0x6a1e:return call(s,0x26844,0x6a24);
 case 0x6a24:return returned(s);
 case 0x6a26:a[2]=(r[0x161]&1u)?0x1c0104u:0x1c00c2u;dw(c,0,0);s->pc=0x6a3e;break;
 case 0x6a3e:a[3]=0x6b58;dw(c,1,(uint16_t)(d(c,0)<<1));s->pc=0x6a48;break;
 case 0x6a48:
  RD(v,indexed(a[3],d(c,1)),2);q(c,1,v);a[3]=0xc0000+c->regs->d[1];if(r[0x161]&1u)a[3]+=8;
  RD(v,a[2],1);q(c,1,v);if(!(r[0x161]&1u))dw(c,1,(uint16_t)(d(c,1)+1u));q(c,3,0);
  if(r[0x161]&1u){
   /* 2790's packed decimal result. Its D0 is restored by 6A8C/6A96;
    * D2/D3 are preserved by 2790 itself, and D4 exits DBRA as FFFF. */
   unsigned decimal=d(c,1);uint32_t bcd=0;for(unsigned shift=0;shift<20;shift+=4){bcd|=(decimal%10u)<<shift;decimal/=10u;}q(c,1,bcd);dw(c,4,0xffffu);
   dw(c,2,d(c,1)&15u);dw(c,1,d(c,1)&0xf0u);
   if(!d(c,1)){a[3]+=4;dw(c,1,d(c,2));}else{dw(c,1,d(c,1)>>4);q(c,3,1);}
  }
  s->pc=0x6ab6;break;
 case 0x6ab6:dw(c,1,(uint16_t)(d(c,1)-0xd15u));s->pc=0x6aba;break;
 case 0x6aba:return request(s,WF_R18_RANK_WRITE,0,0x6abe,a[3]+1u,(uint8_t)d(c,1),1);
 case 0x6abe:dw(c,1,d(c,1)>>8);s->pc=0x6ac0;break;
 case 0x6ac0:return request(s,WF_R18_RANK_WRITE,0,0x6ac4,a[3]+3u,(uint8_t)d(c,1),1);
 case 0x6ac4:dw(c,1,d(c,2));a[3]+=4;dw(c,3,(uint16_t)(d(c,3)-1u));s->pc=d(c,3)!=0xffffu?0x6ab6u:0x6ad0u;break;
 case 0x6ad0:a[3]=0x6b42;dw(c,1,(uint16_t)(d(c,0)<<1));s->pc=0x6ada;break;
 case 0x6ada:RD(v,indexed(a[3],d(c,1)),2);q(c,1,v);a[3]=0xc0000+c->regs->d[1];q(c,4,0);s->pc=0x6aee;break;
 case 0x6aee:
  RD(v,indexed(a[2]+1u,d(c,4)),1);db(c,2,(uint8_t)v);dw(c,2,(uint16_t)((d(c,2)&255u)-0x10u));dw(c,3,d(c,4));
  if(d(c,0)<3u)q(c,3,(uint32_t)d(c,3)*12u);else dw(c,3,(uint16_t)(d(c,3)<<3));
  s->saved_a2=a[2];a[2]=a[3]+c->regs->d[3];s->pc=0x6b14;break;
 case 0x6b14:return call(s,0x6cfa,0x6b18);
 case 0x6b18:a[2]=s->saved_a2;dw(c,4,(uint16_t)(d(c,4)+1u));if(d(c,4)<3u){s->pc=0x6aee;break;}
  a[2]+=6;dw(c,0,(uint16_t)(d(c,0)+1u));s->pc=d(c,0)<11u?0x6a3eu:0x6b38u;break;
 case 0x6b38:return call(s,0xa4d4,0x6b3c);
 case 0x6b3c:return call(s,0x7874,0x6b40);
 case 0x6b40:return returned(s);
 case 0x6b6e:a[2]=0x6b42;dw(c,2,(uint16_t)(d(c,0)<<1));s->pc=0x6b78;break;
 case 0x6b78:
  RD(v,indexed(a[2],d(c,2)),2);q(c,2,v);a[2]=0xc0000+c->regs->d[2];a[1]=0x1c00b2;dw(c,7,(uint16_t)(d(c,1)<<1));
  RDP(0x6b94,v,indexed(a[1],d(c,7)),2);q(c,2,v&15u);if(d(c,0)<3u)q(c,2,(uint32_t)d(c,2)*12u);else dw(c,2,(uint16_t)(d(c,2)<<3));
  a[2]+=c->regs->d[2];a[3]=0x1c00a2;RDP(0x6bb4,v,indexed(a[3],d(c,7)),2);dw(c,2,(uint16_t)v);s->pc=0x6bb8;break;
 case 0x6bb8:return call(s,0x6cfa,0x6bbc);
 case 0x6bbc:return local(s,c,0x6ce8,0x6bc0);
 case 0x6bc0:
  a[2]=0x1c00aa;a[3]=0x1c00a2;dw(c,7,(uint16_t)(d(c,1)<<1));dw(c,0,d(c,0)&15u);
  if(d(c,0)==1u||d(c,0)==2u){
   RDP(d(c,0)==1u?0x6be0:0x6c08,v,indexed(a[2],d(c,7)),2);v=(uint16_t)(v+1u);WR(indexed(a[2],d(c,7)),2,v);
   if(v>=8u){WRP(d(c,0)==1u?0x6bee:0x6c16,indexed(a[2],d(c,7)),2,0);RDP(d(c,0)==1u?0x6bf2:0x6c1a,v,indexed(a[3],d(c,7)),2);
    if(d(c,0)==1u){v=(uint16_t)(v+1u);if(v>=0x4bu)v=0x31;}else{v=(uint16_t)(v-1u);if(v==0x30u)v=0x4b;}
    WR(indexed(a[3],d(c,7)),2,v);
   }
  }
  s->pc=0x6c2e;break;
 case 0x6c2e:
  a[3]=(r[0x161]&1u)?0x1c0104u:0x1c00c2u;dw(c,7,(uint16_t)(d(c,1)<<1));a[2]=0x1c00ba;RDP(0x6c4e,v,indexed(a[2],d(c,7)),2);q(c,0,v&15u);q(c,0,(uint32_t)d(c,0)*6u);
  a[1]=0x1c00b2;RDP(0x6c62,v,indexed(a[1],d(c,7)),2);dw(c,6,(uint16_t)v);dw(c,0,(uint16_t)(d(c,0)+d(c,6)));a[3]+=c->regs->d[0];
  a[2]=0x1c00a2;RDP(0x6c70,v,indexed(a[2],d(c,7)),2);dw(c,6,(uint16_t)(v+0x10u));WRP(0x6c78,a[3]+1u,1,d(c,6));return local(s,c,0x6ce8,0x6c80);
 case 0x6c80:
  a[2]=0x1c0146;a[3]=0x1c014e;dw(c,7,(uint16_t)(d(c,1)<<1));if(d(c,0)&3u)return returned(s);
  dw(c,0,(d(c,0)&0x30u)>>4);RD(v,indexed(a[2],d(c,7)),2);v=(v&d(c,0))^d(c,0);WR(indexed(a[2],d(c,7)),2,v);WR(indexed(a[3],d(c,7)),2,v);WR(indexed(a[2],d(c,7)),2,d(c,0));
  if(v){a[1]=0x1c00b2;a[2]=0x1c00a2;WR(indexed(a[2],d(c,7)),2,0x31);RD(v,indexed(a[1],d(c,7)),2);v=(uint16_t)(v+1u);WR(indexed(a[1],d(c,7)),2,v);if(v>=3u){a[1]=0x1c00ba;WR(indexed(a[1],d(c,7)),2,0xffffu);}}
  return returned(s);
 case 0x6ce8:a[2]=0x140020;dw(c,7,(uint16_t)(d(c,1)<<1));s->pc=0x6cf2;break;
 case 0x6cf2:return request(s,WF_R18_RANK_READ,0,0x6cf6,indexed(a[2],d(c,7)),0,2);
 case 0x6cf6:dw(c,0,(uint16_t)~d(c,0));return returned(s);
 case 0x7720:
  q(c,0,0);a[1]=0x1c05b0;
  do{if(a[0]==a[1]){s->carry=1;return returned(s);}a[1]+=0x10c;dw(c,0,(uint16_t)(d(c,0)+1u));}while(d(c,0)<4u);s->carry=0;return returned(s);
 default:return block(s,c,s->pc);
 }
 (void)w;(void)z;(void)i;return WF_R18_RANK_MORE;
}
