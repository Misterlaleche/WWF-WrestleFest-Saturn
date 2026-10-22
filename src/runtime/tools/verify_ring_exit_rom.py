#!/usr/bin/env python3
"""Raw68/69/6A/6B ring exit and return against unmodified ROM."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();rng=random.Random(0x132f4)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'ring_exit.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/ring_exit_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_ring_exit_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for case in range(9216):
  if case%1024==0:print(f"ring exit ROM case {case}/9216",file=sys.stderr,flush=True)
  selector=[0x68,0x69,0x6a,0x6b][case%4];phase=(case//4)%4;j=case//16
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  w(a,0x8000);w(a+2,rng.randrange(12));w(a+0x20,5);w(a+0x1c,0x8005 if j%2 else 5);w(a+0x60,selector|([0,0x4000,0x8000,0xc000][(j//2)%4]));w(a+0x44,phase|0x8000)
  w(a+0x24,[0,1,2,0xfe][(j//2)%4]);w(a+0x22,[0,1,0xff00][(j//8)%3]);w(a+0x36,0x510 if (j//24)%2 else 0x500)
  w(a+6,rng.choice([0,320,322,329,330,632,633,639,640,641,944,951,952,65535]));w(a+10,rng.choice([0,216,217,223,224,225,231,232,255,256,257,65535]));w(a+14,320);w(a+0xbe,633);w(a+0xc0,224)
  w(a+0x2e,0x8000 if (j//48)%2 else 0);w(a+0x32,rng.choice([0,8]),1);w(a+0x33,[0,1,0xa1][(j//96)%3],1);w(a+0x56,rng.choice([0,0x8000]));w(a+0x74,rng.choice([0,0x8000,0x8001]));w(a+0x1a,0x1234);w(a+0x58,0xffff);w(a+0x5a,0xffff)
  w(0x1c0161,(j//288)%2,1);w(0x1c1678,0x1234);w(0x1c15e4,0xaabbccdd,4);w(0x1c15e8,0x12345678,4)
  if selector!=0x68:w(0x1c0161,(j//144)%4,1)
  if selector==0x69:w(a+0x24,[0,1,3,4,5,6,0xfe][(j//2)%7])
  w(a+0x76,0x1c0f1c if j%2 else 0x1c1028,4);w(0x1c1828,j%16)
  for k in range(1,9):
   p=a+k*0x10c;w(p,0x8000 if (k<(2 if j%2 else 3) or (j%3==0 and k in (7,8))) else 0);w(p+0x33,rng.choice([0,1,5]),1);w(p+0x32,2 if k%2 else 0,1)
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_ring_exit_oracle(nb,nr)
  assert status==0,('native ring exit blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i in range(8,15):u.w_reg(i,0)
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(0x1c03e)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  for at,n in [(a,11*0x10c),(0x1c005c,4),(0x1c1682,10),(0x1c168c,4),(0x1c1692,2),(0x1c1678,3),(0x1c15d2,2),(0x1c15e4,8),(0x1c1808,16),(0x1c1828,2),(0x1c1697,1)]:
   want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,('memory',case,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  assert list(nr)==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  machine.cleanup()
 print('PASS 9216 ring-exit ROM fixtures: all actors, globals, RNG and D0-D7')
