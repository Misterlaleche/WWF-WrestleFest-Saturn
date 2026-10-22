#!/usr/bin/env python3
"""Independent state-1 knockdown decision vectors against the original ROM."""
import ctypes, hashlib, os, random, re, subprocess, sys, tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1]
rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
core.remove('src/wf_cpu_runtime.c')
rng=random.Random(0x1cbf8)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'knockdown.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_knockdown_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 count=int(os.environ.get('WF_KNOCKDOWN_CASES','4457'))
 for case in range(count):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  # Each vector is independently initialized; never patched ROM or stopped at a dependency.
  sub=[8,9,1,0,10,2,3,4,5,6,7,11,12,13,14][case%15]
  w(a+0x20,0x8001);w(a+0x1c,0x8001);w(a+0x57,(case//15)%12,1)
  w(a+0x33,0x81,1);w(a+0xb7,0x80,1);w(a+0xb5,0x10,1)
  w(a+0x7a,t,4);w(a+0x86,t,4);w(t+0x7a,a,4);w(t+0x86,a,4)
  w(a+6,620);w(a+10,340);w(t+6,660 if case%2 else 580);w(t+10,340)
  w(t+0x2e,0x80 if case%3 else 0,1);w(t+0x20,0x8004);w(t+0x64,sub)
  w(a+0xb0,30);w(t+0x66,100);w(a+0x66,150);w(t+0x70,(case//180)%3)
  w(0x1c007c,1);w(0x1c16a0,0x10);w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,493+case,4)
  if 1800<=case<3600:
   j=case-1800
   w(a+0xb0,[0,7,39,40,127,128,180][j%7]);w(t+0x66,0 if j%5==0 else 100)
   w(t+6,[0x1ff,0x200,0x20f,0x210,0x25f,0x260,0x267,0x268,0x277,0x278,0x287,0x288,0x2d7,0x2d8,0x2df,0x2e0][j%16])
   w(t+10,[0x14f,0x150,0x16f,0x170][(j//16)%4])
   w(0x1c0161,2 if j%4==0 else 0,1)
   if j%6==0:w(0x1c007c,0);w(0x1c0166,1)
  if 3600<=case<3840:
   w(a+0x20,0x8000);w(a+0xb5,0x40 if case%4==0 else 0,1);w(a+0xb7,0,1)
   w(a+0x33,0 if case%2 else 0x81,1);w(t+0x33,0x81 if case%2==0 else 0,1)
   w(a+0x56,0x4002 if case%3==0 else 2);w(a+0xb6,1,1)
   w(t+0x66,0 if case%4==0 else 100)
  if 3840<=case<4080:
   j=case-3840
   w(a+0xae,10);w(a+0x60,[0xa,8,0x3d,0x1c,0x1d,0x30][j%6]);w(a+0xbc,40)
   w(a+0xbe,620 if j%3 else 700);w(a+0xc0,340)
   w(a+0xb5,32 if j%4==0 else 0,1);w(a+0xb7,32 if j%7==0 else 0,1)
   w(t+0x64,8 if j%2 else 1)
  if 4080<=case<4092:
   partner=a+2*0x10c
   w(a+0x20,0x8000);w(a+0x33,0,1);w(a+0xb5,64,1);w(a+0xb7,0,1);w(a+0x86,partner,4)
   w(t+0x64,8);w(partner+0x20,5);w(partner+0x60,0x71);w(partner+0xc6,255,1)
  if 4092<=case<4308:
   j=case-4092
   w(t+0x64,[8,9,1][j%3]);w(a+0x57,(j//3)%12,1)
   w(0x1c007c,0);w(0x1c0166,1);w(a+0x103,(j//36)%6,1)
  if case>=4308:
   j=case-4308
   w(a+0x20,5);w(a+0x60,0x3d);w(a+0xb7,64,1);w(a+0xb5,0,1)
   w(t+0x20,j%9);w(t+0x64,[0,1,8,9,10][(j//9)%5]);w(t+0x60,[0x4c,0x4d,0xd,0xc,0x12,0x11,0xf,0x1b,0x15][j%9])
   w(t+0x33,[0,4,64][(j//9)%3],1);w(t+0x32,2 if j%7==0 else 0,1)
  if case>=4452:
   j=case-4452
   mem[a:a+9*0x10c]=bytes(9*0x10c)
   w(a+0x20,5);w(a+0x60,[9,9,0xa508,8,0x10][j]);w(a+0x7a,t,4)
   w(t+6,[0x250,0x310,0x25f,0x260,0x2a0][j]);w(t+0x2e,128 if j in (2,3) else 0,1)
   w(0x1c007c,1 if j==4 else 0)
  regs=[rng.getrandbits(32) for _ in range(8)]
  nr=(ctypes.c_uint32*9)(*regs,0);nb=ctypes.create_string_buffer(bytes(mem))
  if case<int(os.environ.get('WF_KNOCKDOWN_START','0')):continue
  status=native.wf_cpu_oracle(nb,nr,0)
  assert status==0,('native CPU blocked',case,sub,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem
  memory.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)
  for i in range(9,15):u.w_reg(i,0)
  entry=0x1dfe2 if case>=4308 else 0x1c2cc if 3600<=case<3840 or 4080<=case<4092 else 0x1c8f4
  u.w_reg(9,entry) # $1C1E2 loads the handler in A1 before JSR (A1).
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(entry)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  want=bytes(memory.r_block(a,9*0x10c));got=nb.raw[a:a+9*0x10c]
  assert got==want,('actors',case,sub,[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
  for at,n in ((0x1c005c,4),(0x1c1697,1),(0x1c1670,4),(0x1c167a,1),(0x1c1802,2),(0x1c15d2,2)):
   assert nb.raw[at:at+n]==bytes(memory.r_block(at,n)),('global',case,hex(at))
  assert list(nr)[:8]==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS',count-int(os.environ.get('WF_KNOCKDOWN_START','0')),'independent knockdown ROM vectors: actors, globals, RNG, D0-D7')
