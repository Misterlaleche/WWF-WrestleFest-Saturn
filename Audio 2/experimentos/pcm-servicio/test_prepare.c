#include "wf_scsp_sfx.h"
#include "sfx_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *pack,*gold;
static size_t bytes;
static uint16_t pcm[16388];
static unsigned writes;
static uint16_t be16(const uint8_t*p){return (uint16_t)((unsigned)p[0]*256u+p[1]);}
static bool output(uint32_t address,uint16_t value,void*user){
 (void)user;++writes;
 if(address>=0x3000&&address<0xb008){pcm[(address-0x3000)/2u]=value;return true;}
 assert(address>=0x100080&&address<0x100100);return true;
}
static uint8_t*read_file(const char*path,size_t*size){
 FILE*f=NULL;long n;uint8_t*p;
 assert(!fopen_s(&f,path,"rb"));assert(!fseek(f,0,SEEK_END));n=ftell(f);assert(n>0);
 rewind(f);p=malloc((size_t)n);assert(p&&fread(p,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return p;
}
static bool control(unsigned v,bool start,unsigned volume,void*u){(void)u;(void)start;assert(v<4&&volume<16);return true;}
static void all_prepared_samples_are_golden_without_audible_writes(void){
 wf_scsp_sfx_t s,before;wf_sfx_prepared_t prepared;unsigned i,n;
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));before=s;
 for(i=0;i<114;i++){
  const fixture_t*f=&fixtures[i];writes=0;
  assert(wf_scsp_sfx_prepare(&s,&prepared,f->bank,f->id));
  assert(!writes&&!memcmp(&s,&before,sizeof s));assert(prepared.total_frames==f->frames);
  for(n=0;n<256;n++)assert((uint16_t)prepared.samples[n]==(n<f->frames?be16(gold+f->offset+n*2u):0));
 }
}
static void commit_is_same_initial_waveform_and_decoder(void){
 wf_scsp_sfx_t s;wf_sfx_prepared_t prepared;unsigned n;wf_sfx_decoder_t before;
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_configure_pcm(&s,8194,1,control));
 assert(wf_scsp_sfx_bind(&s,pack,bytes));assert(wf_scsp_sfx_prepare(&s,&prepared,1,20));
 assert(wf_scsp_sfx_oki_start_prepared(&s,3,1,20,3,false,&prepared));
 assert(s.pcm[3].playing&&s.pcm[3].write_at==256&&s.pcm[3].played==0);
 assert(!memcmp(&s.pcm[3].decoder,&prepared.decoder,sizeof prepared.decoder));
 for(n=0;n<256;n++)assert(pcm[3*4097u+n]==(uint16_t)prepared.samples[n]);
 assert(pcm[3*4097u+4096u]==(uint16_t)prepared.samples[0]);
 before=s.pcm[3].decoder;assert(wf_scsp_sfx_oki_start(&s,3,0,1,1,true));
 before=s.pcm[3].decoder;writes=0;
 assert(wf_scsp_sfx_oki_start_prepared(&s,3,1,20,3,false,&prepared));
 assert(!writes&&!memcmp(&before,&s.pcm[3].decoder,sizeof before)&&!s.pcm[3].suppress);
}
static void mismatching_preparation_has_no_effect(void){
 wf_scsp_sfx_t s,before;wf_sfx_prepared_t prepared;
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));
 assert(wf_scsp_sfx_prepare(&s,&prepared,1,20));before=s;writes=0;
 assert(!wf_scsp_sfx_oki_start_prepared(&s,0,0,1,3,false,&prepared));
 assert(!writes&&!memcmp(&s,&before,sizeof s));
}
int main(int argc,char**argv){size_t n;assert(argc==3);pack=read_file(argv[1],&bytes);gold=read_file(argv[2],&n);assert(n>1000000);
 all_prepared_samples_are_golden_without_audible_writes();commit_is_same_initial_waveform_and_decoder();mismatching_preparation_has_no_effect();
 free(pack);free(gold);puts("PASS prepared114 golden waveforms, pure preparation, exact commit decoder/sentinel, suppression and immutable rejection");return 0;}
