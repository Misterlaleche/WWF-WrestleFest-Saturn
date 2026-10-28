#!/usr/bin/env python3
"""Complete A654 cursor with intact ROM external-owner delegation and IRQ tape.

All ROM bytes remain unchanged. Native external CALLs run their full original
helpers in Musashi with live RAM/D/A. Only audio2052 is bypassed. Tests supply
the same explicit external ACK/input events to both executions, including WAIT.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser();ap.add_argument('--report',type=Path);ap.add_argument('--pending',action='store_true');ap.add_argument('--smoke',action='store_true');ap.add_argument('--controls-only',action='store_true');args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
REGIONS=((0x1c0000,0x4000),(0xc0000,0x4000),(0x180000,0x9000),(0x100000,0x1000),(0x140000,0x40))
EXTERNAL={0x98ba,0x247c,0x27b8,0x2836,0x2b58,0x7506,0x2a06,0x8eb6,0x2052,0x2aea,0x9a42,0x2208}
ACTORS=[0x1c05b0+0x10c*i for i in range(9)];PRES=0x1c14ce;rng=random.Random(0xa654);coverage=set();cases=0
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Effect=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(Context))
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',Effect),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
class Scene(ctypes.Structure):_fields_=[('pc',ctypes.c_uint32),('blocked_dependency',ctypes.c_uint32),('saved_d0',ctypes.c_uint32)]
IO=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(Context))
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def get(mem,at,n=2):return int.from_bytes(mem[at:at+n],'big')
machine=mus.Machine(mus.CPUType.M68000,2048);machine.mem.w_block(0,rom)
def palette_poll(pc):
 # Delegated $2A06 polls the actual board VBlank status at entry/exit.
 if pc==0x2a0a:machine.mem.w8(0x140026,machine.mem.r8(0x140026)|4)
 elif pc==0x2a88:machine.mem.w8(0x140026,machine.mem.r8(0x140026)&0xfb)
def write_regions(mem):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
def read_regions(mem):
 for at,n in REGIONS:mem[at:at+n]=machine.mem.r_block(at,n)
def signature(mem,regs):return(hashlib.sha256(b''.join(mem[a:a+n]for a,n in REGIONS)).hexdigest(),tuple(regs))
def helper(mem,regs,owner):
 write_regions(mem)
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner)
 for _ in range(500000):
  pc=machine.cpu.r_pc()
  if pc==0x70000:break
  palette_poll(pc)
  if pc==0x2052:machine.cpu.w_pc(machine.mem.r32(machine.cpu.r_sp()));machine.cpu.w_sp(machine.cpu.r_sp()+4);continue
  machine.cpu.execute(1)
 assert machine.cpu.r_pc()==0x70000,('helper runaway',hex(owner),hex(machine.cpu.r_pc()))
 read_regions(mem);return[machine.cpu.r_reg(i)for i in range(15)]
def inputs(fixture,frame):
 words=[0xffff]*4
 if frame==1 and fixture.get('nonaction'):words[fixture.get('port',0)]^=fixture['nonaction']
 if frame>=fixture['skip']:words[fixture.get('port',0)]^=fixture.get('mask',0x10)
 return words
def irq(mem,fixture,irq_state):
 irq_state[0]+=1
 if irq_state[0]<=fixture.get('delay',0):return
 irq_state[0]=0;irq_state[1]+=1
 mem[0x1c006f]|=2
 # This represents a supplied IRQ event; production never invents this clock.
 put(mem,0x1c0080,get(mem,0x1c0080,4)+1,4)
 for i,v in enumerate(inputs(fixture,irq_state[1])):put(mem,0x140020+i*2,v)
def original(mem,regs,fixture):
 write_regions(mem)
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(0xa654)
 tape=[];ignored_return=None;irq_state=[0,0]
 for _ in range(12000000):
  pc=machine.cpu.r_pc();coverage.add(pc)
  if pc==0xa7b2:break
  palette_poll(pc)
  if pc==ignored_return:ignored_return=None
  if ignored_return is None:
   kind='call'if pc in EXTERNAL else'sync'if pc==0xa728 else'read'if pc in(0xa732,0xa73e,0xa74a,0xa756)else None
   if kind:
    read_regions(mem);current=[machine.cpu.r_reg(i)for i in range(15)];tape.append((kind,pc,signature(mem,current)))
    if kind=='sync':irq(mem,fixture,irq_state);write_regions(mem)
    elif kind=='call'and pc!=0x2052:ignored_return=machine.mem.r32(machine.cpu.r_sp())
  if pc==0x2052:machine.cpu.w_pc(machine.mem.r32(machine.cpu.r_sp()));machine.cpu.w_sp(machine.cpu.r_sp()+4);continue
  machine.cpu.execute(1)
 assert machine.cpu.r_pc()==0xa7b2,('scene runaway',fixture,hex(machine.cpu.r_pc()))
 read_regions(mem);return[machine.cpu.r_reg(i)for i in range(15)],tape,irq_state[1]
native_mem=None;native_fixture=None;native_irq=None;native_tape=None;event_at=0;errors=[]
def checkpoint(kind,owner,ptr):
 global event_at
 c=ptr.contents;mem=bytearray(native_mem.raw);regs=list(c.regs.contents.d)+list(c.a)
 got=(kind,owner,signature(mem,regs));assert event_at<len(native_tape),('extra event',got[:2])
 want=native_tape[event_at];assert got==want,('event checkpoint',event_at,got,want)
 event_at+=1;return c,mem,regs
def store(mem,regs,c):
 for at,n in REGIONS:ctypes.memmove(ctypes.addressof(native_mem)+at,bytes(mem[at:at+n]),n)
 for i,v in enumerate(regs[:8]):c.regs.contents.d[i]=v
 for i,v in enumerate(regs[8:]):c.a[i]=v
@Effect
def effect(_user,owner,ptr):
 try:
  c,mem,regs=checkpoint('call',owner,ptr)
  if owner!=0x2052:regs=helper(mem,regs,owner);store(mem,regs,c)
  return 0
 except BaseException as e:errors.append(e);return 1
@IO
def io(_user,kind,owner,address,value,ptr):
 try:
  c,mem,regs=checkpoint('sync'if kind==0 else'read',owner,ptr)
  if kind==0:
   assert address==0x1c006f and value[0]==2
   irq(mem,native_fixture,native_irq);store(mem,regs,c)
  else:
   assert address=={0xa732:0x140020,0xa73e:0x140022,0xa74a:0x140024,0xa756:0x140026}[owner]
   value[0]=get(mem,address)
  return 0
 except BaseException as e:errors.append(e);return 1
def seed(f):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in ((0xc0000,0x4000),(0x180000,0x9000)):mem[at:at+n]=rng.randbytes(n)
 put(mem,0x1c005c,rng.getrandbits(32),4);put(mem,0x1c0080,rng.getrandbits(32),4)
 put(mem,0x1c0161,f['mode'],1);put(mem,0x1c0163,f.get('progress',0),1);put(mem,0x1c0168,f.get('skipflag',0));put(mem,0x1c15fc,1)
 put(mem,0x1c17e6,0x200);put(mem,0x1c17ee,0x250)
 for i,actor in enumerate(ACTORS):
  active=bool(f.get('active',1)&(1<<i));identity=(i+f.get('identity',0))%12
  put(mem,actor,0x8000 if active else 0);put(mem,actor+2,identity);put(mem,actor+0x56,0x8000|identity)
  put(mem,actor+6,0x240+i*8);put(mem,actor+0xa,0x150);put(mem,actor+0xe,0x140)
  put(mem,actor+0x1c,0);put(mem,actor+0x22,f.get('timer',0));put(mem,actor+0x66,0x7000)
 put(mem,PRES+0x1c,f.get('state',0));put(mem,PRES+0x22,f.get('presenter_timer',0));put(mem,PRES,0x8001);put(mem,PRES+2,31);put(mem,PRES+0xa,0x178)
 for i,v in enumerate(inputs(f,0)):put(mem,0x140020+i*2,v)
 return mem
def fixtures():
 yield{'mode':1,'skip':1,'delay':1}
 if args.smoke:return
 for mode,port,mask,delay,active,progress in itertools.product((0,1),range(4),(0x10,0x20,0x30),(0,2),(0,1,0x1ff),(0,1)):
  if mode==0 and active==0:continue # $98BA requires a live participant/A3 producer.
  yield{'mode':mode,'port':port,'mask':mask,'skip':1,'delay':delay,'active':active,'progress':progress}
 for mode,port,mask in itertools.product((0,1),range(4),(1,2,4,8,0x40,0x80,0x100,0x8000)):
  yield{'mode':mode,'port':port,'nonaction':mask,'skip':3,'delay':1,'active':0x3f}
 for state in range(24):yield{'mode':0,'state':state,'skip':2,'delay':1,'active':0x1ff,'identity':state%12}
 for mode in(0,1):yield{'mode':mode,'skip':3,'delay':2,'active':0x1ff,'progress':1}
 # Royal natural terminal (one and six active actors); no action input.
 for active in(1,0x3f):yield{'mode':1,'skip':10000,'delay':0,'active':active}
 yield{'mode':0,'skip':10000,'delay':0,'active':0xf,'identity':0}
source=ROOT/'src/wf_r18_prematch.c'
with tempfile.TemporaryDirectory(prefix='wf-prematch-scene-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'scene.so';src='tests/r18/prematch_pending_scene.c'if args.pending else'src/wf_r18_prematch.c'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',src,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_prematch_scene_step.argtypes=[ctypes.POINTER(Scene),ctypes.POINTER(Context),IO,ctypes.c_void_p]
 native.wf_r18_prematch_scene_init.argtypes=[ctypes.POINTER(Scene)]
 for fixture in (()if args.controls_only else fixtures()):
  print('fixture',cases,fixture,flush=True)
  mem=seed(fixture);regs=[rng.getrandbits(32)for _ in range(15)];want=bytearray(mem);wr,native_tape,frames=original(want,regs,fixture)
  native_mem=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(native_mem)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),effect,None,0)
  scene=Scene();native.wf_r18_prematch_scene_init(ctypes.byref(scene));native_fixture=fixture;native_irq=[0,0];event_at=0;waits=0
  for _ in range(500000):
   status=native.wf_r18_prematch_scene_step(ctypes.byref(scene),ctypes.byref(c),io,None)
   assert not errors,errors
   assert status>=0,('native blocked',fixture,hex(scene.pc),hex(scene.blocked_dependency),hex(c.dependency))
   if status==1:waits+=1
   if status==2:break
  assert status==2 and scene.pc==0xa7b2
  assert event_at==len(native_tape)and native_irq[1]==frames
  assert waits==frames*fixture.get('delay',0),(fixture,'WAIT count',waits,frames)
  for at,n in REGIONS:
   got=native_mem.raw[at:at+n];expected=want[at:at+n]
   assert got==expected,('final memory',fixture,[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:12])
  assert list(sr.d)+list(c.a)==wr,('final D/A',fixture,list(sr.d)+list(c.a),wr)
  cases+=1
 controls=0
 if not args.pending:
  @IO
  def noop_io(_u,kind,owner,address,value,ptr):return 0
  @IO
  def failed_io(_u,kind,owner,address,value,ptr):return 1
  @Effect
  def failed_effect(_u,owner,ptr):
   ptr.contents.ram[0x5c]^=1;ptr.contents.regs.contents.d[3]=0x12345678;return 1
  def context():
   mem=seed({'mode':1,'skip':1});buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
   c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0)
   s=Scene();native.wf_r18_prematch_scene_init(ctypes.byref(s));return s,c,buf,sr
  s,c,buf,sr=context();fn=native.wf_r18_prematch_scene_step
  assert fn(None,ctypes.byref(c),noop_io,None)==-2;controls+=1
  assert fn(ctypes.byref(s),None,noop_io,None)==-2;controls+=1
  assert fn(ctypes.byref(s),ctypes.byref(c),IO(),None)==-2;controls+=1
  for bad in('ram','regs','valid'):
   s,c,buf,sr=context()
   if bad=='ram':c.ram=ctypes.POINTER(ctypes.c_uint8)()
   elif bad=='regs':c.regs=ctypes.POINTER(Shadow)()
   else:sr.valid=0
   assert fn(ctypes.byref(s),ctypes.byref(c),noop_io,None)==-2;controls+=1
  s,c,buf,sr=context();s.pc=0xdead
  assert fn(ctypes.byref(s),ctypes.byref(c),noop_io,None)==-1 and s.blocked_dependency==c.dependency==0xdead;controls+=1
  for mode,pc,dep in((0,0xa666,0x98ba),(1,0xa676,0x2052)):
   s,c,buf,sr=context();c.ram[0x161]=mode;s.pc=pc
   assert fn(ctypes.byref(s),ctypes.byref(c),noop_io,None)==-1 and s.blocked_dependency==c.dependency==dep;controls+=1
  s,c,buf,sr=context();s.pc=0xa728;c.ram[0x6f]=1;before=bytes(buf);dr=list(sr.d);ar=list(c.a)
  for _ in range(2):assert fn(ctypes.byref(s),ctypes.byref(c),noop_io,None)==1 and s.pc==0xa728;controls+=1
  assert bytes(buf)==before and list(sr.d)==dr and list(c.a)==ar
  s.pc=0xa7b2;assert fn(ctypes.byref(s),ctypes.byref(c),noop_io,None)==2
  assert bytes(buf)==before and list(sr.d)==dr and list(c.a)==ar;controls+=1
  s,c,buf,sr=context();s.pc=0xa73e;before=sr.d[1]
  assert fn(ctypes.byref(s),ctypes.byref(c),failed_io,None)==-1 and s.blocked_dependency==0xa73e and sr.d[1]==before;controls+=1
  s,c,buf,sr=context();s.pc=0xa6c0;c.effect=failed_effect;before=c.ram[0x5c]
  assert fn(ctypes.byref(s),ctypes.byref(c),noop_io,None)==-1 and s.blocked_dependency==0x247c
  assert c.ram[0x5c]==(before^1)and sr.d[3]==0x12345678;controls+=1
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':cases,'guard_controls':controls,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'scope':'Full A654..A7B2 join/render/IRQ wait/input/teardown; TAG and Royal split','compared':['event-entry RAM/FG/palette/padding/sprite-port digests','event-entry D0-D7/A0-A6','exact CALL/SYNC/READ16 order','final full memory regions and D/A','WAIT count','terminal original PC A7B2'],'delegated_intact_rom':[hex(x)for x in sorted(EXTERNAL-{0x2052})],'intercepts':['2052 sound output only'],'external_poll_drive':['2A06 VBlank-high2A0A and VBlank-low2A88'],'limitations':['IRQ/physical words are explicit test-supplied events, not hardware timing validation','CallerC6C and Royal1086C setup not part of A654']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
