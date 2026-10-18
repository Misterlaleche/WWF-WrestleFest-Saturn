#!/usr/bin/env python3
"""Independent test-only Musashi oracle (machine68k==0.4.1).
Usage: python3 tools/verify_royal_rom.py canonical-main68k.bin native-royal-probe
No ROM patch, instruction interception, or gameplay callback substitution.
Production runtime has no emulator dependency.
"""
import machine68k as mus
from pathlib import Path
import struct,subprocess,tempfile,random,sys,hashlib
rom=Path(sys.argv[1]).read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
probe=sys.argv[2]
tmp=tempfile.TemporaryDirectory();tmp_path=Path(tmp.name);seen=set();maximum_writes=0
def w(b,a,v):b[a:a+2]=struct.pack('>H',v&65535)
def l(b,a,v):b[a:a+4]=struct.pack('>I',v&0xffffffff)
def actor(b,i,role):
 a=0x5b0+i*0x10c;w(b,a,0x8000);w(b,a+0x56,role);l(b,a+0x26,0x1c05b0+5*0x10c);l(b,a+0x9c,0x1c05b0+6*0x10c);l(b,a+0x8a,0x140020+2*(i%4));return a
cases=[]
def base():
 b=bytearray(0x6040);b[0x2000:0x6000]=bytes([0xa5])*0x4000;b[0x6000:0x6008]=bytes([255])*8
 for i in range(8):l(b,0x6008+i*4,0xa1230000+i*0x119)
 w(b,0x7c,1);return b
cases.append(('idle',base()))
b=base();w(b,0x1586,0x8000);cases.append(('newselect',b))
b=base();w(b,0x1586,0x8000);w(b,0x6000,0xffef);cases.append(('confirm0',b))
b=base();w(b,0x1588,0x8000);w(b,0x6002,0xffef);cases.append(('confirm1',b))
for direction in (0xfffe,0xfffd,0xffff):
 for repeat in (0,5,0xffff):
  b=base();w(b,0x1586,0x8000);w(b,0x1598,0x8000);w(b,0x15b6,repeat);w(b,0x15d0,200);w(b,0x6000,direction);cases.append(('direction%x-repeat%x'%(direction,repeat),b))
b=base();w(b,0x1586,0x8000);w(b,0x1598,0x8000);w(b,0x15d0,1);cases.append(('timeout',b))
for direction in (0xffff,0xffef,0xfffe,0xfffd):
 b=base();w(b,0x1586,0x8000);w(b,0x6000,direction)
 for i,role in enumerate([1,2,3,6,7,8,9,10],1):actor(b,i,role)
 cases.append(('fullpool%x'%direction,b))
for count in (2,3,4):
 b=base()
 for i in range(count):w(b,0x1586+2*i,0x8000);w(b,0x6000+2*i,0xffef)
 cases.append(('concurrent-confirm%d'%count,b))
for slot in range(4):
 b=base();w(b,0x1586+2*slot,0x8000);w(b,0x6000+2*slot,0xffef)
 cases.append(('confirm-slot%d'%slot,b))
 b=base();w(b,0x1586+2*slot,0x8000);w(b,0x6000+2*slot,0xffef)
 for i,role in enumerate([0,1,2,3,6,7,8,9,10]):
  if i==slot:continue
  a=actor(b,i,role);b[a+0x1d]=1;b[a+0xaf]=11
 cases.append(('migrate-slot%d'%slot,b))
for seed in range(16):
 rng=random.Random(seed);b=base();count=rng.randrange(1,5)
 for i in range(count):w(b,0x1586+2*i,0x8000);w(b,0x6000+2*i,rng.choice([0xffff,0xffef,0xffdf,0xfffe,0xfffd]))
 for i in range(8):l(b,0x6008+i*4,rng.getrandbits(32))
 for i in range(9):
  a=0x5b0+i*0x10c
  for off in list(range(4,0x1c))+list(range(0x38,0x56))+list(range(0xb0,0xfe)):b[a+off]=rng.getrandbits(8)
 cases.append(('seed%d'%seed,b))
for name,b in cases:
 m=mus.Machine(mus.CPUType.M68000,2048);u=m.cpu;mem=m.mem;mem.w_block(0,rom);mem.w_block(0x1c0000,bytes(b[:0x2000]));mem.w_block(0xc0000,bytes(b[0x2000:0x6000]));mem.w_block(0x140020,bytes(b[0x6000:0x6008]));u.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000)
 for i in range(8):u.w_reg(i,struct.unpack_from('>I',b,0x6008+i*4)[0])
 for i in range(7):u.w_reg(8+i,0xb1230000+i*111)
 writes=[]
 mem.set_trace_mode(True)
 mem.set_trace_func(lambda op,sz,a,v: writes.append((a,v,1<<sz)) if op=='W' and 0xc0000<=a<0xc4000 else None)
 u.w_pc(0x9132)
 for steps in range(300000):
  if u.r_pc()==0x70000:break
  seen.add(u.r_pc());u.execute(1)
 assert u.r_pc()==0x70000,(name,hex(u.r_pc()))
 out=bytearray(b);out[:0x2000]=mem.r_block(0x1c0000,0x2000);out[0x2000:0x6000]=mem.r_block(0xc0000,0x4000)
 for i in range(8):l(out,0x6008+4*i,u.r_reg(i))
 for i in range(7):assert u.r_reg(8+i)==0xb1230000+i*111,(name,"A register",i)
 m.cleanup()
 (tmp_path/'in').write_bytes(b);subprocess.run([probe,str(tmp_path/'in'),str(tmp_path/'out'),str(tmp_path/'trace')],check=True);native=(tmp_path/'out').read_bytes()
 trace=[tuple(int(v,16) if i<2 else int(v) for i,v in enumerate(line.split())) for line in (tmp_path/'trace').read_text().splitlines()]
 assert writes==trace,(name,'write trace',next(((i,x,y) for i,(x,y) in enumerate(zip(writes,trace)) if x!=y),('length',len(writes),len(trace))))
 diff=[(hex(i),hex(x),hex(y)) for i,(x,y) in enumerate(zip(out,native)) if x!=y]
 assert not diff,(name,diff[:20])
 maximum_writes=max(maximum_writes,len(writes))
 print(name,'PASS',len(writes),'writes')

required={0x7506,0x75ee,0x7680,0x76aa,0x76e8,0x7720,0x776c,0x77d8,0x782c,0x1079e,0x9132,0x9398,0x94ca,0x9556,0x956c,0x9578,0x95ae,0x961c,0x9658,0x96a0,0x96c6,0x972c}
assert required<=seen, sorted(required-seen)
print('PASS %d unmodified-ROM fixtures; all22 native routines reached; fullRAM/FG0/D0-D7/A0-A6 and ordered writes; maximum%d writes'%(len(cases),maximum_writes))
tmp.cleanup()
