#!/usr/bin/env python3
"""Raw59: released links must follow actual ROM dereference timing and bus writes."""
import ctypes, hashlib, random, re, subprocess, sys, tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1]
rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
rng=random.Random(0x19274)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'raw59.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/raw59_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_raw59_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(1536):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  actors=[0x1c05b0+0x10c*k for k in range(9)];a,t,p,q=actors[:4]
  for i,addr in enumerate(actors):
   w(addr,0x8000);w(addr+2,i);w(addr+6,600+i*15);w(addr+10,340+i*5)
   w(addr+0x26,actors[i^1] if i<8 else a,4);w(addr+0x86,actors[(i+2)%4],4);w(addr+0x7a,t,4)
   for at in (0x33,0x34,0x35,0x56,0xb5):w(addr+at,rng.randrange(256),1)
  # First vector is the integration frontier: already-latched animation, no link.
  phase=case%6;link=[0,0x80000000,0x01000000,0xff000000,t][(case//6)%5]
  w(a+0x20,5);w(a+0x1c,5 if phase==1 else 0x8005)
  w(a+0x60,0x59|[0,0x4000,0x8000,0xc000][(case//30)%4])
  frame,timer=[(1,12),(0,0),(0,0),(0xfe,0),(0,1),(1,0xff00)][phase]
  if phase in (0,3,4,5) and case%17==0 and case:link=0x00dead00
  w(a+0x26,link,4);w(a+0x24,frame);w(a+0x22,timer);w(a+0x2e,0x8000 if case%2 else 0)
  w(a+0x18,rng.randrange(65536));w(a+0x1a,rng.randrange(65536));w(a+0x86,p,4);w(t+0x86,q,4)
  w(a+0xfe,0);w(a+0x7e,p if case%7==0 else 0,4)
  w(0x1c0161,[0,1,2][(case//120)%3],1);w(0x1c007c,1);w(0x1c1697,case&255,1)
  w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,case+2496,4)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
  status=native.wf_raw59_oracle(nb,nr)
  assert status==0,('native raw59 blocked',case,hex(link),phase,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for page in range(0,0x80000,0x10000):
   for width in range(3):
    memory.set_special_range_write_func(page,width,lambda address,value:None)
    memory.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for i in range(8,15):u.w_reg(i,0)
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(0x1c03e)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   if u.r_pc()==0x2052:u.w_pc(memory.r32(u.r_sp()));u.w_sp(u.r_sp()+4);continue
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  for at,n in [(a,9*0x10c),(0x1c005c,4),(0x1c1682,2),(0x1c168c,4),(0x1c1692,2),(0x1c167a,1),(0x1c15d2,2),(0x1c1697,1),(0x1c1800,2)]:
   want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,('memory',case,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  assert list(nr)==[u.r_reg(i) for i in range(8)],('D0-D7',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS1536 raw59 released/null/tagged/unused-unmapped link ROM cases: actors, globals, RNG, D0-D7 and bounded tail')
