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
 lib=Path(temp)/'state4.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_knockdown_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 count=int(os.environ.get('WF_STATE4_CASES','2048'))
 for case in range(count):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  w(a+0x20,4 if case<192 else 0x8004);w(a+0x64,case%16)
  w(a+0x7a,t,4);w(a+0x86,t,4);w(t+0x7a,a,4);w(t+0x86,a,4)
  w(a+0xb5,(case*7)&255,1);w(a+0xb7,(case*13)&255,1);w(a+0xb9,255,1)
  w(a+0xae,0xa55a);w(a+0xbc,case%5);w(a+0x34,0xff,1);w(a+0x33,0x81|(2 if case%2 else 0),1)
  w(a+0x56,0x8000|0x4000*((case//2)%2)|case%12);w(a+0x66,100);w(t+0x66,130)
  w(a+0x70,(case//16)%3);w(0x1c0162,(case//48)%10);w(0x1c007c,1)
  w(a+6,733);w(a+10,292);w(t+6,748);w(t+10,297)
  w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,605+case,4)
  if 384<=case<1536:
   j=case-384;w(a+0x64,8 if j%2 else 9);w(a+0xb7,2 if j%5==0 else 0,1)
   w(a+0xb5,16 if j%11==0 else 0,1);w(a+0x26,t if j%13 else 0,4)
   w(t+0x20,5 if j%17 else 0);w(t+0x60,[8,0x10,0x46,0xb,9,0x22,0x11,0xc,0x12,0xf,0xa,0xe,0x23,0x35,0x14,0x13,0x47,0][(j//2)%18])
   w(t+0x35,1 if j%3 else 0,1);w(a+0xe5,j%7,1)
   for k in range(7):w(a+0xd2+k*2,(j+k)%7)
   w(0x1c0161,1 if j%7==0 else 0,1);w(0x1c007c,0 if j%19==0 else 1)
  if case>=1536:
   j=case-1536;w(a+0x20,0x8001);w(a+0x64,0);w(a+0xae,0);w(a+0xb5,0,1);w(a+0xb6,0,1);w(a+0xb7,0,1);w(a+0xb9,0,1);w(a+0x34,0,1);w(a+0x33,0x81,1)
   w(t+0x20,0x8007);w(t+0x33,3,1);w(t+0x34,0x40,1);w(t+0x70,j%3);w(0x1c16a0,0x10 if j%2 else 0);w(0x1c007c,0 if j%7==0 else 1)
  if case>=1920:
   w(a+0xae,2+(case%2)) # Both original table entries point to RTS at $1D524.
  regs=[rng.getrandbits(32) for _ in range(8)]
  if 384<=case<1536:regs[4]=0 # $1DD4E adds the full D4; valid table context has zero upper word.
  nr=(ctypes.c_uint32*9)(*regs,0);nb=ctypes.create_string_buffer(bytes(mem))
  if case<int(os.environ.get('WF_STATE4_START','0')):continue
  status=native.wf_cpu_oracle(nb,nr,0)
  assert status==0,('native CPU blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)
  for i in range(9,15):u.w_reg(i,0)
  entry=0x1c8f4 if case>=1536 else 0x1da7e
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
 print('PASS',count-int(os.environ.get('WF_STATE4_START','0')),'independent CPU state4/stand-up ROM vectors: actors, globals, RNG, D0-D7')
