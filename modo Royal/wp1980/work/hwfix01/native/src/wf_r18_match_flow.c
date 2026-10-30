#include "wf_r18_match_flow.h"
#include <string.h>
static uint16_t r16(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static void dw(wf_r18_prematch_t*c,unsigned i,uint16_t v){wf_r17_m68k_move_w(c->regs,i,v);}
static uint16_t d(wf_r18_prematch_t*c,unsigned i){return(uint16_t)c->regs->d[i];}
static int valid(wf_r18_prematch_t*c){return c&&c->ram&&c->regs&&c->regs->valid;}
static int request(wf_r18_match_flow_t*s,unsigned kind,uint32_t owner,uint32_t ret){
 s->request=(wf_r18_match_request_t){kind,owner,s->pc,ret};s->pending=1;return WF_R18_MATCH_REQUEST;
}
static int blocked(wf_r18_match_flow_t*s,wf_r18_prematch_t*c,uint32_t owner){
 s->blocked_dependency=owner;c->dependency=owner;s->fatal=1;return WF_R18_MATCH_BLOCKED;
}
void wf_r18_match_flow_init(wf_r18_match_flow_t*s,uint32_t pc){
 if(s){memset(s,0,sizeof *s);s->pc=pc;s->entry_pc=pc;}
}
int wf_r18_match_flow_complete(wf_r18_match_flow_t*s,wf_r18_prematch_t*c,wf_status_t status,uint32_t pc){
 if(!s||!valid(c)||!s->pending||s->fatal)return WF_R18_MATCH_BAD_CONTEXT;
 if(status!=WF_OK)return blocked(s,c,c->dependency?c->dependency:s->request.owner);
 if(s->request.kind==WF_R18_MATCH_ENTRY){
  if(!pc)return WF_R18_MATCH_BAD_CONTEXT;
  s->pc=pc;s->pending=0;c->dependency=0;return WF_R18_MATCH_BOUNDARY;
 }
 if(pc&&pc!=s->request.return_pc)return WF_R18_MATCH_BAD_CONTEXT;
 s->pc=s->request.return_pc;s->pending=0;c->dependency=0;return WF_R18_MATCH_MORE;
}
int wf_r18_match_flow_step(wf_r18_match_flow_t*s,wf_r18_prematch_t*c){
 if(!s||!valid(c))return WF_R18_MATCH_BAD_CONTEXT;
 if(s->fatal)return WF_R18_MATCH_BLOCKED;
 if(s->pending)return WF_R18_MATCH_REQUEST;
 c->dependency=0;uint8_t*ram=c->ram;
 switch(s->pc){
 case 0x6fc:case 0x57e0:return request(s,WF_R18_MATCH_ENTRY,s->pc,0);
 case 0xac0:
  for(unsigned i=0;i<4u;i++)w32(ram+0x92u+i*4u,0);
  s->pc=0xad8;break;
 case 0xad8:
  if(wf_r18_match_setup_d1c(c)!=WF_OK)return blocked(s,c,c->dependency?c->dependency:0xd1c);
  s->pc=0xade;break;
 case 0xade:return request(s,WF_R18_MATCH_CALL,0x2988,0xae4);
 case 0xae4:ram[0x161]&=0xfdu;s->pc=0xaec;break;
 case 0xaec:
  c->a[0]=0xb14;dw(c,0,r16(ram+0x7c)?0u:4u);c->a[0]=d(c,0)?0xb1cu:0xbd2u;
  s->pc=0xb06;break;
 case 0xb06:
  if(c->a[0]==0xb1cu)return request(s,WF_R18_MATCH_CALL,0xb1c,0xb08);
  if(c->a[0]!=0xbd2u)return blocked(s,c,0xb06);
  s->pc=0xbd2;break;
 case 0xbd2:w16(ram+0x166,0);s->pc=0xbd8;break;
 case 0xbd8:return request(s,WF_R18_MATCH_CALL,0x8eb6,0xbde);
 case 0xbde:s->pc=ram[0x163]==4u||ram[0x163]==9u?0xbf2u:0xc02u;break;
 case 0xbf2:dw(c,0,0);s->pc=0xbf6;break;
 case 0xbf6:return request(s,WF_R18_MATCH_CALL,0xae20,0xbfc);
 case 0xbfc:return request(s,WF_R18_MATCH_CALL,0x264e2,0xc02);
 case 0xc02:return request(s,WF_R18_MATCH_CALL,0x7b70,0xc08);
 case 0xc08:
  if(!(ram[0x161]&1u)&&(ram[0x66]&8u)){
   ram[0x160]&=0xbfu;
   if((ram[0x5b0]&0x80u)&&(ram[0x7c8]&0x80u)){
    ram[0x160]|=0x40u;
    for(unsigned i=0;i<4u;i++)w16(ram+0x5b0u+i*0x10cu+0x104u,0x8000);
   }
  }
  s->pc=0xc60;break;
 case 0xc60:return request(s,WF_R18_MATCH_CALL,0xc98,0xc66);
 case 0xc66:return request(s,WF_R18_MATCH_CALL,0xa654,0xc6c);
 case 0xc6c:w16(ram+0x15fc,1);s->pc=0xc74;break;
 case 0xc74:return request(s,WF_R18_MATCH_CALL,0x2a06,0xc7a);
 case 0xc7a:{
  static const uint16_t sounds[10]={0x3104,0x310a,0x310c,0x310f,0x3106,0x3104,0x310a,0x310c,0x310f,0x3106};
  c->a[0]=0xdee;dw(c,0,(uint16_t)(r16(ram+0x162)&255u));unsigned index=d(c,0);dw(c,0,(uint16_t)(d(c,0)<<1));
  if(index>=10u)return blocked(s,c,0xdee);
  dw(c,0,sounds[index]);s->pc=0xc90;break;
 }
 case 0xc90:return request(s,WF_R18_MATCH_CALL,0x2052,0xc96);
 case 0xc96:
  if(s->entry_pc!=0xac0u)return WF_R18_MATCH_RETURN;
  s->pc=0xb08;break;
 case 0xb08:s->pc=0x8ae6;break;
 case 0x8ae6:w16(ram+0x72,0xffff);s->pc=0x8aee;break;
 case 0x8aee:return request(s,WF_R18_MATCH_CALL,0x1e92,0x8af2);
 case 0x8af2:c->a[0]=0x1c05b0;dw(c,0,3);s->pc=0x8afc;break;
 case 0x8afc:{
  s->saved_d0=c->regs->d[0];
  if(c->a[0]<0x1c05b0u||c->a[0]>0x1c08d4u||(c->a[0]-0x1c05b0u)%0x10cu)return blocked(s,c,0x8afc);
  uint8_t*p=ram+c->a[0]-0x1c0000u;s->pc=(p[0]&0x80u)&&!(p[0x56]&0x80u)?0x8b10u:0x8b1cu;break;
 }
 case 0x8b10:return request(s,WF_R18_MATCH_CALL,0x782c,0x8b16);
 case 0x8b16:return request(s,WF_R18_MATCH_CALL,0x7506,0x8b1c);
 case 0x8b1c:
  c->regs->d[0]=s->saved_d0;c->a[0]+=0x10cu;dw(c,0,(uint16_t)(d(c,0)-1u));s->pc=d(c,0)==0xffffu?0x8b2au:0x8afcu;break;
 case 0x8b2a:return request(s,WF_R18_MATCH_CALL,0xc1ea,0x8b30);
 case 0x8b30:w16(ram+0x169e,0x8000);s->pc=0x8b38;break;
 case 0x8b38:
  if(s->entry_pc!=0xac0u)return WF_R18_MATCH_RETURN;
  s->pc=0xb0e;break;
 case 0xb0e:s->pc=0xf8c;return WF_R18_MATCH_GAMEPLAY;
 case 0xf8c:return WF_R18_MATCH_GAMEPLAY;
 default:return blocked(s,c,s->pc);
 }
 return WF_R18_MATCH_MORE;
}
