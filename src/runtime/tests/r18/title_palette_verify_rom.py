#!/usr/bin/env python3
"""Original title palette producer/IRQ vs ROM, including sequential lifecycle."""
import argparse,ctypes,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0xb312);count=0;coverage=set()
with tempfile.TemporaryDirectory(prefix='wf-title-pal-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'palette.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_title_palette.c','src/wf_r17_m68k_shadow.c','tests/r18/title_palette_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.title_palette_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
 def call(mem,regs,owner):
  global count
  ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(40000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,('runaway',hex(pc))
  nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
  assert native.title_palette_oracle(nb,nr,owner)==0,('native blocked',hex(owner))
  for at,n in ((0x1c0000,0x2000),(0x180000,0x9000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,('RAM/palette',hex(owner),[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:10])
  assert list(nr)==[cpu.r_reg(i)for i in range(8)],('D0-D7',hex(owner),list(nr),[cpu.r_reg(i)for i in range(8)])
  count+=1;return bytearray(nb.raw[:0x200000]),list(nr)
 for flag in range(1 if a.smoke else 256):
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000);mem[0x180000:0x189000]=rng.randbytes(0x9000)
  mem[0x1c16c0]=flag;mem[0x1c16c2]=flag
  for owner in (0xb312,0xb4b8):call(mem,[rng.getrandbits(32)for _ in range(8)],owner)
 if not a.smoke:
  for phase in (0,1,2,15):
   mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x180000:0x189000]=rng.randbytes(0x9000);mem[0x1c16c1]=phase;mem[0x1c16c3]=phase;regs=[rng.getrandbits(32)for _ in range(8)]
   for tick in range(400):
    mem,regs=call(mem,regs,0xb312);mem,regs=call(mem,regs,0xb4b8)
 m.cleanup()
 result={'status':'PASS','calls':count,'owners':['B312/B362/B3C0/B476','B4B8 IRQ'],'compared':['8KB RAM including flags and palette staging','all palette RAM and padding','D0-D7'],'intercepts':[],'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_title_palette.c').read_bytes()).hexdigest(),'covered_rom_sites':[hex(x)for x in (0xb312,0xb352,0xb362,0xb396,0xb3b2,0xb3c0,0xb452,0xb458,0xb476,0xb4b8,0xb500,0xb528)if x in coverage],'limitations':'Original title palette owners; scene driver and physical transport separate.'}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
