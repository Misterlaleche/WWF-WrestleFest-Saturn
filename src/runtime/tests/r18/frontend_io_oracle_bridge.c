#include "wf_r18_frontend_io.h"
#include <string.h>
typedef struct bus {uint8_t *mem;wf_r18_frontend_io_t *io;} bus_t;
static wf_status_t read_rom(void*u,uint32_t at,void*out,size_t n){
 bus_t*b=u;if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;
 memcpy(out,b->mem+at,n);return WF_OK;
}
static wf_status_t read_ram(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 bus_t*b=u;return wf_r18_frontend_io_read_ram(r,at,out,b->io);
}
static wf_status_t text(wf_runtime_t*r,uint32_t owner,void*u){
 wf_r18_video_t*v=u;uint16_t id=(uint16_t)r->m68k_regs.d[0];
 return owner==0x2503cu?wf_r18_text_2503c(r,id,v):wf_r18_number_26122(r,id,v);
}
int frontend_io_oracle(uint8_t*mem,uint32_t d[8],uint32_t a[7],unsigned owner){
 static wf_runtime_t r;static wf_r18_video_t v;memset(&r,0,sizeof r);memset(&v,0,sizeof v);
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_r18_bootstrap_context_t boot={.text_owner=text,.fg0_batch=wf_r18_video_batch,.video_batch=wf_r18_video_batch,.user=&v};
 wf_r18_frontend_io_t io={.ram=mem+0x1c0000,.regs=&regs,.a=a,.runtime=&r,.video=&v,.bootstrap=&boot};
 bus_t b={mem,&io};v.read_rom=read_rom;v.read_ram=read_ram;v.read_user=&b;
 memcpy(v.fg0,mem+0xc0000,0x4000);memcpy(v.background,mem+0x80000,0x4000);
 memcpy(v.palettes,mem+0x180000,0x4000);memcpy(v.bg_palette,mem+0x186000,0x800);memcpy(v.fg_palette,mem+0x188000,0x800);
 wf_status_t st=wf_r18_frontend_io_call(&io,owner);
 memcpy(mem+0xc0000,v.fg0,0x4000);memcpy(mem+0x80000,v.background,0x4000);
 memcpy(mem+0x180000,v.palettes,0x4000);memcpy(mem+0x186000,v.bg_palette,0x800);memcpy(mem+0x188000,v.fg_palette,0x800);
 memcpy(d,regs.d,32);return st;
}
