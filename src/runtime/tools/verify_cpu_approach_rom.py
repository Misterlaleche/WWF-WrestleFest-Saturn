#!/usr/bin/env python3
"""CPU state1 approach at1CD5C/1CDD0, through its real1C8F4 entry."""
import ctypes, hashlib, os, random, re, subprocess, sys, tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1]
rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();core.remove('src/wf_cpu_runtime.c')
rng=random.Random(0x1cd5c)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'state4.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_knockdown_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 count=int(os.environ.get('WF_STATE4_CASES','864'))
 for case in range(count):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c;p=t if case%3==0 else a+0x218
  w(a+0x20,0x8001);w(a+0x7a,t,4);w(a+0x86,p,4);w(t+0x7a,a,4);w(t+0x86,a,4)
  j=case;tier=((j//12)+1)%3
  w(a+0xae,0);w(a+0x57,j%12,1);w(a+0x33,0x81,1);w(a+0xb5,1 if j%2 else 0,1);w(a+0xb9,0,1)
  w(a+6,563);w(a+10,306);w(t+6,738);w(t+10,306)
  w(t+0x20,0x8000|((j//36)%2));w(t+0x33,3 if j%2 else 1,1);w(t+0x70,tier)
  w(a+0xb4,j%3,1);w(a+0xb2,[12,13,100][(j//3)%3]);w(a+0xb0,[0,11,12,80,176,300][(j//9)%6]);w(a+0xb5,(j//54)%4,1);w(t+0x33,0x11 if (j//216)%2 else 1,1);w(t+0x20,5 if (j//108)%2 else 1);w(t+0x60,0x4c);w(a+0x70,(j//72)%3);w(a+0x66,142);w(t+0x66,45);w(a+0x56,0x8000|j%12)
  w(0x1c007c,0 if j%17==0 and j else 1);w(0x1c16a0,[0x1004,0x10,9][(j//144)%3])
  w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,2583+j,4)
  regs=[rng.getrandbits(32) for _ in range(8)]
  nr=(ctypes.c_uint32*9)(*regs,0);nb=ctypes.create_string_buffer(bytes(mem))
  if case<int(os.environ.get('WF_STATE4_START','0')):continue
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
  for at,n in ((0x1c005c,4),(0x1c1697,1),(0x1c1670,4),(0x1c167a,1),(0x1c1802,2),(0x1c15d2,2),(0x1c15e4,8)):
   assert nb.raw[at:at+n]==bytes(memory.r_block(at,n)),('global',case,hex(at))
  assert list(nr)[:8]==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS',count-int(os.environ.get('WF_STATE4_START','0')),'independent CPU approach ROM vectors: actors, globals, RNG, D0-D7')
