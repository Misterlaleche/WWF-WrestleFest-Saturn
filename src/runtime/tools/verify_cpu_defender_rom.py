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
 count=3584
 for case in range(count):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c;c=t+0x10c;p=c+0x10c
  for i in range(9):
   at=a+i*0x10c;w(at,0x8000);w(at+2,i);w(at+6,600+i*16);w(at+10,320+i*4);w(at+0x86,p if i==0 else a,4)
  w(a+0x20,0x8000);w(a+0x7a,t,4);w(a+0x7e,0 if case%11==0 else c,4);w(a+0x26,t,4)
  w(a+0x33,[0,1,4,0x81,0x85][case%5],1);w(a+0x32,0xff,1);w(a+0x34,[0,4,16,20][(case//5)%4],1)
  w(a+0xb5,0x80|[0,8,64,72][(case//20)%4],1);w(a+0xb6,[0,16,64,80][(case//80)%4],1);w(a+0xb7,[0,1,16,32][(case//320)%4],1)
  w(a+0x56,0x8000 if case%3 else 0);w(a+0x70,(case//10)%3);w(a+0xae,0xa55a);w(a+0x44,0xa55a);w(a+0x46,0x5aa5);w(a+0xbc,0x37)
  for at in (t,c):
   w(at+0x20,[0,4,4,4,5][case%5]);w(at+0x64,[1,6,8,9][(case//5)%4]);w(at+0x60,0x22);w(at+0x33,4 if case%7 else 0,1)
   w(at+6,[0,511,512,591,592,639,640,767,768,65535][(case//20)%10]);w(at+10,[0,311,312,319,320,327,328,65535][(case//200)%8]);w(at+0x2e,0x8000 if case%2 else 0)
  w(c,0 if case%13==0 else 0x8000);w(c+0x32,16 if case%17==0 else 0,1);w(c+0x35,1 if case%19 else 0,1)
  w(0x1c007c,1);w(0x1c0161,0,1);w(0x1c0162,(case//3)%10);w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,case+2313,4)
  if case>=3072:
   j=case-3072;w(a+0xb5,[0,8,64,72][j%4],1);w(a+0x34,4 if j%5 else 20,1);w(a+0xb7,255,1);w(0x1c0161,j%3,1);w(t,0 if j%7==0 else 0x8000);w(t+0x32,16 if j%11==0 else 0,1)
  regs=[rng.getrandbits(32) for _ in range(8)]
  nr=(ctypes.c_uint32*9)(*regs,0);nb=ctypes.create_string_buffer(bytes(mem))
  if case<int(os.environ.get('WF_DEFENDER_START','0')):continue
  status=native.wf_cpu_oracle(nb,nr,0)
  assert status==0,('native CPU blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)
  for i in range(9,15):u.w_reg(i,0)
  entry=0x1c2cc
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
 print('PASS',count-int(os.environ.get('WF_DEFENDER_START','0')),'independent CPU B5.bit7 defender ROM vectors: actors, globals, RNG, D0-D7')
