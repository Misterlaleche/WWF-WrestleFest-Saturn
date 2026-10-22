#!/usr/bin/env python3
"""ROM13D46 four-phase running splash, through full dispatch."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
rng=random.Random(0x13d46)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'raw0e.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/raw0e_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_raw0e_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32) ]
 for case in range(3072):
  j=case//4;phase=case%4;sel=0x0e
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  actors=[0x1c05b0+i*0x10c for i in range(9)];a,t,p,q=actors[:4]
  for i,addr in enumerate(actors):
   w(addr,0x8000);w(addr+2,i);w(addr+0x20,1);w(addr+6,700+i*3);w(addr+10,304+i*3);w(addr+14,304+i*3)
   w(addr+0x26,actors[i^1] if i<8 else a,4);w(addr+0x86,actors[(i+2)%4],4);w(addr+0x7a,t,4)
   w(addr+0x33,[0,1,2,3,0x40,0x81][j%6],1);w(addr+0x34,0,1);w(addr+0x56,0x8000|i);w(addr+0x66,100);w(addr+0x72,145);w(addr+0x70,j%3)
  w(a+0x20,5);w(a+0x1c,5 if j%7==0 else 0x8005);w(a+2,(j//8)%12)
  w(a+0x60,sel|[0,0x4000,0x8000,0xc000][(j//4)%4]);w(a+0x24,[0,2,3,0xfe][j%4]);w(a+0x22,[0,1,0xff00][(j//4)%3]);w(a+0x44,phase)
  w(t+0x20,[4,4,5,0][(j//4)%4]);w(t+0x64,8 if j%2 else 9);w(t+0x60,0x54);w(t+0x66,[0,1,10,22,30,54,100][(j//6)%7]);w(t+0xdc,(j//7)%8);w(a+0xbe,[500,700,800][j%3]);w(a+0x32,[0,1][j%2],1);w(p+0x32,[0,1][(j//2)%2],1);w(a+0xf6,j%24);w(a+0xc7,j%4,1);w(a+0x2e,0x8000 if j%2 else 0);w(t+0x2e,0x8000 if j%3 else 0)
  w(a+6,[703,440,450,829,840,948][(j//12)%6]);w(t+6,700);w(t+10,304);w(a+10,[304,272,416][(j//72)%3]);w(a+14,[304,208,416][(j//216)%3]);w(a+0x36,[0,0x310,0x101,0x502,0x410][(j//3)%5]);w(a+0x3e,0);w(a+0x40,0)
  w(p+0x20,1 if j%2 else 5);w(p+0x33,[0,1,2,3][(j//3)%4],1);w(p+0xae,1);w(t+0x86,p,4)
  w(0x1c1150,j%3);w(0x1c11b0,j%3);w(0x1c007c,0 if j%7==0 else 1);w(0x1c0161,[0,1,2][(j//24)%3],1);w(0x1c0162,j%10)
  w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,2852+j,4)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_raw0e_oracle(nb,nr)
  assert status==0,('native raw0e blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i in range(8,15):u.w_reg(i,0)
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(0x1c03e)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   if u.r_pc()==0x2052:u.w_pc(memory.r32(u.r_sp()));u.w_sp(u.r_sp()+4);continue
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  for at,n in [(a,9*0x10c),(0x1c005c,4),(0x1c167a,1),(0x1c15d2,2),(0x1c1697,1),(0x1c1800,2),(0x1c1682,2),(0x1c168c,4),(0x1c1692,2),(0x1c1150,2),(0x1c1180,2),(0x1c11b0,2),(0x1c11e0,2)]:
   want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,('memory',case,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  assert list(nr)[:8]==[u.r_reg(i) for i in range(8)],('D0-D7',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS3072 raw0E ROM cases: four phases, TAG/other modes, hit/miss, actors/globals and D0-D7')
