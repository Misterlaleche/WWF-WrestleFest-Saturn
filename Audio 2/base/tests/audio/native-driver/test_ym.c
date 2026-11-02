#include "wf_scsp_ym.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static uint16_t regs[32][16];
static bool put(uint32_t a,uint16_t v,void *u){(void)u;assert(a>=0x100000&&a<0x100400&&!(a&1));regs[(a-0x100000)/32][((a-0x100000)%32)/2]=v;return true;}
static unsigned sources(unsigned slot){unsigned w=regs[slot][7];return w>>12;}
int main(void){
 wf_scsp_ym_t s;
 assert(wf_ym_phase_increment(0x55,0,0x61,0)==13088);
 assert(wf_ym_phase_increment(0x55,0,0x21,0)==13104);
 assert(wf_ym_phase_increment(0x55,0,0x41,0)==13096);
 for(uint32_t inc=162;inc<700000;inc+=97){
  uint16_t p=wf_ym_pitch_word(inc);int oct=(p>>11)&15;if(oct&8)oct-=16;
  double target=(double)inc*3579545/(32.0*1048576),hz=(44100.0/128)*pow(2.0,oct)*(1+(p&1023)/1024.0);
  assert(fabs(1200*log(hz/target)/log(2.0))<2.0);
 }
 wf_scsp_ym_init(&s,put,NULL);assert(!s.fault);
 for(unsigned ch=0;ch<8;ch++){
  assert(wf_scsp_ym_write(&s,(uint8_t)(0x20+ch),0xc4));
  assert(wf_scsp_ym_write(&s,(uint8_t)(0x28+ch),0x55));
  for(unsigned op=0;op<4;op++){
   assert(wf_scsp_ym_write(&s,(uint8_t)(0x40+op*8+ch),1));
   assert(wf_scsp_ym_write(&s,(uint8_t)(0x60+op*8+ch),12));
   assert(wf_scsp_ym_write(&s,(uint8_t)(0x80+op*8+ch),31));
  }
  assert(wf_scsp_ym_write(&s,8,(uint8_t)(0x78|ch)));
 }
 for(unsigned slot=0;slot<32;slot++){assert(regs[slot][0]&0x800);assert(regs[slot][3]==128);assert((regs[slot][6]&255)==24);if(slot<4)assert((regs[slot][11]&255)==0xe0);}
 /* Max modulation is avg32768*1024>>(26-MDL), plus interpolation.
  * MDL12 dual-input sums need16 complete128-sample guard cycles. */
 for(unsigned mdl=0;mdl<=12;mdl++){
  unsigned offset=(32768u*1024u)>>(26u-mdl);
  assert(18u*128u>=offset&&18u*128u+128u+offset+1u<36u*128u);
 }
 for(unsigned alg=0;alg<8;alg++){
  static const unsigned carrier[8]={8,8,8,8,10,14,14,15};
  assert(wf_scsp_ym_write(&s,0x20,(uint8_t)(0xc0|alg)));
  for(unsigned op=0;op<4;op++){
   assert((regs[op][11]>>13)==0);
   assert(((regs[op][10]&7u)!=0)==((carrier[alg]&(1u<<op))!=0));
   if(carrier[alg]&(1u<<op))assert(regs[op][10]==((8u<<3)|5u));
  }
 }
 assert(wf_scsp_ym_write(&s,0x20,0xc4));assert(sources(1)==11&&sources(2)==0&&sources(3)==11);
 assert(wf_scsp_ym_write(&s,0xa0,8));
 assert(((regs[0][4]>>6)&31u)==10u); /* measured decay map at effective18 */
 uint32_t before=s.writes;
 assert(wf_scsp_ym_write(&s,0x20,0xc4)&&s.writes==before);
 assert(wf_scsp_ym_write(&s,8,0)&&s.writes>before);
 before=s.writes;assert(wf_scsp_ym_write(&s,8,0)&&s.writes==before);
 wf_scsp_ym_stop(&s);for(unsigned slot=0;slot<32;slot++)assert(!(regs[slot][0]&0x800));
 puts("YM SCSP: eight channels, algorithms, pitch, TL, routing, key-off and DSP sends PASS");return 0;
}
