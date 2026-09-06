#include "arcade_interstitial.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
int main(void){
 FILE*f=fopen("program.bin","rb");assert(f);uint8_t*b=malloc(0xB300);assert(b);assert(fread(b,1,0xB300,f)==0xB300);fclose(f);
 /* $AE20 creates three set-$2C objects. $B0C2 supplies x=48/98/F8. */
 assert(!memcmp(b+0xAEEC,(const uint8_t[]){0x11,0x7c,0x00,0x2c,0x00,0x03},6));
 assert(b[0xB0C2]==0x48&&b[0xB0C3]==0x98&&b[0xB0C4]==0xF8);
 for(unsigned i=0;i<3;i++)assert(arcade_interstitial_objects[i].x==b[0xB0C2+i]);
 /* Three palette resources are loaded as IDs $31,$32,$33. */
 assert(!memcmp(b+0xAF08,(const uint8_t[]){0x30,0x01,0x06,0x40,0x00,0x31,0x4e,0xb9,0x00,0x00,0x2a,0xea},12));
 assert(arcade_interstitial_objects[0].palette_bank==0x33);
 assert(arcade_interstitial_objects[1].palette_bank==0x32);
 assert(arcade_interstitial_objects[2].palette_bank==0x31);
 /* B19C is an object-script table: left, centre, right. All three run. */
 const uint32_t starts[3]={0xB1A8,0xB1E2,0xB25E};
 const uint8_t counts[3]={29,62,30};const uint16_t ticks[3]={353,707,431};
 for(unsigned i=0;i<3;i++)assert(be32(b+0xB19C+i*4)==starts[i]);
 for(unsigned s=0;s<3;s++){
  const arcade_interstitial_object_t*d=&arcade_interstitial_objects[s];assert(d->object_index==s);assert(d->phase_count==counts[s]);assert(d->active_ticks==ticks[s]);
  uint32_t off=starts[s];unsigned sum=0;
  for(unsigned i=0;i<d->phase_count;i++){
   uint8_t raw=b[off++],dur=b[off++];assert(d->phases[i].frame==(raw&0x7f));assert(d->phases[i].duration==dur);assert(d->phases[i].audio_event==!!(raw&0x80));if(dur!=0xfe)sum+=dur;
  }
  assert(sum==ticks[s]&&d->phases[d->phase_count-1].duration==0xfe);
 }
 /* Exact graphics-family split from set $2C scripts. */
 for(unsigned i=0;i<29;i++)assert(arcade_interstitial_objects[0].phases[i].frame>=0x16&&arcade_interstitial_objects[0].phases[i].frame<=0x1e);
 for(unsigned i=0;i<62;i++){unsigned fr=arcade_interstitial_objects[1].phases[i].frame;assert((fr>=0x0a&&fr<=0x15)||fr==0x1f);}
 for(unsigned i=0;i<30;i++)assert(arcade_interstitial_objects[2].phases[i].frame<=0x09);
 /* Mode 1 flags the right object; mode 2 flags the left object. */
 assert(!memcmp(b+0xAFC2,(const uint8_t[]){0x33,0xfc,0x00,0x02,0x00,0x1c,0x12,0x76,0x08,0xf9,0x00,0x00,0x00,0x1c,0x12,0xc9},16));
 assert(!memcmp(b+0xAFD4,(const uint8_t[]){0x33,0xfc,0x00,0x04,0x00,0x1c,0x12,0x76,0x08,0xf9,0x00,0x00,0x00,0x1c,0x12,0x75},16));
 assert(arcade_interstitial_scenes[0].mode==1&&arcade_interstitial_scenes[0].attract_entry_state==1&&arcade_interstitial_scenes[0].speaking_object==2&&arcade_interstitial_scenes[0].initial_audio_table_offset==2);
 assert(arcade_interstitial_scenes[1].mode==2&&arcade_interstitial_scenes[1].attract_entry_state==2&&arcade_interstitial_scenes[1].speaking_object==0&&arcade_interstitial_scenes[1].initial_audio_table_offset==4);
 for(unsigned i=0;i<4;i++)assert(arcade_interstitial_audio_commands[i]==be16(b+0xB14A+i*2));
 free(b);puts("PASS Test255: AE20 three-object Demolition/presenter composition, scripts, speaker-side flags and audio cursor are ROM-exact");return 0;
}
