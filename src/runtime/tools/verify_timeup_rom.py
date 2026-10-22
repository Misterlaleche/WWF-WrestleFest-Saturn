#!/usr/bin/env python3
"""Execute canonical $262D2/$2503C/$21E6 with Vblank IO edges, no ROM patches."""
import ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split();rng=random.Random(0x263d8)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'timeup.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_video.c','tests/r18/timeup_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_timeup_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint),ctypes.c_uint]
 for case in range(128):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
  w(0x1c007c,1);w(0x1c169e,0xc000);w(0x1c16a4,1);w(0x1c16c4,rng.randrange(65536))
  for k in range(9):
   p=0x1c05b0+k*0x10c;mem[p:p+0x10c]=bytes(rng.randrange(256) for _ in range(0x10c));w(p,(0x8000 if (case>> (k%7))&1 else 0)|rng.randrange(0x8000))
  mem[0xc0000:0xc4000]=bytes(rng.randrange(256) for _ in range(0x4000))
  regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));ticks=ctypes.c_uint()
  status=native.wf_timeup_oracle(nb,nr,ctypes.byref(ticks),3);assert status==0,('native timeup blocked',case,hex(-status));assert ticks.value==65
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem;memory.w_block(0,bytes(mem))
  for i in range(8,15):u.w_reg(i,0)
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);u.w_pc(0x262d2);rises=falls=0
  for steps in range(100000):
   pc=u.r_pc()
   if pc==0x70000:break
   if pc==0x21ea:memory.w8(0x140026,4);rises+=1
   if pc==0x21f4:memory.w8(0x140026,0);falls+=1
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  assert rises==falls==65,(case,rises,falls)
  for at,n in [(0x1c05b0,9*0x10c),(0x1c169e,8),(0x1c16c4,2),(0xc0000,0x4000)]:
   want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n];assert got==want,('memory',case,hex(at),[(hex(i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:16])
  assert list(nr)==[u.r_reg(i)for i in range(8)],('regs',case,list(nr),[u.r_reg(i)for i in range(8)])
  machine.cleanup()
  for hooks,block in [(0,0x2503c),(1,0x21e6),(2,0x2503c)]:
   nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));ticks=ctypes.c_uint();status=native.wf_timeup_oracle(nb,nr,ctypes.byref(ticks),hooks)
   assert status==-block,(case,hooks,hex(-status));assert nb.raw[:len(mem)]==mem;assert list(nr)==regs;assert ticks.value==0
 print('PASS 128 complete time-up ROM fixtures, 384 missing-hook rollbacks; exact 65 Vblank edge pairs, nine actors, FG0 and D0-D7')
