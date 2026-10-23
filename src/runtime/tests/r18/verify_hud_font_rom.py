#!/usr/bin/env python3
"""Every font pixel versus original chip bitplanes, plus deterministic asset CLI."""
import hashlib,subprocess,sys,tempfile,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
rom=Path(sys.argv[1]);z=zipfile.ZipFile(rom);member=next(n for n in z.namelist() if Path(n).name=='31e12-0.ic33');chip=z.read(member)
assert hashlib.sha1(chip).hexdigest()=='2e9418e372da85ea597977d912d8b35753655f4e'
with tempfile.TemporaryDirectory() as t:
 out=Path(t)/'FONT8.BIN';subprocess.run([sys.executable,str(root/'tools/build_r18_font.py'),str(rom),str(out)],check=True)
 packed=out.read_bytes();assert len(packed)==131072
 for code in range(4096):
  for y in range(8):
   for x in range(8):
    source=chip[code*32+(x//2)*8+y];expected=0
    for weight,bit in zip((8,4,2,1),(6+x%2,4+x%2,2+x%2,x%2)):expected|=((source>>bit)&1)*weight
    b=packed[code*32+y*4+x//2];actual=(b>>(0 if x%2 else 4))&15
    assert actual==expected,(code,x,y,actual,expected)
 print('PASS 4096 original-ROM glyphs /262144 pixels; FONT8 SHA256',hashlib.sha256(packed).hexdigest())
