#!/usr/bin/env python3
import sys,zipfile,hashlib
from pathlib import Path
MAIN_SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
MAIN_SHA256='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
REC=[0x1C1134,0x1C1164,0x1C1194,0x1C11C4]
STATE=[x+0x1C for x in REC]
def load(p):
 p=Path(p)
 if p.suffix.lower()=='.zip':
  with zipfile.ZipFile(p) as z:e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
  b=bytearray(len(e)*2);b[0::2]=e;b[1::2]=o;return bytes(b)
 return p.read_bytes()
def be32(b,o):return int.from_bytes(b[o:o+4],'big')
def hits(b,p):
 r=[];s=0
 while 1:
  i=b.find(p,s)
  if i<0:return r
  r.append(i);s=i+1
def assert_bytes(b,o,h):
 e=bytes.fromhex(h);g=b[o:o+len(e)];assert g==e,(hex(o),g.hex(),e.hex())
def main():
 b=load(sys.argv[1]);assert hashlib.sha1(b).hexdigest()==MAIN_SHA1;assert hashlib.sha256(b).hexdigest()==MAIN_SHA256
 assert_bytes(b,0x1004A,'41 f9 00 1c 11 34')
 assert [be32(b,0x10162+i*4) for i in range(4)]==REC
 assert [be32(b,0x10172+i*4) for i in range(5)]==[0x10186,0x101B2,0x10228,0x1029E,0x102F6]
 assert b[0x10210:0x10228]==bytes.fromhex('03 06 04 06 05 06 06 06 02 06 04 06 07 06 08 06 0a 06 09 06 0a 06 08 06')
 assert b[0x10286:0x1029E]==bytes.fromhex('03 10 04 06 02 06 06 06 02 06 04 06 07 10 08 06 0a 06 09 06 0a 06 08 06')
 assert b[0x102EE:0x102F4]==bytes.fromhex('04 0a 05 0a 06 06')
 assert b[0x10346:0x1034A]==bytes.fromhex('06 0e 02 0c')
 expected={
  (STATE[0],1):[0x10FF0,0x11060,0x1B9AA],(STATE[1],1):[0x10FF8,0x11068,0x1B9B2],
  (STATE[2],1):[0x11012,0x1107A,0x1B9BC],(STATE[3],1):[0x1101A,0x11082,0x1B9C4],
  (STATE[0],2):[0x11D70],(STATE[1],2):[0x11D78],(STATE[2],2):[0x11D5E],(STATE[3],2):[0x11D66],
  (STATE[0],3):[0x1839C],(STATE[2],3):[0x183A6],
  (STATE[0],4):[0x19F8A],(STATE[2],4):[0x19F94],
 }
 for (addr,imm),exp in expected.items():
  pat=bytes.fromhex('33fc')+imm.to_bytes(2,'big')+addr.to_bytes(4,'big')
  assert hits(b,pat)==exp,(hex(addr),imm,hits(b,pat),exp)
 assert_bytes(b,0x282D0,'08 e8 00 01 00 37')
 assert_bytes(b,0x28306,'08 e8 00 00 00 37')
 assert_bytes(b,0x10FC6,'08 39 00 01 00 1c 01 61')
 assert_bytes(b,0x10FE0,'08 28 00 01 00 37')
 assert_bytes(b,0x11002,'08 28 00 00 00 37')
 assert_bytes(b,0x11D56,'08 28 00 00 00 37')
 assert_bytes(b,0x18394,'0c 68 02 80 00 06')
 assert_bytes(b,0x19F82,'08 28 00 07 00 33')
 abs_pat=bytes.fromhex('4eb900010fc6')
 abs_exp=[0x12952,0x17FC8,0x18444,0x184A8,0x1A0BA,0x1B160,0x1B5A0,0x1B6B8,0x1B75C,0x1B872,0x1B9DC,0x1BB6C,0x1BC58,0x1BD9E,0x1BE1C,0x1BE90,0x1BF4E]
 assert hits(b,abs_pat)==abs_exp
 bsr=[]
 for i in range(0,len(b)-4,2):
  op=int.from_bytes(b[i:i+2],'big')
  if (op&0xff00)!=0x6100:continue
  d=op&0xff
  if d==0:t=i+2+int.from_bytes(b[i+2:i+4],'big',signed=True)
  elif d==0xff:continue
  else:t=i+2+(d if d<128 else d-256)
  if t==0x10FC6:bsr.append(i)
 assert bsr==[0x11DA4,0x12E88,0x12FB0,0x16648,0x16A1E,0x1769C],bsr
 print('PASS main68000 SHA1',MAIN_SHA1)
 print('PASS quartet init/dispatcher/phase tables exact')
 print('PASS complete immediate state1..4 writer inventory for four rope records')
 print('PASS bit1=low-X side, bit0=high-X side geometry identity anchors')
 print('PASS 23 total calls to producer 0x10FC6: 17 JSR + 6 BSR')
 print('PASS state2/state3/state4 side-selection anchors')
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: audit_ring_rope_rom.py <wwfwfest.zip|main68k.bin>')
 main()
