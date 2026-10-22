#!/usr/bin/env python3
"""Original-ROM vectors for CPU state 4 ($1DA7E) and target stand-up ($1DE66)."""
import ctypes, hashlib, os, random, re, subprocess, sys, tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1]
rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();core.remove('src/wf_cpu_runtime.c')
rng=random.Random(0x1da7e)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'defender.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_knockdown_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 count=1296
 for case in range(count):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c;p=t+0x10c
  selectors=[0xff,0x4a,0x53,0x61,9,0x22,0x3b,0x36,0x1c,0x25,0x1d,0x48,0xf,0xe,0x1a,0x23,0x77,0x37]
  for i in range(9):
   at=a+i*0x10c;w(at,0x8000);w(at+2,i);w(at+6,600+i*16);w(at+10,320+i*4);w(at+0x86,p if i==0 else a,4);w(at+0x7a,t if i==0 else a,4)
  w(a+0x20,0x8001);w(a+0xb5,0x80|(case%2),1);w(a+0xb7,0x80,1);w(a+0xb9,2,1);w(a+0x57,(case//18)%12,1);w(a+0x26,t,4);w(a+0x70,case%3)
  w(t+0x20,5);w(t+0x60,selectors[case%18]);w(t+0x70,(case//216)%3);w(t+0x64,8);w(t+0x2e,0x8000 if case%2 else 0)
  w(a+0xb0,case%200);w(a+0xb2,case%12);w(a+6,500 if case%2 else 780);w(a+0x2e,0x8000 if case%3 else 0)
  w(0x1c007c,1);w(0x1c0161,0,1);w(0x1c0162,(case//3)%10);w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,case+2314,4)
  regs=[rng.getrandbits(32) for _ in range(8)]
  nr=(ctypes.c_uint32*9)(*regs,0);nb=ctypes.create_string_buffer(bytes(mem))
  if case<int(os.environ.get('WF_DEFENDER_START','0')):continue
  status=native.wf_cpu_oracle(nb,nr,0)
  assert status==0,('native CPU blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)
  for i in range(9,15):u.w_reg(i,0)
  entry=0x1c8f4
  u.w_reg(9,entry);u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(entry)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  want=bytes(memory.r_block(a,9*0x10c));got=nb.raw[a:a+9*0x10c]
  assert got==want,('actors',case,[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
  for at,n in ((0x1c005c,4),(0x1c1697,1),(0x1c1670,4),(0x1c167a,1),(0x1c1802,2),(0x1c15d2,2)):
   assert nb.raw[at:at+n]==bytes(memory.r_block(at,n)),('global',case,hex(at))
  assert list(nr)[:8]==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS',count-int(os.environ.get('WF_DEFENDER_START','0')),'independent CPU B5.bit7 assist1F5B2 ROM vectors: actors, globals, RNG, D0-D7')
