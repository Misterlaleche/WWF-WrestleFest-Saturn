#include "arcade_attract.h"
#include <assert.h>
#include <stdio.h>
int main(void){static const unsigned dur[14]={848,736,1040,928,656,928,1200,944,1616,1152,800,816,832,864};static const int feat[14]={-1,-1,0,1,2,3,6,7,8,9,10,11,4,5};arcade_attract_t a;arcade_attract_reset(&a);for(unsigned s=0;s<14;s++){const arcade_attract_state_desc_t*d=arcade_attract_current(&a);assert(d&&d->duration==dur[s]&&d->featured_character==feat[s]);if(d->duration>1)assert(!arcade_attract_tick(&a,d->duration-1));assert(arcade_attract_tick(&a,1));assert(a.state==(s+1)%14&&a.tick==0);}puts("PASS Test255: ROM-recovered 14-state attract durations and protagonist order are executable and wrap exactly");return 0;}
