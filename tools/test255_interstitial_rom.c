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
 /* Exact script pointers at $B19C: left, centre, right. */
 assert(be32(b+0xB19C)==0xB1A8);assert(be32(b+0xB1A0)==0xB1E2);assert(be32(b+0xB1A4)==0xB25E);
 for(unsigned o=0;o<ARCADE_INTERSTITIAL_OBJECTS;o++){
  const arcade_interstitial_object_t*d=&arcade_interstitial_objects[o];unsigned ev=0,sum=0;for(unsignedi=0;i<d->phase_count;i++){sum+=d->phases[i].duration==0xfe?0:d->phases[i].duration;ev+=d->phases[i].audio_event;}assert(sum==d->raw_duration_sum);
 }
 /* $B0C8 issues BtST on object+$1D: only the selected speaker runs this script. */
 assert(!memcmp(b+0xB0C8,(const uint8_t[]){0x08,0x28,0x00,0x00,0x00,0x1d,0x67,0x00,0x00,0x78},10));
 /* Mode1 flags right object, mode2 flags left object. */
 assert(!memcmp(b+0xAF62,(const uint8_t[]){0x08,0x29,0x00,0x00,0x00,0x1d,0x30,0x3c},8));
 assert(!memcmp(b+0xAFB0,(const uint8_t[]){0x08,0x29,0x00,0x00,0x00,0x1d,0x30,0x3c},8));
 for(unsigned i=0;i<4;i++)assert(arcade_interstitial_audio_commands[i]==be16(b+0xB14A+i*2));
 /* Right/mode1 has one marker => $316A. Left/mode2 has two => $316B then $3178. */
 unsigned ev_right=0,ev_left=0;for(unsigned i=0;i<30;i++)ev_right+=arcade_interstitial_objects[2].phases[i].audio_event;for(unsigned i=0;i<29;i++)ev_left+=arcade_interstitial_objects[0].phases[i].audio_event;assert(ev_right==1&&ev_left==2);
 free(b);puts("PASS Test255: AE20 three-object Demolition/presenter composition, scripts, speaker-side flags and audio cursor are ROM-exact");return 0;
}
