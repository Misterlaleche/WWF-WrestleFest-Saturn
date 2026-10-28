#!/usr/bin/env python3
"""Complete world setup/queue/transfer owners compared with intact ROM."""
import argparse,ctypes,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x26e66);count=0;counts={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def invoke(cpu,ram,owner):
 cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
 for _ in range(160000):
  if cpu.r_pc()==0x70000:return
  cpu.execute(1)
 raise AssertionError(('ROM runaway',hex(owner),hex(cpu.r_pc())))
with tempfile.TemporaryDirectory(prefix='wf-map-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'map.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_frontend_map.c','src/wf_r17_m68k_shadow.c','tests/r18/frontend_map_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.frontend_map_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
 for mode in (range(1)if a.smoke else range(7)):
  for case in range(1 if a.smoke else 12):
   mem=bytearray(0x200000);mem[:len(rom)]=rom
   for at,n in ((0x1c0000,0x4000),(0x80000,0x4000),(0x100000,8),(0x140010,2)):mem[at:at+n]=rng.randbytes(n)
   put(mem,0x1c007e,mode);put(mem,0x1c16d2,0);put(mem,0x1c1804,0x200+(case%4)*32);put(mem,0x1c1806,0x200+(case//4)*32);mem[0x1c006f]=0x10 if case&1 else 0
   d=[rng.getrandbits(32)for _ in range(8)];ar=[rng.getrandbits(32)for _ in range(7)]
   ram.w_block(0,bytes(mem))
   for i,v in enumerate(d+ar):cpu.w_reg(i,v)
   nb=ctypes.create_string_buffer(bytes(mem));nd=(ctypes.c_uint32*8)(*d);na=(ctypes.c_uint32*7)(*ar)
   sequence=[(0x26e66,None)]
   if not a.smoke:
    sequence += [(0x26e1a,None),(0x26a42,None),(0x26dac,None)]
    for which in range(3):
     for dx,dy in ((32,0),(0,32),(-32,0),(0,-32)):
      sequence += [(0x26a42,(which,dx,dy)),(0x26e1a,None),(0x26dac,None)]
   for owner,drift in sequence:
    if drift:
     which,dx,dy=drift
     for side in ((0,)if which==0 else (1,)if which==1 else (0,1)):
      for off,delta in ((0x17e6+side*4,dx),(0x17ee+side*4,dy)):
       val=(ram.r16(0x1c0000+off)+delta)&65535;ram.w16(0x1c0000+off,val)
       ctypes.memmove(ctypes.addressof(nb)+0x1c0000+off,val.to_bytes(2,'big'),2)
    invoke(cpu,ram,owner)
    assert native.frontend_map_oracle(nb,nd,na,owner)==0,('native blocked',hex(owner),mode,case)
    for at,n in ((0x1c0000,0x4000),(0x80000,0x4000),(0x100000,8),(0x140010,2)):
     got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,('RAM/board',hex(owner),mode,case,[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:8])
    assert list(nd)==[cpu.r_reg(i)for i in range(8)],('D0-D7',hex(owner),mode,case,list(nd),[cpu.r_reg(i)for i in range(8)])
    assert list(na)==[cpu.r_reg(i)for i in range(8,15)],('A0-A6',hex(owner),mode,case,list(na),[cpu.r_reg(i)for i in range(8,15)])
    count+=1;counts[hex(owner)]=counts.get(hex(owner),0)+1
 m.cleanup();result={'status':'PASS','calls':count,'owners':counts,'compared':['RAM16KB','BG16KB','scroll/priority board words','D0-D7/A0-A6'],'intercepts':[],'limitations':'Original stores; Saturn character pack/upload and VBlank separate.'}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
