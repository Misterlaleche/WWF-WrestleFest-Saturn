#!/usr/bin/env python3
"""Exact projection, six raw queues and compositor with intact D1FC callback."""
import argparse,ctypes,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');p.add_argument('--native-leaf',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Emit=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.POINTER(Context))
Clear=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32)
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.POINTER(ctypes.c_uint32)),('emit_d1fc',Emit),('clear_attribute',Clear),('user',ctypes.c_void_p),('blocked_dependency',ctypes.c_uint32)]
rng=random.Random(0x2836);count=0;owners={};errors=[]
m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def invoke(mem,d,ar,owner):
 ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
 for i,v in enumerate(list(d)+list(ar)):cpu.w_reg(i,v)
 for _ in range(300000):
  if cpu.r_pc()==0x70000:break
  cpu.execute(1)
 assert cpu.r_pc()==0x70000,('runaway',hex(owner),hex(cpu.r_pc()))
 for at,n in ((0x1c0000,0x4000),(0xc0000,0x4000)):mem[at:at+n]=ram.r_block(at,n)
 return[cpu.r_reg(i)for i in range(8)],[cpu.r_reg(i)for i in range(8,15)]
current={}
@Emit
def emit(user,cp):
 try:
  c=cp.contents;mem=current['mem'];mem[0x1c0000:0x1c4000]=ctypes.string_at(c.ram,0x4000)
  d,ar=invoke(mem,c.regs.contents.d,[c.a[i]for i in range(7)],0xd1fc)
  for i,v in enumerate(d):c.regs.contents.d[i]=v
  for i,v in enumerate(ar):c.a[i]=v
  ctypes.memmove(c.ram,bytes(mem[0x1c0000:0x1c4000]),0x4000)
  return 0
 except BaseException as e:errors.append(str(e));return 1
@Clear
def clear(user,at):current['mem'][at]=0;return 0
with tempfile.TemporaryDirectory(prefix='wf-render-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'render.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_frontend_render.c','src/wf_r18_frontend_leaf.c','src/wf_r18_metasprite.c','tests/r18/frontend_leaf_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_frontend_render_call.argtypes=[ctypes.POINTER(Context),ctypes.c_uint32]
 native.frontend_leaf_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for case in range(1 if a.smoke else 96):
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c4000]=rng.randbytes(0x4000);mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
  mem[0x1c19b8:0x1c1cd2]=bytes(0x31a);put(mem,0x1c19b4,case%64);put(mem,0x1c17e6,0x200);put(mem,0x1c17ee,0x200)
  d=[rng.getrandbits(32)for _ in range(8)];ar=[rng.getrandbits(32)for _ in range(7)]
  addrs=[0x1c1258+i*0x2a for i in range(12)]if case&1 else[0x1c05b0+i*0x10c for i in range(9)]
  for i,at in enumerate(addrs):
   mem[at:at+0x2a]=bytes(0x2a);put(mem,at,0x8000 if i%4 or case%3 else 0);put(mem,at+2,0x20+i if case&1 else i);put(mem,at+4,(case//2)%5 if case&1 else(0,89,0x8000,0xffff)[i%4]);put(mem,at+6,0x200+rng.randrange(-64,384));put(mem,at+10,0x200+rng.randrange(-64,320));put(mem,at+14,0);put(mem,at+18,i%6);put(mem,at+0x18,case);put(mem,at+0x1a,-case);mem[at+0x21]=255 if case%7==0 else 0
  operations=[(owner,at)for at in addrs for owner in(0x247c,0x27b8)]+[(0x2836,None)]
  native_mem=bytearray(mem)
  for owner,at in operations:
   if at is not None:ar[0]=at|((0x80000000,0xff000000,0x12000000)[case%3]if case&2 else 0)
   want=bytearray(mem);wd,wa=invoke(want,d,ar,owner)
   raw=(ctypes.c_uint8*0x4000).from_buffer_copy(native_mem[0x1c0000:0x1c4000]);nd=Shadow((ctypes.c_uint32*8)(*d),1);na=(ctypes.c_uint32*7)(*ar)
   ctx=Context(raw,ctypes.pointer(nd),na,emit,clear,None,0);current['mem']=native_mem
   if a.native_leaf:
    nb=ctypes.create_string_buffer(bytes(native_mem));assert native.frontend_leaf_oracle(nb,nd.d,na,owner)==0,('native leaf blocked',hex(owner),case)
    native_mem=bytearray(nb.raw[:0x200000])
   else:
    assert native.wf_r18_frontend_render_call(ctypes.byref(ctx),owner)==0,('native blocked',hex(owner),case,hex(ctx.blocked_dependency),errors)
    native_mem[0x1c0000:0x1c4000]=bytes(raw)
   for base,n in ((0x1c0000,0x4000),(0xc0000,0x4000)):
    indices=range(n)if not(a.native_leaf and base==0xc0000)else[i for i in range(n)if i<0x2000 or i%16 in(1,3,5,7,9,11)]
    diff=[(hex(base+i),native_mem[base+i],want[base+i])for i in indices if native_mem[base+i]!=want[base+i]]
    assert not diff,('RAM/FG',hex(owner),case,diff[:8])
   assert list(nd.d)==wd,('D',hex(owner),case,list(nd.d),wd);assert list(na)==wa,('A',hex(owner),case,list(na),wa)
   mem=want;d,ar=wd,wa;count+=1;owners[hex(owner)]=owners.get(hex(owner),0)+1
 m.cleanup();result={'status':'PASS','boundaries':count,'owners':owners,'compared':['RAM16KB','FG16KB'if not a.native_leaf else'FG text and all 512 hardware-consumed sprite fields (reserved bytes normalized)','D0-D7/A0-A6'],'callbacks':'D1FC native parser composed'if a.native_leaf else'D1FC executes original ROM whole, unpatched; native leaf/transport separate','intercepts':[]}
 if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
