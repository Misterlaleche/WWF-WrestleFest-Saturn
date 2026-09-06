#include "arcade_attract.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
int main(void){
 static const unsigned dur[14]={848,736,1040,928,656,928,1200,944,1616,1152,800,816,832,864};
 static const int feat[14]={-1,-1,0,1,2,3,6,7,8,9,10,11,4,5};
 static const uint8_t roster4[14][4]={{0,1,4,5},{3,6,7,8},{0,1,6,7},{1,0,9,8},{2,3,10,11},{3,2,6,9},{6,7,0,1},{7,9,2,0},{8,6,3,2},{9,8,1,3},{10,11,2,1},{11,10,2,1},{4,5,0,1},{5,4,0,1}};
 static const uint8_t royal6[6]={3,6,7,8,9,2};
 FILE*f=fopen("program.bin","rb");assert(f);uint8_t rom[0x8eb6];assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 arcade_attract_t a;arcade_attract_reset(&a);
 for(unsigned s=0;s<14;s++){
  const arcade_attract_state_desc_t*d=arcade_attract_current(&a);assert(d);
  assert(d->duration==dur[s]&&be16(rom + 0x119a + s*2)==dur[s]);
  assert(!memcmp(rom + 0x0f3e + s*4,roster4[s],4));
  if(s==1){
   assert(d->participant_count==6);
   assert(!memcmp(d->roster,royal6,6));
   assert(!memcmp(rom + 0x0f76,royal6,6));
   static const uint8_t state1branch[]={0x0c,0x01,0x00,0x01};
   static const uint8_t sixloop[]={0x30,0x3c,0x00,0x05};
   assert(!memcmp(rom + 0x0e2c,state1branch,sizeof state1branch));
   assert(!memcmp(rom + 0x0ede,sixloop,sizeof sixloop));
  }else{
   assert(d->participant_count==4);
   assert(!memcmp(d->roster,roster4[s],4));
  }
  uint16_t rf=be16(rom + 0x8e9a + s*2);int romfeat=rf==0xffff?-1:(int)rf;assert(d->featured_character==feat[s]&&romfeat==feat[s]);
  if(d->duration>1)assert(!arcade_attract_tick(&a,d->duration-1));
  assert(arcade_attract_tick(&a,1));
  assert(a.state==(s+1)%14&&a.tick==0);
 }
 puts("PASS Test255: 14 attract durations/rosters/portraits match ROM; Royal state 1 uses exact six-actor $0F76 roster");return 0;
}
