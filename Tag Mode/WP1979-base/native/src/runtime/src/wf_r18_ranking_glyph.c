#include "wf_r18_ranking_glyph.h"
#include <string.h>
static uint16_t word(wf_r18_prematch_t*m,unsigned r){return (uint16_t)m->regs->d[r];}
static void putword(wf_r18_prematch_t*m,unsigned r,uint16_t v){wf_r17_m68k_move_w(m->regs,r,v);}
static wf_status_t blocked(wf_r18_ranking_glyph_context_t*c,uint32_t pc,uint32_t at){
 c->blocked_dependency=c->machine->dependency=pc;c->blocked_address=at&0xffffffu;return WF_BLOCKED;
}
static wf_status_t write_bus(wf_r18_ranking_glyph_context_t*c,uint32_t pc,uint32_t at,uint32_t value,unsigned n){
 at&=0xffffffu;
 if(!c->store_bus||c->store_bus(c->user,at,value,n)!=WF_OK)return blocked(c,pc,at);
 return WF_OK;
}
static wf_status_t tile_6d76(wf_r18_ranking_glyph_context_t*c){
 wf_r18_prematch_t*m=c->machine;
 if(write_bus(c,0x6d76,m->a[2]+1u,(uint8_t)word(m,3),1)!=WF_OK)return WF_BLOCKED;
 putword(m,3,(uint16_t)(word(m,3)>>8));
 return write_bus(c,0x6d7c,m->a[2]+3u,(uint8_t)word(m,3),1);
}
wf_status_t wf_r18_ranking_glyph_call(wf_r18_ranking_glyph_context_t*c,uint32_t owner){
 if(!c||!c->machine||!c->machine->ram||!c->machine->regs||!c->machine->regs->valid)return WF_BAD_STATE;
 c->blocked_dependency=c->blocked_address=c->machine->dependency=0;
 if(owner==0x6d76u)return tile_6d76(c);
 if(owner!=0x6cfau)return blocked(c,owner,0);
 wf_r18_prematch_t*m=c->machine;uint32_t saved[5],a2=m->a[2];memcpy(saved,m->regs->d,sizeof saved);
 if(word(m,0)>=3u){
  if(write_bus(c,0x6d04,m->a[2]+1u,(uint8_t)word(m,2),1)!=WF_OK)return WF_BLOCKED;
 }else if(word(m,2)==0x4bu){
  if(write_bus(c,0x6d12,m->a[2],0,4)!=WF_OK)return WF_BLOCKED;
  if(write_bus(c,0x6d1a,m->a[2]+4u,0,4)!=WF_OK)return WF_BLOCKED;
  m->a[2]+=0x100u;
  if(write_bus(c,0x6d28,m->a[2],0,4)!=WF_OK)return WF_BLOCKED;
  if(write_bus(c,0x6d30,m->a[2]+4u,0,4)!=WF_OK)return WF_BLOCKED;
 }else{
  putword(m,2,(uint16_t)(word(m,2)-0x31u));putword(m,2,(uint16_t)(word(m,2)<<2));putword(m,2,(uint16_t)(word(m,2)+0x158u));
  wf_r17_m68k_moveq(m->regs,4,0);
  for(;;){
   putword(m,3,word(m,2));putword(m,3,(uint16_t)(word(m,3)+word(m,4)));
   if(tile_6d76(c)!=WF_OK)return WF_BLOCKED;
   putword(m,4,(uint16_t)(word(m,4)+1u));
   if(word(m,4)==4u)break;
   if(word(m,4)==2u)m->a[2]-=0xfcu;
   if(word(m,4)&1u)m->a[2]+=0x100u;
  }
 }
 memcpy(m->regs->d,saved,sizeof saved);m->a[2]=a2;return WF_OK;
}
