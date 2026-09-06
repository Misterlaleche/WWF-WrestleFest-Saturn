#include "arcade_interstitial.h"
#include <assert.h>
#include <stdio.h>
static void run(unsigned scene,const uint16_t *want,unsigned want_n,unsigned exact_calls){
 arcade_interstitial_player_t p;arcade_interstitial_player_begin(&p,scene);assert(p.active);
 unsigned calls=0,n=0;uint16_t got[4]={0};
 while(p.active&&calls<2000){uint16_t c=arcade_interstitial_player_step(&p,scene);calls++;if(c)got[n++]=c;}
 assert(!p.active);assert(calls==exact_calls);assert(n==want_n);for(unsigned i=0;i<n;i++)assert(got[i]==want[i]);
}
int main(void){
 /* mode1 flags object2/right: one marker -> table offset 2 => $316A. */
 const uint16_t a[]={0x316a};run(0,a,1,arcade_interstitial_objects[2].update_calls_to_terminal);
 /* mode2 flags object0/left: two markers -> offsets 4/6 => $316B,$3178. */
 const uint16_t b[]={0x316b,0x3178};run(1,b,2,arcade_interstitial_objects[0].update_calls_to_terminal);
 puts("PASS Test255: native $B0C8 speaker-script mirror consumes exact mode1/mode2 audio event sequences");return 0;
}
