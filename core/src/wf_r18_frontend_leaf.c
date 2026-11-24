#include "wf_r18_frontend_leaf.h"
static uint16_t rw(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
wf_status_t wf_r18_frontend_leaf_emit(void*u,wf_r18_frontend_render_t*c){
 wf_r18_frontend_leaf_t*l=u;
 if(!l||!l->records||!l->read_rom||!c||!c->ram||!c->regs||!c->a)return WF_BAD_STATE;
 uint32_t address=c->a[0]&0xffffffu;
 if(address<0x1c0000u||address>0x1c3fd6u||c->a[2]<0xc2000u||c->a[2]>0xc4000u||(c->a[2]&15u))return WF_BLOCKED;
 const uint8_t*p=c->ram+address-0x1c0000u;uint32_t link=(((uint32_t)rw(p+0x26)<<16)|rw(p+0x28))&0xffffffu;
 uint16_t partner=0xffffu;
 if(link>=0x1c0000u&&link<=0x1c3ffcu)partner=rw(c->ram+link-0x1c0000u+2u);
 wf_r18_metasprite_ctx_t parser={.read_rom=l->read_rom,.user=l->user,.palette_map=c->ram+0x1610,
  .records=l->records,.capacity=l->capacity,.count=(c->a[2]-0xc2000u)/16u,.d6=c->regs->d[6],.d7=c->regs->d[7],.cache=l->cache};
 wf_status_t st=wf_r18_metasprite_emit(&parser,rw(p+2),rw(p+4),rw(p+0x14),rw(p+0x16),partner);
 if(st!=WF_OK)return st;
 c->a[2]=0xc2000u+(uint32_t)parser.count*16u;c->regs->d[6]=parser.d6;c->regs->d[7]=parser.d7;return WF_OK;
}
wf_status_t wf_r18_frontend_leaf_clear(void*u,uint32_t at){
 wf_r18_frontend_leaf_t*l=u;
 if(!l||!l->records||at<0xc2000u||at>=0xc2000u+l->capacity*16u)return WF_BLOCKED;
 ((uint8_t*)l->records)[at-0xc2000u]=0;return WF_OK;
}
