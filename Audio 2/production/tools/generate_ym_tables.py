"""Convert generate_fm_pitch.c output to the checked-in frequency fact header."""
import argparse,struct
from pathlib import Path

def generate(facts,out):
 data=facts.read_bytes()
 if len(data)!=10240:raise ValueError('Expected4096 KC +1024 FNUM big-endian u16 facts')
 kcode=struct.unpack('>4096H',data[:8192]);fnum=struct.unpack('>1024H',data[8192:])
 # SCSP manual frequency choices, also in MAME scsp.cpp LFOFreq.
 scsp=[.17,.19,.23,.27,.34,.39,.45,.55,.68,.78,.92,1.10,1.39,1.60,1.87,2.27,2.87,3.31,3.92,4.79,6.15,7.18,8.60,10.8,14.4,17.2,21.5,28.7,43.1,57.4,86.1,172.3]
 # OPM4.4 rate accumulator (ymfm_opm.cpp), chip clock/64, 30 phase bits.
 lfo=[min(range(32),key=lambda i:abs(scsp[i]-3579545/64*((16+(x&15))<<(x>>4))/(1<<30))) for x in range(256)]
 text='/* Frequency facts generated from the pinned reference; no synthesizer code. */\n#if defined(__GNUC__)\n#define WF_YM_TABLE __attribute__((section(".wf_fm_tables")))\n#else\n#define WF_YM_TABLE\n#endif\n'
 for name,kind,values,width in [('wf_ym_kcode_map','uint16_t',kcode,16),('wf_ym_fnum','uint16_t',fnum,16),('wf_ym_lfo_rate','uint8_t',lfo,32)]:
  text+=f'static const {kind} {name}[{len(values)}] WF_YM_TABLE={{\n'
  text+=''.join(' '+','.join(map(str,values[i:i+width]))+',\n' for i in range(0,len(values),width))+'};\n'
 out.write_text(text+'#undef WF_YM_TABLE\n')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--facts',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();generate(a.facts,a.out)
