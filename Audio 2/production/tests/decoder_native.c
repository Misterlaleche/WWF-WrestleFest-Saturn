/* Native 68000 decoder regression/benchmark. No game or production scheduler. */
#include <stdint.h>
#include <stdbool.h>
#include "wf_scsp_sfx.h"
#include "wf_scsp_dsp.h"
#include "wf_native_driver.h"
#include "decoder_fixture.h"
_Static_assert(sizeof(wf_sfx_decoder_t)==14,"baseline68000 decoder ABI");
_Static_assert(offsetof(wf_sfx_decoder_t,low)==12,"baseline68000 decoder phase offset");
extern void decoder_baseline(wf_sfx_decoder_t *,const uint8_t *,volatile uint16_t *,unsigned);
extern void decoder_candidate(wf_sfx_decoder_t *,const uint8_t *,volatile uint16_t *,unsigned);
volatile uint32_t decoder_diag[32];
volatile uint32_t decoder_bench_records[4][8];
#ifndef DECODER_BENCH_ONLY
#define DECODER_BENCH_ONLY 0
#endif
static uint16_t expected[256];
static uint8_t edge_data[1024];
static uint32_t crc_baseline,crc_candidate;
static const int steps[49]={16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552};
static const int shifts[8]={-1,-1,-1,-1,2,4,6,8};
static uint16_t tick(void){return *(volatile uint16_t *)WF_DSP_CLOCK;}
static bool output(uint32_t a,uint16_t v,void *u)
{(void)u;*(volatile uint16_t *)(uintptr_t)a=v;return true;}
static void fail(unsigned reason,unsigned at,unsigned expected_value,unsigned actual)
{
 decoder_diag[2]=reason;decoder_diag[16]=at;
 decoder_diag[17]=expected_value;decoder_diag[18]=actual;
 decoder_diag[1]=4;for(;;){}
}
static uint32_t crc_word(uint32_t crc,uint16_t sample)
{
 crc=(crc>>8)^decoder_crc_table[(crc^(sample>>8))&255u];
 return (crc>>8)^decoder_crc_table[(crc^sample)&255u];
}
static bool same(const wf_sfx_decoder_t *a,const wf_sfx_decoder_t *b)
{return a->offset==b->offset&&a->remaining==b->remaining&&a->signal==b->signal&&a->index==b->index&&a->low==b->low;}
static uint16_t scalar(wf_sfx_decoder_t *d,const uint8_t *p)
{
 if(!d->remaining)return 0;
 unsigned n=p[d->offset];n=d->low?(n&15u):(n>>4);
 if(d->low)d->offset++;
 d->low^=1;d->remaining--;
 int step=steps[d->index],delta=(step>>3);
 if(n&1)delta+=step>>2;if(n&2)delta+=step>>1;if(n&4)delta+=step;
 int signal=d->signal+((n&8)?-delta:delta),index=d->index+shifts[n&7];
 d->signal=(int16_t)(signal>2047?2047:(signal< -2048?-2048:signal));
 d->index=(int16_t)(index>48?48:(index<0?0:index));
 return (uint16_t)((uint16_t)d->signal<<4);
}
static void check(wf_sfx_decoder_t *state,const uint8_t *input,unsigned count,bool reference,bool crc)
{
 volatile uint16_t *buf=(volatile uint16_t *)0x3002;
 wf_sfx_decoder_t old=*state,new=*state,ref=*state;
 buf[-1]=0xa55a;buf[count]=0x5aa5;
 decoder_baseline(&old,input,buf,count);
 if(buf[-1]!=0xa55a||buf[count]!=0x5aa5)fail(1,count,0,1);
 for(unsigned i=0;i<count;i++)expected[i]=buf[i];
 for(unsigned i=0;i<count;i++)buf[i]=0xdead;
 decoder_candidate(&new,input,buf,count);
 if(buf[-1]!=0xa55a||buf[count]!=0x5aa5)fail(2,count,0,1);
 for(unsigned i=0;i<count;i++){
  if(expected[i]!=buf[i])fail(3,i,expected[i],buf[i]);
  if(reference){uint16_t v=scalar(&ref,input);if(v!=buf[i])fail(4,i,v,buf[i]);}
  if(crc){crc_baseline=crc_word(crc_baseline,expected[i]);crc_candidate=crc_word(crc_candidate,buf[i]);}
 }
 if(!same(&old,&new))fail(5,0,old.offset,new.offset);
 if(reference&&!same(&ref,&new))fail(6,0,ref.remaining,new.remaining);
 *state=new;decoder_diag[13]++;
}
static void edges(void)
{
 static const int initial[]={-2048,-2047,-1,0,1,2046,2047};
 static const unsigned counts[]={0,1,2,3,7,17,31,63,127,128,129,255,256};
 static const unsigned remaining[]={0,1,2,3,4,15,16,17,128,255,256,257,512};
 decoder_diag[15]=1;
 for(unsigned index=0;index<49;index++)for(unsigned n=0;n<16;n++)
  for(unsigned s=0;s<7;s++)for(unsigned low=0;low<2;low++){
   edge_data[5]=(uint8_t)(n|(n<<4));
   wf_sfx_decoder_t d={5,1,(int16_t)initial[s],(int16_t)index,(uint8_t)low};
   check(&d,edge_data,1,true,false);decoder_diag[3]++;
  }
 for(unsigned i=0;i<1024;i++)edge_data[i]=(uint8_t)((i*73u)^(i>>2)^0xa5u);
 for(unsigned c=0;c<13;c++)for(unsigned r=0;r<13;r++)for(unsigned low=0;low<2;low++){
  wf_sfx_decoder_t d={7,remaining[r],(int16_t)(low?2047:-2048),48,(uint8_t)low};
  check(&d,edge_data,counts[c],true,false);decoder_diag[3]++;
 }
}
static void complete_samples(void)
{
 const uint8_t *pack=(const uint8_t *)0x14000;
 static const unsigned chunks[]={1,17,63,2,31,129,128,255};
 decoder_diag[15]=2;
 for(unsigned f=0;f<114;f++){
  const decoder_fixture_t *fixture=&decoder_fixtures[f];
  wf_sfx_decoder_t d={fixture->offset,fixture->frames,0,0,0};
  unsigned at=0;crc_baseline=crc_candidate=0xffffffffu;
  while(d.remaining){
   unsigned count=chunks[at++&7u];if(count>d.remaining)count=d.remaining;
   check(&d,pack,count,false,true);decoder_diag[4]+=count;
  }
  if((crc_baseline^0xffffffffu)!=fixture->crc)fail(7,f,fixture->crc,crc_baseline^0xffffffffu);
  if((crc_candidate^0xffffffffu)!=fixture->crc)fail(8,f,fixture->crc,crc_candidate^0xffffffffu);
  check(&d,pack,3,true,false);decoder_diag[14]++;decoder_diag[5]++;
 }
}
static void benchmark(void)
{
 const uint8_t *pack=(const uint8_t *)0x14000;
 volatile uint16_t *buf=(volatile uint16_t *)0x3002;
 static const unsigned counts[4]={32,64,128,256};
 decoder_diag[15]=3;
 for(unsigned c=0;c<4;c++){
 volatile uint32_t *record=decoder_bench_records[c];unsigned count=counts[c];
 record[0]=count;record[4]=record[6]=0xffffffffu;
 for(unsigned i=0;i<1024;i++){
  wf_sfx_decoder_t a={decoder_fixtures[0].offset+(i&255u),512,0,0,0},b=a;
  uint16_t begin,base,candidate;
  if(i&1u){
   begin=tick();decoder_candidate(&b,pack,buf,count);candidate=(uint16_t)(tick()-begin);
   for(unsigned n=0;n<count;n++)expected[n]=buf[n];
   begin=tick();decoder_baseline(&a,pack,buf,count);base=(uint16_t)(tick()-begin);
  }else{
   begin=tick();decoder_baseline(&a,pack,buf,count);base=(uint16_t)(tick()-begin);
   for(unsigned n=0;n<count;n++)expected[n]=buf[n];
   begin=tick();decoder_candidate(&b,pack,buf,count);candidate=(uint16_t)(tick()-begin);
  }
  for(unsigned n=0;n<count;n++)if(expected[n]!=buf[n])fail(12,n,expected[n],buf[n]);
  if(!same(&a,&b))fail(9,i,a.offset,b.offset);
  record[2]+=base;record[3]+=candidate;record[1]++;
  if(base<record[4])record[4]=base;if(base>record[5])record[5]=base;
  if(candidate<record[6])record[6]=candidate;if(candidate>record[7])record[7]=candidate;
  if(count==128){
   decoder_diag[6]=record[2];decoder_diag[7]=record[3];decoder_diag[8]=record[1];
   decoder_diag[9]=record[4];decoder_diag[10]=record[5];decoder_diag[11]=record[6];decoder_diag[12]=record[7];
  }
 }
 }
 /* A 5% minimum gain is larger than one-sample timer quantization. */
 decoder_diag[19]=decoder_diag[7]*100u<=decoder_diag[6]*95u;
}
void decoder_main(void)
{
 volatile wf_native_mail_t *mail=(volatile wf_native_mail_t *)WF_NATIVE_MAIL_OFFSET;
 for(unsigned i=0;i<512;i++)*(volatile uint16_t *)(0x100000u+i*2u)=0;
 *(volatile uint16_t *)0x1003e0=0x1000;
 *(volatile uint16_t *)0x10041e=0;*(volatile uint16_t *)0x10042a=0;
 if(!wf_scsp_dsp_load(output,0))fail(10,0,1,0);
 unsigned wait=100000;while(!tick()&&wait)--wait;
 if(!wait)fail(11,0,1,0);
 wf_scsp_dsp_clock_run();decoder_diag[0]=0x44454331;decoder_diag[1]=1;
 mail->magic_hi=0x5746;mail->magic_lo=0x4e31;mail->abi=WF_NATIVE_ABI;mail->state=WF_NATIVE_READY;
 while(decoder_diag[20]!=1){}
 decoder_diag[1]=2;if(!DECODER_BENCH_ONLY){edges();complete_samples();}benchmark();
 decoder_diag[15]=4;decoder_diag[1]=3;for(;;){}
}
