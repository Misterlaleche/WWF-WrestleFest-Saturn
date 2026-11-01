#include "wf_music_module.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t wf_music_sound_rom[65536];
wf_music_module_shared_t wf_music_test_shared;
static uint16_t ram[0x80000/2],regs[512];static unsigned frame,seen,changes,commands,latches[8],latch_count;
void wf_music_test_latch(unsigned value){assert(latch_count<8);latches[latch_count++]=value;}
void wf_music_module_prepare(void);void wf_music_module_run(void);
uint16_t *wf_music_test_sound(unsigned address){
 assert(!(address&1u));
 if(address>=0x100000u){assert(address<0x100400u);seen|=1u<<((address-0x100000u)/32u);return &regs[(address-0x100000u)/2];}
 assert(address<0x80000u);return &ram[address/2];
}
static void command(unsigned c){
 wf_music_module_shared_t*s=&wf_music_test_shared;
 assert((uint16_t)(s->producer-s->consumer)<64);
 s->commands[s->producer&63u]=(uint16_t)c;++s->producer;++commands;
}
void wf_music_test_idle(void){
 ++frame;ram[0xb010/2]=(uint16_t)frame;
 uint16_t *h=ram+0xb020/2;
 if(h[2]!=changes){changes=h[2];h[5]=h[2];}
 for(unsigned n=0;n<12&&h[1]!=h[0];n++){
  uint16_t*e=ram+(0xb100+(h[1]&255)*8)/2;
  if(e[3]==h[2]&&(int16_t)((uint16_t)frame-e[0])<0)break;
  assert((e[1]>>8)==1||(e[1]>>8)==2||(e[1]>>8)==4);++h[1];
 }
 if(wf_music_test_shared.state==WF_MUSIC_RUNNING){
  if(!commands){command(6);command(0x1f);}
  else if(commands==2&&frame>25000)command(18);
  else if(commands==3&&frame>70000)command(0x1f);
  else if(commands==4&&frame>120000)command(0);
  else if(commands==5&&frame>121000)wf_music_test_shared.shutdown=1;
 }
 assert(frame<200000);
}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(wf_music_sound_rom,1,65536,f)==65536);fclose(f);
 ram[0xb026/2]=2;wf_music_module_prepare();assert(wf_music_test_shared.state==WF_MUSIC_BOOTING);
 wf_music_module_run();assert(!wf_music_test_shared.state&&!wf_music_test_shared.error);
 assert(commands==5&&wf_music_test_shared.producer==wf_music_test_shared.consumer);
 for(unsigned i=0;i<latch_count;i++)printf("latch%u=%02x ",i,latches[i]);puts("");fflush(stdout);
 assert(latch_count==5&&latches[0]==0&&latches[1]==6&&latches[2]==0x1f&&latches[3]==18&&latches[4]==0x1f);
 assert(wf_music_test_shared.generation==4&&frame>65536&&seen==0xffffffffu);
 printf("PASS slave module: boot,32 slots,6->18->fade->stop,command FIFO,generation and Fs wrap;events=%u frames=%u\n",wf_music_test_shared.events,frame);return 0;
}
