/* Batch transport; executes the real production prepass, without callbacks replaced. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_actor_prepass.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_runtime_handoff.h"
static wf_runtime_t runtime;
static unsigned get32(const unsigned char*p){return ((unsigned)p[0]<<24)|((unsigned)p[1]<<16)|((unsigned)p[2]<<8)|p[3];}
static void put32(unsigned char*p,unsigned x){p[0]=(unsigned char)(x>>24);p[1]=(unsigned char)(x>>16);p[2]=(unsigned char)(x>>8);p[3]=(unsigned char)x;}
int main(int argc,char**argv){
 assert(argc==3);FILE*in=fopen(argv[1],"rb"),*out=fopen(argv[2],"wb");assert(in&&out);
 unsigned char input[40+0x4000],output[44+0x4000],raw[0x4000];size_t n;unsigned cases=0;
 while((n=fread(input,1,sizeof input,in))!=0){
  assert(n==sizeof input);wf_r17_m68k_shadow_t regs={.valid=1};
  for(unsigned i=0;i<8;i++)regs.d[i]=get32(input+8+4*i);
  memcpy(raw,input+40,sizeof raw);memset(&runtime,0,sizeof runtime);
  assert(wf_r18_runtime_import(&runtime,raw,sizeof raw,&regs)==WF_OK);
  assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);assert(!memcmp(raw,input+40,sizeof raw));
  wf_gameplay_binding_context_t context={0};
  wf_status_t status=wf_phase_actor_prepass_f18a(&runtime,0xf18au,&context);
  assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
  put32(output,(unsigned)status);put32(output+4,context.last_blocked_phase);put32(output+8,context.last_blocked_dependency);
  for(unsigned i=0;i<8;i++)put32(output+12+4*i,runtime.m68k_regs.d[i]);
  memcpy(output+44,raw,sizeof raw);assert(fwrite(output,1,sizeof output,out)==sizeof output);cases++;
 }
 assert(!ferror(in));assert(!fclose(in));assert(!fclose(out));printf("%u records\n",cases);return 0;
}
