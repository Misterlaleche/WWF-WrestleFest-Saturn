#!/usr/bin/env python3
"""Whole Royal $1086C vs intact ROM, RAM/palette padding and live D/A/RNG.

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
rng=random.Random(0x1086c);coverage=set();count=0;natural_fallbacks=set();events=[];errors=[];native_mem=None
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
def seed(credit,mask,ids,flags=0,progress=0):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 put(mem,0x1c007c,credit);put(mem,0x1c0161,1,1);put(mem,0x1c0163,progress,1)
 mem[0x1c16a8:0x1c16c0]=bytes(24);mem[0x1c1600:0x1c1630]=bytes(48)
 for i,actor in enumerate(ACTORS):
  put(mem,actor,0x8000 if mask&(1<<i)else 0)
  put(mem,actor+0x56,ids[i%len(ids)]);put(mem,actor+0x104,flags,1)
 return mem
def fixtures():
 yield'attract-six',seed(0,0x3f,[0,1,2,6,7,8])
 if args.smoke:return
 # Valid live topology: at most three humans in either six-identity half.
 for mask in range(16):
  n=mask.bit_count()
  for ids in itertools.product(range(12),repeat=min(n,2)):
   if n>2:ids=tuple((i*3+ids[i%2])%12 for i in range(n))
   chosen=iter(ids);actors=[next(chosen)if mask&(1<<i)else i for i in range(4)]
   if sum(actors[i]<6 for i in range(4)if mask&(1<<i))>3 or sum(actors[i]>=6 for i in range(4)if mask&(1<<i))>3:continue
   yield f'live-{mask:x}-{actors}',seed(1,mask,actors)
 for credit,flags,progress in itertools.product((0,1,0x100,0xffff),(0,1,0x7f,0x80,0xff),(0,1,0xff)):
  yield f'flags-{credit:x}-{flags:x}-{progress:x}',seed(credit,0xb,[2,4,7,10],flags,progress)
 # Pre-existing occupancy, without consuming all candidates needed by the owner.
 for half,bits in itertools.product((0,6),range(64)):
  # One participant occupies this half; leave at least two free candidates.
  if bits.bit_count()>3:continue
  mem=seed(1,1,[0 if half==0 else 6])
  for i in range(6):
   if bits&(1<<i):put(mem,0x1c16a8+(half+i)*2,0x8000 if i&1 else 1)
  # The original participant slot is also reserved by entry.
  free=sum(not get(mem,0x1c16a8+(half+i)*2)for i in range(1,6))
  if free<2:continue
  yield f'occupancy-{half}-{bits:x}',mem
 for mode,flags,palette_state in itertools.product((1,3,0x81,0xff),(0x0123,0x3f5a,0x7fab),(0,1,15,16)):
  mem=seed(1,1,[0]);put(mem,0x1c0161,mode,1)
  for i,actor in enumerate(ACTORS):put(mem,actor,flags|(0x8000 if i==0 else 0))
  if palette_state:
   mem[0x1c1600:0x1c1600+palette_state]=bytes([0xff])*palette_state
   if palette_state==1:mem[0x1c1610]=0x80 # Existing resource0/bank0 cache hit.
  yield f'preserve-{mode:x}-{flags:x}-{palette_state}',mem
source=ROOT/'src/wf_r18_royal_setup.c'
with tempfile.TemporaryDirectory(prefix='wf-royal-setup-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'royal.so';src='tests/r18/royal_setup_pending.c'if args.pending else'src/wf_r18_royal_setup.c'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',src,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_royal_setup_1086c.argtypes=[ctypes.POINTER(Context)]
 for name,mem in (()if args.controls_only else fixtures()):
  regs=[rng.getrandbits(32)for _ in range(15)];want=bytearray(mem);wr,calls=execute(want,regs,0x1086c)
  native_mem=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(native_mem)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),effect,None,0)
  events.clear();status=native.wf_r18_royal_setup_1086c(ctypes.byref(c));assert not errors,errors
  assert status==0,(name,'pending',hex(c.dependency),status)
  assert events==calls,(name,'palette dependency context',events,calls)
  for at,n in REGIONS:
   got=native_mem.raw[at:at+n];expected=want[at:at+n]
   assert got==expected,(name,'RAM/palettes',[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:12])
  assert list(sr.d)+list(c.a)==wr,(name,'D/A',list(sr.d)+list(c.a),wr)
  assert list(sr.d)[:4]==regs[:4]and list(c.a)[:3]==regs[8:11],(name,'MOVEM preservation')
  count+=1
 controls=0
 if not args.pending:
  fn=native.wf_r18_royal_setup_1086c
  def context():
   mem=seed(1,1,[0]);buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
   c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0)
   return c,buf,sr
  assert fn(None)==2;controls+=1
  for bad in('ram','regs','valid'):
   c,buf,sr=context()
   if bad=='ram':c.ram=ctypes.POINTER(ctypes.c_uint8)()
   elif bad=='regs':c.regs=ctypes.POINTER(Shadow)()
   else:sr.valid=0
   before=bytes(buf);assert fn(ctypes.byref(c))==2 and bytes(buf)==before;controls+=1
  for mode in(0,2,0xfe):
   c,buf,sr=context();c.ram[0x161]=mode;before=bytes(buf);regs=list(sr.d)+list(c.a)
   assert fn(ctypes.byref(c))==1 and c.dependency==0x1034a
   assert bytes(buf)==before and list(sr.d)+list(c.a)==regs;controls+=1
  c,buf,sr=context();assert fn(ctypes.byref(c))==1 and c.dependency==0x2aea;controls+=1
  c,buf,sr=context();c.ram[ACTORS[0]-0x1c0000+0x57]=12
  assert fn(ctypes.byref(c))==1 and c.dependency==0x1090a;controls+=1
  @Effect
  def accept(_u,_owner,_ptr):return 0
  c,buf,sr=context();c.effect=accept
  for actor in ACTORS[:4]:put_at=actor-0x1c0000;c.ram[put_at]=0x80;c.ram[put_at+0x56]=0;c.ram[put_at+0x57]=0
  assert fn(ctypes.byref(c))==1 and c.dependency==0x1098c;controls+=1
  c,buf,sr=context();c.effect=accept
  for actor in ACTORS[:4]:put_at=actor-0x1c0000;c.ram[put_at]=0x80;c.ram[put_at+0x56]=0;c.ram[put_at+0x57]=6
  assert fn(ctypes.byref(c))==1 and c.dependency==0x109dc;controls+=1
  @Effect
  def fail(_u,_owner,ptr):ptr.contents.ram[0x5c]^=1;return 1
  c,buf,sr=context();c.effect=fail;before=c.ram[0x5c]
  assert fn(ctypes.byref(c))==1 and c.dependency==0x2aea and c.ram[0x5c]==(before^1);controls+=1
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 required=(0x10884,0x108e0,0x10924,0x1092e,0x109a4,0x109ee,0x10a42,0x10782,0x1079a,0x107b0,0x1081a,0x10718,0x24d2,0x24e8,0x24f2,0x10af0,0x10b26)
 if not args.smoke and not args.controls_only:
  assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc)for pc in required if pc not in coverage])
 report={'status':'PASS','cases':count,'guard_controls':controls,'rom_sha1':hashlib.sha1(rom).hexdigest(),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'scope':'Whole Royal1086C through10ACA;24CC/21B4,firstfree,10782,10718','compared':['16KB RAM','FG16KB','palette36KB including padding','D0-D7/A0-A6','RNG','palette-call context/order and RAM/palette/FG digests','MOVEM-preserved caller registers'],'delegated_intact_rom':['2AEA palette allocator'],'intercepts':[],'covered_rom_sites':[hex(x)for x in required if x in coverage],'fallbacks':[hex(x)for x in sorted(natural_fallbacks)],'limitations':['No C98 caller, A654 presentation, native palette backend or hardware timing claim','Live humans limited to twelve IDs and at most three in each six-ID half']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
