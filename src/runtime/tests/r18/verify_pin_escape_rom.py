#!/usr/bin/env python3
"""Raw $4B failed-pin escape vs unmodified $1C03E ROM chain.
Catches missing escape dispatch, wrong partner release, repositioning, timer,
register leakage and the $21282 TAG/royal cleanup side effects. No hooks.
"""
import ctypes,hashlib,random,re,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
rng=random.Random(0x1820c)
with tempfile.TemporaryDirectory(prefix='wf-pin-escape-') as temp:
 lib=Path(temp)/'escape.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/pin_actions_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_pin_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(1536):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  actors=[0x1c05b0+i*0x10c for i in range(9)];a,t,p,q=actors[:4]
  for i,at in enumerate(actors):
   mem[at:at+0x10c]=rng.randbytes(0x10c)
   w(at,0x8000);w(at+0x26,actors[i^1] if i<8 else a,4);w(at+0x86,actors[(i+2)%4],4)
   w(at+0x7e,actors[(i+4)%9] if case%3 else 0,4)
   w(at+0x33,rng.choice([0,1,3]),1);w(at+0x32,rng.choice([0,1]),1)
   w(at+0x56,rng.choice([0,0x4000,0x8000,0xc000]));w(at+0x34,rng.getrandbits(8),1)
  w(a+0x18,0);w(a+0x1c,5 if case%2==0 else 0x8005);w(a+0x20,5);w(a+0x60,0x4b)
  w(a+0x24,[0,1,0xfe][case//2%3]);w(a+0x22,[0,1,12,0xff00][case//6%4]);w(a+0x64,[8,9][case//24%2]);w(a+0x2e,0x8000 if case//48%2 else 0)
  w(a+0x9c,p,4);w(p+0x26,a if case//96%2 else t,4);w(p+0x20,5 if case//192%2 else 4);w(p+0x60,0x49 if case//384%2 else 0x48)
  w(a+0x108,rng.choice([0,1,4,5,6,0xff04]));w(0x1c007c,rng.choice([0,1,0x8000]));w(0x1c0161,case//768,1)
  w(0x1c1697,rng.getrandbits(8),1);w(0x1c15d2,rng.getrandbits(16))
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
  status=native.wf_pin_oracle(nb,nr)
  assert status==0,('native escape blocked',case,hex(-status))
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mm=m.cpu,m.mem;mm.w_block(0,bytes(mem))
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  cpu.w_reg(8,a);cpu.w_sp(0x1d0000);mm.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for _ in range(10000):
   if cpu.r_pc()==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,('ROM runaway',case,hex(cpu.r_pc()))
  want=bytes(mm.r_block(0x1c0000,0x2000));got=nb.raw[0x1c0000:0x1c2000]
  assert got==want,('RAM',case,[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  assert list(nr)==[cpu.r_reg(i) for i in range(8)],('D0-D7',case,list(map(hex,nr)),[hex(cpu.r_reg(i)) for i in range(8)])
  m.cleanup()
 print('PASS 1536 raw $4B ROM escape chains: all work RAM/actors, TAG/royal cleanup and D0-D7')
