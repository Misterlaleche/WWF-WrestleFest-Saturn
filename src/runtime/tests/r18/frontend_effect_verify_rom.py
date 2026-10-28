#!/usr/bin/env python3
"""Full palette effects against intact ROM: RAM, palettes, registers, sync order.

External VBlank status is driven to each original poll's requested level.
No helper, ROM instruction, register or gameplay-memory intercept is used.
"""
import argparse,ctypes,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x26772);count=0;edge_counts={}
with tempfile.TemporaryDirectory(prefix='wf-frontend-effect-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'effect.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_frontend_effect.c','src/wf_r18_frontend_fade.c','src/wf_r17_m68k_shadow.c','tests/r18/frontend_effect_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.frontend_effect_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint),ctypes.c_void_p]
 for owner in (0x26772,0x26844,0x264e2):
  for fixture in range(1 if a.smoke else 8):
   mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c4000]=rng.randbytes(0x4000);mem[0x180000:0x189000]=rng.randbytes(0x9000)
   regs=[rng.getrandbits(32)for _ in range(8)]
   m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem;ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   events=[];polls=[]
   for _ in range(2000000):
    pc=cpu.r_pc()
    if pc==0x70000:break
    if rom[pc:pc+8]==bytes.fromhex('0839000200140026'):
     edge=1 if rom[pc+8]==0x67 else 2
     assert rom[pc+8]in(0x66,0x67),hex(pc)
     ram.w8(0x140026,4 if edge==1 else 0);events.append(edge)
     polls.append(bytes(ram.r_block(0x1c0000,0x4000))+bytes(ram.r_block(0x180000,0x9000)))
    cpu.execute(1)
   assert cpu.r_pc()==0x70000,('ROM runaway',hex(pc))
   nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);ne=(ctypes.c_uint*128)()
   snapshots=ctypes.create_string_buffer(128*0xd000)
   n=native.frontend_effect_oracle(nb,nr,owner,ne,snapshots)
   assert n>=0,('native blocked',hex(owner),n)
   assert list(ne[:n])==events,('VBlank edge order',hex(owner),list(ne[:n]),events)
   for poll,want in enumerate(polls):
    got=snapshots.raw[poll*0xd000:(poll+1)*0xd000]
    assert got==want,('intermediate RAM/palettes',hex(owner),fixture,poll,[(hex(i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:8])
   for at,size in ((0x1c0000,0x4000),(0x180000,0x9000)):
    want=bytes(ram.r_block(at,size));got=nb.raw[at:at+size]
    assert got==want,('RAM/palette',hex(owner),[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:12])
   assert list(nr)==[cpu.r_reg(i)for i in range(8)],('D0-D7',hex(owner),list(nr),[cpu.r_reg(i)for i in range(8)])
   edge_counts[hex(owner)]=n;count+=1;m.cleanup()
 result={'status':'PASS','calls':count,'owners':[hex(x)for x in (0x26772,0x26844,0x264e2)],'edge_counts':edge_counts,'compared':['16KB RAM including scratch/flags at every poll','all palette domains and padding at every poll','final D0-D7','ordered VBlank-high/low polls'],'controls':['active reentry rejected','repeated opposite level preserves all state','resume after DONE rejected'],'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_effect.c').read_bytes()).hexdigest(),'intercepts':[],'hardware_test':False,'limitations':'Original palette owner only; platform CRAM transport and scene composition separate.'}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
