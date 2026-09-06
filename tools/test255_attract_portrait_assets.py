#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,struct
import sys
sys.path.insert(0,str((Path(__file__).resolve().parent)))
import arcade_composition as comp
R=Path(__file__).resolve().parents[1]
comp.load_program(R/'program.bin')
r=json.loads((R/'docs/test255-attract-portraits.json').read_text())
assert r['sets']=='0x30-0x3b' and r['characters']==12 and r['poses_per_character']==3 and r['frames']==36
assert len(r['details'])==36
for ch in range(12):
 for pose in range(3):
  q=comp.commands(0x30+ch,pose,partner=0,x=160,y=96)
  assert q
  assert {x['bank'] for x in q}=={0x10+ch,0x1e}
assert [x['pieces'] for x in r['details'] if x['character']==10]==[40,40,47]
assert [x['pieces'] for x in r['details'] if x['character']==11]==[44,44,46]
p=(R/'cd/ATTRPRT.PAK').read_bytes(); assert hashlib.sha256(p).hexdigest()==r['pack_sha256']
magic,nf,nr,nt,off,h,bt,nc=struct.unpack_from('>8I',p,0)
assert magic==0x57464332 and nf==36 and nr==r['records'] and nt==r['textures']
assert len((R/'assets/ATTRPRT.PAL').read_bytes())==13*32
print('PASS Test255: all 12 ROM biography portrait sets $30-$3B, including Hawk $3A and Animal $3B, build exactly')
