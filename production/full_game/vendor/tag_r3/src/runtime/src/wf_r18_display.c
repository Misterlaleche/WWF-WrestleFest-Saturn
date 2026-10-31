#include "wf_r18_display.h"
static void registers(wf_r18_display_t*d,const wf_runtime_t*r){
 d->scroll[0]=r->g.global_17e6;d->scroll[1]=(uint16_t)(2048u-r->g.global_17ee);
 d->scroll[2]=r->g.global_17ea;d->scroll[3]=(uint16_t)(2048u-r->g.global_17f2_word);
}
wf_status_t wf_r18_display_init(wf_r18_display_t*d,const wf_runtime_t*r,wf_r18_rom_read_fn f,void*u){
 uint8_t bytes[30];if(!d||!r||!f)return WF_BAD_STATE;
 if(f(u,0x27048u,bytes,sizeof bytes)!=WF_OK)return WF_BLOCKED;
 d->read_rom=f;d->user=u;
 for(unsigned i=0;i<15;i++)d->shake[i]=(uint16_t)((bytes[i*2u]<<8)|bytes[i*2u+1u]);
 registers(d,r);return WF_OK;
}
static wf_status_t delta(wf_r18_display_t*d,uint16_t count,uint16_t*out){
 *out=0;if(!count)return WF_OK;
 if(count<15u){*out=d->shake[count];return WF_OK;}
 /* $2702A LSL.W then signed D0.W address indexing, including word wrap. */
 uint32_t at=(uint32_t)(0x27048+(int16_t)(uint16_t)(count<<1));uint8_t b[2];
 if(d->read_rom(d->user,at,b,2)!=WF_OK)return WF_BLOCKED;
 *out=(uint16_t)((b[0]<<8)|b[1]);return WF_OK;
}
wf_status_t wf_r18_display_irq(wf_runtime_t*r,wf_r18_display_t*d){
 if(!r||!d||!d->read_rom)return WF_BAD_STATE;
 /* $0842/$084E/$085A: the display owners run only on a ready, enabled,
  * non-reentrant IRQ. Hardware ACK remains with the frame coordinator. */
 if(!(r->g.gameplay_vblank_enable&128u)||(r->g.frame_handshake[1]&129u)!=1u)return WF_OK;
 uint16_t dy,dx;
 if(delta(d,r->g.global_1800,&dy)!=WF_OK||delta(d,r->g.global_1802,&dx)!=WF_OK)return WF_BLOCKED;
 if(r->g.global_1800){r->g.global_17ee=(uint16_t)(r->g.global_17ee+dy);r->g.global_17f2_word=(uint16_t)(r->g.global_17f2_word+dy);--r->g.global_1800;}
 if(r->g.global_1802){r->g.global_17e6=(uint16_t)(r->g.global_17e6+dx);r->g.global_17ea=(uint16_t)(r->g.global_17ea+dx);--r->g.global_1802;}
 if(!(r->g.frame_handshake[1]&16u))registers(d,r); /* $26E1A */
 return WF_OK;
}
