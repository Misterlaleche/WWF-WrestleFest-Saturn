#!/usr/bin/env python3
"""$7B70/$7F0E/$7E86 against intact ROM, including all external owner bodies."""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser();ap.add_argument('--report',type=Path);ap.add_argument('--pending',action='store_true');ap.add_argument('--smoke',action='store_true');ap.add_argument('--helpers-only',action='store_true');ap.add_argument('--controls-only',action='store_true');args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
REGIONS=((0x1c0000,0x4000),(0x80000,0x10000),(0xc0000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40));ACTORS=[0x1c05b0+0x10c*i for i in range(10)]
EXTERNAL={0x2052,0x1fc0,0x26e66,0x2a06,0x285da,0x2aea,0x247c,0x27b8,0x2836,0x8fd6,0x1f9e,0x2503c,0xb29e,0x2208,0x8f50}
WAITS={0x7cde:0x7ce8,0x7dfc:0x7e06,0x7e5e:0x7e68}
rng=random.Random(0x7b70);coverage=set();count=0;wait_cases=0
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Effect=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(Context))
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',Effect),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
class Request(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('kind','owner','call_pc','return_pc','address','value')]
class Scene(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('pc','entry_pc','saved_d0','saved_a0','sub_return','blocked_dependency')]+[('request',Request),('pending',ctypes.c_uint8),('fatal',ctypes.c_uint8)]
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def get(mem,at,n=2):return int.from_bytes(mem[at:at+n],'big')
def digest(mem):return hashlib.sha256(b''.join(mem[at:at+n]for at,n in REGIONS)).hexdigest()
machine=mus.Machine(mus.CPUType.M68000,2048);machine.mem.w_block(0,rom)
def snapshot(mem):
 out=bytearray(mem)
 for at,n in REGIONS:out[at:at+n]=machine.mem.r_block(at,n)
 return out
def poll(pc):
 if rom[pc:pc+8]==bytes.fromhex('08390001001c006f'):machine.mem.w8(0x1c006f,machine.mem.r8(0x1c006f)|2)
 if rom[pc:pc+8]==bytes.fromhex('0839000200140026'):
  assert rom[pc+8]in(0x66,0x67)
  value=machine.mem.r8(0x140026);machine.mem.w8(0x140026,(value|4)if rom[pc+8]==0x67 else(value&0xfb))
 if pc in(0x298c,0x2a0a):machine.mem.w8(0x140026,machine.mem.r8(0x140026)|4)
 elif pc in(0x29f6,0x2a88):machine.mem.w8(0x140026,machine.mem.r8(0x140026)&0xfb)
def execute(mem,regs,owner,stops=(0x70000,),trace=False,delay=False):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner);events=[];ignored=None;delayed=False
 for _ in range(30000000):
  pc=machine.cpu.r_pc()
  if pc in stops:break
  coverage.add(pc)
  if pc==ignored:ignored=None
  dr=tuple(machine.cpu.r_reg(i)for i in range(15))if trace and ignored is None else None
  if trace and ignored is None and pc in EXTERNAL:
   ret=machine.mem.r32(machine.cpu.r_sp());call=ret-6 if rom[ret-6:ret-4]==bytes.fromhex('4eb9')else ret-4
   events.append((0,pc,call,ret,0,0,dr,digest(snapshot(mem))));ignored=ret
  if trace and ignored is None and pc==0x7bfe:events.append((2,pc,pc,0x7c06,0x140011,0x7b,dr,digest(snapshot(mem))))
  if trace and ignored is None and pc in WAITS and not(machine.mem.r8(0x1c006f)&2):
   events.append((3,pc,pc,WAITS[pc],0x1c006f,2,dr,digest(snapshot(mem))))
   if delay and not delayed:delayed=True
   else:machine.mem.w8(0x1c006f,machine.mem.r8(0x1c006f)|2)
  else:poll(pc)
  if pc==0x2052:
   machine.cpu.w_pc(machine.mem.r32(machine.cpu.r_sp()));machine.cpu.w_sp(machine.cpu.r_sp()+4)
   if machine.cpu.r_pc()==ignored:ignored=None
   continue
  machine.cpu.execute(1)
 assert machine.cpu.r_pc()in stops,('ROM runaway',hex(owner),hex(machine.cpu.r_pc()))
 for at,n in REGIONS:mem[at:at+n]=machine.mem.r_block(at,n)
 return [machine.cpu.r_reg(i)for i in range(15)],events,machine.cpu.r_pc()
