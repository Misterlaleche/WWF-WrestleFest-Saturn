#!/usr/bin/env python3
"""TAG $A68A/$A7B4 against intact ROM, with original dependencies delegated.

Controller port owns all branch, register, RNG and byte/word semantics.
The bridge executes $2AEA/$9A42/$2208 in a second intact Musashi context;
only device audio $2052 is intercepted on both sides. Thus this does not
claim a native implementation of delegated dependencies or the $A654 loop.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');p.add_argument('--pending',action='store_true');p.add_argument('--actor-pending',action='store_true');p.add_argument('--controls-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
CALLBACKS=struct.unpack('>24I',rom[0xa7b4:0xa814]);ACTORS=[0x1c05b0+0x10c*i for i in range(9)];PRES=0x1c14ce
REGIONS=((0x1c0000,0x4000),(0xc0000,0x4000),(0x180000,0x9000))
rng=random.Random(0xa7b4);coverage=set();cases=0;events=[]
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Effect=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(Context))
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',Effect),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def initial(state=0,latched=False,identity=0):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 put(mem,0x1c0163,0,1);put(mem,0x1c0168,0);put(mem,PRES,0x8001);put(mem,PRES+0x1c,state|(0x8000 if latched else 0))
 put(mem,PRES+0xa,0x178);put(mem,PRES+0x2a,8);put(mem,PRES+0x2c,0)
 # Palette map and occupancy are real inputs, not a patched allocator.
 mem[0x1c1600:0x1c1630]=bytes(0x30)
 for actor in ACTORS:put(mem,actor,0x8000);put(mem,actor+0x56,identity)
 return mem
def machine():
 m=mus.Machine(mus.CPUType.M68000,2048);m.mem.w_block(0,rom);return m
expected=machine();helper=expected # Musashi binding has one active memory context.
def execute(m,mem,regs,start,stop=0x70000,record=False):
 for at,n in REGIONS:m.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):m.cpu.w_reg(i,v)
 m.cpu.w_sp(0x1d0000);m.mem.w32(0x1d0000,stop);m.cpu.w_pc(start);calls=[]
 for _ in range(120000):
  pc=m.cpu.r_pc()
  if pc==stop:break
  if record:coverage.add(pc)
  if pc in (0x2052,0x2aea,0x9a42,0x2208):calls.append(pc)
  if pc==0x2052:m.cpu.w_pc(m.mem.r32(m.cpu.r_sp()));m.cpu.w_sp(m.cpu.r_sp()+4);continue
  m.cpu.execute(1)
 assert m.cpu.r_pc()==stop,('ROM runaway',hex(start),hex(m.cpu.r_pc()))
 for at,n in REGIONS:mem[at:at+n]=m.mem.r_block(at,n)
 return [m.cpu.r_reg(i)for i in range(15)],calls
native_mem=None;callback_error=[]
@Effect
def effect(_user,owner,ptr):
 try:
  c=ptr.contents;events.append(owner);regs=list(c.regs.contents.d)+list(c.a)
  if owner not in (0x2052,0x2aea,0x9a42,0x2208):raise AssertionError(('unknown effect',hex(owner)))
  if owner!=0x2052:
   current=bytearray(native_mem.raw);regs,_=execute(helper,current,regs,owner)
   for at,n in REGIONS:ctypes.memmove(ctypes.addressof(native_mem)+at,bytes(current[at:at+n]),n)
   for i,v in enumerate(regs[:8]):c.regs.contents.d[i]=v
   for i,v in enumerate(regs[8:]):c.a[i]=v
  return 0
 except BaseException as e:callback_error.append(e);return 1
def compare(native,name,mem,regs,entry=0xa68a):
 global native_mem,cases
 want=bytearray(mem);wr,we=execute(expected,want,regs,entry,0xa6a2 if entry==0xa68a else 0x70000,True)
 native_mem=ctypes.create_string_buffer(bytes(mem));shadow=Shadow((ctypes.c_uint32*8)(*regs[:8]),1)
 c=Context(ctypes.cast(ctypes.addressof(native_mem)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(shadow),(ctypes.c_uint32*7)(*regs[8:]),effect,None,0)
 events.clear();fn=native.wf_r18_prematch_tag_step_a68a if entry==0xa68a else native.wf_r18_prematch_actor_step_acb6
 status=fn(ctypes.byref(c))
 assert not callback_error,callback_error
 assert status==0,(name,'pending owner',hex(c.dependency),status)
 assert events==we,(name,'dependency order',events,we)
 for at,n in REGIONS:
  got=native_mem.raw[at:at+n];w=want[at:at+n]
  assert got==w,(name,'RAM/FG/palettes',[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,w))if g!=e][:12])
 gotregs=list(shadow.d)+list(c.a)
 assert gotregs==wr,(name,'D0-D7/A0-A6',[(i,hex(g),hex(w))for i,(g,w)in enumerate(zip(gotregs,wr))if g!=w])
 cases+=1;return want,wr
def fixtures():
 for state,latch,timer,index,aux in itertools.product(range(24),(False,True),(0,1,6,11,12,16,17,20,32,33,63,64,128,129,160,161,256,257,0x7fff,0x8000,0xfffe,0xffff),(0,1,2,3,4),(0,1,0x80,0xff)):
  if args.smoke and(state!=0 or latch or timer or index or aux):continue
  mem=initial(state,latch);put(mem,PRES+0x22,timer);put(mem,PRES+0x24,index);put(mem,PRES+0x1e,timer);put(mem,PRES+0x25,aux,1)
  yield f'boundary-{state:x}-{latch}-{timer:x}-{index:x}-{aux:x}',mem
 for state,identity,active,flags in itertools.product((4,8,13,17,5,9,14,18),range(12),(0,0x80),(0,1,0x8000,0xffff)):
  if args.smoke:break
  mem=initial(state,False,identity);put(mem,0x1c0168,flags)
  for actor in ACTORS:put(mem,actor,active,1)
  yield f'identity-{state:x}-{identity}-{active:x}-{flags:x}',mem
 if not args.smoke:
  for state,high in itertools.product(range(24),(0x40,0xc0)):
   mem=initial(state);put(mem,PRES+0x1c,(high<<8)|state)
   yield f'dispatch-latch-alias-{state:x}-{high:x}',mem
  for state,flag in itertools.product((5,9,14,18),(0,1,0x8000,0xffff)):
   mem=initial(state,True);put(mem,PRES+0x22,63);put(mem,0x1c0168,flag)
   yield f'activation-skip-{state:x}-{flag:x}',mem
  for progress,timer in itertools.product((0,1,0xff),(127,128,129,0xffff)):
   mem=initial(0,True);put(mem,PRES+0x22,timer);put(mem,0x1c0163,progress,1)
   yield f'presenter-progress-{progress:x}-{timer:x}',mem
  for y,frac,angle in itertools.product((0x178,0x1a6,0x1a7,0xffff),(0,1,0x7fff,0x8000,0xffff),(0,0x20,0x40,0x80,0xff)):
   mem=initial(21,True);put(mem,PRES+0xa,y);put(mem,PRES+0xc,frac);put(mem,PRES+0x2c,angle)
   yield f'move-border-{y:x}-{frac:x}-{angle:x}',mem
source=ROOT/'src/wf_r18_prematch.c';pending=args.pending or not source.exists()
with tempfile.TemporaryDirectory(prefix='wf-prematch-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'prematch.so';src='tests/r18/prematch_pending_guard.c'if pending or args.actor_pending else'src/wf_r18_prematch.c'
 sources=[src]+(['tests/r18/prematch_pending_actor.c']if args.actor_pending else[])
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_prematch_tag_step_a68a.argtypes=[ctypes.POINTER(Context)]
 if args.actor_pending:
  mem=initial();put(mem,ACTORS[0]+0x1d,1,1);put(mem,ACTORS[0]+0x22,0);put(mem,ACTORS[0]+0x24,0)
  regs=[rng.getrandbits(32)for _ in range(15)];regs[8]=ACTORS[0]
  compare(native,'actor-first-entry',mem,regs,0xacb6)
 if not args.controls_only:
  for name,mem in fixtures():compare(native,name,mem,[rng.getrandbits(32)for _ in range(15)])
 if not args.smoke and not args.controls_only:
  # Complete natural 24-state route. Frame counter remains externally supplied.
  # Identity0 gives four65-update windows; identity1=129 and9=257.
  timelines={}
  for identity in (0,1,9):
   mem=initial(identity=identity);regs=[rng.getrandbits(32)for _ in range(15)];counts=[0]*24
   put(mem,PRES+0xc,0) # Explicit fresh-presenter fractional Y prerequisite.
   while mem[PRES+0x1d]<24:
    state=mem[PRES+0x1d];counts[state]+=1
    assert sum(counts)<3000,('timeline runaway',identity,counts)
    mem,regs=compare(native,f'timeline-{identity}-{sum(counts)}',mem,regs)
   fixed=sum(n for i,n in enumerate(counts)if i not in(4,8,13,17));assert fixed==989,(identity,fixed,counts)
   window={0:65,1:129,9:257}[identity];assert all(counts[i]==window for i in(4,8,13,17)),(identity,counts)
   timelines[str(identity)]=counts
  for slot,identity,mode,flags,phase,timer in itertools.product((0,4,8),range(12),(0,1),(0,0x80),(0,1),(0,6,7,8,16,17,32,33,63,64,0x7fff,0xffff)):
   mem=initial(identity=identity);actor=ACTORS[slot];put(mem,0x1c0161,mode,1);put(mem,actor+0x33,flags,1);put(mem,actor+0x1d,phase,1);put(mem,actor+0x22,timer);put(mem,actor+0x24,0)
   regs=[rng.getrandbits(32)for _ in range(15)];regs[8]=actor
   compare(native,f'actor-{slot}-{identity}-{mode}-{flags}-{phase}-{timer}',mem,regs,0xacb6)
  visual_counts=list(rom[0xad9c:0xada8])
  for identity,count in enumerate(visual_counts):
   for index,timer in itertools.product(range(count),(0,6,7,8,16,17,32,33,89,90,91,92,0xfffe,0xffff)):
    mem=initial(identity=identity);actor=ACTORS[identity%9];put(mem,actor+0x1d,1,1);put(mem,actor+0x22,timer);put(mem,actor+0x24,index)
    regs=[rng.getrandbits(32)for _ in range(15)];regs[8]=actor
    compare(native,f'visual-{identity}-{index}-{timer}',mem,regs,0xacb6)
  required=[0xa814,0xa8ba,0xa8c0,0xa8ec,0xa952,0xa99a,0xa9e2,0xaa5e,0xaa96,0xaaea,0xab54,0xab78,0xabc2]
  assert all(pc in coverage for pc in required),('missing branches',[hex(pc)for pc in required if pc not in coverage])
 else:timelines={}
 controls=0
 if not args.pending and not args.actor_pending:
  fn=native.wf_r18_prematch_tag_step_a68a
  assert fn(None)==2;controls+=1
  for invalid in('ram','regs','valid'):
   mem=initial();buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
   c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0)
   if invalid=='ram':c.ram=ctypes.POINTER(ctypes.c_uint8)()
   elif invalid=='regs':c.regs=ctypes.POINTER(Shadow)()
   else:sr.valid=0
   assert fn(ctypes.byref(c))==2;assert buf.raw[:len(mem)]==mem;controls+=1
  for state,word,status,dependency in((24,24,0,0),(25,25,1,0xa7b4),(0,0x100,1,0xa7b4)):
   mem=initial(state);put(mem,PRES+0x1c,word);buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
   c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0)
   assert fn(ctypes.byref(c))==status and c.dependency==dependency
   assert buf.raw[:len(mem)]==mem and list(sr.d)==list(range(8))and list(c.a)==list(range(7));controls+=1
  mem=initial();buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
  c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0)
  assert fn(ctypes.byref(c))==1 and c.dependency==0x2aea
  assert buf.raw[0x1c1600:0x1c1630]==mem[0x1c1600:0x1c1630]and buf.raw[PRES+0x1c]==0x80;controls+=1
  for address,identity,index,dep in((0,0,0,0xacb6),(ACTORS[0],12,0,0xad6c),(ACTORS[0],0,11,0xada8)):
   mem=initial(identity=identity);put(mem,ACTORS[0]+0x1d,1,1);put(mem,ACTORS[0]+0x24,index);buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
   c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(address,0,0,0,0,0,0),Effect(),None,0)
   assert native.wf_r18_prematch_actor_step_acb6(ctypes.byref(c))==1 and c.dependency==dep;controls+=1
 assert bytes(expected.mem.r_block(0,len(rom)))==rom;expected.cleanup()
 result={'status':'PASS','cases':cases,'guard_controls':controls,'rom_sha1':hashlib.sha1(rom).hexdigest(),'controller_callbacks':[hex(x)for x in CALLBACKS],'timelines':timelines,'timeline_prerequisite':'Synthetic fresh presenter fractional Y zero; A9E2 itself preserves fractional coordinates','compared':['16KB RAM','FG16KB','palette36KB including padding','D0-D7/A0-A6','RNG','dependency order'],'delegated_intact_rom':['2AEA','9A42','2208'],'intercepts':['2052 sound output only'],'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'limitations':['One TAG presenter update and actor ACB6 only; A654 join/render/input/teardown not ported','No native platform dependency or physical Saturn equivalence claim']}
 if args.report:args.report.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))
