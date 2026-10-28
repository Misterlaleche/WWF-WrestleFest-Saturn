#!/usr/bin/env python3
"""Post TAG raw controller, original ROM helpers and explicit hardware tape."""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser();ap.add_argument('--report',type=Path);ap.add_argument('--pending',action='store_true');ap.add_argument('--smoke',action='store_true');ap.add_argument('--milestone-only',action='store_true');ap.add_argument('--continue-only',action='store_true');args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
REGIONS=((0x1c0000,0x4000),(0x80000,0x10000),(0xc0000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40))
EXTERNAL={0x21e6,0x2503c,0x8eb6,0x65c0,0xb608,0xbda6,0x206fe,0x2067c,0x1f6c,0x1fc0,0x2052,0x26e66,0x2a06,0x1fde,0x1f9e,0x15f4,0x139c,0xae20,0x2aea,0x27b8,0x2836,0x26772,0x26844,0x264e2,0x9aca,0x2b58,0x2988}
WAITS={0xbe7a:0xbe84,0xbf34:0xbf3e,0xb75c:0xb766,0xb7f0:0xb7fa,0x150e:0x1518,0x1742:0x174c,0x1864:0x186e}
WRITES={0xae64:(0x140010,0xae6c,2),0xbea4:(None,0xbea8,2),0xb654:(0x140010,0xb65c,2),0xb8e8:(0x140010,0xb8f0,2)}
READS={0x16d4:(1,0x16d8,None),0x16e2:(1,0x16e6,None),0x17be:(4,0x17c6,0x140021),0x17c8:(4,0x17d0,0x140023),0x17d6:(4,0x17de,0x140021),0x17e0:(4,0x17e8,0x140023),0x1804:(4,0x180c,0x140021),0x1812:(4,0x181a,0x140023)}
rng=random.Random(0x11b6122c);coverage=set();count=0;waits=0
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Machine(ctypes.Structure):_fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',ctypes.c_void_p),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
Rom=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t)
class Context(ctypes.Structure):_fields_=[('machine',ctypes.POINTER(Machine)),('read_rom',Rom),('user',ctypes.c_void_p)]
class Request(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('kind','owner','call_pc','return_pc','address','value')]
class State(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('pc','entry_pc','saved_d0','sub_return','blocked_dependency')]+[('saved_d',ctypes.c_uint32*8),('saved_a',ctypes.c_uint32*4),('request',Request),('pending',ctypes.c_uint8),('fatal',ctypes.c_uint8),('carry',ctypes.c_uint8),('callback_a0',ctypes.c_uint32)]
assert ctypes.sizeof(State)==100
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
def execute(mem,regs,owner,stops=(0x70000,),trace=False,delay=False):
 for at,n in REGIONS:machine.mem.w_block(at,bytes(mem[at:at+n]))
 for i,v in enumerate(regs):machine.cpu.w_reg(i,v)
 machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(owner);events=[];ignored=None;delayed=False
 for _ in range(20000000):
  pc=machine.cpu.r_pc()
  if pc in stops:break
  coverage.add(pc)
  if pc==ignored:ignored=None
  dr=tuple(machine.cpu.r_reg(i)for i in range(15))if trace and ignored is None else None
  if dr is not None and pc in EXTERNAL and pc!=owner:
   ret=machine.mem.r32(machine.cpu.r_sp());call=ret-6 if rom[ret-6:ret-4]==bytes.fromhex('4eb9')else ret-4
   events.append((0,pc,call,ret,0,0,dr,digest(snapshot(mem))));ignored=ret
  if dr is not None and pc in WRITES:
   at,ret,width=WRITES[pc];at=at if at is not None else machine.cpu.r_reg(8)+(machine.cpu.r_reg(1)&0xffff)
   value=0x78 if pc==0xb8e8 else 0x7b if pc in(0xae64,0xb654)else machine.cpu.r_reg(2)&0xffff
   events.append((2,pc,pc,ret,at,value,dr,digest(snapshot(mem))))
  if dr is not None and pc in READS:
   kind,ret,at=READS[pc];at=at if at is not None else machine.cpu.r_reg(12)
   events.append((kind,pc,pc,ret,at,0,dr,digest(snapshot(mem))))
  if dr is not None and pc in WAITS and not(machine.mem.r8(0x1c006f)&2):
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
 return[machine.cpu.r_reg(i)for i in range(15)],events,machine.cpu.r_pc()
@Rom
def read_rom(_,at,dest,n):
 if at+n>len(rom):return 1
 ctypes.memmove(dest,rom[at:at+n],n);return 0
