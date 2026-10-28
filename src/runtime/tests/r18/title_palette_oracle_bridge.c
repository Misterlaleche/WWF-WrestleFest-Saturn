#include "wf_r18_title_palette.h"
#include <string.h>
static wf_status_t read_rom(void*u,uint32_t at,void*out,size_t n){
 if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;
 memcpy(out,(uint8_t*)u+at,n);return WF_OK;
}
static wf_status_t write_video(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){
 (void)r;uint8_t*mem=u;
 for(size_t i=0;i<n;i++){
  if(w[i].width!=2||w[i].addr<0x180000u||w[i].addr>=0x180800u)return WF_BAD_STATE;
  mem[w[i].addr]=(uint8_t)(w[i].value>>8);mem[w[i].addr+1]=(uint8_t)w[i].value;
 }return WF_OK;
}
int title_palette_oracle(uint8_t*mem,uint32_t d[8],unsigned owner){
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_r18_title_palette_context_t c={.ram=mem+0x1c0000,.regs=&regs,.read_rom=read_rom,.write=write_video,.user=mem};
 wf_status_t st=owner==0xb312?wf_r18_title_palette_b312(&c):wf_r18_title_palette_irq_b4b8(&c);
 memcpy(d,regs.d,32);return st;
}
