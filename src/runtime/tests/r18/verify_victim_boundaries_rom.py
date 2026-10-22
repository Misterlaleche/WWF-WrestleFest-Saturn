#!/usr/bin/env python3
"""Additional recovery boundaries: CPU flags, lockout, target gating,
rope busy state and timing vs untouched $1C03E. No decision mocks.
"""
import ctypes, hashlib, itertools, random, re, subprocess, sys, tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
core.remove('src/wf_cpu_runtime.c')
rng=random.Random(0x11e42)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'victim.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O1','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_decision_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 count=0
 for case in range(1536):
  subtype=rng.choice([0x1c,2,3,4,8,9,0x107,0,1,0xa,0x20,0xb,0xc,0xd])
  latched=rng.choice([False,True]);frame=rng.choice([0,1,0xfe]);timer=rng.choice([0,1,2,0xff00,0xffff])
  face=rng.choice([0,0x8000]);contact=rng.choice([0,0x510,0x501,0x300,0x102,0x502,0x110,0x310]);subflag=rng.choice([0,0x8000])
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  domain=7 if subtype==0x107 else 4
  w(a+0x20,domain);w(a+0x1c,domain+(0x8000 if latched else 0));w(a+0x64,(subtype&255)|subflag)
  w(a+0x26,t,4);w(t+0x26,a,4);w(a+0x86,t,4);w(t+0x86,a,4)
  w(a+6,620);w(a+10,340);w(t+6,640);w(t+10,340)
  w(a+0x46,[0,1,2][count%3]);w(a+0x52,count%2);w(a+0x24,frame);w(a+0x22,timer);w(a+0x2e,face)
  w(a+0x33,0xc3,1);w(t+0x33,0x43,1);w(a+0x18,0x8000);w(a+0x1a,0x1234)
  w(a+0x36,contact);w(a+0x7a,t,4);w(a+0x9a,[0,1,2][count%3]);w(a+0xaa,2);w(a+0xa3,1,1);w(a+0x70,count%3)
  if subtype==0x107:w(a+0x64,0x2000 if count%3 else 0);w(a+0x1e,5 if count%3==0 else 0)
  if count%5==0:w(a+0xfe,0x8000)
  if count%7==0:w(a+0x32,16,1)
  w(a+10,[280,340,400][count%3]);w(a+0x56,[2,0x8002,0x4002][count%3]);w(0x1c0161,2 if count%5==0 else 0,1)
  w(a+0x32,rng.choice([0,1,3,0x10,0x11,0x80]),1)
  w(a+0x33,rng.choice([0,4,0x20,0x24,0xff]),1);w(t+0x34,rng.choice([0,0x10,0xf0]),1)
  w(a+0xfe,rng.choice([0,1,0x4000,0x8000,0xc001]));w(a+0x1e,rng.choice([0,5,0x105,7]))
  w(a+0x70,rng.choice([0,1,2,0x100,0x101,0x102]));w(a+0x56,rng.choice([0,0x8000,0x4000,0xc000]))
  w(a+0x9a,rng.choice([0,1,2,0x8000,0xffff]));w(a+0xaa,rng.choice([0,1,0x4000,0x8000,0xffff]));w(a+0xa3,rng.choice([0,1,0xff]),1)
  w(a+0x46,rng.choice([0,1,0xffff]));w(a+0x52,rng.choice([0,1,0xffff]));w(0x1c0161,rng.choice([0,1,2,3]),1)
  for address in (0x1150,0x1180,0x11b0,0x11e0):w(0x1c0000+address,rng.choice([0,1,0x100,0xffff]))
  regs=[rng.getrandbits(32) for _ in range(8)]
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*9)(*regs,0)
  label=(hex(subtype),latched,frame,timer,face,contact,subflag)
  status=native.wf_cpu_oracle(nb,nr,257)
  assert status==0,('native blocked',label,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)

  for ar in range(9,15):u.w_reg(ar,0)
  u.w_reg(8,a);u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(0x1c03e)
  for step in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',label,hex(u.r_pc()))
  for at,n in [(a,9*0x10c),(0x1c1670,4),(0x1c167a,1),(0x1c1802,2),(0x1c15d2,2),(0x1c1697,1),(0x1c1150,2),(0x1c1180,2),(0x1c11b0,2),(0x1c11e0,2)]:
   got=nb.raw[at:at+n];want=bytes(m.r_block(at,n))
   assert got==want,('memory',label,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
  assert list(nr)[:8]==[u.r_reg(i) for i in range(8)],('registers',label,list(nr)[:8],[u.r_reg(i) for i in range(8)])
  machine.cleanup();count+=1
 print('PASS',count,'additional victim/main7 boundary ROM cases: actors, globals, D0-D7')
