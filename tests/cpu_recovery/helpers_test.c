#include "wf_actor.h"
#include "wf_r17_m68k_shadow.h"
#include "wf_jo_clock.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define DECL(p) \
 uint8_t p##get8(const wf_actor_raw_t *,size_t); \
 uint16_t p##get16(const wf_actor_raw_t *,size_t); \
 uint32_t p##get32(const wf_actor_raw_t *,size_t); \
 void p##set8(wf_actor_raw_t *,size_t,uint8_t); \
 void p##set16(wf_actor_raw_t *,size_t,uint16_t); \
 void p##set32(wf_actor_raw_t *,size_t,uint32_t); \
 void p##move(wf_r17_m68k_shadow_t *,unsigned,unsigned,uint32_t); \
 size_t p##layout(size_t *)
DECL(reference_);DECL(candidate_);
int reference_wf_jo_clock_advance(wf_jo_clock_t *,uint32_t);
void reference_wf_jo_clock_start(wf_jo_clock_t *,uint32_t);
int reference_wf_jo_clock_take(wf_jo_clock_t *);
static uint32_t seed=0xa17034deu;
static uint32_t rng(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint64_t actor_cases,shadow_cases,clock_cases;
static void actors(void){
 struct guarded{uint32_t before[2];wf_actor_raw_t a;uint32_t after[2];} x,y;
 size_t rl[64],cl[64];size_t rn=reference_layout(rl),cn=candidate_layout(cl);
 assert(rn==cn&&!memcmp(rl,cl,rn*sizeof *rl));
 for(unsigned trial=0;trial<1024;trial++){
  for(size_t i=0;i<sizeof x;i++)((uint8_t*)&x)[i]=(uint8_t)rng();y=x;
  for(size_t o=0;o<WF_ACTOR_BYTES;o++){
   assert(reference_get8(&x.a,o)==candidate_get8(&y.a,o));
   uint32_t v=rng();reference_set8(&x.a,o,(uint8_t)v);candidate_set8(&y.a,o,(uint8_t)v);actor_cases++;
   assert(!memcmp(&x,&y,sizeof x));
   if(o+2<=WF_ACTOR_BYTES){assert(reference_get16(&x.a,o)==candidate_get16(&y.a,o));
    reference_set16(&x.a,o,(uint16_t)v);candidate_set16(&y.a,o,(uint16_t)v);actor_cases++;assert(!memcmp(&x,&y,sizeof x));}
   if(o+4<=WF_ACTOR_BYTES){assert(reference_get32(&x.a,o)==candidate_get32(&y.a,o));
    reference_set32(&x.a,o,v);candidate_set32(&y.a,o,v);actor_cases++;assert(!memcmp(&x,&y,sizeof x));}
  }
 }
 printf("ABI: actor=%zu runtime=%zu bridge=%zu, %zu offsets exact\n",rl[0],rl[1],rl[2],rn-3);
}
typedef void (*word_move_fn)(wf_r17_m68k_shadow_t *,unsigned,uint16_t);
word_move_fn candidate_word_move_address(void);
static void shadows(void){
 assert(candidate_word_move_address()==wf_r17_m68k_move_w);
 const unsigned regs[]={0,1,2,3,4,5,6,7,8,9,255,UINT32_MAX};
 for(unsigned n=0;n<4096;n++)for(unsigned op=0;op<6;op++)for(unsigned j=0;j<sizeof regs/sizeof regs[0];j++){
  wf_r17_m68k_shadow_t a,b;
  for(size_t k=0;k<sizeof a;k++)((uint8_t*)&a)[k]=(uint8_t)rng();b=a;
  uint32_t v=rng();reference_move(&a,op,regs[j],v);candidate_move(&b,op,regs[j],v);assert(!memcmp(&a,&b,sizeof a));
  reference_move(NULL,op,regs[j],v);candidate_move(NULL,op,regs[j],v);shadow_cases+=2;
 }
}
static void clock_case(uint32_t rem,uint32_t elapsed,uint32_t pending,uint32_t max){
 wf_jo_clock_t a={0xfffffff0u,rem,pending,max},b=a;
 int ar=reference_wf_jo_clock_advance(&a,a.last_field+elapsed),br=wf_jo_clock_advance(&b,b.last_field+elapsed);
 assert(ar==br&&!memcmp(&a,&b,sizeof a));
 assert(reference_wf_jo_clock_take(&a)==wf_jo_clock_take(&b)&&!memcmp(&a,&b,sizeof a));clock_cases++;
}
static void clocks(void){
 for(uint32_t r=0;r<39168;r++)for(uint32_t e=0;e<=26;e++)clock_case(r,e,e==25?UINT32_MAX-24:10,5);
 const uint32_t es[]={0,1,25,26,27,100,1000000,UINT32_MAX};
 const uint32_t rs[]={0,39167,39168,39169,UINT32_MAX};
 const uint32_t ps[]={0,UINT32_MAX-25,UINT32_MAX-1,UINT32_MAX};
 for(unsigned i=0;i<sizeof es/sizeof es[0];i++)for(unsigned j=0;j<sizeof rs/sizeof rs[0];j++)for(unsigned k=0;k<sizeof ps/sizeof ps[0];k++)clock_case(rs[j],es[i],ps[k],ps[k]);
 for(unsigned i=0;i<100000;i++)clock_case(rng(),rng(),rng(),rng());
 wf_jo_clock_t a,b;memset(&a,0xa5,sizeof a);b=a;reference_wf_jo_clock_start(&a,123);wf_jo_clock_start(&b,123);assert(!memcmp(&a,&b,sizeof a));
}
int main(void){actors();shadows();clocks();printf("PASS actors=%llu shadow=%llu clock=%llu\n",(unsigned long long)actor_cases,(unsigned long long)shadow_cases,(unsigned long long)clock_cases);return 0;}
