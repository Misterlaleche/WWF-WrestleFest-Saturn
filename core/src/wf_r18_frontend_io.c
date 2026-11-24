#include "wf_r18_frontend_io.h"
#include <string.h>
static uint16_t r16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static wf_status_t fill(wf_r18_frontend_io_t*c,uint32_t at,unsigned bytes,uint32_t value){
 wf_r17_fg0_write_t writes[64];
 while(bytes){unsigned n=bytes/4u;if(n>64u)n=64u;
  for(unsigned i=0;i<n;i++)writes[i]=(wf_r17_fg0_write_t){at+i*4u,value,4};
  wf_status_t st=wf_r18_video_batch(c->runtime,writes,n,c->video);if(st!=WF_OK)return st;
  at+=n*4u;bytes-=n*4u;
 }
 return WF_OK;
}
static void palette_from_ram(wf_r18_frontend_io_t*c){
 memcpy(c->video->palette_used,c->ram+0x1600,16);
 memcpy(c->video->palette_map,c->ram+0x1610,256);
}
static void credit_to_ram(wf_r18_frontend_io_t*c){
 c->ram[0x63]=c->runtime->g.global_0063;w16(c->ram+0x72,c->runtime->g.global_0072);
}
static wf_status_t credit_text(wf_runtime_t*r,uint32_t owner,void*u){
 wf_r18_frontend_io_t*c=u;credit_to_ram(c);
 return c->bootstrap->text_owner(r,owner,c->bootstrap->user);
}
static wf_status_t credit_fg(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){
 wf_r18_frontend_io_t*c=u;credit_to_ram(c);
 return c->bootstrap->fg0_batch(r,w,n,c->bootstrap->user);
}
static wf_status_t panels(wf_r18_frontend_io_t*c,uint32_t owner){
 uint8_t bytes[6];unsigned id=(uint16_t)c->regs->d[0];uint32_t p;
 if(id>2u||!c->video->read_rom)return WF_BLOCKED;
 if(c->video->read_rom(c->video->read_user,0x5224u+id*4u,bytes,4)!=WF_OK)return WF_BLOCKED;
 p=((uint32_t)bytes[0]<<24)|((uint32_t)bytes[1]<<16)|(bytes[2]<<8)|bytes[3];
 for(unsigned guard=0;guard<128u;guard++,p+=6u){
  if(c->video->read_rom(c->video->read_user,p,bytes,2)!=WF_OK)return WF_BLOCKED;
  unsigned off=r16(bytes);if(off==0xfffu)return WF_OK;
  if(c->video->read_rom(c->video->read_user,p+2,bytes+2,4)!=WF_OK)return WF_BLOCKED;
  unsigned tile=r16(bytes+2),count=r16(bytes+4);
  for(unsigned i=0;i<=count;i++){
   wf_r17_fg0_write_t w={0xc1770u+off+i*4u,owner==0x51d2u?((uint32_t)(uint16_t)(tile+i)<<16)|0x1eu:0u,4};
   wf_status_t st=wf_r18_video_batch(c->runtime,&w,1,c->video);if(st!=WF_OK)return st;
  }
 }
 return WF_BLOCKED;
}
wf_status_t wf_r18_frontend_io_call(wf_r18_frontend_io_t*c,uint32_t owner){
 if(!c||!c->ram||!c->regs||!c->regs->valid||!c->a||!c->runtime||!c->video)return WF_BAD_STATE;
 c->blocked_dependency=0;c->runtime->m68k_regs=*c->regs;
 wf_status_t st=WF_OK;uint16_t id=(uint16_t)c->regs->d[0];
 switch(owner){
 case 0x1f38:case 0x1f52:{
  wf_r18_frontend_boot_context_t b={.ram_1c0000=c->ram,.ram_bytes=0x4000,.regs=c->regs,.a=c->a};
  int result=owner==0x1f38?wf_r18_frontend_clear_1f38(&b):wf_r18_frontend_clear_1f52(&b);
  st=result<0?WF_BLOCKED:WF_OK;palette_from_ram(c);break;
 }
 case 0x1f6c:{
  memset(c->ram+0x16d2,0,0x2dcu);
  /* The original clear changes board RAM; it must not mutate a currently
   * bound immutable scene pack through the gameplay ring-cell callback. */
  wf_r18_background_cell_fn saved=c->video->ring_cell;c->video->ring_cell=NULL;
  st=fill(c,0x80000,0x1000,0);if(st==WF_OK)st=fill(c,0x82000,0x1000,0);
  c->video->ring_cell=saved;c->background_cleared=1;palette_from_ram(c);break;
 }
 case 0x1f9e:
  st=fill(c,0xc0000,0x2000,0);if(st==WF_OK)c->ram[0x72]|=0x80u;break;
 case 0x1fc0:st=fill(c,0xc0000,0x4000,0xffc5ff04u);break;
 case 0x1fde:
  /* CLR.L starts at the unaligned-to-long $19B2 and rounds the end up. */
  memset(c->ram+0x19b2,0,0x320u);st=fill(c,0xc2000,0x1000,0);
  wf_r17_m68k_move_w(&c->runtime->m68k_regs,0,0xffff);c->a[0]=0xc3000;c->sprites_cleared=1;break;
 case 0x2aea:case 0x2b58:
  palette_from_ram(c);st=owner==0x2aea?wf_r18_palette_2aea(c->runtime,id,c->video):wf_r18_palette_2b58(c->runtime,id,c->video);
  memcpy(c->ram+0x1600,c->video->palette_used,16);memcpy(c->ram+0x1610,c->video->palette_map,256);break;
 case 0x51d2:case 0x527e:st=panels(c,owner);break;
 case 0x9a42:
  for(uint32_t at=0xc0100;at<0xc1e00&&st==WF_OK;at+=0x100)st=fill(c,at,160,0);
  wf_r17_m68k_move_w(&c->runtime->m68k_regs,1,0xffff);c->a[1]=0xc1da0;break;
 case 0x2503c:st=wf_r18_text_2503c(c->runtime,id,c->video);break;
 case 0x26122:st=wf_r18_number_26122(c->runtime,id,c->video);break;
 case 0x9aca:st=wf_r18_strip_9aca(c->runtime,(uint16_t)c->regs->d[2],(uint16_t)c->regs->d[7],c->a[2],c->video);break;
 case 0x1e92:
  if(!c->bootstrap){st=WF_BLOCKED;break;}
  c->runtime->g.credit_cycle_units_004b=c->ram[0x4b];c->runtime->g.credit_remainder_004e=c->ram[0x4e];
  c->runtime->g.credit_count_004f=c->ram[0x4f];c->runtime->g.global_0063=c->ram[0x63];c->runtime->g.global_0072=r16(c->ram+0x72);
  {wf_r18_bootstrap_context_t b=*c->bootstrap;b.text_owner=credit_text;b.fg0_batch=credit_fg;b.user=c;
   st=wf_r18_credit_1e92(c->runtime,&b);}
  credit_to_ram(c);break;
 default:
  if(c->platform_call){st=c->platform_call(c,owner);c->runtime->m68k_regs=*c->regs;}
  else st=WF_BLOCKED;
  break;
 }
 *c->regs=c->runtime->m68k_regs;
 if(st!=WF_OK)c->blocked_dependency=owner;
 return st;
}
wf_status_t wf_r18_frontend_io_read_ram(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 wf_r18_frontend_io_t*c=u;(void)r;
 if(!c||!c->ram||!out)return WF_BAD_STATE;
 if(at<0x1c0000u||at>0x1c3ffeu)return WF_BLOCKED;
 *out=r16(c->ram+at-0x1c0000u);return WF_OK;
}
