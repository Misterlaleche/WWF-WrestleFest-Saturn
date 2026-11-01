"""Materialize two readable, versioned C templates; no patch-generation chain."""
from pathlib import Path
import argparse,json
from pins import load,sha,verify_rom,verify_sources

def generate(package,rom,out):
 package,rom,out=map(lambda p:Path(p).resolve(),(package,rom,out))
 pin=load(package);verify_sources(package,pin);verify_rom(rom,pin)
 for name in ('z80-native.c','z80-cold.c'):
  data=(package/'templates'/name).read_bytes()
  if b'\r'in data or sha(data)!=pin['generated'][name]['lf_sha256']:
   raise ValueError('Template must match its pinned UTF-8 LF bytes: '+name)
  target=out if name=='z80-native.c'else out.with_name(name)
  target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
 report=dict(pin['native_components']);report.update(snapshot=pin['snapshot'],template_mode=True,
  line_endings='UTF-8 LF, no other normalization',sha256_lf=pin['generated']['z80-native.c']['lf_sha256'])
 return report

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--package',type=Path,default=Path(__file__).resolve().parents[1])
 p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
 a=p.parse_args();print(json.dumps(generate(a.package,a.rom,a.out),indent=2))
