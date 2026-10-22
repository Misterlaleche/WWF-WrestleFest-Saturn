#!/usr/bin/env python3
"""Main-state one movement callbacks against unmodified ROM."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();rng=random.Random(0x132f4)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'movement.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/movement_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_movement_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(1664):
  sub=case%13;j=case//13
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  w(a,0x8000);w(t,0x8000);w(a+2,j%12);w(a+0x20,1);w(a+0x1c,0x8001 if j%2 else 1);w(a+0xae,sub|(0x8000 if j%5==0 else 0))
  w(a+0x26,0x80000000 if j%7==0 else t,4);w(t+0x26,a if j%3 else 0,4);w(a+0x86,t,4);w(t+0x86,a,4);w(a+0x7a,t,4)
  w(a+6,[550,596,600,604,608,1000][j%6]);w(a+10,[287,288,303,304,343,344,345,386,402,403][j%10]);w(t+6,600);w(t+10,344)
  w(a+0xbe,600);w(a+0xc0,344);w(t+0x20,1);w(t+0xae,0x800c)
  w(a+0x24,j%4);w(a+0x22,[0,1,2,0xff00][j%4]);w(a+0x2e,0x8000 if j%3 else 0)
  w(a+0x32,rng.choice([0,8,0x10]),1);w(a+0x33,rng.choice([0,0x80,0xa0,0x21,0xc3]),1);w(a+0x34,rng.choice([0,1,2,0x20,0x40,0x80,0xc0,0xff]),1);w(t+0x34,0xc0,1)
  w(a+0x56,0x8000 if j%3 else 0);w(a+0x74,rng.choice([0,0x8000,0x8001]));w(a+0x36,rng.choice([0,0x501,0x502]));w(a+0xfe,rng.choice([0,0x8000,0x8001]))
  w(a+0xe6,rng.choice([0,1,2]));w(a+0xc6,0x40 if j%3 else 0,1);w(a+0xc8,0xffff if j%3 else 0);w(a+0x44,rng.choice([0,1,2,148]));w(a+0x46,rng.choice([0,1,2]));w(a+0x66,100);w(a+0x72,100 if j%3 else 150);w(a+0x2c,rng.choice([0,8,0x40,0x78,0x80,0xc0,0xf8]))
  w(0x1c007c,[1,0x81,0x8001][j%3]);w(0x1c0161,1 if j%3 else 0,1)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_movement_oracle(nb,nr)
  assert status==0,('native movement blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(0x1c03e)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  for at,n in [(a,9*0x10c),(0x1c005c,4),(0x1c1682,10),(0x1c168c,4),(0x1c1692,2),(0x1c167a,1),(0x1c15d2,2),(0x1c15e4,8),(0x1c1697,1)]:
   want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,('memory',case,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  assert list(nr)==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS 1664 main-one movement ROM fixtures: all actors, globals, RNG and D0-D7')
