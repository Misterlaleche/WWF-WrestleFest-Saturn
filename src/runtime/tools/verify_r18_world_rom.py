#!/usr/bin/env python3
"""Untouched-ROM differential for 288F0 animations and 26B68 map transport."""
import ctypes,random,subprocess,tempfile,argparse
from pathlib import Path
from wf_r18_rom_graphics import Rom
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);a=p.parse_args();rom=Rom(a.rom);root=Path(__file__).resolve().parents[1];rng=random.Random(0x288f0);count=0
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'world.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'include'),str(root/'tests/r17/world_oracle_bridge.c'),str(root/'src/wf_r18_world.c'),str(root/'src/wf_r18_ring.c'),'-o',str(lib)],check=True);native=ctypes.CDLL(str(lib));native.wf_r18_world_oracle.argtypes=[ctypes.c_uint,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for op in (0,1):
  for case in range(200):
   mode=rng.choice([0,1,2,4,5,6]);obj=rng.randrange(47);mem=bytearray(0x200000);mem[:len(rom.p)]=rom.p
   def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
   w(0x1c007e,mode);w(0x1c17fe,(case%2)*0x8000);w(0x1c16d4,case%4);w(0x1c17e2,rng.randrange(10,20));w(0x1c17e4,rng.randrange(16,22))
   for i in range(48):
    b=0x1c188a+i*4;mem[b]=0x80|rng.randrange([32,32,44,0,12,36,44][mode]);mem[b+1]=rng.randrange(8);mem[b+2]=rng.choice([0,1,4,255]);mem[b+3]=rng.choice([0,1,2,129,130])
   mem[0x1c188a+obj*4]|=(case%2)*64
   regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_r18_world_oracle(op,nb,nr,obj);assert status==0,(op,case,status)
   machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
   for i in range(8,15):u.w_reg(i,0)
   for i,v in enumerate(regs):u.w_reg(i,v)
   u.w_reg(8,0x1c188a+obj*4);u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(0x288f0 if op==0 else 0x26b68)
   for steps in range(100000):
    if u.r_pc()==0x70000:break
    u.execute(1)
   assert u.r_pc()==0x70000,('ROM runaway',op,case,hex(u.r_pc()))
   want=[u.r_reg(i) for i in range(8)];assert list(nr)==want,(op,case,'regs',list(nr),want)
   lo,hi=(0x1c188a,0x1c19ae) if op==0 else (0x1c16d6,0x1c17d6)
   got=nb.raw[lo:hi];expected=bytes(m.r_block(lo,hi-lo));assert got==expected,(op,case,mode,'memory',[(hex(lo+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:10])
   machine.cleanup();count+=1
print('PASS',count,'direct-ROM world animation/map-transport cases; all8Dregisters,records,queues and16 source/destinationpairs; bothlayers,all4directions,6worldmodes')
