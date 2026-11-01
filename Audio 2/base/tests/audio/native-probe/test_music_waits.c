#define WF_MUSIC_MODULE_TEST 1
#include "wf_music_module.c"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
wf_music_module_shared_t wf_music_test_shared;
const uint8_t wf_music_sound_rom[0xc000]={0};
static uint16_t sound[0x100400/2];
static unsigned idles,action;
static jmp_buf unbounded;
uint16_t *wf_music_test_sound(unsigned a){assert(!(a&1u)&&a<sizeof(sound)*2);return sound+a/2;}
void wf_music_test_idle(void){
 ++idles;++SHARED.master_frame;
 if(idles==500)longjmp(unbounded,1);
 if(action==1&&idles==10)SOUND16(0xb02a)=generation;
 if(action==2&&idles==10)SOUND16(0xb022)=1;
}
bool wf_music_seq_init(wf_music_seq_t*a,const uint8_t*b,size_t c,wf_music_emit_fn d,void*e,bool f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return true;}
bool wf_music_seq_send(wf_music_seq_t*a,uint8_t b){(void)a;(void)b;return true;}
int wf_music_seq_run(wf_music_seq_t*a,uint64_t b,unsigned c){(void)a;(void)b;(void)c;return 0;}
void wf_scsp_ym_init(wf_scsp_ym_t*a,wf_sfx_write16_fn b,void*c){memset(a,0,sizeof *a);a->write16=b;a->user=c;}
bool wf_scsp_ym_write(wf_scsp_ym_t*a,uint8_t b,uint8_t c){(void)a;(void)b;(void)c;return true;}
void wf_scsp_ym_stop(wf_scsp_ym_t*a){(void)a;}
static void reset(void){
 memset(sound,0,sizeof sound);memset(&SHARED,0,sizeof SHARED);memset(&synth,0,sizeof synth);
 idles=action=0;generation=7;SOUND16(0xb026)=2;SHARED.master_frame=1000;
 fm_producer=fm_consumer=0;booting=false;
}
int main(void){
 if(setjmp(unbounded)){puts("RED: slave wait remains unbounded after 500 independent frames");return 1;}
 reset();assert(!generation_ready());assert(idles<=121&&idles>=120);
 reset();action=1;assert(generation_ready());assert(idles==10);
 reset();SHARED.master_frame=0xfff0u;assert(!generation_ready());assert(idles<=121&&idles>=120);
 reset();SOUND16(0xb020)=256;assert(!publish(0x100,0x80,0));assert(idles<=121);
 reset();action=2;SOUND16(0xb020)=256;assert(publish(0x100,0x80,0));assert(idles==10&&SOUND16(0xb020)==257);
 reset();fm_producer=512;fm_events[0].when=4096;
 assert(!synth_write(0x100000,0,NULL));assert(idles<=121);
 reset();fm_producer=512;fm_events[0].when=4096;SOUND16(0xb026)=3;
 assert(!synth_write(0x100000,0,NULL));assert(idles==0);
 reset();fm_producer=1;fm_events[0].when=100;SOUND16(0xb010)=2306;
 if(fm_flush()){puts("RED: missed FM deadline is silently cleared and the overloaded worker continues");return 1;}
 puts("PASS bounded slave waits: generation, full ring, full FM, FAULT, progress, frame wrap");
 return 0;
}
