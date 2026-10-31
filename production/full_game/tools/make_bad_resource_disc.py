#!/usr/bin/env python3
"""Negative test only: alter SELPAL.BIN in a copy, keep executable unchanged."""
import argparse,hashlib,importlib.util,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('iso',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
a.output.mkdir(parents=True,exist_ok=False)
data=bytearray(a.iso.read_bytes());pvd=data[16*2048:17*2048]
assert pvd[1:6]==b'CD001'
root_lba=int.from_bytes(pvd[158:162],'little');root_bytes=int.from_bytes(pvd[166:170],'little')
found=None;pos=0
while pos<root_bytes:
 at=root_lba*2048+pos;size=data[at]
 if not size:pos=(pos//2048+1)*2048;continue
 row=data[at:at+size];name=bytes(row[33:33+row[32]])
 if name==b'SELPAL.BIN;1':
  found=int.from_bytes(row[2:6],'little')*2048
  assert int.from_bytes(row[10:14],'little')==512
  break
 pos+=size
assert found is not None
before=data[found];data[found]^=1
bad=a.output/'corrupt-resource.iso';bad.write_bytes(data)
spec=importlib.util.spec_from_file_location('iso_to_bin',Path(__file__).resolve().parents[1]/'vendor/frontend_test03/iso_to_bin.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
report=mod.convert(bad,a.output/'disc')
report.update(negative_test=True,asset='SELPAL.BIN',iso_byte_offset=found,before=before,after=data[found],original_iso_sha256=hashlib.sha256(a.iso.read_bytes()).hexdigest())
(a.output/'fault.json').write_text(json.dumps(report,indent=2)+'\n')
