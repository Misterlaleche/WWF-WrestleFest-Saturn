#!/usr/bin/env python3
"""Composed native frontend clear/palette/text helpers against intact ROM."""
import argparse,ctypes,hashlib,json,os,random,re,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x1f6c);count=0;owners={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def fixtures():
 for owner in (0x1f38,0x1f52,0x1f6c,0x1f9e,0x1fc0,0x1fde,0x2aea,0x2b58,0x51d2,0x527e,0x9a42,0x2503c,0x9aca,0x1e92):
  for case in range(1 if a.smoke else 16):
   mem=bytearray(0x200000);mem[:len(rom)]=rom
   for at,n in ((0x1c0000,0x4000),(0xc0000,0x4000),(0x80000,0x4000),(0x180000,0x9000)):mem[at:at+n]=rng.randbytes(n)
   d=[rng.getrandbits(32)for _ in range(8)];ar=[rng.getrandbits(32)for _ in range(7)]
   if owner in (0x2aea,0x2b58):
    d[0]=case*3;mem[0x1c1600:0x1c1710]=bytes(0x110)
    if case&1:mem[0x1c1600:0x1c1600+(case&15)]=bytes([255])*(case&15)
    if case&2:mem[0x1c1610+d[0]]=0x80|(case&15)
   if owner in (0x51d2,0x527e):d[0]=case%3
   if owner==0x2503c:d[0]=(0,1,2,3,4,0x53,0x54,0x55)[case%8]|(0x8000 if case&8 else 0)
   if owner==0x9aca:d[2]=(0xd,0xe,0xf,0x27)[case%4];d[7]=(0x10,0x60,0x70,0x20)[case%4];ar[2]=0xc0344
   if owner==0x1e92:
    put(mem,0x1c0072,0xffff if case&1 else 0);put(mem,0x1c004a,1+(case%3));mem[0x1c004e]=case%3;mem[0x1c004f]=case*11
   yield owner,case,mem,d,ar
make=(ROOT/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
for f in ('src/wf_r18_frontend_io.c','src/wf_r18_frontend_boot.c'):
 if f not in sources:sources.append(f)
with tempfile.TemporaryDirectory(prefix='wf-frontend-io-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'io.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/frontend_io_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.frontend_io_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
 for owner,case,mem,d,ar in fixtures():
  ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
  for i,v in enumerate(d+ar):cpu.w_reg(i,v)
  for _ in range(150000):
   pc=cpu.r_pc()
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,('ROM runaway',hex(owner),hex(pc))
  nb=ctypes.create_string_buffer(bytes(mem));nd=(ctypes.c_uint32*8)(*d);na=(ctypes.c_uint32*7)(*ar)
  assert native.frontend_io_oracle(nb,nd,na,owner)==0,('native blocked',hex(owner),case)
  for at,n in ((0x1c0000,0x4000),(0xc0000,0x4000),(0x80000,0x4000),(0x180000,0x9000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,('RAM/video',hex(owner),case,[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:8])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],('D0-D7',hex(owner),case,list(nd),[cpu.r_reg(i)for i in range(8)])
  assert list(na)==[cpu.r_reg(i)for i in range(8,15)],('A0-A6',hex(owner),case,list(na),[cpu.r_reg(i)for i in range(8,15)])
  count+=1;owners[hex(owner)]=owners.get(hex(owner),0)+1
 m.cleanup();result={'status':'PASS','calls':count,'owners':owners,'compared':['RAM16KB','FG16KB','BG16KB','paletteRAM/padding','D0-D7/A0-A6'],'intercepts':[],'limitations':'Payload helpers; MMIO ACK, display, timing and scene orchestration separate.'}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
