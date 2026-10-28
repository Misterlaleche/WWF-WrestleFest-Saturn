#!/usr/bin/env python3
"""Selected raw frontend state survives literal F8C into the combat view."""
import argparse,ctypes,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0xf8c);count=0
def get(m,off,n=2):return int.from_bytes(m[off:off+n],'big')
with tempfile.TemporaryDirectory(prefix='wf-handoff-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'handoff.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_runtime_handoff.c','src/wf_r17_bootstrap.c','src/wf_r17_m68k_shadow.c','tests/r18/runtime_handoff_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.runtime_handoff_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem;ram.w_block(0,rom)
 for case in range(1 if a.smoke else 512):
  raw=bytearray(rng.randbytes(0x4000));raw[0x160]=case&255;raw[0x161]=(case//256)&1;raw[0x4f]=case%100
  raw[0x5c:0x60]=(0 if case==0 else rng.getrandbits(32)).to_bytes(4,'big')
  regs=[rng.getrandbits(32)for _ in range(8)];ram.w_block(0x1c0000,bytes(raw))
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  cpu.w_sp(0x1d0000);cpu.w_pc(0xf8c)
  for _ in range(10000):
   if cpu.r_pc()==0xfa4:break
   cpu.execute(1)
  assert cpu.r_pc()==0xfa4
  nb=ctypes.create_string_buffer(bytes(raw));nd=(ctypes.c_uint32*8)(*regs);out=(ctypes.c_uint32*44)()
  assert native.runtime_handoff_oracle(nb,nd,out)==0,('native blocked',case)
  want=bytes(ram.r_block(0x1c0000,0x4000));assert nb.raw[:0x4000]==want,('RAM',case)
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],('D',case)
  expected=[1,want[0x160],want[0x161],get(want,0x162),get(want,0x66),get(want,0x7c),get(want,0x5c,4),get(want,0x80,4),get(want,0x82),3,get(want,0x1804),get(want,0x1806),get(want,0x16a0),get(want,0x16a2),get(want,0x169e),want[0x4f],want[0x4e]]
  for i in range(9):expected += [get(want,0x5b0+i*0x10c+0x56),get(want,0x5b0+i*0x10c+0x66),get(want,0x5b0+i*0x10c+0x8a,4)]
  assert list(out)==expected,('live selected-state import',case,list(out),expected);count+=1
 m.cleanup();result={'status':'PASS','calls':count,'compared':['RAM16KB','D0-D7','selected identities/health/ports','mode/progression/credits/RNG/clock','bindings retained','FA4 handoff stage'],'intercepts':[]}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
