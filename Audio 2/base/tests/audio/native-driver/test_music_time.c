#include "wf_music_time.h"
#include <assert.h>
#include <stdio.h>
static void check(wf_music_time_t *t,uint64_t cycle){
 uint16_t actual=wf_music_time_advance(t,cycle);
 assert(actual==(uint16_t)(cycle*44100u/3579545u));
 assert(t->fraction==(uint32_t)(cycle*44100u%3579545u));
}
int main(void){
 wf_music_time_t t={0};uint32_t random=1;uint64_t cycle=0;
 check(&t,0);check(&t,90000);check(&t,180001);cycle=180001;
 for(unsigned i=0;i<100000;i++){
  random=random*1664525u+1013904223u;cycle+=random%200001u;check(&t,cycle);
 }
 /* Multiple16-bit frame wraps and a gap far beyond the32-bit fast path. */
 cycle+=1000000000000ull;check(&t,cycle);check(&t,cycle);
 puts("music time:100000 incremental gaps, remainder and frame wraps PASS");return 0;
}
