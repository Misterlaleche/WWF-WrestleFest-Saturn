// Validation only: MAME 0.264 YMFM with original timed register writes.
// The generated files are never assets for the Saturn implementation.
#include "ymfm/ymfm_opm.h"
#include "ymfm/ymfm_fm.ipp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Event { long double time; unsigned reg, value; };

class StemChip:public ymfm::ym2151 {
public:
 explicit StemChip(ymfm::ymfm_interface &intf):ym2151(intf){}
 // Channel output is tapped before the common YM3012 DAC quantization.
 // Calling output twice does not clock feedback again; the assigned feedback
 // input is the same function of the already-clocked operator phase/state.
 void stems(std::array<output_data,9> &out){
  m_fm.clock(fm_engine::ALL_CHANNELS);
  for(unsigned ch=0;ch<8;ch++)m_fm.output(out[ch].clear(),0,32767,1U<<ch);
  m_fm.output(out[8].clear(),0,32767,fm_engine::ALL_CHANNELS);
  out[8].roundtrip_fp();
 }
};

static void write_reg(ymfm::ym2151 &chip,unsigned reg,unsigned value){
 chip.write_address(static_cast<uint8_t>(reg));chip.write_data(static_cast<uint8_t>(value));
}

int main(int argc,char **argv){
 if(argc!=5){std::fprintf(stderr,"usage: render_fm input.csv output.f32le start_seconds end_seconds\n");return 2;}
 const long double begin=std::strtold(argv[3],nullptr),end=std::strtold(argv[4],nullptr);
 std::ifstream input(argv[1]);std::string line;std::vector<Event> events;
 std::getline(input,line);
 while(std::getline(input,line)){
  std::stringstream s(line);std::vector<std::string> col;std::string v;
  while(std::getline(s,v,','))col.push_back(v);
  if(col.size()!=7 || col[3]!="YM")continue;
  events.push_back({std::strtold(col[2].c_str(),nullptr),static_cast<unsigned>(std::stoul(col[4])),static_cast<unsigned>(std::stoul(col[5]))});
 }
 ymfm::ymfm_interface intf,check_intf;
 StemChip chip(intf);ymfm::ym2151 check(check_intf);chip.reset();check.reset();
 const unsigned rate=chip.sample_rate(3579545);
 const uint64_t frames=static_cast<uint64_t>(std::ceil(end*rate));
 FILE *out=std::fopen(argv[2],"wb");if(!out)return 3;
 size_t ei=0;uint64_t written=0;uint64_t mismatch=0;
 for(uint64_t n=0;n<frames;n++){
  const long double t=static_cast<long double>(n)/rate;
  while(ei<events.size() && events[ei].time<=t){
   write_reg(chip,events[ei].reg,events[ei].value);
   write_reg(check,events[ei].reg,events[ei].value);ei++;
  }
  std::array<ymfm::ym2151::output_data,9> samples;
  ymfm::ym2151::output_data normal;chip.stems(samples);check.generate(&normal);
  if(normal.data[0]!=samples[8].data[0] || normal.data[1]!=samples[8].data[1])mismatch++;
  if(t>=begin){
   std::array<float,9> f;
   for(unsigned i=0;i<9;i++)f[i]=static_cast<float>((samples[i].data[0]+samples[i].data[1])*0.45/32768.0);
   std::fwrite(f.data(),sizeof(float),f.size(),out);written++;
  }
 }
 std::fclose(out);
 std::printf("{\"native_rate\":%u,\"clock\":3579545,\"channels\":9,\"frames\":%llu,\"first_frame\":%llu,\"events_applied\":%zu,\"normal_generate_mismatches\":%llu}\n",rate,static_cast<unsigned long long>(written),static_cast<unsigned long long>(std::ceil(begin*rate)),ei,static_cast<unsigned long long>(mismatch));
 return mismatch?4:0;
}
