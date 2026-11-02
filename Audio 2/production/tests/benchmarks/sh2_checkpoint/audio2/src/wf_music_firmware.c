#include "wf_scsp_sfx.h"
#include "wf_scsp_ym.h"
#include "wf_scsp_dsp.h"
#include "wf_native_driver.h"
#include "wf_music_clock.h"
#ifndef WF_MUSIC_TIMING
#define WF_MUSIC_TIMING 1
#endif
enum {MUSIC_PROFILE=0x4d46};
static volatile wf_native_mail_t *const mail=(volatile wf_native_mail_t *)WF_NATIVE_MAIL_OFFSET;
static volatile uint16_t *const ring=(volatile uint16_t *)WF_MUSIC_RING_HEADER;
wf_scsp_sfx_t wf_sfx_engine;
wf_scsp_ym_t wf_ym_engine;
static uint16_t last_dsp,generation;
static uint32_t clock_frames,last_poll;
static int oki_pending;
static unsigned music_bank;
static wf_sfx_prepared_t prepared_music;
static bool prepared_music_valid;
static uint16_t prepared_music_frames;
/* Private observation words, read with the matching ELF symbols:
 * 0 last fallback filled count; 3 last fallback cache bank/id;
 * 1/2/4 max due by kind; 6/7/9 late counts by kind; 10 max prepare step work;
 * 11/12 max fallback/prepared launch work; 13 prepares; 14/15 hits/misses;
 * 16 max audible-launch lateness; 17..21 last late kind/when/now/cache/busy;
 * 22 smallest prepare lead; 23 last fallback sample; 24 max refill work.
 * Durations are DSP Fs, not inclusive CPU-cycle profiles. Counts wrap16. */
volatile uint16_t wf_music_timing[25];
#if WF_MUSIC_TIMING
static void __attribute__((noinline)) timing_max(unsigned index,uint16_t value)
{
 if(value>wf_music_timing[index])wf_music_timing[index]=value;
}
#else
#define timing_max(index,value) ((void)0)
#endif
static bool write16(uint32_t a,uint16_t v,void *u)
{
 (void)u;
 if((a&1u)||!((a>=0x3000&&a<0xb018)||(a>=0x100000&&a<0x100404)||(a>=0x100700&&a<0x100c00)))return false;
 *(volatile uint16_t *)(uintptr_t)a=v;return true;
}
static uint32_t clock_now(void *user)
{
 (void)user;uint16_t word=*(volatile uint16_t *)WF_DSP_CLOCK;
 clock_frames+=wf_music_clock_delta(&last_dsp,word);return clock_frames;
}
static void poll_clock(void)
{
 uint32_t frames=clock_now(NULL),elapsed=frames-last_poll;last_poll=frames;
 if(elapsed>512u){ring[10]=(uint16_t)elapsed;ring[11]=(uint16_t)(wf_sfx_engine.pcm[0].playing|
  (wf_sfx_engine.pcm[1].playing<<1)|(wf_sfx_engine.pcm[2].playing<<2)|
  (wf_sfx_engine.pcm[3].playing<<3));}
 if(elapsed)wf_scsp_sfx_advance(&wf_sfx_engine,(unsigned)elapsed);
}
static void stop_music(void)
{
 /* FM stores belong to the slave: its KEYON state can differ from this
  * firmware's reset cache. Force every hardware key off on generation. */
 for(unsigned slot=0;slot<32;slot++)(void)write16(0x100000u+slot*32u,0x21,NULL);
 wf_scsp_ym_stop(&wf_ym_engine);wf_scsp_sfx_stop(&wf_sfx_engine,3);
 oki_pending=-1;music_bank=0;mail->fm_active=0;
 prepared_music_valid=false;prepared_music_frames=0;
}
static void oki(unsigned value,uint16_t when,uint16_t before)
{
 (void)before; /* Observation-free builds do not sample launch duration. */
 if(oki_pending>=0){
  unsigned mask=value>>4,sample=(unsigned)oki_pending;oki_pending=-1;
  if(mask&~1u){ring[9]++;return;}
  if((mask&1u)&&!wf_sfx_engine.pcm[3].playing){
   bool ok,prepared=prepared_music_valid&&prepared_music.bank==music_bank&&prepared_music.id==sample;
   if(WF_MUSIC_TIMING&&!prepared){
    wf_music_timing[0]=prepared_music_frames;
    wf_music_timing[3]=(uint16_t)((prepared_music.bank<<8)|prepared_music.id);
   }
   if(prepared)
    ok=wf_scsp_sfx_oki_start_prepared(&wf_sfx_engine,3,music_bank,sample,value&15u,false,&prepared_music);
   else ok=wf_scsp_sfx_oki_start(&wf_sfx_engine,3,music_bank,sample,value&15u,false);
   if(WF_MUSIC_TIMING){
    uint16_t now=*(volatile uint16_t *)WF_DSP_CLOCK;
    ++wf_music_timing[prepared?14:15];timing_max(prepared?12:11,(uint16_t)(now-before));
    int16_t late=wf_music_clock_due(now,when);if(late>0)timing_max(16,(uint16_t)late);
    if(!prepared)wf_music_timing[23]=(uint16_t)sample;
   }
   if(!ok)ring[9]++;
  }
 }else if(value&128u)oki_pending=(int)(value&127u);
 else if((value>>3)&1u)wf_scsp_sfx_stop(&wf_sfx_engine,3);
}
/* Preview keeps ring order, bank, pending byte and hardware unchanged. The
 * measured256-sample decode took294Fs, so work is split into64-sample steps.
 * Admit each step only with128Fs before the NEAREST live event, including
 * earlier metadata. A new cache still requires the original256Fs target lead.
 * Main services transport between steps; only a complete cache can launch.
 * Preview16 records: the ROM emits a14-record burst whose PCM start was
 * invisible to12-record preview until the useful preceding idle gap ended.
 * This bounds local sound-RAM reads; no ring or timestamp lead is enlarged. */
