#include "wf_r18_frontend_map.h"
#include <string.h>
static wf_status_t read_rom(void*u,uint32_t at,void*out,size_t n){
 if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(out,(uint8_t*)u+at,n);return WF_OK;
}
static wf_status_t store(void*u,uint32_t at,uint32_t v,unsigned n){
 if(!((at>=0x80000u&&at+n<=0x84000u)||(at>=0x100000u&&at+n<=0x100008u)||at==0x140010u))return WF_BLOCKED;
 for(unsigned i=0;i<n;i++)((uint8_t*)u)[at+i]=(uint8_t)(v>>(8u*(n-i-1u)));
 return WF_OK;
}
int frontend_map_oracle(uint8_t*mem,uint32_t d[8],uint32_t a[7],unsigned owner){
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_r18_frontend_map_t c={.ram=mem+0x1c0000,.regs=&regs,.a=a,.read_rom=read_rom,.store=store,.user=mem};
 wf_status_t st=wf_r18_frontend_map_call(&c,owner);memcpy(d,regs.d,32);return st;
}
