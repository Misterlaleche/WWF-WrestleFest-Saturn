#include <string.h>
#include "wf_r18_display.h"
static const uint8_t *rom;
static wf_status_t read_rom(void*u,uint32_t at,void*out,size_t n){(void)u;if(at+n>524288u)return WF_BLOCKED;memcpy(out,rom+at,n);return WF_OK;}
int display_case(const uint8_t*p,const uint16_t*in,uint16_t*out){
 wf_runtime_t rt;wf_r18_display_t d;memset(&rt,0,sizeof rt);rom=p;
 rt.g.gameplay_vblank_enable=(uint8_t)in[0];rt.g.frame_handshake[1]=(uint8_t)in[1];
 rt.g.global_1800=in[2];rt.g.global_1802=in[3];rt.g.global_17e6=in[4];rt.g.global_17ee=in[5];rt.g.global_17ea=in[6];rt.g.global_17f2_word=in[7];
 if(wf_r18_display_init(&d,&rt,read_rom,0)!=WF_OK)return 1;
 for(unsigned i=0;i<4;i++)d.scroll[i]=0x1234;
 int status=wf_r18_display_irq(&rt,&d);
 out[0]=rt.g.global_1800;out[1]=rt.g.global_1802;out[2]=rt.g.global_17e6;out[3]=rt.g.global_17ee;out[4]=rt.g.global_17ea;out[5]=rt.g.global_17f2_word;
 memcpy(out+6,d.scroll,8);return status;
}

#include "wf_r18_hud.h"
int hud_irq_case(const uint16_t*in,uint8_t*out){
 wf_runtime_t r;wf_r18_video_t v;memset(&r,0,sizeof r);memset(&v,0,sizeof v);
 r.g.gameplay_vblank_enable=(uint8_t)in[0];r.g.frame_handshake[1]=(uint8_t)in[1];r.g.global_169e=in[2];r.g.global_16a4=in[3];
 memset(v.palettes+0x700,0xa5,32);
 int s=wf_r18_hud_irq(&r,&v);memcpy(out,v.palettes+0x700,32);return s;
}