static bool __attribute__((noinline,section(".wf_audio_high"))) music_prepare(void)
{
 uint16_t producer=ring[0],consumer=ring[1],now=*(volatile uint16_t *)WF_DSP_CLOCK;
 unsigned bank=music_bank;int pending=oki_pending;bool first=true;
 if(!wf_sfx_engine.bound||ring[2]!=generation||(uint16_t)(producer-consumer)>WF_MUSIC_RING_COUNT)return false;
 for(unsigned n=0;n<16&&consumer!=producer;n++,consumer++){
  volatile uint16_t *e=(volatile uint16_t *)(WF_MUSIC_RING+(consumer&255u)*8u);
  if(e[3]!=generation)continue;
  if(first){if(wf_music_clock_due(now,e[0])>-128)return false;first=false;}
  unsigned kind=e[1]>>8,value=e[2];
  if(kind==2)bank=value&1u;
  else if(kind==1){
   if(pending>=0){
    unsigned sample=(unsigned)pending;pending=-1;
    if((value>>4)==1u){
     int16_t due=wf_music_clock_due(now,e[0]);
     bool same=prepared_music_frames&&prepared_music.bank==bank&&prepared_music.id==sample;
     if((!same&&due<=-256)||(same&&!prepared_music_valid)){
      if(!same){
       prepared_music_valid=false;prepared_music_frames=0;
       if(WF_MUSIC_TIMING){
        ++wf_music_timing[13];uint16_t lead=(uint16_t)-due;
        if(!wf_music_timing[22]||lead<wf_music_timing[22])wf_music_timing[22]=lead;
       }
      }
      if(!wf_scsp_sfx_prepare_step(&wf_sfx_engine,&prepared_music,bank,sample,&prepared_music_frames,64))return false;
      prepared_music_valid=prepared_music_frames==256;
      timing_max(10,(uint16_t)(*(volatile uint16_t *)WF_DSP_CLOCK-now));
      return true;
     }
     return false;
    }
   }else if(value&128u)pending=(int)(value&127u);
  }
 }
 return false;
}
static bool music_service(void)
{
 if(wf_ym_engine.fault)return false;
 if(ring[2]!=generation){generation=ring[2];stop_music();ring[5]=generation;}
 for(unsigned budget=0;budget<12;budget++){
  uint16_t producer=ring[0],consumer=ring[1];
  if((uint16_t)(producer-consumer)>WF_MUSIC_RING_COUNT){wf_ym_engine.fault=1;return false;}
  if(consumer==producer)return true;
  volatile uint16_t *event=(volatile uint16_t *)(WF_MUSIC_RING+(consumer&255u)*8u);
  if(event[3]!=generation){ring[1]=(uint16_t)(consumer+1u);continue;}
  /* SH2 timestamps come from this same DSP word. The accumulated SFX clock
   * has a different epoch after bind and must not schedule music events. */
  uint16_t dsp_now=*(volatile uint16_t *)WF_DSP_CLOCK;
  int16_t due=wf_music_clock_due(dsp_now,event[0]);
  if(due<0)return true;
  unsigned code=event[1],kind=code>>8,value=event[2];
  if(kind<5)timing_max(kind,(uint16_t)due);
  if(due>256){
   ring[4]++;
   if(WF_MUSIC_TIMING){
    if(kind<5)++wf_music_timing[5+kind];
    wf_music_timing[17]=(uint16_t)kind;wf_music_timing[18]=event[0];wf_music_timing[19]=dsp_now;
    wf_music_timing[20]=prepared_music_valid;wf_music_timing[21]=wf_sfx_engine.pcm[3].playing;
   }
  }
  if(due>2205){
   /* Count the late event once above. Do not replay an irrecoverably late
    * PCM/bank/mask event. Fault2 preserves a distinct 0xb040 first cause. */
   wf_ym_engine.fault=2;return false;
  }
  if(kind==4)mail->fm_active=(uint16_t)value;
  else if(kind==1)oki(value,event[0],dsp_now);
  else if(kind==2)music_bank=value&1u;
  else{wf_ym_engine.fault=1;return false;}
  ring[1]=(uint16_t)(consumer+1u);
  poll_clock();
 }
 return true;
}
static void __attribute__((noinline,section(".wf_audio_high"))) music_work(void)
{
 /* Finish pending cache steps at the existing service boundaries before
  * background refill. Normal refill remains128 samples, round-robin. */
 if(music_prepare())return;
 uint16_t before=*(volatile uint16_t *)WF_DSP_CLOCK;
 /* Refill measured148Fs. Starting it across an imminent head event leaves
  * too little time for that event's118Fs launch and the rest of its burst.
  * Reserve192Fs before the nearest event; return to service when it cannot
  * fit. These are work-admission budgets, not changed lateness thresholds. */
 uint16_t consumer=ring[1];
 if(consumer!=ring[0]){
  volatile uint16_t *head=(volatile uint16_t *)(WF_MUSIC_RING+(consumer&255u)*8u);
  if(head[3]!=generation||wf_music_clock_due(before,head[0])>-192)return;
 }
 wf_scsp_sfx_work(&wf_sfx_engine);
 timing_max(24,(uint16_t)(*(volatile uint16_t *)WF_DSP_CLOCK-before));
}
static void service(void)
{
 uint16_t req=mail->request,arg=mail->argument,result=WF_NATIVE_OK,value=0;int status;
 if(req==mail->ack)return;
 switch(mail->command){
 case WF_NATIVE_FM_STOP:stop_music();break;
 case WF_NATIVE_SFX_BIND:{
  const uint8_t *p=(const uint8_t *)WF_SFX_PACK_BASE;
  uint32_t bytes=(uint32_t)p[8]<<24|(uint32_t)p[9]<<16|(uint32_t)p[10]<<8|p[11];
  if(!wf_scsp_sfx_bind(&wf_sfx_engine,p,bytes))result=WF_NATIVE_BAD_STATE;
  last_dsp=*(volatile uint16_t *)WF_DSP_CLOCK;clock_frames=last_poll=wf_sfx_engine.frames;break;}
 case WF_NATIVE_SFX_STOP:wf_scsp_sfx_stop_all(&wf_sfx_engine);wf_sfx_engine.bound=0;break;
 case WF_NATIVE_SFX_QUERY:
  for(unsigned v=0;v<4;v++)if(wf_sfx_engine.pcm[v].playing)value|=(uint16_t)(1u<<v);
  if(wf_sfx_engine.bound)value|=0x100;
  break;
 case WF_NATIVE_OKI_START:
  if(!wf_scsp_sfx_oki_start(&wf_sfx_engine,(arg>>12)&3u,(arg>>11)&1u,(arg>>4)&127u,arg&15u,false))result=WF_NATIVE_BAD_STATE;
  break;
 case WF_NATIVE_OKI_STOP:
  for(unsigned v=0;v<4;v++)if(arg&(1u<<v))wf_scsp_sfx_stop(&wf_sfx_engine,v);
  break;
 case WF_NATIVE_ARCADE_WORD:
  mail->reserved[0]=value=arg;
  if(!(arg&255u)){
   /* Music stop is ordered by the slave FIFO/generation. A delayed mailbox
    * effect-stop must never key off a newer music command. */
   for(unsigned v=0;v<3;v++){wf_scsp_sfx_stop(&wf_sfx_engine,v);wf_sfx_engine.sequence[v].active=0;}
  }
  else if((arg&255u)>=0x20u&&(arg&255u)<=0x81u){
   status=wf_scsp_sfx_command(&wf_sfx_engine,(uint8_t)arg);
   if(status== -1)result=WF_NATIVE_PRIORITY_REJECTED;else if(status<0)result=WF_NATIVE_BAD_STATE;
  }else result=WF_NATIVE_UNIMPLEMENTED;
  break;
 case WF_NATIVE_FM_START:case WF_NATIVE_RING_PREPARE:case WF_NATIVE_RING_START:case WF_NATIVE_RING_STOP:case WF_NATIVE_RING_CA:
  result=WF_NATIVE_UNIMPLEMENTED;break;
 default:result=WF_NATIVE_BAD_COMMAND;break;
 }
 mail->reserved[1]=(uint16_t)wf_sfx_engine.underruns;mail->value=value;mail->result=result;mail->ack=req;
}
static void fail_stop(uint16_t init_faults)
{
 if(mail->state==WF_NATIVE_FAULT)return;
 mail->reserved[0]=(uint16_t)(0xb000u|init_faults|
     ((uint16_t)wf_sfx_engine.fault<<4)|((uint16_t)wf_ym_engine.fault<<5));
 stop_music();wf_scsp_sfx_stop_all(&wf_sfx_engine);ring[3]=WF_NATIVE_FAULT;
 mail->reserved[1]=(uint16_t)wf_sfx_engine.underruns;mail->state=WF_NATIVE_FAULT;
}
void wf_music_main(void)
{
 uint16_t init_faults=0;
 mail->state=WF_NATIVE_INIT;
 oki_pending=-1;
 for(unsigned i=0;i<512;i++)*(volatile uint16_t *)(0x100000u+i*2u)=0;
 *(volatile uint16_t *)0x1003e0=0x1000;
 *(volatile uint16_t *)0x10041e=0;*(volatile uint16_t *)0x10042a=0;
 wf_scsp_sfx_init(&wf_sfx_engine,write16,NULL);
 if(!wf_scsp_sfx_configure_pcm(&wf_sfx_engine,8194,1,wf_scsp_dsp_voice)){
  wf_sfx_engine.fault=1;init_faults|=1u;
 }
 wf_sfx_engine.now=clock_now;
 wf_scsp_ym_init(&wf_ym_engine,write16,NULL);
 if(!wf_scsp_dsp_load(write16,NULL)){wf_sfx_engine.fault=1;init_faults|=2u;}
 unsigned wait=100000;
 /* The DSP may have advanced past its first sample before the 68000 reads
  * it. Any nonzero value proves that the clock program has started. */
 while(*(volatile uint16_t *)WF_DSP_CLOCK==0&&wait)--wait;
 if(!wait){wf_sfx_engine.fault=1;init_faults|=4u;}
 wf_scsp_dsp_clock_run();last_dsp=*(volatile uint16_t *)WF_DSP_CLOCK;
 for(unsigned i=0;i<6;i++)ring[i]=0;
 ring[3]=WF_NATIVE_READY;
 mail->magic_hi=0x5746;mail->magic_lo=0x4e31;mail->abi=WF_NATIVE_ABI;
 mail->reserved[2]=MUSIC_PROFILE;mail->state=WF_NATIVE_READY;
 for(;;){
  poll_clock();if(!music_service())break;music_prepare();
  poll_clock();if(!music_service())break;service();
  poll_clock();if(!music_service())break;music_work();
  poll_clock();if(!music_service())break;music_work();
  if(wf_sfx_engine.fault||wf_ym_engine.fault)break;
 }
 fail_stop(init_faults);for(;;){}
}