def seed(progress=0,mask=1):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,n in REGIONS:mem[at:at+n]=rng.randbytes(n)
 mem[0x1c0000:0x1c1cd4]=bytes(0x1cd4)
 put(mem,0x1c005c,rng.getrandbits(32),4);put(mem,0x1c0080,rng.getrandbits(32),4)
 put(mem,0x1c0162,progress);put(mem,0x1c007c,1);put(mem,0x1c0072,1);put(mem,0x1c004b,1,1);put(mem,0x1c004f,3,1)
 for i in range(9):
  p=0x1c05b0+0x10c*i;put(mem,p,0x8000 if mask&(1<<i)else 0);put(mem,p+2,i%12);put(mem,p+0x56,i%12);put(mem,p+0xa0,0x4001 if mask&(1<<i)else 0)
 for i in range(4):put(mem,0x140020+i*2,0xffff)
 return mem
def fixtures():
 if args.continue_only:
  for owner,arg,config,rem,total,cycle in itertools.product((0x55a,0xc404),(0,1,2,3,4,5,0x100,0x101,0xff03,0xffff),(0,1,3,0x10,0x18),(0,1,2,255),(0,1,2,255),(1,3)):
   mem=seed();put(mem,0x1c0066,config);put(mem,0x1c004b,cycle,1);put(mem,0x1c004e,rem,1);put(mem,0x1c004f,total,1)
   yield f'credit-owner-{owner:x}-{arg:x}-{config:x}-{rem}-{total}-{cycle}',owner,mem,arg,(0x652 if owner==0x55a else 0xc496,),False
  for team,flags,mask,credit_count in((0,0,1,0),(0,0,3,2),(2,0,2,3),(0,0x800,1,1),(2,0x800,3,2),(0,0x800,3,2)):
   mem=seed(0,15);put(mem,0x1c0066,flags);put(mem,0x1c0160,0x80 if flags and mask==3 else 0,1);put(mem,0x1c004f,credit_count,1);put(mem,0x1c0168,mask);put(mem,0x1c016a,mask)
   if credit_count:put(mem,0x140021 if team==0 else 0x140023,0x7f,1)
   yield f'continue139C-{team}-{flags}-{mask}-{credit_count}',0x139c,mem,team,(0x15cc,0x15f2),True
  for team,flags,mode,mask,credits,pressed in itertools.product((0,2),(0,0x800),(0,0x80),(1,2,3),(0,1,2),(0,1)):
   mem=seed(0,15);put(mem,0x1c0066,flags);put(mem,0x1c0160,mode,1);put(mem,0x1c004f,credits,1);put(mem,0x1c0168,mask);put(mem,0x1c016a,mask)
   if pressed:put(mem,0x140020,0x7f7f);put(mem,0x140022,0x7f7f);put(mem,0x140024,0x7f7f);put(mem,0x140026,0x7f7f)
   yield f'credit-check160E-{team}-{flags}-{mode}-{mask}-{credits}-{pressed}',0x160e,mem,team,(0x175a,0x187c,0x1882),False
  for team,mode,port0,port1 in itertools.product((0,2),(0,0x80),(0xff7f,0x7fff,0xffbf,0xffff),(0xff7f,0x7fff,0xffbf,0xffff)):
   mem=seed(0,15);put(mem,0x1c0066,0x800);put(mem,0x1c0160,mode,1);put(mem,0x1c004f,3,1);put(mem,0x1c0168,3);put(mem,0x1c016a,3);put(mem,0x140020,port0);put(mem,0x140022,port1)
   yield f'READ8-asymmetric-{team}-{mode}-{port0:x}-{port1:x}',0x160e,mem,team,(0x187c,0x1882),False
  return
 if args.milestone_only:
  for mask,ident in((3,0),(1,11),(5,9),(15,4)):
   mem=seed(4,mask)
   for i in range(4):put(mem,0x1c0606+i*0x10c,(ident+i)%12)
   yield f'B608-stage4-{mask}-{ident}',0xb608,mem,None,(0xb812,),True
  for ident in range(12):
   mem=seed();put(mem,0x1c1138,ident);put(mem,0x1c1168,(ident+7)%12);yield f'B94C-{ident}',0xb94c,mem,None,(0xb9a6,),False
  for special in(0,0x1e):
   mem=seed();put(mem,0x1c142a,special);yield f'B94C-literal-special-{special}',0xb94c,mem,None,(0xb9a6,),False
  for phase,timer,latched in itertools.product(range(24),(0,1,8,0xffff),(0,0x80)):
   mem=seed();put(mem,0x1c11b6,timer);put(mem,0x1c11b8,phase);put(mem,0x1c127a,latched,1);yield f'B8A4-{phase}-{timer}-{latched}',0xb8a4,mem,2,(0xb934,),False
  for index,pose,xpos,next_active in itertools.product((0,16),(0xe,0x12,0x22,0x23,0xffff),(0,1,0x140,0x141,0x8000,0xffff),(0,0x80)):
   mem=seed();put(mem,0x1c1258,0x8000);put(mem,0x1c125c,pose);put(mem,0x1c126c,xpos);put(mem,0x1c1282,next_active,1);yield f'BA48-{index}-{pose}-{xpos}-{next_active}',0xba48,mem,index,(0xbaaa,),False
  return
 yield 'main-stage3',0x11b6,seed(3),None,(0xac0,),False
 if args.smoke:return
 for mode,progress in itertools.product((0,1,2,3),(0,1,3,0x104,9)):
  mem=seed(progress);put(mem,0x1c0161,mode,1)
  yield f'B608-fast-{mode}-{progress:x}',0xb608,mem,None,(0xb812,),False
 for progress,mask,config in itertools.product((2,4,7,0xff),(0,1,4,15),(0,0x8000)):
  mem=seed(progress,mask);put(mem,0x1c0066,config);yield f'progress-history-{progress}-{mask}-{config}',0x1a76,mem,None,(0xac0,),False
 for mask in(0,1,4,15):
  mem=seed(9,mask);yield f'last-victory-{mask}',0x19ba,mem,None,(0x1b4e,0x6fc),False
 mem=seed(0,0);yield 'all-eliminated-title',0x11b6,mem,None,(0x6fc,),False
 for progress,mask,result in itertools.product((0,1,2,3,5,8,0x103),(1,4,5,15),(0x4001,0x4000)):
  mem=seed(progress,mask)
  for i in range(4):put(mem,0x1c0650+0x10c*i,result if mask&(1<<i)else 0)
  yield f'main-{progress:x}-{mask:x}-{result:x}',0x11b6,mem,None,(0xac0,),True
 for progress in(0,1,2,3,5,6,7,8,9):
  mem=seed(progress);yield f'interview-{progress}',0xbda6,mem,None,(0xbf56,),True
 for ident in range(12):
  mem=seed();put(mem,0x1c0162,0xabcd);yield f'AE20-setup-{ident}',0xae20,mem,3,(0xb0c0,),False
 for index,latch,timer,phase in itertools.product(range(5),(0,0x80),(0,1,10,0xff00),(0,1,2,3,4)):
  mem=seed();p=0x1c1258+index*0x2a;put(mem,p+0x1c,latch,1);put(mem,p+0x22,timer);put(mem,p+0x24,phase)
  yield f'BF5E-{index}-{latch:x}-{timer:x}-{phase}',0xbf5e,mem,index,(0xbffc,),False
 yield 'release',0xc004,seed(),None,(0xc042,),False
 for owner in(0x15f4,):yield 'continue-objects-clear',owner,seed(),None,(0x160c,),False
