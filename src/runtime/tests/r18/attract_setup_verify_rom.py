#!/usr/bin/env python3
"""Original B1C/E02 and physical21E6 delay, all helpers execute intact ROM."""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser();ap.add_argument('--report',type=Path);ap.add_argument('--pending',action='store_true');ap.add_argument('--smoke',action='store_true');ap.add_argument('--controls-only',action='store_true');args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
REGIONS=((0x1c0000,0x4000),(0x80000,0x10000),(0xc0000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40));ACTORS=[0x1c05b0+0x10c*i for i in range(10)]
EXTERNAL={0xae20,0x264e2,0x69c8,0x8490,0xc98,0x8fd6,0x1f9e,0x2a06,0x2aea,0x2988,0x1086c}
rng=random.Random(0xb1ce02);coverage=set();count=0;waits=0
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):pass
Effect=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(Context))
Context._fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',Effect),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
class Request(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('kind','owner','call_pc','return_pc','address','value')]
class State(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('pc','entry_pc','saved_d0','sub_return','blocked_dependency')]+[('request',Request),('pending',ctypes.c_uint8),('fatal',ctypes.c_uint8)]
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
  v=machine.mem.r8(0x140026);machine.mem.w8(0x140026,(v|4)if rom[pc+8]==0x67 else(v&0xfb))
 if pc in(0x298c,0x2a0a):machine.mem.w8(0x140026,machine.mem.r8(0x140026)|4)
 elif pc in(0x29f6,0x2a88):machine.mem.w8(0x140026,machine.mem.r8(0x140026)&0xfb)
def execute(mem,regs,owner,stop=0x70000,trace=False,delay=False):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner);events=[];ignored=None;delayed=set()
 for _ in range(30000000):
  pc=machine.cpu.r_pc()
  if pc==stop:break
  coverage.add(pc)
  if pc==ignored:ignored=None
  if trace and ignored is None and pc in EXTERNAL:
   dr=tuple(machine.cpu.r_reg(i)for i in range(15));ret=machine.mem.r32(machine.cpu.r_sp());call=ret-6 if rom[ret-6:ret-4]==bytes.fromhex('4eb9')else ret-4
   events.append(((0,pc,call,ret,0,0,dr,digest(snapshot(mem))),None));ignored=ret
  if trace and ignored is None and pc in(0x21ea,0x21f4):
   word=0xa155|(0x400 if pc==0x21ea else 0)
   if pc==0x21f4:word&=0xfbff
   if delay and pc not in delayed:word^=0x400;delayed.add(pc)
   dr=tuple(machine.cpu.r_reg(i)for i in range(15));events.append(((1,pc,pc,0x21f4 if pc==0x21ea else 0x21fe,0x140026,0,dr,digest(snapshot(mem))),word));machine.mem.w16(0x140026,word)
  else:poll(pc)
  if pc==0x2052:
   machine.cpu.w_pc(machine.mem.r32(machine.cpu.r_sp()));machine.cpu.w_sp(machine.cpu.r_sp()+4);continue
  machine.cpu.execute(1)
 assert machine.cpu.r_pc()==stop,('ROM runaway',hex(owner),hex(machine.cpu.r_pc()))
 for at,n in REGIONS:mem[at:at+n]=machine.mem.r_block(at,n)
 return [machine.cpu.r_reg(i)for i in range(15)],events
def seed(selector=0,mode=0):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 mem[0x1c0000:0x1c1cd4]=bytes(0x1cd4);mem[0x1c00c2:0x1c0146]=rom[0x6db0:0x6e34]
 put(mem,0x1c005c,rng.getrandbits(32),4);put(mem,0x1c0080,rng.getrandbits(32),4);put(mem,0x1c0166,selector);put(mem,0x1c0161,mode,1);put(mem,0x1c0072,1);put(mem,0x1c15fc,1)
 put(mem,0x1c004b,1,1);put(mem,0x1c004d,1,1)
 for i,p in enumerate(ACTORS):
  put(mem,p+2,i%12);put(mem,p+0x56,i%12);put(mem,p+0x8a,0x140020+(i%4)*2,4)
  for off in(8,0xc,0x10):put(mem,p+off,rng.getrandbits(16))
 for i in range(4):put(mem,0x140020+i*2,0xffff)
 return mem
