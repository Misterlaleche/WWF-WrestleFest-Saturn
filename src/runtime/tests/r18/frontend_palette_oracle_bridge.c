#include "wf_r18_frontend_palette.h"
#include <string.h>
static unsigned char *memory;
static wf_status_t read_rom(void *u,uint32_t at,void *out,size_t n){
 (void)u;if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;
 memcpy(out,memory+at,n);return WF_OK;
}
int frontend_palette_oracle(unsigned char *mem,uint32_t d[8],unsigned world,unsigned text){
 wf_runtime_t r={0};wf_r18_video_t v={0};memory=mem;
 r.m68k_regs.valid=1;memcpy(r.m68k_regs.d,d,32);v.read_rom=read_rom;
 memcpy(v.palettes,mem+0x180000,0x4000);
 memcpy(v.fg_palette,mem+0x188000,0x800);memcpy(v.bg_palette,mem+0x186000,0x800);
 wf_status_t st=wf_r18_frontend_palette_2a06(&r,&v,(uint16_t)world,(uint16_t)text);
 memcpy(mem+0x180000,v.palettes,0x4000);
 memcpy(mem+0x188000,v.fg_palette,0x800);memcpy(mem+0x186000,v.bg_palette,0x800);
 memcpy(d,r.m68k_regs.d,32);return st;
}
