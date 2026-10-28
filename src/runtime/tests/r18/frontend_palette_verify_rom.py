#!/usr/bin/env python3
"""$2A06 payload vs intact ROM, including all palette-bank padding and D0-D7.

The only external events are VBlank input bit2 high then low at its original
polls. No ROM patches or intercepted helpers. All four palette domains are
compared, including the sprite domain this owner must leave unchanged.
"""
import argparse,ctypes,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x2a06);count=0
with tempfile.TemporaryDirectory(prefix='wf-frontend-pal-',ignore_cleanup_errors=os.name=='nt') as t:
 lib=Path(t)/'palette.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_frontend_palette.c','src/wf_r17_m68k_shadow.c','tests/r18/frontend_palette_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.frontend_palette_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint]
 for world in range(7):
  for text in range(4):
   mem=bytearray(0x200000);mem[:len(rom)]=rom
   mem[0x180000:0x189000]=rng.randbytes(0x9000)
   mem[0x1c15f4:0x1c15f6]=world.to_bytes(2,'big');mem[0x1c15fc:0x1c15fe]=text.to_bytes(2,'big')
   regs=[rng.getrandbits(32)for _ in range(8)]
   m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem;ram.w_block(0,bytes(mem));ram.w8(0x140026,4)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x2a06)
   for _ in range(20000):
    pc=cpu.r_pc()
    if pc==0x70000:break
    if pc==0x2a88:ram.w8(0x140026,0)
    cpu.execute(1)
   assert cpu.r_pc()==0x70000,('ROM runaway',hex(cpu.r_pc()))
   nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
   assert native.frontend_palette_oracle(nb,nr,world,text)==0,('native blocked',world,text)
   want=bytes(ram.r_block(0x180000,0x9000));got=nb.raw[0x180000:0x189000]
   assert got==want,('palette mismatch',world,text,[(hex(0x180000+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:8])
   assert list(nr)==[cpu.r_reg(i)for i in range(8)],('D0-D7',world,text,list(nr),[cpu.r_reg(i)for i in range(8)])
   count+=1;m.cleanup()
 result={'status':'PASS','configurations':count,'compared':['all palette RAM180000..188FFF','padding','D0-D7'],'intercepts':[],'external_events':['VBlank input bit2 high at entry, low at trailing poll'],'limitations':'Payload only. Platform synchronization/CRAM transport and composed scenes are separate.'}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
