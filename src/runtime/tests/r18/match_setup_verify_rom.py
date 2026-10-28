#!/usr/bin/env python3
"""Raw TAG/Royal $1086C vs intact ROM, RAM/palette padding and live D/A/RNG.

Only native external $2AEA is delegated to its intact ROM owner. No ROM
instruction, register, random result or memory operation is intercepted.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser();ap.add_argument('--report',type=Path);ap.add_argument('--pending',action='store_true');ap.add_argument('--smoke',action='store_true');ap.add_argument('--controls-only',action='store_true');args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
REGIONS=((0x1c0000,0x4000),(0xc0000,0x4000),(0x180000,0x9000));ACTORS=[0x1c05b0+0x10c*i for i in range(10)]
rng=random.Random(0x1034a);coverage=set();count=0;natural_fallbacks=set();events=[];errors=[];native_mem=None
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Effect=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(Context))
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',Effect),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def get(mem,at,n=2):return int.from_bytes(mem[at:at+n],'big')
def digest(mem):return hashlib.sha256(b''.join(mem[at:at+n]for at,n in REGIONS)).hexdigest()
machine=mus.Machine(mus.CPUType.M68000,2048);machine.mem.w_block(0,rom)
def execute(mem,regs,owner):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner);calls=[]
 for _ in range(300000):
  pc=machine.cpu.r_pc()
  if pc==0x70000:break
  coverage.add(pc)
  if pc in(0x10af0,0x10b26):natural_fallbacks.add(pc)
  if pc==0x2aea:
   current=bytearray(mem)
   for at,n in REGIONS:current[at:at+n]=machine.mem.r_block(at,n)
   calls.append((pc,tuple(machine.cpu.r_reg(i)for i in range(15)),digest(current)))
  machine.cpu.execute(1)
 assert machine.cpu.r_pc()==0x70000,('ROM runaway',hex(owner),hex(machine.cpu.r_pc()))
 for at,n in REGIONS:mem[at:at+n]=machine.mem.r_block(at,n)
 return[machine.cpu.r_reg(i)for i in range(15)],calls
@Effect
def effect(_user,owner,ptr):
 try:
  c=ptr.contents;regs=list(c.regs.contents.d)+list(c.a);events.append((owner,tuple(regs),digest(native_mem.raw)))
  assert owner==0x2aea,('unknown effect',hex(owner))
  mem=bytearray(native_mem.raw);regs,_=execute(mem,regs,owner)
  for at,n in REGIONS:ctypes.memmove(ctypes.addressof(native_mem)+at,bytes(mem[at:at+n]),n)
  for i,v in enumerate(regs[:8]):c.regs.contents.d[i]=v
  for i,v in enumerate(regs[8:]):c.a[i]=v
  return 0
 except BaseException as e:errors.append(e);return 1
def seed(mask=1,replay=0,progress=0,identity=0,history=0,ownership=0,difficulty=0):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 put(mem,0x1c0161,0,1);put(mem,0x1c0162,progress);put(mem,0x1c008c,replay);put(mem,0x1c0066,(difficulty<<8)|0xfc)
 mem[0x1c1600:0x1c1630]=bytes(48)
 for i,actor in enumerate(ACTORS):
  active=mask&(1<<i);put(mem,actor,0x8000 if active else 0);put(mem,actor+2,(identity+i)%12);put(mem,actor+0x56,(identity+i)%12);put(mem,actor+0xa1,ownership,1)
 for i in range(12):put(mem,0x1c0380+i*2,(identity+i)%12)
 for i in range(11):put(mem,0x1c0598+i*2,0xffff)
 if history<10:put(mem,0x1c0598+history*2,0x7c)
 elif history==10:
  for i,x in enumerate((0,1,2,3,6,7,8,9,10,11)):put(mem,0x1c0598+i*2,x)
 elif history==11:
  for i,x in enumerate((0,1,2,3,6,7,0xffff,0xffff,0xffff,0xffff)):put(mem,0x1c0598+i*2,x)
 return mem

def fixtures():
 yield 'live-P1',seed(history=12)
 if args.smoke:return
 for mask,replay,progress,ownership,history in itertools.product((1,4,5,15),(0,1,0x100,0xffff),range(10),(0,1),(0,9,10,11,12)):
  yield f'route-{mask:x}-{replay:x}-{progress}-{ownership}-{history}',seed(mask,replay,progress,(mask+progress)%12,history,ownership,progress%4)
 for identity,progress,oldflag,alias in itertools.product(range(12),(0,1,4,9),(0,0x80,0xff),(0,5,6,0xff)):
  mem=seed(1,0,progress,identity,11,1,identity%4)
  for actor in ACTORS:put(mem,actor+0x104,oldflag,1)
  put(mem,0x1c05ef,alias,1)
  yield f'fields-{identity}-{progress}-{oldflag:x}-{alias:x}',mem
 for credit in (0,1,0x100,0xffff):
  mem=seed(1,0,0,0,12);put(mem,0x1c0161,1,1);put(mem,0x1c007c,credit);mem[0x1c16a8:0x1c16c0]=bytes(24)
  yield f'royal-dispatch-{credit:x}',mem
source=ROOT/'src/wf_r18_match_setup.c'
with tempfile.TemporaryDirectory(prefix='wf-match-setup-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'match.so';src='tests/r18/match_setup_pending.c'if args.pending else'src/wf_r18_match_setup.c'
 sources=[src]+([]if args.pending else['src/wf_r18_royal_setup.c'])
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_match_setup_1086c.argtypes=[ctypes.POINTER(Context)]
 for name,mem in fixtures():
  regs=[rng.getrandbits(32)for _ in range(15)];want=bytearray(mem);wr,calls=execute(want,regs,0x1086c)
  native_mem=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(native_mem)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),effect,None,0)
  events.clear();status=native.wf_r18_match_setup_1086c(ctypes.byref(c));assert not errors,errors
  assert status==0,(name,'pending',hex(c.dependency),status)
  assert events==calls,(name,'palette dependency context',events,calls)
  for at,n in REGIONS:
   got=native_mem.raw[at:at+n];expected=want[at:at+n]
   assert got==expected,(name,'RAM/palettes',[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:12])
  assert list(sr.d)+list(c.a)==wr,(name,'D/A',list(sr.d)+list(c.a),wr)
  assert list(sr.d)[:4]==regs[:4]and list(c.a)[:3]==regs[8:11],(name,'MOVEM preservation')
  count+=1
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 required=(0x1034a,0x1037c,0x103be,0x103e6,0x103ee,0x10430,0x1044c,0x1046c,0x1049c,0x104ee,0x10518,0x1054e,0x10570,0x105b8,0x10634,0x106a8,0x106be,0x10718,0x1079a,0x107e0,0x10810)
 if not args.smoke:assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc)for pc in required if pc not in coverage])
 report={'status':'PASS','cases':count,'rom_sha1':hashlib.sha1(rom).hexdigest(),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'scope':'Raw TAG1034A through106DE plus reused Royal1086C dispatcher','compared':['16KB RAM/history/roster/replay/padding','FG16KB','palette36KB including padding','D0-D7/A0-A6','RNG','palette-call context/order and RAM/FG/palette digests','MOVEM preservation'],'delegated_intact_rom':['2AEA palette allocator'],'intercepts':[],'covered_rom_sites':[hex(pc)for pc in required if pc in coverage],'limitations':['No C98 caller, native palette backend or hardware claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
