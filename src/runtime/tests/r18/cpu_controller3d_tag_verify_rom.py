#!/usr/bin/env python3
"""Natural1928 controller3D TAG request vs intact ROM, no intercepted owner."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'cpu_controller3d_tag_manifest.json').read_text());capture=(HERE/'cpu_controller3d_tag_snapshot_A600.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['sha256']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x1d5a2);coverage=set();count=0
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def fixtures():
 mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=capture[56:8248]
 yield 'natural-A600-frame1928',mem,4,struct.unpack_from('=8I',capture,24),0
 if a.natural_only:return
 for slot,selector,f33,f34,mode in itertools.product((0,4,8),(0x3d,0x403d,0x803d,0xff3d),(0,0x80,0x40,0xfe,1,0x81),(2,0x82,0x42,0xfe),range(4)):
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=capture[56:8248]
  own=actors[slot];target=actors[(slot+1)%9];partner=actors[(slot+2)%9]
  mem[own:own+0x10c]=capture[56+0x9e0:56+0xaec]
  put(mem,own+0x20,0x8005);put(mem,own+0x60,selector);put(mem,own+0x33,f33,1);put(mem,own+0x34,f34,1);put(mem,own+0x7a,target,4);put(mem,own+0x86,partner,4)
  # Isolate the original1EC24 TAG entry in every mode; ringside's
  # separate1ED80 and1EBFE producers are outside this admission change.
  put(mem,own+0x74,0,1);put(mem,own+0x56,mem[own+0x56]|0x20,1)
  put(mem,own+0xb5,0,1);put(mem,own+0xb7,0x40,1)
  put(mem,target+0x32,0,1);put(mem,target+0x33,4,1);put(mem,0x1c0161,mode,1)
  yield f'controller3d-{slot}-{selector:x}-{f33:x}-{f34:x}-{mode}',mem,slot,[rng.getrandbits(32)for _ in range(8)],0
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-pursuit-tag-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'tag.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_controller3d_tag_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_controller3d_tag_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
 for name,mem,slot,regs,direct in fixtures():
  mem[0xc0000:0xc4000]=rng.randbytes(0x4000);ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x1d5a2 if direct else 0x1dfe2);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(10000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(pc))
  nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_cpu_controller3d_tag_oracle(nb,nr,slot)
  assert status==0,(name,'native blocked',hex(-status))
  for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:8])
  assert list(nr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(nr),[cpu.r_reg(i)for i in range(8)])
  count+=1
 m.cleanup()
 result={'status':'PASS','calls':count,'snapshot_sha256':meta['sha256'],'source_sha256':hashlib.sha256((ROOT/'src/wf_cpu_runtime.c').read_bytes()).hexdigest(),'state5_source_sha256':hashlib.sha256((ROOT/'src/wf_state5_exec.c').read_bytes()).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG','partner/global TAG pointers'],'intercepts':[],'covered_rom_sites':[hex(x)for x in (0x1ec24,0x1ec3a,0x1c5fa,0x1c602,0x1c61a,0x1c622,0x1c62e)if x in coverage],'limitations':'NaturalCPU call plus isolated controller3D TAG branches. Ringside1ED80 and1EBFE are separate; free combat not qualified.'}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
