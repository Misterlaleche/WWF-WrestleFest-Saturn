#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "wp02_audio_assets.h"
#ifdef GAIN_NOT_IMPLEMENTED
static bool wp02_apply_oki_gain_be(uint8_t *p,unsigned n,unsigned v){(void)p;(void)n;(void)v;return true;}
#else
#include "wp02_pcm16_gain.h"
#endif
static int get(const uint8_t *p){return (int16_t)(((uint16_t)p[0]<<8)|p[1]);}
int main(void){
 /* At raw PCM16 value1280, every original cue's expected output is exact. */
 static const int want[14]={240,880,640,440,440,440,440,440,440,440,440,640,640,640};
 for(unsigned i=0;i<14;i++){
  uint8_t sample[4]={0x05,0x00,0xFB,0x00};
  assert(wp02_apply_oki_gain_be(sample,4,wp02_pcm_assets[i].volume));
  assert(get(sample)==want[i] && get(sample+2)==-want[i]);
 }
 /* Signed half-LSB cases round to nearest even without introducing DC bias. */
 uint8_t edge[8]={0x00,0x10,0x00,0x30,0xFF,0xF0,0xFF,0xD0};
 assert(wp02_apply_oki_gain_be(edge,8,3));
 assert(get(edge)==6 && get(edge+2)==16 && get(edge+4)==-6 && get(edge+6)==-16);
 uint8_t full[4]={0x80,0x00,0x7F,0xF0};
 assert(wp02_apply_oki_gain_be(full,4,0));
 assert(get(full)==-32768 && get(full+2)==32752);
 assert(wp02_apply_oki_gain_be(full,4,15));
 assert(get(full)==0 && get(full+2)==0);
 assert(!wp02_apply_oki_gain_be(full,3,1));
 assert(!wp02_apply_oki_gain_be(full,4,16));
 puts("PASS: all14 original OKI gains, signed PCM16BE, unbiased rounding, no clipping");
 return 0;
}
