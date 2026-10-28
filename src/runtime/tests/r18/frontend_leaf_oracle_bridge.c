#include "wf_r18_frontend_leaf.h"
#include <string.h>
static wf_status_t read_rom(void*u,uint32_t at,void*out,size_t n){
 if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;
 memcpy(out,(uint8_t*)u+at,n);return WF_OK;
}
int frontend_leaf_oracle(uint8_t*mem,uint32_t d[8],uint32_t a[7],unsigned owner){
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_r18_frontend_leaf_t l={.read_rom=read_rom,.user=mem,.records=(wf_r18_sprite_record_t*)(mem+0xc2000),.capacity=512};
 wf_r18_frontend_render_t c={.ram=mem+0x1c0000,.regs=&regs,.a=a,.emit_d1fc=wf_r18_frontend_leaf_emit,.clear_attribute=wf_r18_frontend_leaf_clear,.user=&l};
 wf_status_t st=wf_r18_frontend_render_call(&c,owner);memcpy(d,regs.d,32);return st;
}
