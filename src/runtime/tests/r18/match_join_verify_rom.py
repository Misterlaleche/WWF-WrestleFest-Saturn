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
REGIONS=((0x1c0000,0x4000),(0x80000,0x10000),(0xc0000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40));ACTORS=[0x1c05b0+0x10c*i for i in range(10)]
rng=random.Random(0x1034a);coverage=set();count=0;natural_fallbacks=set();events=[];errors=[];native_mem=None
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Effect=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(Context))
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',Effect),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def get(mem,at,n=2):return int.from_bytes(mem[at:at+n],'big')
def digest(mem):return hashlib.sha256(b''.join(mem[at:at+n]for at,n in REGIONS)).hexdigest()
machine=mus.Machine(mus.CPUType.M68000,2048);machine.mem.w_block(0,rom)
EXTERNAL={0x2aea,0x2988,0x1fc0,0x1f6c,0x26e66,0x2a06,0x1f9e}
def poll(pc):
 if pc in(0x298c,0x2a0a):machine.mem.w8(0x140026,machine.mem.r8(0x140026)|4)
 elif pc in(0x29f6,0x2a88):machine.mem.w8(0x140026,machine.mem.r8(0x140026)&0xfb)
def execute(mem,regs,owner,stop=0x70000):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner);calls=[];ignored_return=None
 for _ in range(2000000):
  pc=machine.cpu.r_pc()
  if pc==stop:break
  poll(pc)
  if pc==ignored_return:ignored_return=None
  coverage.add(pc)
  if pc in(0x10af0,0x10b26):natural_fallbacks.add(pc)
  if pc in EXTERNAL and ignored_return is None:
   current=bytearray(mem)
   for at,n in REGIONS:current[at:at+n]=machine.mem.r_block(at,n)
   calls.append((pc,tuple(machine.cpu.r_reg(i)for i in range(15)),digest(current)));ignored_return=machine.mem.r32(machine.cpu.r_sp())
  machine.cpu.execute(1)
 assert machine.cpu.r_pc()==stop,('ROM runaway',hex(owner),hex(machine.cpu.r_pc()))
 for at,n in REGIONS:mem[at:at+n]=machine.mem.r_block(at,n)
 return[machine.cpu.r_reg(i)for i in range(15)],calls
@Effect
def effect(_user,owner,ptr):
 try:
  c=ptr.contents;regs=list(c.regs.contents.d)+list(c.a);events.append((owner,tuple(regs),digest(native_mem.raw)))
  assert owner in EXTERNAL,('unknown effect',hex(owner))
  mem=bytearray(native_mem.raw);regs,_=execute(mem,regs,owner)
  for at,n in REGIONS:ctypes.memmove(ctypes.addressof(native_mem)+at,bytes(mem[at:at+n]),n)
  for i,v in enumerate(regs[:8]):c.regs.contents.d[i]=v
  for i,v in enumerate(regs[8:]):c.a[i]=v
  return 0
 except BaseException as e:errors.append(e);return 1
class Join(ctypes.Structure):_fields_=[('pc',ctypes.c_uint32),('blocked_dependency',ctypes.c_uint32)]
def seed(mode=0,credit=1,progress=0,mask=1):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 put(mem,0x1c0161,mode,1);put(mem,0x1c0162,progress);put(mem,0x1c007c,credit);put(mem,0x1c007e,0);put(mem,0x1c008c,0)
 for i,actor in enumerate(ACTORS):
  put(mem,actor,0x8000 if mask&(1<<i)else 0);put(mem,actor+2,i%12);put(mem,actor+0x56,i%12);put(mem,actor+0xa0,1)
 for i in range(10):put(mem,0x1c0598+i*2,0xffff)
 for i in range(12):put(mem,0x1c0380+i*2,i)
 return mem
def fixtures():
 yield 'preserve-d1c',0xd1c,seed()
 if args.smoke:return
 for owner in (0xd1c,0xffd2,0x1004a):
  for _ in range(64):yield f'owner-{owner:x}',owner,seed(rng.randrange(256),rng.getrandbits(16),rng.randrange(10),rng.randrange(512))
 for mode,credit,progress,mask in itertools.product((0,1),(0,1,0x100,0xffff),range(10),(1,4,5,15)):
  if mode==1 and mask==15:continue # Four identities0..3 exceed Royal lower-half3.
  yield f'join-{mode}-{credit:x}-{progress}-{mask:x}',0xc98,seed(mode,credit,progress,mask)
source=ROOT/'src/wf_r18_match_setup.c'
with tempfile.TemporaryDirectory(prefix='wf-match-join-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'join.so';src='tests/r18/match_join_pending.c'if args.pending else'src/wf_r18_match_setup.c'
 sources=[src]+([]if args.pending else['src/wf_r18_royal_setup.c'])
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib))
 for name,owner,mem in fixtures():
  regs=[rng.getrandbits(32)for _ in range(15)];want=bytearray(mem);wr,calls=execute(want,regs,owner,0xd10 if owner==0xc98 else 0x70000)
  native_mem=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(native_mem)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),effect,None,0)
  events.clear()
  if owner==0xc98:
   state=Join();native.wf_r18_match_join_init(ctypes.byref(state))
   for _ in range(100):
    status=native.wf_r18_match_join_step(ctypes.byref(state),ctypes.byref(c))
    if status!=0:break
   assert status==2 and state.pc==0xd10,(name,'join blocked',hex(state.pc),hex(state.blocked_dependency),hex(c.dependency),status)
  else:
   fn=getattr(native,{0xd1c:'wf_r18_match_setup_d1c',0xffd2:'wf_r18_match_setup_ffd2',0x1004a:'wf_r18_match_setup_1004a'}[owner]);fn.argtypes=[ctypes.POINTER(Context)];status=fn(ctypes.byref(c))
   assert status==0,(name,'pending',hex(c.dependency),status)
  assert not errors,errors
  assert events==calls,(name,'effect context/order',events,calls)
  for at,n in REGIONS:
   got=native_mem.raw[at:at+n];expected=want[at:at+n]
   assert got==expected,(name,'full memory',[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:12])
  assert list(sr.d)+list(c.a)==wr,(name,'D/A',list(sr.d)+list(c.a),wr)
  count+=1
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'rom_sha1':hashlib.sha1(rom).hexdigest(),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'scope':'D1C exact preservation/1F38,FFD2,1004A,and complete C98..D10 join','compared':['16KB RAM/history/roster/replay/padding','BG64KB','FG16KB','palette36KB/padding','board register windows','D0-D7/A0-A6','RNG','external-call context/order and all region digests'],'delegated_intact_rom':[hex(x)for x in sorted(EXTERNAL)],'external_events':['VBlank high/low at298C/29F6 and2A0A/2A88'],'intercepts':[],'limitations':['C98 only; preceding live selection AC0/BD2 and post-presentation C6C caller not in this owner','No native dependency or hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
