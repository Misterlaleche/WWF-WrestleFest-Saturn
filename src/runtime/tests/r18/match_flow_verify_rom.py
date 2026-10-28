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
EXTERNAL={0x2988,0x8eb6,0xae20,0x264e2,0x7b70,0xc98,0xa654,0x2a06,0x2052,0x1e92,0x782c,0x7506,0xc1ea,0xb1c}
def poll(pc):
 if rom[pc:pc+8]==bytes.fromhex('08390001001c006f'):machine.mem.w8(0x1c006f,machine.mem.r8(0x1c006f)|2)
 if rom[pc:pc+8]==bytes.fromhex('0839000200140026'):
  assert rom[pc+8]in(0x66,0x67)
  value=machine.mem.r8(0x140026)
  machine.mem.w8(0x140026,(value|4)if rom[pc+8]==0x67 else(value&0xfb))

 if pc in(0x298c,0x2a0a):machine.mem.w8(0x140026,machine.mem.r8(0x140026)|4)
 elif pc in(0x29f6,0x2a88):machine.mem.w8(0x140026,machine.mem.r8(0x140026)&0xfb)
def execute(mem,regs,owner,stop=0x70000):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner);calls=[];ignored_return=None
 for _ in range(20000000):
  pc=machine.cpu.r_pc()
  if pc==stop:break
  poll(pc)
  if pc==ignored_return:ignored_return=None
  coverage.add(pc)
  if pc in(0x10af0,0x10b26):natural_fallbacks.add(pc)
  if pc in EXTERNAL and ignored_return is None:
   current=bytearray(mem)
   for at,n in REGIONS:current[at:at+n]=machine.mem.r_block(at,n)
   ret=machine.mem.r32(machine.cpu.r_sp());calls.append((pc,tuple(machine.cpu.r_reg(i)for i in range(15)),digest(current),ret));ignored_return=ret
  if pc==0x2052:
   machine.cpu.w_pc(machine.mem.r32(machine.cpu.r_sp()));machine.cpu.w_sp(machine.cpu.r_sp()+4)
   if machine.cpu.r_pc()==ignored_return:ignored_return=None
   continue
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
class Request(ctypes.Structure):_fields_=[('kind',ctypes.c_uint32),('owner',ctypes.c_uint32),('call_pc',ctypes.c_uint32),('return_pc',ctypes.c_uint32)]
class Flow(ctypes.Structure):_fields_=[('pc',ctypes.c_uint32),('entry_pc',ctypes.c_uint32),('saved_d0',ctypes.c_uint32),('blocked_dependency',ctypes.c_uint32),('request',Request),('pending',ctypes.c_uint8),('fatal',ctypes.c_uint8)]
def seed(mode=0,progress=1,replay=1,config=0,mask=1):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 put(mem,0x1c0161,mode,1);put(mem,0x1c0162,progress);put(mem,0x1c007c,1);put(mem,0x1c007e,0);put(mem,0x1c008c,replay);put(mem,0x1c0066,config)
 put(mem,0x1c004b,1,1);put(mem,0x1c004e,0x0001);put(mem,0x1c0072,0x0001)
 put(mem,0x1c15f4,0);put(mem,0x1c15f8,0);put(mem,0x1c15fc,1)
 for i,actor in enumerate(ACTORS):
  put(mem,actor,0x8000 if mask&(1<<i)else 0);put(mem,actor+2,i%12);put(mem,actor+0x56,i%12);put(mem,actor+0xa0,1);put(mem,actor+0x68,0)
  put(mem,actor+0x8a,0x140020+(i%4)*2,4);put(mem,actor+0x86,ACTORS[i^1],4)
  for off in(0x66,0x6e,0x72):put(mem,actor+off,110)
  for off in(0x6a,0x6c,0x70):put(mem,actor+off,0)
 for i in range(10):put(mem,0x1c0598+i*2,0xffff)
 for i in range(12):put(mem,0x1c0380+i*2,i)
 for i in range(4):put(mem,0x140020+i*2,0xffef)
 return mem
