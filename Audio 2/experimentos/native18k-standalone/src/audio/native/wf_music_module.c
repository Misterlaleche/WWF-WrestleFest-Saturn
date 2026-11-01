/* Runs only on the dedicated slave SH2. Synthesis remains entirely SCSP. */
#include "wf_music_module.h"
#include "wf_music_seq.h"
#include "wf_scsp_ym.h"
#include "wf_music_time.h"
#include "wf_music_clock.h"
#include <string.h>
extern const uint8_t wf_music_sound_rom[];
extern const uint8_t wf_hotcode_start[],wf_hotcode_end[],wf_hotcode_load[];
#ifdef WF_MUSIC_MODULE_TEST
extern wf_music_module_shared_t wf_music_test_shared;
extern uint16_t *wf_music_test_sound(unsigned);
extern void wf_music_test_idle(void);
#define SHARED wf_music_test_shared
#define SOUND16(a) (*wf_music_test_sound(a))
#else
#define SHARED (*(volatile wf_music_module_shared_t *)(uintptr_t)WF_MUSIC_MODULE_SHARED)
#define SOUND16(a) (*(volatile uint16_t *)(uintptr_t)(0x25a00000u+(a)))
#endif
static wf_music_seq_t boot_snapshot;
static wf_music_time_t event_time;
static uint16_t output_frame;
typedef struct {uint16_t when,index,value,reserved;} fm_event_t;
typedef struct {wf_music_seq_t seq;wf_scsp_ym_t ym;} wf_music_hot_t;
_Static_assert(sizeof(wf_music_hot_t)==3220,"hotstate layout changed");
static wf_music_hot_t *hot;
#define sequence (hot->seq)
#define synth (hot->ym)
static fm_event_t fm_events[512];
static uint16_t fm_producer,fm_consumer;
static bool booting,playing,heartbeat_started;
static uint16_t heartbeat_frame;
static uint16_t heartbeat_dsp;
static void heartbeat(void)
{
 uint16_t frame=SHARED.master_frame;
 if(heartbeat_started&&frame==heartbeat_frame)return;
 uint16_t dsp=SOUND16(0xb010);
 if(!heartbeat_started||dsp!=heartbeat_dsp){heartbeat_dsp=dsp;SHARED.dsp_progress_frame=frame;}
 heartbeat_started=true;heartbeat_frame=frame;SHARED.worker_frame=frame;
}
static bool __attribute__((section(".wf_hotcode"))) fm_flush(void)
{
 uint16_t now=SOUND16(0xb010);
 while(fm_consumer!=fm_producer){
  const fm_event_t *e=&fm_events[fm_consumer&511u];
  int16_t late=wf_music_clock_due(now,e->when);
  if(late<0)break;
  if(late>2205){
   SOUND16(0xb03au)=(uint16_t)(SOUND16(0xb03au)+1u);
   fm_consumer=fm_producer;
   booting=true;wf_scsp_ym_stop(&synth);booting=false;
   return !synth.fault;
  }
  SOUND16(0x100000u+e->index*2u)=e->value;++fm_consumer;
 }
 return true;
}
static uint64_t target_cycle;
static uint32_t fraction,accumulated;
static uint16_t last_frame,generation;
static void idle(void)
{
 heartbeat();
 /* Cached two-instruction loop: no SCSP/bus polling while waiting. */
#ifdef WF_MUSIC_MODULE_TEST
 wf_music_test_idle();
#else
 unsigned count=2048;
 __asm__ volatile("1: dt %0\n\tbf 1b":"+r"(count)::"t");
#endif
}
static bool publish(unsigned code,unsigned value,uint16_t when)
{
 uint16_t started=SHARED.master_frame;
 uint16_t producer=SOUND16(0xb020),consumer;
 do{
  consumer=SOUND16(0xb022);
  if(SOUND16(0xb026)!=2||SHARED.shutdown||(uint16_t)(SHARED.master_frame-started)>=120u)return false;
  if((uint16_t)(producer-consumer)<256)break;
  idle();
 }while(true);
 uint32_t address=0xb100u+(producer&255u)*8u;
 if((code>>8)==4u&&wf_music_clock_due(SOUND16(0xb010),when)>2205){
  SOUND16(0xb038u)=(uint16_t)(SOUND16(0xb038u)+1u);
  return true;
 }
 SOUND16(address)=when;SOUND16(address+2u)=(uint16_t)code;
 SOUND16(address+4u)=(uint16_t)value;SOUND16(address+6u)=generation;
 (void)SOUND16(address+6u); /* drain preceding uncached sound-bus stores */
 SOUND16(0xb020)=(uint16_t)(producer+1u);SHARED.events++;return true;
}
static bool generation_ready(void)
{
 uint16_t started=SHARED.master_frame;
 while(SOUND16(0xb02a)!=generation){
  uint16_t state=SOUND16(0xb026);
  if(SHARED.shutdown||state!=2){
   SHARED.events=SHARED.shutdown?1201u:1202u;
   SHARED.steps_hi=((uint32_t)generation<<16)|SOUND16(0xb02a);
   SHARED.steps_lo=(uint32_t)state<<16;
   return false;
  }
  if((uint16_t)(SHARED.master_frame-started)>=120u)return false;
  idle();
 }
 return true;
}
static bool __attribute__((section(".wf_hotcode"))) synth_write(uint32_t address,uint16_t value,void *user)
{
 (void)user;
 if(address<0x100000u||address>=0x100400u||(address&1u))return false;
 if(booting){SOUND16(address)=value;return true;}
 uint16_t started=SHARED.master_frame;
 while((uint16_t)(fm_producer-fm_consumer)>=512u){
  if(SHARED.shutdown||SOUND16(0xb026)!=2||
     (uint16_t)(SHARED.master_frame-started)>=120u||!fm_flush())return false;
  idle();
 }
 fm_event_t *e=&fm_events[fm_producer&511u];
 e->when=output_frame;
 e->index=(uint16_t)((address-0x100000u)/2u);e->value=value;e->reserved=0;
 ++fm_producer;return true;
}
static bool __attribute__((section(".wf_hotcode"))) event(unsigned kind,unsigned reg,unsigned value,uint64_t cycle,void *user)
{
 (void)user;
 if(kind==WF_MUSIC_IRQ)return booting||fm_flush();
 output_frame=booting?SOUND16(0xb010):wf_music_time_advance(&event_time,cycle);
 if(kind==WF_MUSIC_YM){
  bool key_change=reg==8&&synth.key[value&7u]!=((value>>3)&15u);
  if(!wf_scsp_ym_write(&synth,(uint8_t)reg,(uint8_t)value))return false;
  if(key_change&&!booting){unsigned mask=0;for(unsigned ch=0;ch<8;ch++)if(synth.key[ch])mask|=1u<<ch;
   return publish(0x400u,mask,output_frame);}
  return true;
 }
 if(booting)return true; /* Sound-ROM initialization predates any game music command. */
 return publish((kind<<8)|reg,value,output_frame);
}
static void fault(unsigned error)
{
 for(unsigned slot=0;slot<32;slot++)SOUND16(0x100000u+slot*32u)=0x21;
 SOUND16(0x1003e0u)=0x1021;
 SHARED.error=error;SHARED.state=WF_MUSIC_FAULT;
 ++generation;SOUND16(0xb024)=generation;
}
void wf_music_unknown_opcode(void){sequence.error=true;}
void wf_music_module_prepare(void)
{
 uint32_t workspace=SHARED.workspace;
 uintptr_t code=(uintptr_t)wf_hotcode_start,load=(uintptr_t)wf_hotcode_load;
 size_t code_bytes=(uintptr_t)wf_hotcode_end-code;
 if((workspace&3u)||workspace<0x06004000u||workspace>0x06100000u-18432u||code_bytes+sizeof(wf_music_hot_t)>18432u){
  SHARED.error=13;SHARED.state=WF_MUSIC_FAULT;return;
 }
 if(code!=load){
  if(code!=workspace){SHARED.error=14;SHARED.state=WF_MUSIC_FAULT;return;}
  volatile uint16_t *dst=(volatile uint16_t *)(workspace+0x20000000u);
  const volatile uint16_t *src=(const volatile uint16_t *)(load+0x20000000u);
  for(size_t n=0;n<code_bytes/2u;n++)dst[n]=src[n];
  (void)dst[code_bytes/2u-1u];
 }
 hot=(wf_music_hot_t *)(uintptr_t)(workspace+code_bytes);
 memset(hot,0,sizeof(*hot));
 memset((void *)&SHARED,0,sizeof(wf_music_module_shared_t));
 SHARED.workspace=workspace;
 heartbeat_started=false;
 fm_producer=fm_consumer=0;
 if(!wf_music_seq_init(&sequence,wf_music_sound_rom,0xc000,event,NULL,true)){fault(1);return;}
 SHARED.state=WF_MUSIC_BOOTING;booting=true;playing=false;generation=1;SOUND16(0xb024)=generation;
}
void wf_music_module_run_body(void)
{
 if(SHARED.state!=WF_MUSIC_BOOTING)return;
 if(!generation_ready()){fault(12);return;}
 wf_scsp_ym_init(&synth,synth_write,NULL);if(synth.fault){fault(9);return;}
 while(wf_music_seq_run(&sequence,2ull*3579545u,1024)>0){if(SHARED.shutdown)return;}
 if(sequence.error){fault(2);return;}
 boot_snapshot=sequence;booting=false;last_frame=SOUND16(0xb010);
 heartbeat();
 SHARED.state=WF_MUSIC_RUNNING;
 while(!SHARED.shutdown){
  heartbeat();
  if(SOUND16(0xb026)!=2){fault(11);return;}
  if(!fm_flush()){fault(10);return;}
  uint16_t now=SOUND16(0xb010);
  uint16_t delta=wf_music_clock_delta(&last_frame,now);
  if(delta>22050){SOUND16(0xb03cu)=(uint16_t)(SOUND16(0xb03cu)+1u);SOUND16(0xb03eu)=delta;}
  if(playing)accumulated+=delta;
  uint16_t producer=SHARED.producer,consumer=SHARED.consumer;
  if((uint16_t)(producer-consumer)>64){fault(4);return;}
  if(consumer!=producer&&!sequence.command_pending){
   unsigned command=SHARED.commands[consumer&63u]&255u;SHARED.last_command=(uint16_t)command;
   if(!command||(command>=1&&command<=18)){
    fm_consumer=fm_producer=0;
    ++generation;SOUND16(0xb024)=generation;SHARED.generation=generation;
    if(!generation_ready()){fault(12);return;}
    now=SOUND16(0xb010);last_frame=now;
    sequence=boot_snapshot;sequence.cpu.userdata=&sequence;
    accumulated=fraction=0;target_cycle=sequence.cycles;
    event_time=(wf_music_time_t){sequence.cycles,0,(uint16_t)(now+4096u)};
    output_frame=event_time.frame;wf_scsp_ym_stop(&synth);
    playing=command!=0;
    if(playing&&!wf_music_seq_send(&sequence,(uint8_t)command)){fault(5);return;}
   }else if(command==0x1f){
    if(playing&&!wf_music_seq_send(&sequence,0x1f)){fault(6);return;}
   }else{fault(7);return;}
   SHARED.consumer=(uint16_t)(consumer+1u);
  }
  if(playing&&accumulated>=512u){
   if(accumulated<=1000u){
    uint32_t numerator=accumulated*3579545u+fraction;
    target_cycle+=numerator/44100u;fraction=numerator%44100u;
   }else{
    uint64_t numerator=(uint64_t)accumulated*3579545u+fraction;
    target_cycle+=numerator/44100u;fraction=(uint32_t)(numerator%44100u);
   }
   accumulated=0;
  }
  if(playing&&sequence.cycles<target_cycle){
   if(wf_music_seq_run(&sequence,target_cycle,4096)<0){fault(8);return;}
  }else idle();
  SHARED.steps_hi=(uint32_t)(sequence.steps>>32);SHARED.steps_lo=(uint32_t)sequence.steps;
  SHARED.skipped_hi=(uint32_t)(sequence.skipped>>32);SHARED.skipped_lo=(uint32_t)sequence.skipped;
 }
 for(unsigned slot=0;slot<32;slot++)SOUND16(0x100000u+slot*32u)=0x21;
 SOUND16(0x1003e0u)=0x1021;++generation;SOUND16(0xb024)=generation;
 SHARED.state=0;
}
