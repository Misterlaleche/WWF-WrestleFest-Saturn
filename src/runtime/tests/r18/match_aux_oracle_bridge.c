#include "wf_r18_match_aux.h"
#include "wf_r18_frontend_io.h"
#include <string.h>
static wf_runtime_t r;
static wf_r18_video_t v;
static uint8_t *memory;
static wf_status_t rom(void*u,uint32_t at,void*out,size_t n){(void)u;if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(out,memory+at,n);return WF_OK;}
static wf_status_t effect(void*u,uint32_t owner,wf_r18_prematch_t*m){
 (void)u;wf_r18_frontend_io_t io={.ram=m->ram,.regs=m->regs,.a=m->a,.runtime=&r,.video=&v};return wf_r18_frontend_io_call(&io,owner);
}
int match_aux_oracle(uint8_t*mem,uint32_t d[8],uint32_t a[7],unsigned owner){
 memory=mem;memset(&r,0,sizeof r);memset(&v,0,sizeof v);v.read_rom=rom;
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_r18_prematch_t m={.ram=mem+0x1c0000,.regs=&regs,.effect=effect};memcpy(m.a,a,28);
 memcpy(v.fg0,mem+0xc0000,0x4000);
 int st=wf_r18_match_aux_call(&m,&v,owner);
 memcpy(mem+0xc0000,v.fg0,0x4000);memcpy(d,regs.d,32);memcpy(a,m.a,28);return st;
}
int match_aux_clear_guards(void){
 static wf_r18_video_t before;
 uint8_t opaque=0;wf_r17_m68k_shadow_t regs={.valid=1};
 wf_r18_prematch_t m={.ram=&opaque,.regs=&regs};
 memset(&v,0x5a,sizeof v);before=v;
 if(wf_r18_match_aux_call(NULL,&v,0xb2e4)!=WF_BAD_STATE)return 1;
 m.ram=NULL;if(wf_r18_match_aux_call(&m,&v,0xb2e4)!=WF_BAD_STATE)return 2;
 m.ram=&opaque;regs.valid=0;if(wf_r18_match_aux_call(&m,&v,0xb2e4)!=WF_BAD_STATE)return 3;
 regs.valid=1;if(wf_r18_match_aux_call(&m,NULL,0xb2e4)!=WF_BLOCKED||m.dependency!=0xb2e4)return 4;
 return memcmp(&v,&before,sizeof v)?5:0;
}
