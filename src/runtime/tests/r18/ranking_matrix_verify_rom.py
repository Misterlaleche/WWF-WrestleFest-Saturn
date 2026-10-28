"""Ranking local owners against intactROM, explicit physical input/ACK tape.

Glyph and record auxiliaries execute production C. Other closed scene owners
execute original ROM and supply an opaque dependency response snapshot. This
tests their caller's live prefix, not the native implementation of those
dependencies; the platform integration test covers native composition.
"""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');p.add_argument('--seed',type=lambda s:int(s,0),default=0x65c020261024);p.add_argument('--filter');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'ranking_capture_manifest.json').read_text());capture=(HERE/'ranking_snapshot_stage9b_guard.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
REGIONS=((0x1c0000,0x4000),(0xc0000,0x4000),(0x80000,0x10000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40))
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Machine(ctypes.Structure):_fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',ctypes.c_void_p),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
ReadROM=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t)
Store=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint)
class Context(ctypes.Structure):_fields_=[('machine',ctypes.POINTER(Machine)),('read_rom',ReadROM),('user',ctypes.c_void_p)]
class Request(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('kind','owner','call_pc','return_pc','address','value','width')]
class State(ctypes.Structure):_fields_=[('pc',ctypes.c_uint32),('entry_pc',ctypes.c_uint32),('blocked_dependency',ctypes.c_uint32),('return_stack',ctypes.c_uint32*8),('saved_d',ctypes.c_uint32*5),('saved_a2',ctypes.c_uint32),('saved_d0',ctypes.c_uint32),('io_value',ctypes.c_uint32),('request',Request),('depth',ctypes.c_uint8),('pending',ctypes.c_uint8),('fatal',ctypes.c_uint8),('carry',ctypes.c_uint8)]
class Glyph(ctypes.Structure):_fields_=[('machine',ctypes.POINTER(Machine)),('store',Store),('user',ctypes.c_void_p),('dep',ctypes.c_uint32),('address',ctypes.c_uint32)]
class Aux(ctypes.Structure):_fields_=[('machine',ctypes.POINTER(Machine)),('read_rom',ReadROM),('user',ctypes.c_void_p),('store',Store),('dep',ctypes.c_uint32),('address',ctypes.c_uint32)]
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def get(mem,at,n=2):return int.from_bytes(mem[at:at+n],'big')
coverage=set();rng=random.Random(a.seed);total=prefixes=requests=acks=api_guards=0;delegated=set();reads=set();writes=set()
cpu=mus.Machine(mus.CPUType.M68000,2048);cpu.mem.w_block(0,rom)
def poll(pc):
 if rom[pc:pc+8]==bytes.fromhex('08390001001c006f'):cpu.mem.w8(0x1c006f,cpu.mem.r8(0x1c006f)|2)
 if rom[pc:pc+8]==bytes.fromhex('0839000200140026'):
  v=cpu.mem.r8(0x140026);cpu.mem.w8(0x140026,(v|4)if rom[pc+8]==0x67 else(v&0xfb))
 if pc in(0x298c,0x2a0a):cpu.mem.w8(0x140026,cpu.mem.r8(0x140026)|4)
 elif pc in(0x29f6,0x2a88):cpu.mem.w8(0x140026,cpu.mem.r8(0x140026)&0xfb)
def run_to(pc,limit=5000000):
 for _ in range(limit):
  now=cpu.cpu.r_pc()
  if now==pc:return
  coverage.add(now);poll(now);cpu.cpu.execute(1)
 raise AssertionError(('ROM runaway',hex(pc),hex(cpu.cpu.r_pc())))
def seed():
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 for at,size in REGIONS:mem[at:at+size]=rng.randbytes(size)
 mem[0x1c0000:0x1c1cd4]=bytes(0x1cd4)
 for base in(0xc2,0x104):
  for row in range(11):mem[0x1c0000+base+row*6:0x1c0000+base+row*6+6]=bytes((11-row,0x5b,0x5b,0x5b,row%12,(row+1)%12))
 for i in range(9):put(mem,0x1c05b0+0x10c*i+0x56,i%12)
 for i in range(4):put(mem,0x140020+i*2,0xffff);put(mem,0x1c00ba+i*2,0xffff)
 return mem
