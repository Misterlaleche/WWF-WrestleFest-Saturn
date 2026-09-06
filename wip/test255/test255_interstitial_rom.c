#include "arcade_interstitial.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
int main(void){
 FILE*f=fopen("program.bin","rb");assert(f);uint8_t*b=malloc(0xB29C);assert(b);assert(fread(b,1,0xB29C,f)==0xB29C);fclose(f);
 static const uint8_t setup[]={0x30,0x3c,0x00,0x01,0x0c,0x79,0x00,0x01,0x00,0x1c,0x01,0x66};
 assert(!memcmp(b+0x0b1c,setup,sizeof setup));
 assert(be16(b+0xB14A)==0x3179); assert(be16(b+0xB14C)==0x316A); assert(be16(b+0xB14E)==0x316B); assert(be16(b+0xB150)==0x3178);
 const uint32_t ptr1=(b[0xB1A0]<<24)|(b[0xB1A1]<<16)|(b[0xB1A2]<<8)|b[0xB1A3];
 const uint32_t ptr2=(b[0xB1A4]<<24)|(b[0xB1A5]<<16)|(b[0xB1A6]<<8)|b[0xB1A7];
 assert(ptr1==0xB1E2 && ptr2==0xB25E);
 const uint32_t starts[2]={0xB1E2,0xB25E};
 const uint16_t commands[2]={0x316A,0x316B};
 const uint8_t counts[2]={62,30}; const uint16_t ticks[2]={707,431};
 for(unsigned s=0;s<2;s++){
  const arcade_interstitial_desc_t*d=&arcade_interstitials[s]; assert(d->voice_command==commands[s]);assert(d->phase_count==counts[s]);assert(d->active_ticks==ticks[s]);assert(d->attract_entry_state==s+1);
  uint32_t off=starts[s];unsigned sum=0,ev=0;
  for(unsigned i=0;i<d->phase_count;i++){
   uint8_t raw=b[off++],dur=b[off++];assert(d->phases[i].frame==(raw&0x7f));assert(d->phases[i].duration==dur);assert(d->phases[i].audio_event==!!(raw&0x80));
   if(raw&0x80)ev++;
   if(dur!=0xfe)sum+=dur;
  }
  assert(ev==1 && sum==ticks[s] && d->phases[d->phase_count-1].duration==0xfe);
 }
 assert(arcade_interstitials[0].phases[3].audio_event && arcade_interstitials[0].phases[3].frame==0x0d);
 assert(arcade_interstitials[1].phases[1].audio_event && arcade_interstitials[1].phases[1].frame==0x03);
 free(b);puts("PASS Test255: two state-entry presentation scripts, exact phase timing and 0x316A/0x316B voice events match ROM");return 0;
}
