/* Runs only on the dedicated slave SH2. Synthesis remains entirely SCSP. */
#include "wf_music_module.h"
#include "wf_music_seq.h"
#include "wf_music_trace.h"
#include "wf_music_trace_clock.h"
#include "wf_scsp_ym.h"
#include "wf_music_time.h"
#include "wf_music_clock.h"
#include <string.h>
extern const uint8_t wf_music_sound_rom[];
extern const uint8_t wf_music_result5[],wf_music_result9[];
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
static wf_music_seq_t sequence,boot_snapshot;
static wf_music_trace_t trace;
static wf_music_trace_clock_t trace_clock;
static bool tracing,result_waiting,result_tracing;
static wf_scsp_ym_t synth;
static wf_music_time_t event_time;
static uint16_t output_frame;
typedef struct {uint16_t when,index,value,reserved;} fm_event_t;
static fm_event_t fm_events[512];
static uint16_t fm_producer,fm_consumer;
static bool booting,playing;
static bool fm_flush(void)
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

 uint16_t producer=SOUND16(0xb020),consumer;
 do{
  consumer=SOUND16(0xb022);
  if(SOUND16(0xb026)!=2||SHARED.shutdown)return false;
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
 while(SOUND16(0xb02a)!=generation){
  uint16_t state=SOUND16(0xb026);
  if(SHARED.shutdown||state!=2){
   SHARED.events=SHARED.shutdown?1201u:1202u;
   SHARED.steps_hi=((uint32_t)generation<<16)|SOUND16(0xb02a);
   SHARED.steps_lo=(uint32_t)state<<16;
   return false;
  }
  idle();
 }
 return true;
}
static bool synth_write(uint32_t address,uint16_t value,void *user)
{
 (void)user;
 if(address<0x100000u||address>=0x100400u||(address&1u))return false;
 if(booting){SOUND16(address)=value;return true;}
 while((uint16_t)(fm_producer-fm_consumer)>=512u){
  if(SHARED.shutdown||!fm_flush())return false;
  idle();
 }
 fm_event_t *e=&fm_events[fm_producer&511u];
 e->when=output_frame;
 e->index=(uint16_t)((address-0x100000u)/2u);e->value=value;e->reserved=0;
 ++fm_producer;return true;
}
static bool trace_clock_only(unsigned kind,unsigned reg,unsigned value,uint64_t cycle,void *user)
{
 (void)user;wf_music_trace_clock_event(&trace_clock,kind,reg,value,cycle);return true;
}
static bool event(unsigned kind,unsigned reg,unsigned value,uint64_t cycle,void *user)
{
 (void)user;
 if(kind==WF_MUSIC_IRQ)return true;
 if(tracing||result_tracing)wf_music_trace_clock_event(&trace_clock,kind,reg,value,cycle);
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
 memset((void *)&SHARED,0,sizeof(wf_music_module_shared_t));
 fm_producer=fm_consumer=0;
 if(!wf_music_seq_init(&sequence,wf_music_sound_rom,0xc000,event,NULL,true)){fault(1);return;}
 SHARED.state=WF_MUSIC_BOOTING;booting=true;playing=false;generation=1;SOUND16(0xb024)=generation;
}
void wf_music_module_run(void)
{
 if(SHARED.state!=WF_MUSIC_BOOTING)return;
 if(!generation_ready()){fault(12);return;}
 wf_scsp_ym_init(&synth,synth_write,NULL);if(synth.fault){fault(9);return;}
 while(wf_music_seq_run(&sequence,2ull*3579545u,1024)>0){if(SHARED.shutdown)return;}
 if(sequence.error){fault(2);return;}
 boot_snapshot=sequence;booting=false;last_frame=SOUND16(0xb010);
 SHARED.state=WF_MUSIC_RUNNING;
 while(!SHARED.shutdown){
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
    bool preserve_clock=tracing&&(command==5u||command==9u);
    uint16_t switch_frame=now;
    fm_consumer=fm_producer=0;
    ++generation;SOUND16(0xb024)=generation;SHARED.generation=generation;
    if(!generation_ready()){fault(12);return;}
    now=SOUND16(0xb010);last_frame=now;
    uint64_t resume_cycle=0;uint32_t resume_fraction=0;
    if(preserve_clock){
     uint64_t numerator=((uint64_t)accumulated+(uint16_t)(now-switch_frame))*3579545u+fraction;
     resume_cycle=target_cycle+numerator/44100u;resume_fraction=(uint32_t)(numerator%44100u);
     int result;
     do{result=wf_music_trace_run(&trace,resume_cycle,1024,trace_clock_only,NULL);}while(result>0);
     if(result<0){fault(16);return;}
     wf_music_trace_clock_advance(&trace_clock,resume_cycle);
    }
    sequence=boot_snapshot;sequence.cpu.userdata=&sequence;
    if(preserve_clock)wf_music_trace_clock_resume(&trace_clock,&sequence,resume_cycle);
    accumulated=0;fraction=resume_fraction;target_cycle=sequence.cycles;
    event_time=(wf_music_time_t){sequence.cycles,0,(uint16_t)(now+4096u)};
    output_frame=event_time.frame;wf_scsp_ym_stop(&synth);
    playing=command!=0;tracing=command==4;
    result_tracing=false;result_waiting=preserve_clock;
    if(tracing)wf_music_trace_clock_init(&trace_clock,&sequence);
    if(tracing&&!wf_music_trace_open(&trace,(const void *)(uintptr_t)0x25c08120u,188280u,4)){fault(13);return;}
    if(tracing&&trace.boot_cycle!=sequence.cycles){fault(14);return;}
    if(playing&&!tracing&&!wf_music_seq_send(&sequence,(uint8_t)command)){fault(5);return;}
   }else if(command==0x1f){
    if(tracing||result_tracing||result_waiting){fault(15);return;} /* Trace command4 has no fade-state reconstruction. This request is outside the verified direct TAG combat command set and must fail explicitly. */
    if(playing&&!wf_music_seq_send(&sequence,0x1f)){fault(6);return;}
   }else{fault(7);return;}
   SHARED.consumer=(uint16_t)(consumer+1u);
  }
  if(playing&&accumulated>=64u){
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
   if(tracing||result_tracing){
    int result=wf_music_trace_run(&trace,target_cycle,1024,event,NULL);
    if(result<0){fault(16);return;}
    sequence.cycles=result?trace.next_cycle-1u:target_cycle;
   }else if(result_waiting){
    int result=wf_music_seq_run_to_poll(&sequence,target_cycle,1024);
    if(result<0){fault(8);return;}
    if(result==2){
     unsigned command=SHARED.last_command;
     wf_music_trace_clock_init(&trace_clock,&sequence);
     const uint8_t *data=command==5u?wf_music_result5:wf_music_result9;
     unsigned bytes=command==5u?3168u:4010u;
     if(!wf_music_trace_open_phase(&trace,data,bytes,command,&trace_clock,sequence.cycles)){fault(17);return;}
     result_waiting=false;result_tracing=true;
    }
   }else if(wf_music_seq_run(&sequence,target_cycle,1024)<0){fault(8);return;}
  }else idle();
  SHARED.steps_hi=(uint32_t)(sequence.steps>>32);SHARED.steps_lo=(uint32_t)sequence.steps;
  SHARED.skipped_hi=(uint32_t)(sequence.skipped>>32);SHARED.skipped_lo=(uint32_t)sequence.skipped;
 }
 for(unsigned slot=0;slot<32;slot++)SOUND16(0x100000u+slot*32u)=0x21;
 SOUND16(0x1003e0u)=0x1021;++generation;SOUND16(0xb024)=generation;
 SHARED.state=0;
}
