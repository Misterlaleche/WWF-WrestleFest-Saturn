/* Batch transport only. All game behavior is compiled from production sources. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_runtime_handoff.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_r17_rng1129e.h"
#include "wf_r17_human_raw23.h"
static wf_runtime_t runtime;
static unsigned get32(const unsigned char*p){return ((unsigned)p[0]<<24)|((unsigned)p[1]<<16)|((unsigned)p[2]<<8)|p[3];}
static void put32(unsigned char*p,unsigned x){p[0]=(unsigned char)(x>>24);p[1]=(unsigned char)(x>>16);p[2]=(unsigned char)(x>>8);p[3]=(unsigned char)x;}
int main(int argc,char **argv){
 assert(argc==3);FILE*in=fopen(argv[1],"rb"),*out=fopen(argv[2],"wb");assert(in&&out);
 unsigned char input[40+0x4000],output[44+0x4000],raw[0x4000];size_t n;unsigned cases=0;
 while((n=fread(input,1,sizeof input,in))!=0){
  assert(n==sizeof input);unsigned op=get32(input),slot=get32(input+4);assert(slot<WF_ACTOR_SLOTS);
  wf_r17_m68k_shadow_t regs={.valid=1};for(unsigned i=0;i<8;i++)regs.d[i]=get32(input+8+4*i);
  memcpy(raw,input+40,sizeof raw);memset(&runtime,0,sizeof runtime);
  assert(wf_r18_runtime_import(&runtime,raw,sizeof raw,&regs)==WF_OK);
  assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);assert(!memcmp(raw,input+40,sizeof raw));
  wf_status_t st=WF_BAD_STATE;unsigned dep=0;int carry=0;
  if(op==0)st=wf_r17_resolver_280dc(&runtime,&runtime.actors[slot]);
  else if(op==1){
   wf_r17_1129e_trace_t tr;
   st=wf_r17_1129e_live(&runtime,slot,runtime.g.mode_0161,runtime.g.progression_0162,
       &runtime.g.rng_state,runtime.g.frame_counter,&runtime.m68k_regs,&tr,&carry);dep=tr.blocked_dependency_rom;
  }else if(op==2){
   wf_r17_human_raw23_trace_t tr;wf_r17_human_raw23_ctx_t ctx={0};
   ctx.mode_0161=runtime.g.mode_0161;ctx.global_1670=&runtime.g.global_1670;
   ctx.fixed_object0=&runtime.fixed_object0;ctx.fixed_object1=&runtime.fixed_object1;
   ctx.global_0162=runtime.g.progression_0162;ctx.m68k_live=&runtime.m68k_regs;
   ctx.rng_state_1c005c=&runtime.g.rng_state;ctx.global_1c0080=runtime.g.frame_counter;
   st=wf_r17_human_dcde_raw23(&runtime,slot,&ctx,&tr);dep=tr.blocked_dependency_rom;
  }else assert(0);
  assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
  put32(output,(unsigned)st);put32(output+4,dep);put32(output+8,(unsigned)carry);
  for(unsigned i=0;i<8;i++)put32(output+12+4*i,runtime.m68k_regs.d[i]);
  memcpy(output+44,raw,sizeof raw);assert(fwrite(output,1,sizeof output,out)==sizeof output);cases++;
 }
 assert(!ferror(in));assert(!fclose(in));assert(!fclose(out));printf("%u records\n",cases);return 0;
}
