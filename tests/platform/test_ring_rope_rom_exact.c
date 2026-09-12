#include <assert.h>
#include <stdio.h>
#include "../../src/runtime/ring_rope_rom_exact.h"

static void expect_static(wwf_rope_quartet_t *q){assert(q->r[0].selector==2);assert(q->r[1].selector==10);assert(q->r[2].selector==2);assert(q->r[3].selector==10);}
static unsigned run_until_static(wwf_rope_quartet_t *q,unsigned side,unsigned state){unsigned n=0,b=side?2U:0U;wwf_rope_write_pair_state(q,side,state);do{wwf_rope_step(q);n++;assert(n<100);}while((q->r[b].state_word&15U)!=0);wwf_rope_step(q);n++;return n;}
int main(void){
 wwf_rope_quartet_t q;wwf_rope_quartet_reset(&q);wwf_rope_step(&q);expect_static(&q);
 assert(q.r[0].world_x==0x1c0&&q.r[2].world_x==0x330);assert(q.r[0].orient==0&&q.r[2].orient==0x8000);
 wwf_rope_write_pair_state(&q,WWF_ROPE_LEFT,1);wwf_rope_step(&q);assert(q.r[0].selector==3&&q.r[1].selector==7);
 for(unsigned i=0;i<6;i++){wwf_rope_step(&q);}
 assert(q.r[0].selector==4&&q.r[1].selector==8);
 while((q.r[0].state_word&15U)!=0){wwf_rope_step(&q);}
 assert(q.r[0].selector==4&&q.r[1].selector==8);
 wwf_rope_step(&q);assert(q.r[0].selector==2&&q.r[1].selector==10);
 wwf_rope_quartet_reset(&q);wwf_rope_step(&q);assert(run_until_static(&q,WWF_ROPE_RIGHT,1)==38U);expect_static(&q);
 assert(run_until_static(&q,WWF_ROPE_LEFT,2)==48U);expect_static(&q);
 wwf_rope_write_state(&q,0,3);wwf_rope_step(&q);assert(q.r[0].selector==4);assert(q.r[1].selector==10);assert(q.r[2].selector==2);assert(q.r[3].selector==10);
 while((q.r[0].state_word&15U)!=0){wwf_rope_step(&q);}
 wwf_rope_step(&q);expect_static(&q);
 wwf_rope_write_state(&q,2,4);wwf_rope_step(&q);assert(q.r[2].selector==6);assert(q.r[0].selector==2);assert(q.r[1].selector==10);assert(q.r[3].selector==10);
 while((q.r[2].state_word&15U)!=0){wwf_rope_step(&q);}
 wwf_rope_step(&q);expect_static(&q);
 wwf_rope_write_state(&q,0,3);unsigned n3=0;do{wwf_rope_step(&q);n3++;}while((q.r[0].state_word&15U)!=0);assert(n3==27U);wwf_rope_step(&q);
 wwf_rope_write_state(&q,2,4);unsigned n4=0;do{wwf_rope_step(&q);n4++;}while((q.r[2].state_word&15U)!=0);assert(n4==27U);wwf_rope_step(&q);expect_static(&q);
 puts("PASS ring_rope_rom_exact: quartet identity, state tables, cadence, terminal hold, restore, no side aliasing");return 0;
}
