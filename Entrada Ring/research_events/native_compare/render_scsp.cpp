// Standalone validation adapter around an unchanged Mednafen SCSP audio core.
// No 68000/SH-2 emulation or CPU timing claim. Register order/sample timing is
// supplied explicitly by the caller. This program is never run on Saturn.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

#define INLINE inline
#define NO_INLINE __attribute__((noinline))
#define FORCE_INLINE inline __attribute__((always_inline))
#define MDFN_COLD
#define MDFN_LIKELY(x) __builtin_expect(!!(x),1)
#define MDFN_UNLIKELY(x) __builtin_expect(!!(x),0)
#define MDFN_ALIGN(n) alignas(n)
static inline unsigned MDFN_tzcount16(uint16_t x){return x?__builtin_ctz(x):16;}
static inline unsigned MDFN_lzcount32(uint32_t x){return x?__builtin_clz(x):32;}
static inline int32_t sign_x_to_s32(unsigned bits,uint32_t x){return static_cast<int32_t>(x<<(32-bits))>>(32-bits);}
static unsigned sound_irq;static bool main_irq;
static void SCSP_SoundIntChanged(unsigned level){sound_irq=level;}
static void SCSP_MainIntChanged(bool level){main_irq=level;}
#include "primary/scsp_mdfn.h"
#include "primary/scsp_mdfn.inc"

struct Event { uint64_t sample;uint32_t address;uint16_t value; };
static uint64_t number(const std::string &s){return std::stoull(s,nullptr,0);}
static void word(FILE*f,uint16_t v){std::fputc(v&255,f);std::fputc(v>>8,f);}
static void dword(FILE*f,uint32_t v){word(f,v&65535);word(f,v>>16);}
static void wav_header(FILE*f,uint32_t frames){
 std::fwrite("RIFF",1,4,f);dword(f,36+frames*4);std::fwrite("WAVEfmt ",1,8,f);dword(f,16);word(f,1);word(f,2);dword(f,44100);dword(f,44100*4);word(f,4);word(f,16);std::fwrite("data",1,4,f);dword(f,frames*4);
}
int main(int ac,char**av){
 if(ac<5||ac>6){std::fprintf(stderr,"usage: render_scsp RAM_BE.bin writes.csv output.wav frames [debug.csv]\nCSV columns: sample,address,value; addresses are byte offsets from SCSPregisterbase.\n");return 2;}
 auto z=std::make_unique<SS_SCSP>();SS_SCSP_Reset(z.get(),true);
 FILE*ram=std::fopen(av[1],"rb");if(!ram){std::perror(av[1]);return 3;}
 unsigned bytes=0;int hi,lo;
 while((hi=std::fgetc(ram))!=EOF){lo=std::fgetc(ram);if(lo==EOF||bytes>=0x80000){std::fprintf(stderr,"RAM image odd or larger than512KiB\n");return 4;}z->RAM[bytes/2]=(hi<<8)|lo;bytes+=2;}
 std::fclose(ram);
 std::vector<Event> events;std::ifstream in(av[2]);std::string line;std::getline(in,line);
 while(std::getline(in,line)){
  if(line.empty())continue;
  std::stringstream ss(line);std::string a,b,c;std::getline(ss,a,',');std::getline(ss,b,',');std::getline(ss,c,',');
  Event e{number(a),static_cast<uint32_t>(number(b)),static_cast<uint16_t>(number(c))};
  if(e.address>=0x25b00000 && e.address<0x25b01000)e.address-=0x25b00000;
  if(e.address>=0x100000 && e.address<0x101000)e.address-=0x100000;
  if(e.address>=0x1000||e.address&1){std::fprintf(stderr,"Bad register address %x\n",e.address);return 5;}
  if(!events.empty()&&e.sample<events.back().sample){std::fprintf(stderr,"Events not ordered\n");return 6;}
  events.push_back(e);
 }
 FILE*out=std::fopen(av[3],"wb");if(!out)return 7;unsigned frames=std::strtoul(av[4],nullptr,0);wav_header(out,frames);
 FILE*debug=ac==6?std::fopen(av[5],"w"):nullptr;
 if(debug)std::fprintf(debug,"sample,slot,env_phase,env_level,current_addr,phase,octave,fns,key,modlevel,modx,mody\n");
 size_t ei=0;uint64_t clipped=0;int peak=0;
 for(uint64_t n=0;n<frames;n++){
  while(ei<events.size()&&events[ei].sample<=n){auto&e=events[ei++];SS_SCSP_RW_W16(z.get(),0x100000+e.address,e.value);}
  int16_t lr[2];SS_SCSP_RunSample(z.get(),lr);
  word(out,lr[0]);word(out,lr[1]);
  for(auto v:lr){int a=std::abs(static_cast<int>(v));peak=std::max(peak,a);if(a>=32767)clipped++;}
  if(debug && n%44==0)for(unsigned slot=0;slot<24;slot++){
   auto&s=z->Slots[slot];std::fprintf(debug,"%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",static_cast<unsigned long long>(n),slot,s.EnvPhase,s.EnvLevel,s.CurrentAddr,s.PhaseWhacker,s.Octave,s.FreqNum,s.KeyBit,s.ModLevel,s.ModInputX,s.ModInputY);
  }
 }
 std::fclose(out);if(debug)std::fclose(debug);
 std::printf("{\"frames\":%u,\"events_applied\":%zu,\"events_total\":%zu,\"ram_bytes\":%u,\"peak\":%d,\"clipped_channel_samples\":%llu}\n",frames,ei,events.size(),bytes,peak,static_cast<unsigned long long>(clipped));
 return 0;
}
