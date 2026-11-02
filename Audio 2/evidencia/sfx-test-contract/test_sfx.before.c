#include "wf_scsp_sfx.h"
#include "wf_audio_sfx.h"
#include "sfx_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *pack,*gold;
static size_t bytes;
static uint16_t regs[512],pcm[16384];
static unsigned writes;
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t be16(const uint8_t *p){return (uint16_t)(p[0]*256u+p[1]);}
static bool output(uint32_t a,uint16_t v,void *u){
 (void)u;++writes;
 if(a>=0x3000&&a<0xb000){pcm[(a-0x3000)/2]=v;return true;}
 assert(a>=0x100080&&a<0x100100);regs[(a-0x100000)/2]=v;return true;
}
static uint8_t *read_file(const char *p,size_t *sz){
 FILE *f=NULL;long n;uint8_t *v;
 assert(fopen_s(&f,p,"rb")==0);assert(fseek(f,0,SEEK_END)==0);n=ftell(f);assert(n>0);
 rewind(f);v=malloc((size_t)n);assert(v);assert(fread(v,1,(size_t)n,f)==(size_t)n);fclose(f);*sz=(size_t)n;return v;
}
static void decoder_all(void){
 unsigned i;for(i=0;i<114;i++){
  const fixture_t *f=&fixtures[i];const uint8_t *e=pack+32+(f->bank*74+f->id)*8;
  wf_sfx_decoder_t d={0};unsigned n;d.offset=be32(e);d.remaining=be16(e+4)*2u;
  for(n=0;n<f->frames;n++)assert((uint16_t)wf_scsp_sfx_decode(&d,pack)==be16(gold+f->offset+n*2u));
  assert(!d.remaining);assert(wf_scsp_sfx_decode(&d,pack)==0);
 }
}
static void sequence_compare(wf_scsp_sfx_t *s){
 unsigned v;for(v=0;v<3;v++){
  const wf_sfx_voice_t *a=wf_sfx_voice(v);const wf_scsp_sfx_sequence_t *b=&s->sequence[v];
  assert(a->active==(b->active!=0));
  if(a->active){assert(a->command==b->command);assert(a->event_index==b->event_index);
   assert(a->ticks_remaining==b->ticks_remaining);assert(a->priority==b->priority);assert(a->bank==b->bank);}
 }
}
static void advance_tick(wf_scsp_sfx_t *s){
 uint32_t old=s->ticks;
 while(s->ticks==old){wf_scsp_sfx_work(s);wf_scsp_sfx_advance(s,16);}
 wf_sfx_timer_a_tick(NULL,NULL,NULL);sequence_compare(s);assert(!s->fault);
}
static void sequences_all(void){
 wf_scsp_sfx_t s;unsigned c,t;
 for(c=0x20;c<=0x81;c++){
  wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));wf_sfx_runtime_reset();
  assert(wf_scsp_sfx_command(&s,(uint8_t)c)==wf_sfx_start_voice(c,NULL,NULL,NULL));sequence_compare(&s);
  for(t=0;t<2400;t++)advance_tick(&s);
 }
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));wf_sfx_runtime_reset();
 for(t=0;t<30000;t++){
  if(t%3==0){c=0x20+(t*73u)%98u;
   assert(wf_scsp_sfx_command(&s,(uint8_t)c)==wf_sfx_start_voice(c,NULL,NULL,NULL));sequence_compare(&s);}
  advance_tick(&s);
 }
}
static void refill_and_suppression(void){
 wf_scsp_sfx_t s;unsigned i,n=0;uint32_t old;
 const fixture_t *f=&fixtures[93]; /* long bank-1 voice */
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));
 assert(wf_scsp_sfx_oki_start(&s,0,f->bank,f->id,3,true));old=s.pcm[0].decoder.offset;
 assert(wf_scsp_sfx_oki_start(&s,0,0,1,1,false));assert(s.pcm[0].decoder.offset==old);assert(!s.pcm[0].suppress);
 assert(regs[(4*32+6)/2]==4096); /* exclusive loop end */
 for(i=0;i<50000&&s.pcm[0].playing;i++){
  unsigned played=s.pcm[0].played;
  while(n<=played&&n<f->frames){assert(pcm[n&4095]==be16(gold+f->offset+n*2));++n;}
  writes=0;wf_scsp_sfx_work(&s);assert(writes<=128);wf_scsp_sfx_advance(&s,16);assert(!s.fault);
 }
 assert(n==f->frames);assert(!s.underruns);
 wf_scsp_sfx_stop_all(&s);assert(!s.pcm[0].playing);
 assert(wf_scsp_sfx_oki_start(&s,0,1,20,3,false));
 for(i=0;i<50;i++)wf_scsp_sfx_advance(&s,32); /* intentionally starve producer */
 assert(s.fault&&s.underruns==1&&!s.pcm[0].playing);
}
static void levels_and_invalid_bind(void){
 static const unsigned expected[16]={0,9,16,25,32,39,48,55,64,255,255,255,255,255,255,255};
 wf_scsp_sfx_t s;unsigned i;uint8_t old;
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));
 for(i=0;i<16;i++){
  assert(wf_scsp_sfx_oki_start(&s,0,0,1,i,false));
  assert((regs[(4*32+12)/2]&255u)==expected[i]);
 }
 old=pack[960+1];pack[960+1]=255;
 assert(!wf_scsp_sfx_bind(&s,pack,bytes));assert(!s.bound&&!s.pcm[0].playing);pack[960+1]=old;
}
static void late_start_must_not_truncate(void){
 wf_scsp_sfx_t s;unsigned i;
 wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));
 s.timer_phase=714067200u-16u*3579545u;
 assert(wf_scsp_sfx_oki_start(&s,0,0,1,0,false));
 for(i=0;i<2000&&s.pcm[0].playing;i++){wf_scsp_sfx_work(&s);wf_scsp_sfx_advance(&s,16);}
 assert(!s.fault&&!s.pcm[0].playing);assert(s.pcm[0].played>=1802);
}
static uint32_t late_clock(void *u){(void)u;return 128;}
static void prefill_time_is_not_playback(void){
 wf_scsp_sfx_t s;wf_scsp_sfx_init(&s,output,NULL);assert(wf_scsp_sfx_bind(&s,pack,bytes));s.now=late_clock;
 assert(wf_scsp_sfx_oki_start(&s,0,0,1,0,false));
 wf_scsp_sfx_advance(&s,128);assert(!s.pcm[0].played);
 wf_scsp_sfx_advance(&s,64);assert(s.pcm[0].played==5&&!s.fault);
}
static unsigned controlled;
static bool pcm_backend(unsigned v,bool start,unsigned vol,void *u){(void)u;assert(v<4&&vol<16);controlled+=start?1u:0u;return true;}
static void dsp_backend_and_guard_sample(void){
 wf_scsp_sfx_t s;wf_scsp_sfx_init(&s,output,NULL);
 assert(wf_scsp_sfx_configure_pcm(&s,8194,1,pcm_backend));assert(wf_scsp_sfx_bind(&s,pack,bytes));
 assert(wf_scsp_sfx_oki_start(&s,0,0,1,0,false));assert(controlled==1);
 assert(pcm[4096]==pcm[0]);assert(s.start_guard==1);
}
int main(int argc,char **argv){
 size_t g;uint8_t save;assert(argc==3);pack=read_file(argv[1],&bytes);gold=read_file(argv[2],&g);assert(g>1000000);
 assert(wf_scsp_sfx_pack_valid(pack,bytes));save=pack[20];pack[20]=0xff;assert(!wf_scsp_sfx_pack_valid(pack,bytes));pack[20]=save;
 assert(!wf_scsp_sfx_pack_valid(pack,bytes-2));decoder_all();sequences_all();refill_and_suppression();levels_and_invalid_bind();late_start_must_not_truncate();prefill_time_is_not_playback();dsp_backend_and_guard_sample();
 free(pack);free(gold);puts("114 PCM exactos; 98 secuencias; prioridades mixtas; refills y starvation: OK");return 0;
}
