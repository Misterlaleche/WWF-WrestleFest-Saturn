#include "../native/wf_music_clock.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
 uint16_t previous=100;
 assert(wf_music_clock_delta(&previous,101)==1&&previous==101);
 previous=0xffffu;
 assert(wf_music_clock_delta(&previous,0)==1&&previous==0);
 previous=0x8785u;
 assert(wf_music_clock_delta(&previous,0x0783u)==32766u&&previous==0x0783u);
 assert(wf_music_clock_due(0x0001u,0xffffu)==2);
 assert(wf_music_clock_due(0xffffu,0x0001u)==-2);
 puts("MUSIC_CLOCK_PASS");
 return 0;
}