def seed(mode=0,progress=0,mask=3):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 mem[0x1c0000:0x1c1cd4]=bytes(0x1cd4)
 put(mem,0x1c005c,rng.getrandbits(32),4);put(mem,0x1c0080,rng.getrandbits(32),4)
 put(mem,0x1c0161,mode,1);put(mem,0x1c0162,progress);put(mem,0x1c007c,1);put(mem,0x1c007e,0);put(mem,0x1c008c,0)
 put(mem,0x1c004b,1,1);put(mem,0x1c004e,1);put(mem,0x1c0072,1);put(mem,0x1c15fc,1)
 for i,p in enumerate(ACTORS):
  put(mem,p,0x8000 if mask&(1<<i)else 0);put(mem,p+2,(i*3)%12);put(mem,p+0x56,(i*3)%12)
  put(mem,p+6,0x240);put(mem,p+0xa,0x130);put(mem,p+0xe,0x100);put(mem,p+0x8a,0x140020+(i%4)*2,4)
  for offset in(8,0xc,0x10):put(mem,p+offset,rng.getrandbits(16))
 for i in range(4):put(mem,0x140020+i*2,0xffef)
 return mem
def fixtures():
 mem=seed();put(mem,0x1c008c,1);yield 'replay-fast',0x7b70,mem,None,False
 if args.smoke:return
 if not args.helpers_only:
  for mode,mask,progress in [(0,0,0),(0,3,0),(0,12,0),(0,15,0),(0,1,4),(0,4,9),(1,0,0),(1,1,0),(1,5,4),(1,15,9)]:
   yield f'whole-{mode}-{mask:x}-{progress}',0x7b70,seed(mode,progress,mask),None,True
  for mode,mask,progress in((2,0,0x100),(0xfe,0,0xff04),(0xff,1,0xfe09)):
   yield f'whole-byte-width-{mode:x}-{progress:x}',0x7b70,seed(mode,progress,mask),None,True
  for mode,progress,replay in itertools.product((0,1),(1,3,5,8,255),(0,1,0x8000)):
   mem=seed(mode,progress);put(mem,0x1c008c,replay);yield f'fast-{mode}-{progress}-{replay}',0x7b70,mem,None,False
 for mode,slot,ident in itertools.product((0,1),(0,1,2,3,4,5),range(12)):
  mem=seed(mode);p=ACTORS[slot];put(mem,p,0x8000);put(mem,p+2,ident);put(mem,p+0x10c+2,(ident+7)%12)
  yield f'init-{mode}-{slot}-{ident}',0x7f0e,mem,p,False
 # BYTE identity/table selection, but the $4B correction is a full WORD CMP.
 for mode,ident in itertools.product((0,1),(0x100,0x10b,0xff0b)):
  mem=seed(mode);put(mem,ACTORS[0]+2,ident);yield f'word-identity-{mode}-{ident:x}',0x7f0e,mem,ACTORS[0],False
 for mode in(0,1):
  mem=seed(mode);put(mem,ACTORS[0]+0x1c,0x7f,1);put(mem,ACTORS[0]+4,0xa5ff)
  yield f'init-preserved-latch-{mode}',0x7f0e,mem,ACTORS[0],False
 for mode,slot,char,index,timer in itertools.product((0,1),(0,1),(0x40,0x41,0x42,0x47,0x49,0x4a),range(8),(0,11,12,0xffff)):
  mem=seed(mode);p=ACTORS[slot];put(mem,p,0x8001);put(mem,p+2,char);put(mem,p+0x1c,0x80,1);put(mem,p+0x24,0x300|index);put(mem,p+0x22,timer);put(mem,p+0x1b,0xab,1)
  put(mem,p+0x2a,8);put(mem,p+0x2c,0x80);put(mem,p+0xa,0x130)
  yield f'animate-{mode}-{slot}-{char:x}-{index}-{timer:x}',0x7f0e,mem,p,False
 for mode,slot,y,active in itertools.product((0,1),(0,1,3),(0x8f,0x90,0x91),(0,0x8001)):
  mem=seed(mode);p=ACTORS[slot];put(mem,p,active);put(mem,p+2,0x41);put(mem,p+0x1c,0x80,1);put(mem,p+0xa,y);put(mem,p+0x2a,0)
  yield f'finish-{mode}-{slot}-{y:x}-{active:x}',0x7f0e,mem,p,False
 for mode,slot,phase,timer in itertools.product((0,1),(0,2),(0,1,2,0x100,0xffff),(0,1,2,3,0xffff)):
  mem=seed(mode);p=ACTORS[slot+(1 if mode==0 else 0)];put(mem,0x1c14ce+0x24,phase);put(mem,0x1c14ce+0x22,timer)
  yield f'control-{mode}-{slot}-{phase:x}-{timer:x}',0x7e86,mem,p,False
