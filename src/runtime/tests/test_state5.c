#include <assert.h>
#include <stdio.h>
#include "wf_state5.h"
int main(void){unsigned forced=0,rts=0;for(unsigned i=0;i<143;i++){const wf_state5_map_t*m=wf_state5_lookup((uint8_t)i);assert(m);forced+=m->forced_bit31?1u:0u;rts+=(m->callback_rom==0x1E254u)?1u:0u;}assert(forced==19u);assert(rts==74u);assert(wf_state5_lookup(143)==0);puts("PASS test_state5");return 0;}
