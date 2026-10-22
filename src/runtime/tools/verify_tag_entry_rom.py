#!/usr/bin/env python3
"""Raw08/24/4E and main11/12 linked holds against unmodified ROM."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();rng=random.Random(0x132f4)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'tag_entry.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/tag_entry_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_tag_entry_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(3840):
  j=case//5;selector=[8,0x4e,0x10b,0x10c,0x24][case%5]
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  w(a,0x8000);w(t,0x8000);w(a+2,(j//3)%12);w(a+0x20,5);w(a+0x1c,0x8005 if j%3 else 5);w(a+0x60,selector|([0,0x4000,0x8000,0xc000][j%4]))
  w(a+0x26,t,4);w(t+0x26,a if j%5 else 0,4);w(a+0x86,t,4);w(t+0x86,a,4);w(a+0x7a,t,4)
  w(a+6,[399,450,500,600,750,800,1023][j%7]);w(a+10,[255,280,320,344,400,500][j%6]);w(a+14,[0,256,320,384][j%4]);w(t+6,600);w(t+10,344);w(t+14,320)
  w(t+0x20,4 if j%3 else 0);w(t+0x64,8 if j%4 else 9);w(a+0x24,[0,1,2,0xfe][(j//3)%4]);w(a+0x22,[0,1,2,0xff00][(j//12)%4]);w(a+0x2e,0x8000 if (j//3)%3 else 0);w(t+0x2e,0x8000 if j%5 else 0)
  w(a+0x32,rng.choice([0,1,4,0x20]),1);w(a+0x33,rng.choice([0,0x80,0xa0,0x21,0xc3,0xc7]),1);w(a+0x34,0xff,1);w(t+0x33,rng.choice([0,0x20]),1);w(t+0x32,rng.choice([0,1,4]),1)
  w(a+0x56,0x8000 if (j//3)%3 else 0);w(t+0x56,0x8000 if j%5 else 0);w(a+0x36,rng.choice([0,0x501,0x502]));w(t+0x36,0x510)
  w(a+0xc6,0x40,1);w(a+0x66,[0,23,24,25,98,99,100,101,150,65535][j%10]);w(a+0x72,[0,1,2,3,150,65535][(j//3)%6]);w(a+0x70,0x5555);w(a+0xae,4)
  if selector==0x24:w(a+0x24,[0,1,2,3,4,0xfe][(j//3)%6]);w(t+0xc7,255 if (j//3)%2 else 0,1)
  if selector>=0x100:w(a+0x20,selector&255);w(a+0x1c,selector&255);w(a+0x44,0);w(a+0x46,4);w(a+0x36,[0x1234,0x501,0x502,0x510][j%4])
  w(0x1c007e,(j//48)%7)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_tag_entry_oracle(nb,nr)
  assert status==0,('native tag entry blocked',case,hex(-status))
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
 print('PASS 3840 tag-entry/hold ROM fixtures: all actors, globals, RNG and D0-D7')
