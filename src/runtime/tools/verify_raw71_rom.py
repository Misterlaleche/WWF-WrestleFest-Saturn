#!/usr/bin/env python3
"""Raw71 tag encouragement and immediate effects against unmodified ROM."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();rng=random.Random(0x132f4)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'raw71.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/raw71_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_raw71_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(1152):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0
  w(a,0x80ff);w(a+2,case%12);w(a+0x20,5);w(a+0x1c,5 if case%3==0 else 0x8005);w(a+0x60,0x71)
  w(a+6,600);w(a+10,344);w(a+14,32);w(a+0x19,rng.randrange(256),1)
  w(a+0x24,[0,1,0xfe,0x1fe][(case//3)%4]);w(a+0x22,[0,1,12,0xff00][(case//12)%4]);w(a+0x44,[0,1,6,65535][(case//48)%4])
  w(a+0x33,rng.choice([0,0x80,0x20,0xa0]),1);w(a+0x34,rng.choice([0,1,2,3,255]),1);w(a+0xe6,[0,1,2][(case//4)%3]);w(a+0xc6,255,1)
  w(a+0x2e,0x8000 if case%2 else 0);w(0x1c17e6,480);w(0x1c17ee,120)
  for i in range(12):
   at=0x1c1258+i*0x2a;mem[at:at+0x2a]=bytes(rng.randrange(256) for _ in range(0x2a));w(at,0x8000 if i<case%12 else 0)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_raw71_oracle(nb,nr)
  assert status==0,('native raw71 blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i in range(8,15):u.w_reg(i,0)
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(0x1c03e)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  for at,n in [(a,9*0x10c),(0x1c005c,4),(0x1c1682,10),(0x1c168c,4),(0x1c1692,2),(0x1c167a,1),(0x1c15d2,2),(0x1c15e4,8),(0x1c1697,1),(0x1c1258,12*0x2a),(0x1c1150,2),(0x1c11b0,2)]:
   want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,('memory',case,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  for k in range(6):
   lp=int.from_bytes(rom[0x2806+k*8:0x280a+k*8],'big');cp=int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big')
   assert nb.raw[cp:cp+2]==bytes(memory.r_block(cp,2)),('queue count',case,k)
   assert nb.raw[lp:lp+4]==bytes(memory.r_block(lp,4)),('queue list',case,k)
  assert list(nr)==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS 1152 raw71 ROM fixtures: all actors, effects, queues, globals and D0-D7')
