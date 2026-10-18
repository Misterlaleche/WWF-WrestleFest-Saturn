/* Include the owner directly to exercise its private native helper without adding production test APIs. */
#include "../../src/wf_r17_referee.c"
#include "wf_gameplay_bindings.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){wf_runtime_t r;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];wf_r17_referee_ctx_t rc={0};int mode=argc>1?atoi(argv[1]):0;size_t n=wf_gameplay_bindings_build(&c,b,40);wf_runtime_init(&r,b,n);for(unsigned i=0;i<8;i++)r.m68k_regs.d[i]=0xabc00000+i*0x1111;
 if(mode<2){for(unsigned i=0;i<2;i++){r.actors[i].b[0]=128;r.actors[i].b[0x33]=1;wf_set_be16(&r.actors[i],6,0x200+0x40*i);wf_set_be16(&r.actors[i],10,0x180-0x20*i);}if(mode)r.actors[0].b[0x33]|=64;assert(target_204b2(&r,&r.referee,&rc)==WF_OK);}
 else{r.g.credited_route=1;r.g.mode_0161=1;r.g.config_0066=8;r.g.credit_count_004f=mode==2?1:0;c.cabinet_words_valid=1;c.cabinet_word_140020=c.cabinet_word_140022=0xffff;r.actors[3].b[0xa2]=1;r.actors[3].b[0xa1]=1;wf_set_be16(&r.actors[3],0x72,100);assert(wf_phase_refill_8b3a(&r,0x8b3a,&c)==WF_OK);}
 for(unsigned i=0;i<8;i++)printf("%08x%c",r.m68k_regs.d[i],i==7?'\n':' ');
 if(mode<2){for(unsigned i=0;i<WF_ACTOR_BYTES;i++)printf("%02x",r.referee.b[i]);puts("");assert(r.m68k_regs.d[1]==(mode?8u:0x160u));}
 else{assert(r.m68k_regs.d[0]==3);printf("%u %u\n",r.g.credit_count_004f,wf_be16(&r.actors[3],0x66));}
 return 0;}
