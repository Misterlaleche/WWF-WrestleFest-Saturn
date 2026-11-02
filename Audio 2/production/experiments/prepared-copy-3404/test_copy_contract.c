/* Portable memory/engine contract fixtures, not a 68000 timing emulator.
 * base is the unchanged C reference; model substitutes the emitted copy's
 * 31*8+7 ordered word operation schedule into the portable backend. */
#include "wf_scsp_sfx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void base_wf_scsp_sfx_init(wf_scsp_sfx_t *,wf_sfx_write16_fn,void *);
extern void model_wf_scsp_sfx_init(wf_scsp_sfx_t *,wf_sfx_write16_fn,void *);
extern bool base_wf_scsp_sfx_bind(wf_scsp_sfx_t *,const uint8_t *,size_t);
extern bool model_wf_scsp_sfx_bind(wf_scsp_sfx_t *,const uint8_t *,size_t);
extern bool base_wf_scsp_sfx_prepare(const wf_scsp_sfx_t *,wf_sfx_prepared_t *,unsigned,unsigned);
extern bool base_wf_scsp_sfx_oki_start_prepared(wf_scsp_sfx_t *,unsigned,unsigned,unsigned,unsigned,bool,const wf_sfx_prepared_t *);
extern bool model_wf_scsp_sfx_oki_start_prepared(wf_scsp_sfx_t *,unsigned,unsigned,unsigned,unsigned,bool,const wf_sfx_prepared_t *);
typedef struct {uint16_t ram[0xb008/2],regs[512];uint32_t address[300];uint16_t value[300];unsigned writes,controls;} memory_t;
static memory_t a_mem,b_mem;
static bool output(uint32_t address,uint16_t value,void *u){
 memory_t *m=u;assert(!(address&1)&&m->writes<300);
 m->address[m->writes]=address;m->value[m->writes++]=value;
 if(address<0xb008)m->ram[address/2]=value;
 else {assert(address>=0x100000&&address<0x100400);m->regs[(address-0x100000)/2]=value;}
 return true;
}
static bool control(unsigned voice,bool start,unsigned volume,void *u){
 memory_t *m=u;assert(voice<4&&volume<16);m->controls+=1u+voice*2u+(start?16u:0u)+volume*32u;return true;
}
static uint32_t now(void *u){(void)u;return 123456;}
int main(int argc,char **argv){
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);
 uint8_t *pack=malloc((size_t)n);assert(pack&&fread(pack,1,(size_t)n,f)==(size_t)n);fclose(f);
 unsigned cases=0;unsigned long long compared_words=0;
 for(unsigned bank=0;bank<2;bank++)for(unsigned id=1;id<=(bank?41u:73u);id++)
 for(unsigned voice=0;voice<4;voice++)for(unsigned mode=0;mode<2;mode++)for(unsigned volume=0;volume<16;volume++){
  wf_scsp_sfx_t a,b;wf_sfx_prepared_t prepared,saved;
  memset(&a_mem,0,sizeof a_mem);memset(&b_mem,0,sizeof b_mem);
  base_wf_scsp_sfx_init(&a,output,&a_mem);model_wf_scsp_sfx_init(&b,output,&b_mem);
  assert(base_wf_scsp_sfx_bind(&a,pack,(size_t)n)&&model_wf_scsp_sfx_bind(&b,pack,(size_t)n));
  a.now=b.now=now;
  if(mode){a.pcm_stride=b.pcm_stride=8194;a.start_guard=b.start_guard=1;a.pcm_control=b.pcm_control=control;}
  memset(&prepared,0x5a,sizeof prepared);assert(base_wf_scsp_sfx_prepare(&a,&prepared,bank,id));saved=prepared;
  for(unsigned i=0;i<sizeof a_mem.ram/sizeof a_mem.ram[0];i++)a_mem.ram[i]=b_mem.ram[i]=(uint16_t)(0xa5a5u+i*101u+id*29u);
  a_mem.writes=b_mem.writes=0;a_mem.controls=b_mem.controls=0;
  assert(base_wf_scsp_sfx_oki_start_prepared(&a,voice,bank,id,volume,false,&prepared));
  assert(model_wf_scsp_sfx_oki_start_prepared(&b,voice,bank,id,volume,false,&prepared));
  assert(!memcmp(&a_mem,&b_mem,sizeof a_mem));
  assert(!memcmp(&prepared,&saved,sizeof prepared));
  assert(!memcmp(&a.pcm[voice].decoder,&prepared.decoder,sizeof prepared.decoder));
  assert(!memcmp(&b.pcm[voice].decoder,&prepared.decoder,sizeof prepared.decoder));
  assert(a.pcm[voice].write_at==256&&a.pcm[voice].write_end==4096&&a.pcm[voice].playing&&a.pcm[voice].initial_fill);
  unsigned first=(0x3000+voice*a.pcm_stride)/2;
  for(unsigned i=0;i<256;i++)assert(a_mem.ram[first+i]==(uint16_t)prepared.samples[i]);
  if(mode)assert(a_mem.ram[first+4096]==(uint16_t)prepared.samples[0]);
  a.user=b.user=NULL;assert(!memcmp(&a,&b,sizeof a));
  ++cases;compared_words+=sizeof a_mem.ram/sizeof a_mem.ram[0];
 }
 printf("{\"cases\":%u,\"whole_ram_words_compared\":%llu,\"ordered_writes_equal\":true,\"engine_and_decoder_state_equal\":true,\"prepared_source_unchanged\":true,\"guards_and_non_target_words_equal\":true}\n",cases,compared_words);
 free(pack);return 0;
}