source=ROOT/'src/wf_r18_card_scene.c'
with tempfile.TemporaryDirectory(prefix='wf-card-scene-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'scene.so';src='tests/r18/card_scene_pending.c'if args.pending else'src/wf_r18_card_scene.c'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',src,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_card_scene_init.argtypes=[ctypes.POINTER(Scene),ctypes.c_uint32];native.wf_r18_card_scene_step.argtypes=[ctypes.POINTER(Scene),ctypes.POINTER(Context)];native.wf_r18_card_scene_complete.argtypes=[ctypes.POINTER(Scene),ctypes.POINTER(Context),ctypes.c_int]
 for name,owner,mem,a0,delay in (()if args.controls_only else fixtures()):
  regs=[rng.getrandbits(32)for _ in range(15)]
  if a0 is not None:regs[8]=a0
  want=bytearray(mem);stops={0x7b70:(0x7e14,0x7e84),0x7f0e:(0x8134,),0x7e86:(0x7f0c,)}[owner]
  wr,events,stop=execute(want,regs,owner,stops,True,delay)
  buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),Effect(),None,0)
  state=Scene();native.wf_r18_card_scene_init(ctypes.byref(state),owner);at=0;delayed=False
  for _ in range(200000):
   status=native.wf_r18_card_scene_step(ctypes.byref(state),ctypes.byref(c));assert status>=0,(name,'blocked',hex(state.pc),hex(state.blocked_dependency))
   if status==1:
    q=state.request;cur=bytearray(buf.raw);dr=list(sr.d)+list(c.a);event=tuple(getattr(q,x)for x in('kind','owner','call_pc','return_pc','address','value'))+(tuple(dr),digest(cur))
    assert at<len(events)and event==events[at],(name,'request',at,event,events[at]if at<len(events)else None);at+=1
    before=bytes(buf);before_regs=dr[:];assert native.wf_r18_card_scene_step(ctypes.byref(state),ctypes.byref(c))==1
    assert bytes(buf)==before and list(sr.d)+list(c.a)==before_regs
    if q.kind==0:
     if q.owner!=0x2052:dr,_,_=execute(cur,dr,q.owner)
    elif q.kind==2:put(cur,q.address,q.value,1)
    else:
     if delay and not delayed:delayed=True
     else:put(cur,q.address,get(cur,q.address,1)|q.value,1)
    for base,n in REGIONS:ctypes.memmove(ctypes.addressof(buf)+base,bytes(cur[base:base+n]),n)
    for i,v in enumerate(dr[:8]):sr.d[i]=v
    for i,v in enumerate(dr[8:]):c.a[i]=v
    rc=native.wf_r18_card_scene_complete(ctypes.byref(state),ctypes.byref(c),0)
    assert rc==(5 if q.kind==3 and not(get(cur,q.address,1)&2)else 0),(name,'complete',rc)
    wait_cases+=rc==5
   elif status==3:break
  assert state.pc==stop and status==3,(name,'terminal',hex(state.pc),status)
  assert at==len(events),(name,'missing requests',at,len(events))
  for base,n in REGIONS:
   got=buf.raw[base:base+n];expected=want[base:base+n]
   assert got==expected,(name,'memory',[(hex(base+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:16])
  assert list(sr.d)+list(c.a)==wr,(name,'D/A',list(sr.d)+list(c.a),wr)
  count+=1
 controls=0
 if not args.pending:
  mem=seed();buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1);c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0);s=Scene()
  assert native.wf_r18_card_scene_step(None,ctypes.byref(c))==-2;controls+=1
  native.wf_r18_card_scene_init(ctypes.byref(s),0x7b70);assert native.wf_r18_card_scene_step(ctypes.byref(s),None)==-2;controls+=1
  assert native.wf_r18_card_scene_complete(ctypes.byref(s),ctypes.byref(c),0)==-2;controls+=1
  native.wf_r18_card_scene_init(ctypes.byref(s),0x7bfe);assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==1
  c.dependency=0xdead;assert native.wf_r18_card_scene_complete(ctypes.byref(s),ctypes.byref(c),1)==-1 and s.blocked_dependency==0xdead;controls+=1
  before=bytes(buf);dr=list(sr.d)+list(c.a);assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==-1 and bytes(buf)==before and list(sr.d)+list(c.a)==dr;controls+=1
  assert native.wf_r18_card_scene_complete(ctypes.byref(s),ctypes.byref(c),0)==-2;controls+=1
  for pc in(0x7e14,0x7e84,0x8134,0x7f0c):
   native.wf_r18_card_scene_init(ctypes.byref(s),pc);assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==3;controls+=1
  native.wf_r18_card_scene_init(ctypes.byref(s),0xdead);assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0xdead;controls+=1
  c.a[0]=0x1c05b1;native.wf_r18_card_scene_init(ctypes.byref(s),0x7f0e)
  assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0x7f0e;controls+=1
  c.a[0]=ACTORS[0];ctypes.memmove(ctypes.addressof(buf)+ACTORS[0]+2,b'\x00\x0c',2);native.wf_r18_card_scene_init(ctypes.byref(s),0x7fd0)
  assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0x813e;controls+=1
  ctypes.memmove(ctypes.addressof(buf)+ACTORS[0]+0x24,b'\x00\x08',2);native.wf_r18_card_scene_init(ctypes.byref(s),0x80aa)
  assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0x8136;controls+=1
  for pc,ret in WAITS.items():
   ctypes.memmove(ctypes.addressof(buf)+0x1c006f,b'\x00',1);native.wf_r18_card_scene_init(ctypes.byref(s),pc)
   assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==1;controls+=1
   assert native.wf_r18_card_scene_complete(ctypes.byref(s),ctypes.byref(c),0)==5 and s.pc==pc and not s.pending;controls+=1
   assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==1;ctypes.memmove(ctypes.addressof(buf)+0x1c006f,b'\x02',1)
   assert native.wf_r18_card_scene_complete(ctypes.byref(s),ctypes.byref(c),0)==0 and s.pc==ret;controls+=1
   native.wf_r18_card_scene_init(ctypes.byref(s),pc)
   assert native.wf_r18_card_scene_step(ctypes.byref(s),ctypes.byref(c))==0 and s.pc==ret;controls+=1
 required={0x7f4c,0x7f9a,0x7fc6,0x8010,0x8098,0x809e,0x80d2,0x8122,0x812e,0x7ef0}
 if not args.helpers_only:required|={0x7bfe,0x7c9a,0x7db8,0x7e20,*WAITS}
 if not(args.smoke or args.controls_only or args.pending):
  assert required<=coverage,('missing branch coverage',[hex(x)for x in sorted(required-coverage)])
  if not args.helpers_only:assert wait_cases>0,'missing delayed ACK'
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guard_controls':controls,'delayed_ack_waits':wait_cases,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'owners':['7B70','7F0E','7E86'],'compared':['full16KiBRAM/BG64KiB/FG16KiB/palette36KiB including padding/board regions','D0-D7/A0-A6/RNG','all CALL/WRITE8/SYNC entry contexts/order/region digests and original call/return PCs','pending REQUEST inertness','original RTS boundaries'],'delegated_intact_rom':[hex(x)for x in sorted(EXTERNAL-{0x2052})],'intercepts':['2052 sound output only'],'limitations':['External owners remain explicit composition dependencies','CCR/A7 temporary stack outside ABI','Real IRQ mutations/controller input/hardware timing not proven by deterministic ACK/VBlank test feed']}
 report['required_rom_coverage']=[hex(x)for x in sorted(required&coverage)]
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
