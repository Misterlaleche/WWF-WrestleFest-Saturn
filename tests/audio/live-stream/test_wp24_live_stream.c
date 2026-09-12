#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "wf24_music_stream.h"
typedef struct { unsigned render_calls, copy_calls, prep, start, stop, ca_i; uint8_t val; unsigned cas[4]; } F;
static bool read_unused(uint32_t o,void*d,uint32_t b,void*u){(void)o;(void)d;(void)b;(void)u;return false;}
static bool render(void*d,uint32_t b,void*u){F*f=u; memset(d,f->val++,b); f->render_calls++; return true;}
static bool copy(unsigned h,const void*s,uint32_t b,void*u){F*f=u;(void)h;(void)s;assert(b==WF24_MUSIC_HALF_BYTES);f->copy_calls++;return true;}
static bool prep(const wf24_slot31_format_t *fmt,void*u){F*f=u;assert(fmt->slot==31u&&fmt->lea==32767u);f->prep++;return true;}
static bool start(void*u){((F*)u)->start++;return true;} static bool stop(void*u){((F*)u)->stop++;return true;}
static bool ca(unsigned*out,void*u){F*f=u;*out=f->cas[f->ca_i++%4u];return true;}
int main(void){F f={0};wf24_music_stream_t s;wf24_stream_ops_t ops={read_unused,render,copy,prep,start,stop,ca,&f};f.cas[0]=0;f.cas[1]=4;f.cas[2]=4;f.cas[3]=0;assert(wf24_stream_init(&s,&ops));assert(wf24_stream_start_live(&s,0x04));assert(f.render_calls==2&&f.copy_calls==2&&f.prep==1&&f.start==1);assert(wf24_stream_service(&s,1000));assert(f.render_calls==2);assert(wf24_stream_service(&s,1000));assert(f.render_calls==3&&f.copy_calls==3);assert(wf24_stream_service(&s,1000));assert(f.render_calls==3);assert(wf24_stream_service(&s,1000));assert(f.render_calls==4);assert(wf24_stream_stop(&s));assert(f.stop==1);return 0;}
