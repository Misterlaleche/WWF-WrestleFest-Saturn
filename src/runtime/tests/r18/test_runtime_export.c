#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_runtime_handoff.h"
static uint8_t original[0x4000],out[0x4000],before[0x4000];
static uint32_t random_word(uint32_t *s){*s=*s*1664525u+1013904223u;return *s;}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int main(void){
 wf_runtime_t r;wf_r17_m68k_shadow_t regs={.valid=1};uint32_t seed=0xf8c122cu;
 memset(&r,0,sizeof r);
 for(unsigned n=0;n<64u;n++){
  for(unsigned i=0;i<sizeof original;i++)original[i]=(uint8_t)(random_word(&seed)>>24);
  for(unsigned i=0;i<8u;i++)regs.d[i]=random_word(&seed);
  assert(wf_r18_runtime_import(&r,original,sizeof original,&regs)==WF_OK);
  memcpy(out,original,sizeof out);
  assert(wf_r18_runtime_export(&r,out,sizeof out)==WF_OK);
  assert(!memcmp(out,original,sizeof out));
 }
 /* A post-combat export must keep cabinet records, roster/history and bytes
  * outside the typed view, while publishing live frame/RNG/player state. */
 r.g.frame_counter=0x12345678u;r.g.global_0082=0xaa;r.g.pipeline_mode=0xbb;
 r.g.rng_state=0x1badc0deu;r.g.credit_count_004f=7;
 r.g.global_007e=5;r.g.global_007f=0xff;r.g.global_169a=0x4567;r.g.global_169b=0xff;
 r.g.royal_selector_1576[0x10]=0xff;r.g.royal_slots_1586[0]=0x2345;
 r.g.royal_selector_1576[0x58]=0xff;r.g.global_15ce=0x6789;
 wf_set_be16(&r.actors[0],WF_A_STAMINA,12);
 r.g.global_1150=0x8003;wf_set_be16(&r.fixed_four_1134,0x1c,0x7777);
 assert(wf_r18_runtime_export(&r,out,sizeof out)==WF_OK);
 assert(be32(out+0x80)==0x12345678u&&be32(out+0x5c)==0x1badc0deu);
 assert(out[0x4f]==7&&out[0x7e]==0&&out[0x7f]==5);
 assert(out[0x169a]==0x45&&out[0x169b]==0x67);
 assert(out[0x1586]==0x23&&out[0x1587]==0x45&&out[0x15ce]==0x67&&out[0x15cf]==0x89);
 assert(out[0x5b0+WF_A_STAMINA]==0&&out[0x5b1+WF_A_STAMINA]==12);
 assert(out[0x1150]==0x80&&out[0x1151]==3);
 assert(!memcmp(out+0xc2,original+0xc2,0x94));
 assert(!memcmp(out+0x168,original+0x168,0x438));
 assert(!memcmp(out+0x1cd2,original+0x1cd2,0x4000-0x1cd2));
 /* No partial export on invalid contracts. */
 memcpy(before,out,sizeof before);
 assert(wf_r18_runtime_export(0,out,sizeof out)==WF_BAD_STATE);
 assert(wf_r18_runtime_export(&r,0,sizeof out)==WF_BAD_STATE);
 assert(wf_r18_runtime_export(&r,out,sizeof out-1u)==WF_BAD_STATE);
 r.m68k_regs.valid=0;
 assert(wf_r18_runtime_export(&r,out,sizeof out)==WF_BAD_STATE);
 assert(!memcmp(before,out,sizeof out));
 puts("PASS runtime export:64 RAM round trips, live combat state, aliases, retained cabinet/records and guards");return 0;
}
