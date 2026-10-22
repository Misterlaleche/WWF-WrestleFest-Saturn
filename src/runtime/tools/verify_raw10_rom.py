#!/usr/bin/env python3
"""Raw $10 jumping interrupt, independently seeded against unmodified ROM."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();rng=random.Random(0x132f4)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'pin.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/raw10_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_raw10_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(1920):
  j=case
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  actors=[0x1c05b0+0x10c*k for k in range(9)];a,t,p,q=actors[:4];sel=0x10
  for i,addr in enumerate(actors):
   w(addr,0x8000);w(addr+2,i);w(addr+6,600+i*15);w(addr+10,340+i*5);w(addr+0x26,actors[i^1] if i<8 else a,4);w(addr+0x86,actors[(i+2)%4],4);w(addr+0x7a,actors[i^1] if i<8 else a,4)
   w(addr+0x33,rng.choice([0,1,2,3]),1);w(addr+0x32,rng.choice([0,1]),1);w(addr+0x56,rng.choice([0,0x8000,0x2000,0xa000]));w(addr+0x34,rng.choice([0,0xc0,2]),1);w(addr+0xb5,rng.choice([0,8,0x80]),1)
  w(a+0x20,5);w(a+0x1c,0x8005 if j%7 else 5);w(a+0x60,sel|(0x8000 if j%5==0 else 0));w(a+0x24,rng.choice([0,1,3,4,5,0xfe]));w(a+0x22,rng.choice([0,1,2,0xff00]));w(a+0x36,0x510 if j%3 else 0x500);w(a+0x58,0x80 if j%7 else 0,1);w(a+0x2e,0x8000 if j%3 else 0)
  w(t+0x20,5 if j%11 else 0);w(t+0x60,(0x48 if sel==0x4a else 0x4a)|(0x8000 if j%3 else 0));w(a+0x64,0x8008 if j%3 else 0x8009);w(a+0x66,rng.choice([0,1,41,42,43,100]));w(a+0xaa,rng.choice([0,1,2,0x4000]));w(a+0xa3,j%3,1);w(a+0xfe,0x8000 if j%7==0 else 0);w(a+0xff,rng.choice([0,3,5]),1)
  w(a+0x9c,p,4);w(p+0x26,a,4);w(p+0x20,5);w(p+0x60,0x49);w(a+0x32,0x10 if j%13==0 else 0,1)
  w(0x1c007c,[0,1,0x80,0x8000][(j//2)%4]);w(0x1c0161,1 if j%3==0 else 0,1);w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,case+0x123,4)
  w(a+0x44,[0,1,2,80,65535][j%5]);w(a+0xf8,j%24);w(t+0x86,p,4);w(p+0x20,1);w(p+0xae,1);w(0x1c0162,j%10);w(a+2,(j//7)%12);w(a+0x60,0x10|[0,0x4000,0x8000,0xc000][(j//7)%4]);w(a+0x24,[0,1,0xfe,0x1fe][(j//7)%4]);w(a+0x22,[0,1,0xff00][(j//28)%3]);w(a+0x36,0x510 if j%3 else 0x500);w(t+0x20,[4,4,5,0][(j//84)%4]);w(t+0x64,8 if j%2 else 9);w(t+0x60,0x8061);w(t+0xc7,255 if j%2 else 0,1);w(t+0xd2,65535 if j%3 else 0)
  w(t+6,640);w(t+10,320);w(t+14,320);w(a+6,[535,536,537,631,632,633,647,648,649,743,744,745][j%12]);w(a+10,[247,248,249,343,344,345][(j//12)%6]);w(a+14,320);w(t+0x2e,0x8000 if j%2 else 0)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_raw10_oracle(nb,nr)
  assert status==0,('native throw blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
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
  assert list(nr)==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS 1920 raw10 jumping interrupt ROM fixtures: all actors, globals, RNG and D0-D7')
