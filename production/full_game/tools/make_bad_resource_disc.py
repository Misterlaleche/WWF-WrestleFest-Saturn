#!/usr/bin/env python3
"""Negative test only: alter one asset byte in a copy, keep executable unchanged."""
import argparse,hashlib,importlib.util,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('iso',type=Path);p.add_argument('output',type=Path)
p.add_argument('--asset',choices=['SELPAL.BIN','FRONT.TEX'],default='SELPAL.BIN');a=p.parse_args()
a.output.mkdir(parents=True,exist_ok=False)
data=bytearray(a.iso.read_bytes());pvd=data[16*2048:17*2048]
assert pvd[1:6]==b'CD001'
root_lba=int.from_bytes(pvd[158:162],'little');root_bytes=int.from_bytes(pvd[166:170],'little')
found=None;pos=0
while pos<root_bytes:
 at=root_lba*2048+pos;size=data[at]
 if not size:pos=(pos//2048+1)*2048;continue
 row=data[at:at+size];name=bytes(row[33:33+row[32]])
 if name==(a.asset+';1').encode():
  found=int.from_bytes(row[2:6],'little')*2048
  size=int.from_bytes(row[10:14],'little');assert size>0 and found+size<=len(data)
  if a.asset=='SELPAL.BIN':assert size==512
  break
 pos+=size
assert found is not None
before=data[found];data[found]^=1
bad=a.output/'corrupt-resource.iso';bad.write_bytes(data)
spec=importlib.util.spec_from_file_location('iso_to_bin',Path(__file__).resolve().parents[1]/'vendor/frontend_test03/iso_to_bin.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
report=mod.convert(bad,a.output/'disc')
report.update(negative_test=True,asset=a.asset,iso_byte_offset=found,before=before,after=data[found],original_iso_sha256=hashlib.sha256(a.iso.read_bytes()).hexdigest())
(a.output/'fault.json').write_text(json.dumps(report,indent=2)+'\n')
