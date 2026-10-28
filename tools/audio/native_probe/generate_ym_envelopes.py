"""Generate the native rate maps from the measured host calibration."""
import argparse,json
from pathlib import Path
def generate(source,out):
 fit=json.loads(source.read_text())
 text='''/* Native rate quantization measured against the pinned R04 OPM and MAME EG.
 * Attack matches90% time; decay/release matches~12dB time. Not physical calibration. */
#if defined(__GNUC__)
#define WF_EG_TABLE __attribute__((section(".wf_fm_tables")))
#else
#define WF_EG_TABLE
#endif
'''
 for name,key in [('wf_ym_attack_rate','attack_map'),('wf_ym_decay_rate','decay_map')]:
  values=fit[key]
  if len(values)!=64 or any(not isinstance(v,int) or not 0<=v<=31 for v in values):raise ValueError('Invalid rate map')
  text+=f'static const uint8_t {name}[64] WF_EG_TABLE={{\n '+','.join(map(str,values))+'\n};\n'
 out.write_text(text+'#undef WF_EG_TABLE\n',encoding='utf-8',newline='\n')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('out',type=Path);a=p.parse_args();generate(a.source,a.out)
