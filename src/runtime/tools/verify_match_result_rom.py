#!/usr/bin/env python3
"""Compare terminal TAG bonus and silent result sound selector with original ROM."""
import ctypes,hashlib,random,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x90d6)
with tempfile.TemporaryDirectory() as tmp:
 lib=Path(tmp)/'result.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O1','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_match_result.c','src/wf_r17_m68k_shadow.c','tests/r18/match_result_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_match_result_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
 for case in range(128):
  actors=bytearray(rng.randbytes(9*0x10c));regs=[rng.getrandbits(32) for _ in range(8)]
  config=(case%4)<<13;stage=case%12;result=case//64
  nr=(ctypes.c_uint32*8)(*regs);na=ctypes.create_string_buffer(bytes(actors))
  native.wf_match_result_oracle(na,nr,config,stage,result)
  machine=mus.Machine(mus.CPUType.M68000,2048);cpu=machine.cpu;mem=machine.mem;mem.w_block(0,rom);mem.w_block(0x1c05b0,bytes(actors));mem.w16(0x1c0066,config);mem.w16(0x1c0162,stage);mem.w16(0x1c007c,1)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x20156 if result else 0x90d6)
  for _ in range(10000):
   if cpu.r_pc()==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,('runaway',case)
  assert na.raw[:len(actors)]==bytes(mem.r_block(0x1c05b0,len(actors))),('actors',case)
  assert list(nr)==[cpu.r_reg(i) for i in range(8)],('registers',case)
  machine.cleanup()
 print('PASS 128 ROM TAG result cases: all actors, D0-D7, bonus overflow and sound selection')

 native.wf_match_endpoint_oracle.argtypes=[ctypes.c_uint]*7
 cases=0
 for a0 in [0,1,0x3fff,0x4000,0x8000,0xc000,0xffff]:
  for a2 in [0,1,0x3fff,0x4000,0x8000,0xc000,0xffff]:
   machine=mus.Machine(mus.CPUType.M68000,2048);cpu=machine.cpu;mem=machine.mem;mem.w_block(0,rom)
   mem.w16(0x1c0650,a0);mem.w16(0x1c0868,a2);cpu.w_pc(0x1178)
   for _ in range(40):
    if cpu.r_pc() in [0x11b6,0xf9a]:break
    cpu.execute(1)
   assert cpu.r_pc() in [0x11b6,0xf9a],hex(cpu.r_pc())
   ended=int(cpu.r_pc()==0x11b6)
   assert native.wf_match_endpoint_oracle(a0,a2,2,1,0,1,1)==ended,(a0,a2)
   for stage,credited,mode,ready,ack in [(0,1,0,1,1),(1,1,0,1,1),(2,0,0,1,1),(2,1,1,1,1),(2,1,0,0,1),(2,1,0,1,0)]:
    assert native.wf_match_endpoint_oracle(a0,a2,stage,credited,mode,ready,ack)==0
   cases+=1;machine.cleanup()
 print('PASS',cases,'ROM scheduler endpoints plus 294 invalid-boundary checks')