@ReadROM
def read_rom(_,at,p,n):
 if at+n>len(rom):return 1
 ctypes.memmove(p,rom[at:at+n],n);return 0
with tempfile.TemporaryDirectory(prefix='wf-ranking-',ignore_cleanup_errors=os.name=='nt')as td:
 f=Path(td)
 for name in('src','include'):shutil.copytree(ROOT/name,f/name)
 sha=hashlib.sha256((f/'src/wf_r18_ranking.c').read_bytes()).hexdigest();lib=f/'ranking.dll'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_ranking.c','src/wf_r18_ranking_glyph.c','src/wf_r18_records_aux.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=f,check=True)
 n=ctypes.CDLL(str(lib));n.wf_r18_ranking_init.argtypes=[ctypes.POINTER(State),ctypes.c_uint32];n.wf_r18_ranking_step.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context)];n.wf_r18_ranking_complete.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context),ctypes.c_int,ctypes.c_uint32]
 n.wf_r18_ranking_glyph_call.argtypes=[ctypes.POINTER(Glyph),ctypes.c_uint32];n.wf_r18_records_aux_call.argtypes=[ctypes.POINTER(Aux),ctypes.c_uint32]
 def case(label,owner,mem,regs=None,tape=(0xffff,),stop=False,carry=False,fail_read=None,guard_pc=None):
  global total,prefixes,requests,acks
  if regs is None:regs=[rng.getrandbits(32)for _ in range(15)]
  @ReadROM
  def reading(_,at,out,size):
   if at==fail_read:return 1
   return read_rom(_,at,out,size)
  native=bytearray(mem);ram=ctypes.create_string_buffer(bytes(mem[0x1c0000:0x1c4000]));dr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);m=Machine(ctypes.cast(ram,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(dr),(ctypes.c_uint32*7)(*regs[8:]),None,None,0);ctx=Context(ctypes.pointer(m),reading,None);s=State();n.wf_r18_ranking_init(ctypes.byref(s),owner)
  for at,size in REGIONS:cpu.mem.w_block(at,bytes(mem[at:at+size]))
  for i,v in enumerate(regs):cpu.cpu.w_reg(i,v)
  cpu.cpu.w_sp(0x1d0000);cpu.mem.w32(0x1d0000,0x70000);cpu.cpu.w_pc(owner)
  def compare(where):
   got=list(dr.d)+list(m.a);expected=[cpu.cpu.r_reg(i)for i in range(15)]
   assert got==expected,(label,where,'D/A',[(i,hex(x),hex(y))for i,(x,y)in enumerate(zip(got,expected))if x!=y])
   raw=bytes(cpu.mem.r_block(0x1c0000,0x4000));assert ram.raw[:0x4000]==raw,(label,where,'RAM',next((hex(i),hex(x),hex(y))for i,(x,y)in enumerate(zip(ram.raw,raw))if x!=y))
   for at,size in REGIONS:
    if at==0x1c0000:continue
    expected=bytes(cpu.mem.r_block(at,size));assert native[at:at+size]==expected,(label,where,'BUS',hex(at),next((hex(i),hex(x),hex(y))for i,(x,y)in enumerate(zip(native[at:at+size],expected))if x!=y))
  @Store
  def store(_,at,value,width):
   at&=0xffffff
   if not(0xc0000<=at<=0xc4000-width):return 1
   put(native,at,value,width);return 0
  read_index=0
  for step in range(400000):
   result=n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))
   if result<0 and guard_pc is not None:
    assert result==-1 and s.blocked_dependency==guard_pc,(label,'guard PC',hex(s.pc),hex(s.blocked_dependency));run_to(guard_pc);compare(hex(guard_pc));prefixes+=1;return
   assert result>=0,(label,'C block',hex(s.pc),hex(s.blocked_dependency))
   if result==2:
    run_to(0x70000);compare('return')
    if carry:assert s.carry==bool(cpu.cpu.r_sr()&1),(label,'carry')
    break
   if result==0:continue
   assert result==1,(label,'unexpected WAIT',result)
   r=s.request;run_to(r.call_pc);compare(hex(r.call_pc));requests+=1
   before=(bytes(s),ram.raw,list(dr.d),list(m.a));assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==1;assert before==(bytes(s),ram.raw,list(dr.d),list(m.a))
   if stop:
    assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),1,0)==-1;assert ram.raw==before[1]and list(dr.d)==before[2]and list(m.a)==before[3];assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==-1;prefixes+=1;return
   value=0
   if r.kind==0:
    if r.owner==0x6cfa:
     g=Glyph(ctypes.pointer(m),store,None,0,0);assert n.wf_r18_ranking_glyph_call(ctypes.byref(g),r.owner)==0,(label,'glyph',hex(g.dep))
    elif r.owner in(0xa4d4,0x7874):
     g=Aux(ctypes.pointer(m),read_rom,None,store,0,0);assert n.wf_r18_records_aux_call(ctypes.byref(g),r.owner)==0,(label,'aux',hex(g.dep))
    run_to(r.return_pc)
    if r.owner not in(0x6cfa,0xa4d4,0x7874):
     delegated.add(r.owner)
     for i in range(8):dr.d[i]=cpu.cpu.r_reg(i)
     for i in range(7):m.a[i]=cpu.cpu.r_reg(i+8)
     ctypes.memmove(ram,bytes(cpu.mem.r_block(0x1c0000,0x4000)),0x4000)
     for at,size in REGIONS:
      if at!=0x1c0000:native[at:at+size]=cpu.mem.r_block(at,size)
   elif r.kind==1:
    coverage.add(r.call_pc)
    at=r.address&0xffffff;reads.add((r.call_pc,r.width))
    if r.call_pc==0x6cf2:
     value=tape[min(read_index,len(tape)-1)];read_index+=1;put(native,at,value,r.width);cpu.mem.w16(at,value)
    else:value=get(native,at,r.width)
    cpu.cpu.execute(1)
   elif r.kind==2:
    coverage.add(r.call_pc)
    at=r.address&0xffffff;writes.add((r.call_pc,r.width));put(native,at,r.value,r.width);cpu.cpu.execute(1)
   elif r.kind==3:
    ram[0x6f]=bytes((ram.raw[0x6f]|2,));cpu.mem.w8(0x1c006f,cpu.mem.r8(0x1c006f)|2);acks+=1
   else:raise AssertionError((label,r.kind))
   assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),0,value)>=0
  else:raise AssertionError((label,'C runaway',hex(s.pc)))
  total+=1
 def fixtures():
  mem=seed();mem[0x1c0000:0x1c4000]=capture[60:60+0x4000];mem[0xc0000:0xc4000]=capture[60+0x4000:];yield'native9b',0x65c0,mem,list(struct.unpack_from('=15I',capture)),(0xffff,),False
  for mode,candidate,player in itertools.product((0,1,2,3),(0,1,99,100,101,255,256,0xffff),range(4)):
   mem=seed();mem[0x1c0161]=mode;put(mem,0x1c0164,candidate);put(mem,0x1c05b0+0x10c*player+0xc4,candidate)
   regs=[rng.getrandbits(32)for _ in range(15)];regs[0]=(regs[0]&0xffff0000)|player;regs[8]=0x1c05b0+0x10c*player
   yield f'qualify-{mode}-{candidate}-{player}',0x67b2,mem,regs,(0xffff,),True
  for mode in(0,1,0x80,0x81):
   for k in range(20):
    mem=seed();mem[0x1c0161]=mode
    for base in(0xc2,0x104):
     for row in range(11):mem[0x1c0000+base+row*6:0x1c0000+base+row*6+6]=bytes((rng.randrange(4),0x5b,rng.choice((0x5b,0x5c)),rng.randrange(4),rng.randrange(12),rng.randrange(12)))
    yield f'sort-{mode}-{k}',0x683e,mem,None,(0xffff,),False
    yield f'consume-{mode}-{k}',0x68ac,mem,None,(0xffff,),False
  for mode in(0,1,2,3):
   mem=seed();mem[0x1c0161]=mode;yield f'table-{mode}',0x6a26,mem,None,(0xffff,),False
  for slot,rank,flags in itertools.product(range(4),range(11),(0,6,7,8,14,15,16,0xff)):
   mem=seed();put(mem,0x1c00ba+slot*2,rank);mem[0x1c1cd4+slot]=flags;regs=[rng.getrandbits(32)for _ in range(15)];regs[1]=(regs[1]&0xffff0000)|slot
   yield f'blink-{slot}-{rank}-{flags}',0x68f6,mem,regs,(0xffff,),False
  for mode,slot,rank,k in itertools.product((0,1),range(4),(0,3,10),range(48)):
   cursor=(0,1,2,3)[k%4];char=(0x30,0x31,0x4a,0x4b)[k//4%4];repeat=(0,7,8,0xffff)[k//12];held=k%4;ports=((0xffff,0xffff),(0xfffe,0xfffe),(0xfffd,0xfffd),(0xffff,0xffef),(0xffff,0xffdf),(0xfffc,0xffcf))[k//8]
   mem=seed();mem[0x1c0161]=mode;put(mem,0x1c00ba+slot*2,rank);put(mem,0x1c00b2+slot*2,cursor);put(mem,0x1c00a2+slot*2,char);put(mem,0x1c00aa+slot*2,repeat);put(mem,0x1c0146+slot*2,held)
   regs=[rng.getrandbits(32)for _ in range(15)];regs[0]=(regs[0]&0xffff0000)|rank;regs[1]=(regs[1]&0xffff0000)|slot
   yield f'input-{mode}-{slot}-{rank}-{k}',0x6b6e,mem,regs,ports,False
  for mode,config,entrants in itertools.product((0,1,2,3),(0,8,0x80),((0,0,0,0),(0x1c07c8,0x1c08d4,0,0),(0x801c07c8,0x801c08d4,0,0),(0x1c05b0,0x1c06bc,0x1c07c8,0x1c08d4))):
   mem=seed();mem[0x1c0161]=mode;mem[0x1c0066]=config;put(mem,0x1c0164,0)
   for i,ptr in enumerate(entrants):put(mem,0x1c0092+i*4,ptr,4)
   yield f'entry-noqualify-{mode}-{config}-{entrants}',0x65c0,mem,None,(0xffff,),False
  for mode in(0,1):
   mem=seed();mem[0x1c0161]=mode;put(mem,0x1c0092+12,0x1c08d4,4);put(mem,0x1c0164,20);put(mem,0x1c08d4+0xc4,20)
   tape=(0xffff,0xffef,0xffff,0xffff)*3+(0xffff,0xffef)
   yield f'whole-confirm-{mode}',0x65c0,mem,None,tape,False
  for mode in(0,1):
   mem=seed();mem[0x1c0161]=mode;put(mem,0x1c0092,0x1c05b0,4);put(mem,0x1c0164,20);put(mem,0x1c05b0+0xc4,20)
   yield f'whole-timeout-slot0-{mode}',0x65c0,mem,None,(0xffff,),False
  for slot,word in itertools.product(range(4),(0xffff,0xfffe,0xfffd,0xffef,0xffdf,0xffbf,0xff7f,0x7fff,0xff00,0)):
   mem=seed();regs=[rng.getrandbits(32)for _ in range(15)];regs[1]=(regs[1]&0xffff0000)|slot
   yield f'physical-input-{slot}-{word}',0x6ce8,mem,regs,(word,),False
  for alias,values in itertools.product((0,0x80000000),(0,1,4,15)):
   mem=seed();regs=[rng.getrandbits(32)for _ in range(15)];regs[8]=alias|0xc0300
   for i in range(4):put(mem,0xc0300+i*2,0 if values&(1<<i)else 0xffff)
   yield f'completion-A0-bus-{alias}-{values}',0x6794,mem,regs,(0xffff,),True
 for idx,(label,owner,mem,regs,tape,carry)in enumerate(fixtures()):
  if a.smoke and idx>300:break
  if a.filter and not re.search(a.filter,label):continue
  case(label,owner,mem,regs,tape,carry=carry)
 for owner in(0x6ce8,0x68f6,0x6a26,0x69c8):
  mem=seed();put(mem,0x1c00ba,0);mem[0x1c1cd4]=7;regs=[0]*15;regs[8]=0x1c00ba
  case(f'failed-{owner:x}',owner,mem,regs,stop=True)
 for owner,at,pc,changes in((0x6a26,0x6b58,0x6a48,{}),(0x6a26,0x6b42,0x6ada,{}),(0x6b6e,0x6b42,0x6b78,{0:0,1:0}),(0x68f6,0x69c2,0x694c,{1:0})):
  mem=seed();put(mem,0x1c00ba,0);regs=[rng.getrandbits(32)for _ in range(15)]
  for i,v in changes.items():regs[i]=(regs[i]&0xffff0000)|v
  case(f'failed-ROM-{owner:x}-{pc:x}',owner,mem,regs,fail_read=at,guard_pc=pc)
 for owner,pc,d1 in((0x6b6e,0x6b94,0x4000),(0x68f6,0x691a,0x4000)):
  mem=seed();regs=[rng.getrandbits(32)for _ in range(15)];regs[0]&=0xffff0000;regs[1]=(regs[1]&0xffff0000)|d1
  case(f'failed-raw-{owner:x}-{d1:x}',owner,mem,regs,guard_pc=pc)
 # Invalid API calls leave caller-owned storage untouched. A wait completion
 # without ACK consumes no timer or input, and the next request remains live.
 ram=ctypes.create_string_buffer(bytes(0x4000));dr=Shadow((ctypes.c_uint32*8)(*range(8)),1);m=Machine(ctypes.cast(ram,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(dr),(ctypes.c_uint32*7)(*range(7)),None,None,0);ctx=Context(ctypes.pointer(m),read_rom,None);s=State();n.wf_r18_ranking_init(ctypes.byref(s),0x673e)
 for which in range(4):
  bad=Context(ctypes.pointer(m),read_rom,None)
  if which==0:bad.machine=ctypes.POINTER(Machine)()
  elif which==1:m.ram=ctypes.POINTER(ctypes.c_uint8)()
  elif which==2:m.regs=ctypes.POINTER(Shadow)()
  else:dr.valid=0
  before=bytes(s);assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(bad))==-2 and bytes(s)==before;api_guards+=1
  m.ram=ctypes.cast(ram,ctypes.POINTER(ctypes.c_uint8));m.regs=ctypes.pointer(dr);dr.valid=1
 before=bytes(s);assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),0,0)==-2 and bytes(s)==before;api_guards+=1
 assert n.wf_r18_ranking_step(None,ctypes.byref(ctx))==-2;assert n.wf_r18_ranking_step(ctypes.byref(s),None)==-2;api_guards+=2
 assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==1
 before=(ram.raw,list(dr.d),list(m.a));assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),0,0)==3;assert before==(ram.raw,list(dr.d),list(m.a));api_guards+=1
 assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==1
 ram[0x6f]=b'\x02';assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),0,0)==0;assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==0 and s.pc==0x6748;api_guards+=1
 n.wf_r18_ranking_init(ctypes.byref(s),0x65c2);assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==-1 and s.blocked_dependency==0x65c2;api_guards+=1
 n.wf_r18_ranking_init(ctypes.byref(s),0x6ce8)
 while n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==0:pass
 m.dependency=0x1234;assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),1,0)==-1 and s.blocked_dependency==0x1234;api_guards+=1
 assert bytes(cpu.mem.r_block(0,len(rom)))==rom;cpu.cleanup()
 disassembler=mus.Machine(mus.CPUType.M68000,2048);disassembler.mem.w_block(0,rom);owned=set()
 for start,end in((0x65c0,0x69c2),(0x69c8,0x6b42),(0x6b6e,0x6cfa),(0x7720,0x7744)):
  pc=start
  while pc<end:owned.add(pc);pc+=disassembler.cpu.disassemble(pc)[0]
 disassembler.cleanup()
 if not a.smoke and not a.filter:assert owned<=coverage,sorted(hex(x)for x in owned-coverage)
 report={'status':'PASS','cases':total,'failure_prefixes':prefixes,'API_wait_guards':api_guards,'requests':requests,'ack_events':acks,'seed':a.seed,'source_sha256':sha,'snapshot_sha256':meta['snapshot_sha256'],'compared':['raw16KB','D0-D7/A0-A6','FG16KB','board/palette regions','every request boundary'],'intact_rom':True,'intercepts':[],'opaque_original_rom_dependencies':[hex(x)for x in sorted(delegated)],'read_widths':sorted(reads),'write_widths':sorted(writes),'owned_PC_coverage':[len(owned&coverage),len(owned)],'coverage':[hex(x)for x in sorted(coverage)]}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k!='coverage'}))