def fixtures():
 yield 'common-C6C',0xc6c,seed()
 if args.smoke:return
 for mode,progress,credit in itertools.product((0,1),range(10),(0,1,0x100,0xffff)):
  mem=seed(mode,progress);put(mem,0x1c007c,credit)
  yield f'return-{mode}-{progress}-{credit:x}',0xc6c,mem
 for mode,mask,config in itertools.product((0,1),(0,1,4,5,0xf),(0,0x800,0xffff)):
  if mode==1 and mask==15:continue
  mem=seed(mode,1,1,config,mask)
  yield f'finalize-{mode}-{mask:x}-{config:x}',0x8ae6,mem
 # Full paid body. Replay makes the existing 7B70 owner return immediately;
 # the mandatory CALL is still executed and its complete ABI is compared.
 for mode,progress,mask,config in itertools.product((0,1),(0,1,4,9),(1,4,5),(0,0x800)):
  yield f'paid-{mode}-{progress}-{mask:x}-{config:x}',0xac0,seed(mode,progress,1,config,mask)
source=ROOT/'src/wf_r18_match_flow.c'
with tempfile.TemporaryDirectory(prefix='wf-match-flow-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'flow.so';src='tests/r18/match_flow_pending.c'if args.pending else'src/wf_r18_match_flow.c'
 sources=[src]+([]if args.pending else['src/wf_r18_match_setup.c','src/wf_r18_royal_setup.c'])
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_match_flow_init.argtypes=[ctypes.POINTER(Flow),ctypes.c_uint32];native.wf_r18_match_flow_step.argtypes=[ctypes.POINTER(Flow),ctypes.POINTER(Context)];native.wf_r18_match_flow_complete.argtypes=[ctypes.POINTER(Flow),ctypes.POINTER(Context),ctypes.c_int,ctypes.c_uint32]
 for name,owner,mem in (()if args.controls_only else fixtures()):
  regs=[rng.getrandbits(32)for _ in range(15)];want=bytearray(mem);stop={0xc6c:0xc96,0x8ae6:0x8b38,0xac0:0xf8c}[owner];wr,calls=execute(want,regs,owner,stop)
  native_mem=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(native_mem)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),effect,None,0)
  state=Flow();native.wf_r18_match_flow_init(ctypes.byref(state),owner);at=0
  for _ in range(10000):
   status=native.wf_r18_match_flow_step(ctypes.byref(state),ctypes.byref(c))
   assert status>=0,(name,'blocked',hex(state.pc),hex(state.blocked_dependency))
   if status==1:
    q=state.request;current=bytearray(native_mem.raw);dr=list(sr.d)+list(c.a)
    got=(q.owner,tuple(dr),digest(current));assert at<len(calls)and got==calls[at][:3],(name,'request context',at,got,calls[at]if at<len(calls)else None)
    ret=calls[at][3];assert q.return_pc==ret,(name,'return PC',hex(q.return_pc),hex(ret))
    callpc=ret-6 if rom[ret-6:ret-4]==bytes.fromhex('4eb9')else ret-4 if rom[ret-4:ret-2]==bytes.fromhex('6100')else ret-2
    assert q.call_pc==callpc,(name,'call PC',hex(q.call_pc),hex(callpc))
    assert q.kind==0;at+=1
    # A pending request cannot replay any prefix.
    before=bytes(native_mem);before_regs=list(sr.d)+list(c.a)
    assert native.wf_r18_match_flow_step(ctypes.byref(state),ctypes.byref(c))==1
    assert bytes(native_mem)==before and list(sr.d)+list(c.a)==before_regs
    if q.owner!=0x2052:dr,_=execute(current,dr,q.owner)
    for base,n in REGIONS:ctypes.memmove(ctypes.addressof(native_mem)+base,bytes(current[base:base+n]),n)
    for i,v in enumerate(dr[:8]):sr.d[i]=v
    for i,v in enumerate(dr[8:]):c.a[i]=v
    assert native.wf_r18_match_flow_complete(ctypes.byref(state),ctypes.byref(c),0,0)==0
   elif status in(2,3):break
  assert state.pc==stop and status==(2 if owner==0xac0 else 3),(name,'terminal',hex(state.pc),status)
  assert at==len(calls),(name,'missing requests',at,len(calls))
  for base,n in REGIONS:
   got=native_mem.raw[base:base+n];expected=want[base:base+n]
   assert got==expected,(name,'memory',[(hex(base+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:12])
  assert list(sr.d)+list(c.a)==wr,(name,'D/A',list(sr.d)+list(c.a),wr)
  count+=1
 controls=0
 if not args.pending:
  def context():
   mem=seed();buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1)
   c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0);return c,buf,sr
  c,buf,sr=context();s=Flow();native.wf_r18_match_flow_init(ctypes.byref(s),0xc6c)
  assert native.wf_r18_match_flow_step(None,ctypes.byref(c))==-2;controls+=1
  assert native.wf_r18_match_flow_step(ctypes.byref(s),None)==-2;controls+=1
  assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),0,0)==-2;controls+=1
  for pc in(0x6fc,0x57e0):
   c,buf,sr=context();before=bytes(buf);dr=list(sr.d)+list(c.a);native.wf_r18_match_flow_init(ctypes.byref(s),pc)
   assert native.wf_r18_match_flow_step(ctypes.byref(s),ctypes.byref(c))==1
   assert(s.request.kind,s.request.owner,s.request.call_pc,s.request.return_pc)==(1,pc,pc,0);controls+=1
   assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),0,0)==-2 and s.pending;controls+=1
   assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),0,0xac0)==4 and s.pc==0xac0 and not s.pending;controls+=1
   assert bytes(buf)==before and list(sr.d)+list(c.a)==dr
  c,buf,sr=context();native.wf_r18_match_flow_init(ctypes.byref(s),0xc74)
  assert native.wf_r18_match_flow_step(ctypes.byref(s),ctypes.byref(c))==1
  assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),0,0xdead)==-2 and s.pending;controls+=1
  assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),0,0)==0 and s.pc==0xc7a;controls+=1
  c,buf,sr=context();native.wf_r18_match_flow_init(ctypes.byref(s),0xc02)
  assert native.wf_r18_match_flow_step(ctypes.byref(s),ctypes.byref(c))==1;c.dependency=0x7f0e
  assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),1,0)==-1 and s.fatal and s.blocked_dependency==0x7f0e;controls+=1
  before=bytes(buf);dr=list(sr.d)+list(c.a)
  assert native.wf_r18_match_flow_step(ctypes.byref(s),ctypes.byref(c))==-1 and bytes(buf)==before and list(sr.d)+list(c.a)==dr;controls+=1
  assert native.wf_r18_match_flow_complete(ctypes.byref(s),ctypes.byref(c),0,0)==-2;controls+=1
  for pc,result in((0xf8c,2),(0xc96,3),(0x8b38,3)):
   c,buf,sr=context();native.wf_r18_match_flow_init(ctypes.byref(s),pc);before=bytes(buf);dr=list(sr.d)+list(c.a)
   assert native.wf_r18_match_flow_step(ctypes.byref(s),ctypes.byref(c))==result and bytes(buf)==before and list(sr.d)+list(c.a)==dr;controls+=1
  native.wf_r18_match_flow_init(ctypes.byref(s),0xdead)
  assert native.wf_r18_match_flow_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0xdead;controls+=1
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guard_controls':controls,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'scope':'Pure paidAC0/BD2..C96 + C6C/8AE6 + exact F8C handoff','compared':['all raw RAM/BG/FG/palette padding/board regions','D0-D7/A0-A6/RNG','CALL request entry contexts/order/digests and call/return PCs','inert repeated REQUEST','original return/gameplay PC'],'delegated_intact_rom':[hex(x)for x in sorted(EXTERNAL-{0x2052})],'intercepts':['2052 sound output only'],'limitations':['External owners are explicit composition dependencies, not claimed native by this controller','Paid fixtures use replay for7B70 fast return; full7B70 has its own separate owner proof','6FC/57E0 ENTRY gates are caller-owned boundaries']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
