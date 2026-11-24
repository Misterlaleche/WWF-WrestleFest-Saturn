/* Test-only translation unit: call real static leaves, no copied gameplay. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_runtime_handoff.h"
#ifndef WF_REF_SOURCE
#define WF_REF_SOURCE "../../core/src/wf_r17_referee.c"
#endif
#include WF_REF_SOURCE
static wf_runtime_t runtime;
static uint32_t get32(const unsigned char *p) {
 return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static void put32(unsigned char *p,uint32_t v) {
 p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;
}
int main(int argc,char **argv) {
 assert(argc==3);
 FILE *in=fopen(argv[1],"rb"),*out=fopen(argv[2],"wb");assert(in&&out);
 unsigned char input[36+0x4000],output[40+0x4000],raw[0x4000];
 size_t n;unsigned cases=0;
 while((n=fread(input,1,sizeof input,in))!=0) {
  assert(n==sizeof input);
  memset(&runtime,0,sizeof runtime);
  wf_r17_m68k_shadow_t regs={.valid=1};
  for(unsigned i=0;i<8;i++)regs.d[i]=get32(input+4+i*4);
  memcpy(raw,input+36,sizeof raw);
  assert(wf_r18_runtime_import(&runtime,raw,sizeof raw,&regs)==WF_OK);
  assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
  assert(!memcmp(raw,input+36,sizeof raw));
  wf_r17_referee_ctx_t ctx={0};wf_status_t status;
  switch(get32(input)) {
   case 0:status=primary2_7(&runtime,&runtime.referee,&ctx);break;
   case 1:status=action7(&runtime,&runtime.referee,&ctx);break;
   case 2:status=wf_r17_referee_logic_1f914(&runtime,&runtime.referee,&ctx);break;
   default:abort();
  }
  assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
  put32(output,status);put32(output+4,ctx.blocked_dependency_rom);
  for(unsigned i=0;i<8;i++)put32(output+8+i*4,runtime.m68k_regs.d[i]);
  memcpy(output+40,raw,sizeof raw);
  assert(fwrite(output,1,sizeof output,out)==sizeof output);cases++;
 }
 assert(!ferror(in));assert(!fclose(in));assert(!fclose(out));
 printf("probe records=%u\n",cases);return 0;
}