def fixtures():
 yield 'selector0-entry',0xb1c,seed(),None,False
 if args.smoke:return
 for selector in range(1,14):yield f'B1C-{selector}',0xb1c,seed(selector,0xfe),None,True
 for selector,high in itertools.product(range(14),(0,0x1200)):
  mem=seed(selector)
  for p in ACTORS:put(mem,p+2,get(mem,p+2)|high)
  yield f'E02-{selector}-{high:x}',0xe02,mem,None,False
 for selector in(0x101,0xff01):yield f'E02-BYTE-Royal-{selector:x}',0xe02,seed(selector),None,False
 for index in range(64):yield f'E02-random-{index}',0xe02,seed(rng.randrange(14),rng.getrandbits(8)),None,False
 for counter in(0,1,2,3,7,0x20,0x100):yield f'delay-{counter}',0x21e6,seed(),counter,True
source=ROOT/'src/wf_r18_attract_setup.c'
with tempfile.TemporaryDirectory(prefix='wf-attract-setup-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'attract.so';src='tests/r18/attract_setup_pending.c'if args.pending else'src/wf_r18_attract_setup.c'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',src,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_attract_setup_init.argtypes=[ctypes.POINTER(State),ctypes.c_uint32];native.wf_r18_attract_setup_step.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context)];native.wf_r18_attract_setup_complete.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context),ctypes.c_int,ctypes.c_uint16]
 for name,owner,mem,d0,delay in(()if args.controls_only else fixtures()):
  regs=[rng.getrandbits(32)for _ in range(15)]
  if d0 is not None:regs[0]=(regs[0]&0xffff0000)|d0
  stop={0xb1c:0xbd0,0xe02:0xf20,0x21e6:0x2206}[owner];want=bytearray(mem);wr,events=execute(want,regs,owner,stop,True,delay)
  buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),Effect(),None,0);s=State();native.wf_r18_attract_setup_init(ctypes.byref(s),owner);at=0
  for _ in range(100000):
   status=native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c));assert status>=0,(name,'blocked',hex(s.pc),hex(s.blocked_dependency))
   if status==1:
    q=s.request;cur=bytearray(buf.raw);dr=list(sr.d)+list(c.a);event=tuple(getattr(q,x)for x in('kind','owner','call_pc','return_pc','address','value'))+(tuple(dr),digest(cur))
    assert at<len(events)and event==events[at][0],(name,'request',at,event,events[at]if at<len(events)else None)
    word=events[at][1];at+=1;before=bytes(buf);before_regs=dr[:];assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==1 and bytes(buf)==before and list(sr.d)+list(c.a)==before_regs
    if q.kind==0:dr,_=execute(cur,dr,q.owner)
    else:put(cur,0x140026,word)
    for base,n in REGIONS:ctypes.memmove(ctypes.addressof(buf)+base,bytes(cur[base:base+n]),n)
    for i,v in enumerate(dr[:8]):sr.d[i]=v
    for i,v in enumerate(dr[8:]):c.a[i]=v
    rc=native.wf_r18_attract_setup_complete(ctypes.byref(s),ctypes.byref(c),0,word or 0)
    blocked_level=q.kind==1 and(bool(word&0x400)!=(q.owner==0x21ea))
    assert rc==(5 if blocked_level else 0),(name,'complete',rc);waits+=rc==5
   elif status==3:break
  assert status==3 and s.pc==stop and at==len(events),(name,'terminal',hex(s.pc),status,at,len(events))
  for base,n in REGIONS:
   got=buf.raw[base:base+n];expected=want[base:base+n];assert got==expected,(name,'memory',[(hex(base+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:16])
  assert list(sr.d)+list(c.a)==wr,(name,'D/A',list(sr.d)+list(c.a),wr);count+=1
 controls=0
 if not args.pending:
  if args.controls_only:
   mem=seed();buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1);c=Context(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*range(7)),Effect(),None,0);s=State()
  assert native.wf_r18_attract_setup_step(None,ctypes.byref(c))==-2;controls+=1
  native.wf_r18_attract_setup_init(ctypes.byref(s),0xb1c);assert native.wf_r18_attract_setup_step(ctypes.byref(s),None)==-2;controls+=1
  assert native.wf_r18_attract_setup_complete(ctypes.byref(s),ctypes.byref(c),0,0)==-2;controls+=1
  for pc in(0xb40,0xb7e):
   native.wf_r18_attract_setup_init(ctypes.byref(s),pc);assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==1;c.dependency=0xdead
   assert native.wf_r18_attract_setup_complete(ctypes.byref(s),ctypes.byref(c),1,0)==-1 and s.blocked_dependency==0xdead;controls+=1
   before=bytes(buf);dr=list(sr.d)+list(c.a);assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==-1 and bytes(buf)==before and list(sr.d)+list(c.a)==dr;controls+=1
  for pc in(0xbd0,0xf20,0x2206):
   native.wf_r18_attract_setup_init(ctypes.byref(s),pc);assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==3;controls+=1
  native.wf_r18_attract_setup_init(ctypes.byref(s),0xdead);assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0xdead;controls+=1
  ctypes.memmove(ctypes.addressof(buf)+0x1c0166,b'\x00\x0e',2);native.wf_r18_attract_setup_init(ctypes.byref(s),0xe02)
  assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==0
  assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0xf3e;controls+=1
  native.wf_r18_attract_setup_init(ctypes.byref(s),0x21ea);assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==1
  assert native.wf_r18_attract_setup_complete(ctypes.byref(s),ctypes.byref(c),2,0)==-1 and s.blocked_dependency==0x21ea;controls+=1
  before=bytes(buf);dr=list(sr.d)+list(c.a);assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==-1 and bytes(buf)==before and list(sr.d)+list(c.a)==dr;controls+=1
  for counter in(0,1,0x7fff,0x8000,0xffff):
   sr.d[0]=0xabcd0000|counter;cur=bytearray(buf.raw);dr=list(sr.d)+list(c.a)
   wr,_=execute(cur,dr,0x21fe,0x2202 if counter==0 else 0x21ea)
   native.wf_r18_attract_setup_init(ctypes.byref(s),0x21fe)
   assert native.wf_r18_attract_setup_step(ctypes.byref(s),ctypes.byref(c))==0
   assert s.pc==machine.cpu.r_pc() and list(sr.d)+list(c.a)==wr and bytes(buf)==before;controls+=1
 if not(args.smoke or args.controls_only or args.pending):assert{0xb34,0xb38,0xb7e,0xe38,0xecc,0xf1a,0xec2,0x21ea,0x21f4,0x21fe}<=coverage
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guard_controls':controls,'wrong_level_waits':waits,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'owners':['B1C..BD0','E02..F20','21E6..2206'],'compared':['Full RAM16KiB/BG64KiB/FG16KiB/palette36KiB/board padding','D0-D7/A0-A6/RNG','All CALL/READ16 contexts/digests/order and original call/return PCs','Inert pending requests, original RTS and physical poll boundaries'],'intercepts':['2052 sound output only'],'delegated_intact_rom':[hex(x)for x in sorted(EXTERNAL)],'limits':['Special ranking69C8 and visual8490 remain explicit native dependencies','CCR/A7 temporary stack outside ABI','No attract gameplay loop, IRQ cadence or hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
