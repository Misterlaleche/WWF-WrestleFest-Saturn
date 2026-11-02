/* Compare complete mapper/queue states; never alter or suppress ROM callbacks. */
#include "wf_music_time.h"
static unsigned time_advances;
static uint16_t observed_advance(wf_music_time_t *t,uint64_t cycle)
{++time_advances;return wf_music_time_advance(t,cycle);}
#define wf_music_time_advance observed_advance
#define main preserved_deadline_main
#include "test_music_module_deadline.c"
#undef main
#undef wf_music_time_advance
static FILE *trace;
static uint64_t origin_cycle,input_cycle;
static uint32_t origin_fraction;
static uint16_t origin_frame;
static void record(unsigned kind,unsigned reg,unsigned value,uint64_t delta)
{
 input_cycle+=delta;
 unsigned before=fm_producer,commands=SHARED.events;
 bool ok=event(kind,reg,value,input_cycle,NULL);
 uint64_t numerator=(input_cycle-origin_cycle)*44100u+origin_fraction;
 uint16_t expected=(uint16_t)(origin_frame+numerator/3579545u);
 if(ok&&(fm_producer!=before||SHARED.events!=commands))assert(output_frame==expected);
 wf_scsp_ym_t state=synth;state.write16=NULL;state.user=NULL;
 assert(fwrite(&ok,sizeof ok,1,trace)==1);
 assert(fwrite(&state,sizeof state,1,trace)==1);
 unsigned n=(uint16_t)(fm_producer-fm_consumer);assert(n<=512);
 assert(fwrite(&n,sizeof n,1,trace)==1);
 for(unsigned i=0;i<n;i++)assert(fwrite(&fm_events[(fm_consumer+i)&511],sizeof(fm_event_t),1,trace)==1);
 assert(fwrite(&sound_ram[0xb020u/2u],(0xb900u-0xb020u),1,trace)==1);
 fm_consumer=fm_producer;
 sound_ram[0xb022u/2u]=sound_ram[0xb020u/2u];
 model_frame=expected;
}
int main(int argc,char **argv)
{
 assert(argc==2);trace=fopen(argv[1],"wb");assert(trace);
 reset_fixture();origin_cycle=UINT64_C(0xfffffff0);input_cycle=origin_cycle;
 origin_fraction=3579544;origin_frame=65530;
 event_time=(wf_music_time_t){origin_cycle,origin_fraction,origin_frame};
 unsigned before=time_advances;
 record(WF_MUSIC_YM,0x60,127,100);
 unsigned duplicate_advances=time_advances-before;
 /* Repeated accepted writes cross the narrow/wide division threshold and
    frame16/cycle32 boundaries before a real write reuses the rational clock. */
 for(unsigned i=0;i<1000;i++)record(WF_MUSIC_YM,0x60,127,901);
 record(WF_MUSIC_YM,0x60,126,1);
 for(unsigned group=0;group<20;group++){
  for(unsigned reg=0x20;reg<256;reg++){
   unsigned value=(reg*13u+group*7u)&127u;
   record(WF_MUSIC_YM,reg,value,8);
   record(WF_MUSIC_YM,reg,value,17);
  }
  record(WF_MUSIC_YM,0x18,group,90001);
  record(WF_MUSIC_YM,0x18,group,90000);
  record(WF_MUSIC_YM,0x1b,group&3,71);
  record(WF_MUSIC_YM,0x1b,group&3,71);
  record(WF_MUSIC_YM,0x19,group,100);
  record(WF_MUSIC_YM,0x19,group,100);
  record(WF_MUSIC_YM,0x14,0x2a,71);
 }
 /* A failed synthesizer must still reject an otherwise duplicate write. */
 synth.fault=1;record(WF_MUSIC_YM,0x60,synth.reg[0x60],100);assert(synth.fault);
 assert(fclose(trace)==0);
 printf("{\"duplicate_time_advances\":%u,\"total_time_advances\":%u,\"trace_matches_clock_algebra\":true}\n",duplicate_advances,time_advances);
 return duplicate_advances?2:0;
}
