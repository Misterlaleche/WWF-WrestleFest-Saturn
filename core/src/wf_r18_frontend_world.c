#include "wf_r18_frontend_world.h"
#include <string.h>
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void ww(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static wf_status_t block(wf_r18_frontend_world_t*c,uint32_t pc,uint32_t at){c->blocked_dependency=pc;c->blocked_address=at;return WF_BLOCKED;}
static wf_status_t read(wf_r18_frontend_world_t*c,uint32_t pc,uint32_t at,unsigned n,uint32_t*out){
 uint8_t b[4];uint32_t physical=at&0xffffffu;
 if(!c->read_rom)return block(c,pc,physical);
 wf_status_t st=c->read_rom(c->user,physical,b,n);
 if(st!=WF_OK){c->blocked_dependency=pc;c->blocked_address=physical;return st;}
 *out=0;for(unsigned i=0;i<n;i++)*out=(*out<<8)|b[i];return WF_OK;
}
static wf_status_t list_28608(wf_r18_frontend_world_t*c){
 uint8_t*r=c->ram;uint32_t source,x,y,id;unsigned out=0;
 uint8_t left=(uint8_t)(((uint16_t)(rw(r+0x17da)+1u)>>1)-2u);
 uint8_t right=(uint8_t)(((uint16_t)(rw(r+0x17da)-1u)>>1)+6u);
 uint8_t top=(uint8_t)(((uint16_t)(rw(r+0x17dc)+1u)>>1)+2u),bottom=(uint8_t)(top-8u);
 uint32_t table=0x2867au+(int32_t)(int16_t)(uint16_t)(rw(r+0x7e)<<2);
 if(read(c,0x2864a,table,4,&source)!=WF_OK)return WF_BLOCKED;
 for(unsigned guard=0;guard<128;guard++,source+=3){
  if(read(c,0x28654,source,1,&x)!=WF_OK)return WF_BLOCKED;
  if(x&128u){r[0x182a+out]=255;return WF_OK;}
  if(read(c,0x28658,source+1u,1,&y)!=WF_OK||read(c,0x2865a,source+2u,1,&id)!=WF_OK)return WF_BLOCKED;
  c->regs->d[6]=(c->regs->d[6]&0xffffff00u)|id;
  if(x>=left&&x<right&&y<top&&y>=bottom){
   if(out>=47u)return block(c,0x2866c,0x1c182au+out);
   r[0x182a+out++]=(uint8_t)id;
  }
 }
 return block(c,0x28654,source);
}
static int contains(wf_r18_frontend_world_t*c,unsigned off,unsigned id,uint32_t pc){
 for(unsigned i=0;i<48;i++){unsigned b=c->ram[off+i];if(b&128u)return 0;if(b==id)return 1;}
 block(c,pc,0x1c0000u+off+48u);return -1;
}
static wf_status_t difference(wf_r18_frontend_world_t*c,unsigned remove){
 unsigned from=remove?0x185au:0x182au,other=remove?0x182au:0x185au;
 uint8_t*r=c->ram;
 for(unsigned i=0;i<48;i++){
  unsigned id=r[from+i];if(id&128u)return WF_OK;
  int found=contains(c,other,id,remove?0x28862u:0x2881cu);if(found<0)return WF_BLOCKED;if(found)continue;
  unsigned j;
  for(j=0;j<48;j++){
   uint8_t*p=r+0x188a+j*4u;
   if(remove){if((p[0]&128u)&&(p[0]&63u)==id){memset(p,0,4);break;}}
   else if(!(p[0]&128u)){p[0]=(uint8_t)(id|128u);break;}
  }
  if(j==48)return block(c,remove?0x28878u:0x28832u,0x1c194a);
 }
 return block(c,remove?0x28858u:0x28812u,0x1c0000u+from+48u);
}
static wf_status_t object(wf_r18_frontend_world_t*c,uint8_t**out){
 uint32_t at=c->a[0]&0xffffffu;
 if(at<0x1c0000u||at>0x1c3ffcu)return block(c,0x28b4a,at);
 *out=c->ram+at-0x1c0000u;return WF_OK;
}
static wf_status_t descriptor(wf_r18_frontend_world_t*c,unsigned id,uint32_t*desc,unsigned fresh){
 uint32_t table=0x28c66u+(int32_t)(int16_t)(uint16_t)(rw(c->ram+0x7e)<<2),list;
 if(read(c,fresh?0x28b62u:0x28be8u,table,4,&list)!=WF_OK)return WF_BLOCKED;
 return read(c,fresh?0x28b70u:0x28bf8u,list+id*4u,4,desc);
}
static wf_status_t animate_28b46(wf_r18_frontend_world_t*c){
 uint8_t*p;uint32_t desc=0,value;unsigned fresh;
 if(object(c,&p)!=WF_OK)return WF_BLOCKED;
 fresh=!(p[0]&64u);p[0]|=64u;
 if(fresh){
  if(descriptor(c,p[0]&63u,&desc,1)!=WF_OK||read(c,0x28b74,desc+1u,1,&value)!=WF_OK)return WF_BLOCKED;
  p[3]=(uint8_t)value;int synchronized=0;
  if(p[3])for(unsigned i=0;i<46;i++){
   uint32_t address=0x1c188au+i*4u;uint8_t*q=c->ram+0x188a+i*4u;
   /* Original CMPA.L compares raw pointers, even for24-bit aliases. */
   if(c->a[0]!=address&&(q[3]&63u)==(p[3]&63u)){p[1]=q[1];p[2]=q[2];synchronized=1;break;}
  }
  if(!synchronized){p[1]=0;if(read(c,0x28bae,desc+3u,1,&value)!=WF_OK)return WF_BLOCKED;p[2]=(uint8_t)value;}
 }else{
  unsigned old=p[2];p[2]--;if(old)return WF_OK;
  p[1]++;
  if(descriptor(c,p[0]&63u,&desc,0)!=WF_OK||read(c,0x28bfc,desc,1,&value)!=WF_OK)return WF_BLOCKED;
  if(value<p[1])p[1]=0;
  if(read(c,0x28c10,desc+3u+p[1]*2u,1,&value)!=WF_OK)return WF_BLOCKED;
  p[2]=(uint8_t)value;
 }
 unsigned back=!!(p[3]&128u),counter=back?0x19aau:0x19acu,queue=back?0x197au:0x194au;
 unsigned count=rw(c->ram+counter);uint32_t pc=back?0x28c52u:0x28c30u;
 if(count>=48u)return block(c,pc,0x1c0000u+queue+count);
 if(read(c,pc,desc+2u+p[1]*2u,1,&value)!=WF_OK)return WF_BLOCKED;
 c->ram[queue+count]=(uint8_t)value;ww(c->ram+counter,(uint16_t)(count+1u));return WF_OK;
}
static wf_status_t dispatch_288f0(wf_r18_frontend_world_t*c){
 uint8_t*p;if(object(c,&p)!=WF_OK)return WF_BLOCKED;uint32_t table,handler;
 c->a[1]=0x28916;c->regs->d[1]=(c->regs->d[1]&0xffff0000u)|(c->ram[0x7f]<<2);
 if(read(c,0x28900,0x28916u+(c->ram[0x7f]<<2),4,&table)!=WF_OK)return WF_BLOCKED;
 c->a[1]=table;c->regs->d[1]=(c->regs->d[1]&0xffff0000u)|((p[0]&63u)<<2);
 if(read(c,0x2890e,table+(p[0]&63u)*4u,4,&handler)!=WF_OK)return WF_BLOCKED;
 c->a[1]=handler;
 if(handler!=0x28b3eu)return block(c,0x28912,handler);
 return animate_28b46(c);
}
static wf_status_t update_28896(wf_r18_frontend_world_t*c){
 uint32_t saved_d[2],saved_a[3];memcpy(saved_d,c->regs->d,sizeof saved_d);memcpy(saved_a,c->a,sizeof saved_a);
 for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<47;i++){
  unsigned b=c->ram[0x188a+i*4u];
  if(!(b&128u)||!!(b&64u)!=(pass==0u))continue;
  c->a[0]=0x1c188au+i*4u;
  if(dispatch_288f0(c)!=WF_OK)return WF_BLOCKED;
 }
 memcpy(c->regs->d,saved_d,sizeof saved_d);memcpy(c->a,saved_a,sizeof saved_a);return WF_OK;
}
wf_status_t wf_r18_frontend_world_call(wf_r18_frontend_world_t*c,uint32_t owner){
 if(!c||!c->ram||c->ram_bytes<0x4000||!c->regs||!c->regs->valid||!c->a)return WF_BAD_STATE;
 c->blocked_dependency=c->blocked_address=0;
 switch(owner){
 case 0x285da:
  if(list_28608(c)!=WF_OK||difference(c,1)!=WF_OK||difference(c,0)!=WF_OK)return WF_BLOCKED;
  memcpy(c->ram+0x185a,c->ram+0x182a,48);return update_28896(c);
 case 0x28608:return list_28608(c);
 case 0x2884e:return difference(c,1);
 case 0x28808:return difference(c,0);
 case 0x28896:return update_28896(c);
 case 0x288f0:return dispatch_288f0(c);
 case 0x28b3e:case 0x28b46:return animate_28b46(c);
 default:return block(c,owner,0);
 }
}