source=ROOT/'src/wf_r18_post_tag.c'
with tempfile.TemporaryDirectory(prefix='wf-post-tag-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'post.so';src='tests/r18/post_tag_pending.c'if args.pending else'src/wf_r18_post_tag.c'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',src,'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_r18_post_tag_init.argtypes=[ctypes.POINTER(State),ctypes.c_uint32];native.wf_r18_post_tag_step.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context)];native.wf_r18_post_tag_complete.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context),ctypes.c_int,ctypes.c_uint16]
 for name,owner,mem,d0,stops,delay in fixtures():
  regs=[rng.getrandbits(32)for _ in range(15)]
  if d0 is not None:regs[0]=(regs[0]&0xffff0000)|d0
  if owner in(0x139c,0x160e):regs[9]=0x1c05b0+d0*0x10c;regs[7]=(regs[7]&0xffff0000)|get(mem,0x1c0168 if d0==0 else 0x1c016a)
  if owner==0xbf5e:regs[8]=0x1c1258+d0*0x2a
  if owner==0xb8a4:regs[8]=0x1c1194
  if owner==0xba48:regs[8]=0x1c1258
  want=bytearray(mem);wr,events,stop=execute(want,regs,owner,stops,True,delay);want_carry=machine.cpu.r_sr()&1
  buf=ctypes.create_string_buffer(bytes(mem));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);m=Machine(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),None,None,0);c=Context(ctypes.pointer(m),read_rom,None);s=State();native.wf_r18_post_tag_init(ctypes.byref(s),owner);at=0;delayed_native=False
  for _ in range(1000000):
   status=native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c));assert status>=0,(name,'blocked',hex(s.pc),hex(s.blocked_dependency))
   if status==1:
    q=s.request;cur=bytearray(buf.raw);dr=list(sr.d)+list(m.a);event=tuple(getattr(q,x)for x in('kind','owner','call_pc','return_pc','address','value'))+(tuple(dr),digest(cur))
    assert at<len(events)and event==events[at],(name,'request',at,event,events[at]if at<len(events)else None)
    at+=1;before=bytes(buf);before_regs=dr[:];assert native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))==1 and bytes(buf)==before and list(sr.d)+list(m.a)==before_regs
    value=0
    if q.kind==0:dr,_,_=execute(cur,dr,q.owner)
    elif q.kind==2:put(cur,q.address,q.value)
    elif q.kind==3:
     # The first delayed ACK is a real unchanged event, not a fabricated tick.
     value=get(cur,0x1c006f,1)
     if delay and not delayed_native:delayed_native=True
     else:put(cur,0x1c006f,value|2,1)
     value=0
    elif q.kind in(1,4):value=get(cur,q.address,2 if q.kind==1 else 1)
    else:raise AssertionError(('unknown request',q.kind))
    for base,n in REGIONS:ctypes.memmove(ctypes.addressof(buf)+base,bytes(cur[base:base+n]),n)
    for i,v in enumerate(dr[:8]):sr.d[i]=v
    for i,v in enumerate(dr[8:]):m.a[i]=v
    rc=native.wf_r18_post_tag_complete(ctypes.byref(s),ctypes.byref(c),0,value);assert rc in(0,5),(name,'complete',rc);waits+=rc==5
   elif status in(2,3,4):break
  assert status in(2,3,4)and s.pc==stop and at==len(events),(name,'terminal',hex(s.pc),status,at,len(events))
  for base,n in REGIONS:
   got=buf.raw[base:base+n];expected=want[base:base+n];assert got==expected,(name,'memory',[(hex(base+i),g,e)for i,(g,e)in enumerate(zip(got,expected))if g!=e][:16])
  assert list(sr.d)+list(m.a)==wr,(name,'D/A',list(sr.d)+list(m.a),wr);count+=1
  if owner in(0x160e,0x55a,0xc404):assert s.carry==want_carry,(name,'carry',s.carry,want_carry)
  if count%1000==0:print('verified',count,flush=True)
 required_continue=(0x175c,0x17be,0x17c8,0x17d6,0x17e0,0x1804,0x1812,0x16d4,0x16e2)if args.continue_only else()
 assert all(pc in coverage for pc in required_continue),('missing continue byte/word input branches',[hex(pc)for pc in required_continue if pc not in coverage])
 controls=0
 assert native.wf_r18_post_tag_step(None,ctypes.byref(c))==-2;controls+=1
 assert native.wf_r18_post_tag_step(ctypes.byref(s),None)==-2;controls+=1
 assert native.wf_r18_post_tag_complete(ctypes.byref(s),ctypes.byref(c),0,0)==-2;controls+=1
 for pc in(0x11d2,0xbdb8,0xbe66):
  native.wf_r18_post_tag_init(ctypes.byref(s),pc);assert native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))==1;m.dependency=0xdead
  assert native.wf_r18_post_tag_complete(ctypes.byref(s),ctypes.byref(c),1,0)==-1 and s.blocked_dependency==0xdead;controls+=1
  before=bytes(buf);dr=list(sr.d)+list(m.a);assert native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))==-1 and bytes(buf)==before and list(sr.d)+list(m.a)==dr;controls+=1
  assert native.wf_r18_post_tag_complete(ctypes.byref(s),ctypes.byref(c),0,0)==-2;controls+=1
 for pc in(0xdead,0x123456):
  native.wf_r18_post_tag_init(ctypes.byref(s),pc);assert native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==pc;controls+=1
 # AE20 program 2 is implemented now; the old unsupported-owner guard was
 # stale. Unknown-PC guards above continue to test fail-closed behavior.
 saved_reader=c.read_rom;c.read_rom=Rom();native.wf_r18_post_tag_init(ctypes.byref(s),0xbe1e);m.a[0]=0x1c12d6;m.a[1]=0xbf58;sr.d[0]=1
 assert native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0xbe1e;controls+=1;c.read_rom=saved_reader
 native.wf_r18_post_tag_init(ctypes.byref(s),0xbf5e);m.a[0]=0xdeadbeef
 assert native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))==-1 and s.blocked_dependency==0xbf5e;controls+=1
 assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guard_controls':controls,'waits':waits,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'coverage_pcs':len(coverage),'compared':['raw16KB/BG/FG/palettes/board','D0-D7/A0-A6','request entry/return PCs/order/live context/digests','unchanged pending request'],'delegated_intact_rom':[hex(x)for x in sorted(EXTERNAL-{0x2052})],'intercepts':['2052 silent sound output only'],'limits':['This regression suite covers AE20 argument3; other programs require separate evidence','65C0 ranking and ending1B4E not supplied by this module','206FE/2067C are explicit calls to existing native referee video helpers']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
