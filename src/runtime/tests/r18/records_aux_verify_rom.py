"""Records A4D4/7874 versus intact ROM, full D/A/RAM/FG/bus and fault prefixes."""
from pathlib import Path
import argparse,ctypes as ct,hashlib,itertools,json,os,random,shutil,subprocess,tempfile,zipfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--freeze',type=Path);p.add_argument('--smoke',action='store_true');p.add_argument('--seed',type=lambda s:int(s,0),default=0xa4d47874);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
class Shadow(ct.Structure):_fields_=[('d',ct.c_uint32*8),('valid',ct.c_uint8)]
class Machine(ct.Structure):_fields_=[('ram',ct.POINTER(ct.c_uint8)),('regs',ct.POINTER(Shadow)),('a',ct.c_uint32*7),('effect',ct.c_void_p),('user',ct.c_void_p),('dependency',ct.c_uint32)]
Read=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_void_p,ct.c_size_t)
Store=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_uint32,ct.c_uint)
class Context(ct.Structure):_fields_=[('machine',ct.POINTER(Machine)),('read_rom',Read),('user',ct.c_void_p),('store_bus',Store),('blocked_dependency',ct.c_uint32),('blocked_address',ct.c_uint32)]
rng=random.Random(args.seed);coverage=set();groups={};count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(mode=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);m[0xc0000:0xc4000]=rng.randbytes(0x4000);m[RAM+0x161]=mode
 m[RAM+0xc2:RAM+0x146]=rom[0x6db0:0x6e34]
 return m
def initial_regs(owner,mode):
 values=[rng.getrandbits(32)for _ in range(15)]
 # A4D4's BYTE load of partner ID retains D2.W's high byte. The real 6A26
 # caller leaves it zero after processing a valid initial; arbitrary high
 # bytes are tested separately as exact unsupported-read prefixes.
 if owner==0xa4d4 and not mode&1:values[2]&=0xffff00ff
 return values
def fixtures():
 for owner,mode in itertools.product((0xa4d4,0x7874),(0,1)):
  yield 'factory-rows',owner,seed(mode)
 if args.smoke:return
 for mode,char1,char2 in itertools.product((0,1,2,3,0x80,0xff),range(12),(0,1,4,5,10,11)):
  m=seed(mode);base=RAM+(0x104 if mode&1 else 0xc2)
  for row in range(11):m[base+row*6+4]=char1;m[base+row*6+5]=char2
  yield 'all-identities-both-table-paths',0xa4d4,m
 for mode in range(256):
  yield 'all-MODE-BYTE-frame-table-selection',0x7874,seed(mode)
 for _ in range(80):
  mode=rng.randrange(256);m=seed(mode);base=RAM+(0x104 if mode&1 else 0xc2)
  for row in range(11):m[base+row*6+4]=rng.randrange(12);m[base+row*6+5]=rng.randrange(12)
  yield 'mixed-identities-and-raw-padding',0xa4d4,m
def event(cpu,mem):
 pc=cpu.r_pc();D=lambda n:cpu.r_reg(n);W=lambda n:D(n)&0xffff;S=lambda n:(W(n)^0x8000)-0x8000;A=lambda n:cpu.r_reg(8+n)
 reads={0x77e4:(A(1)+S(0),2),0xa5e6:(A(2)+S(4)+2,2),0xa5ea:(A(2)+S(4),2),0x78d8:(A(1)+S(1),2),0x78ea:(A(1)+S(1)+2,2),0x78ee:(A(1)+S(1)+4,2),0x7902:(A(1)+S(1),2),0x7908:(A(1)+S(1)+2,1),0x790c:(A(1)+S(1)+3,1)}
 stores={0x77ec:(A(0)+1,1,D(0)&255),0x77f4:(A(0)+3,1,D(1)&255),0xa5ee:(A(1)+1,1,D(4)&255),0xa5f6:(A(1)+3,1,D(5)&255),0xa590:(A(1),4,0xffa2fffb),0x7940:(A(3)+1,1,D(4)&255),0x7950:(A(3)+3,1,D(6)&255)}
 if pc in reads:
  at,n=reads[pc];at&=0xffffff;return (0,at,n,int.from_bytes(mem.r_block(at,n),'big'),pc)
 if pc in stores:
  at,n,v=stores[pc];return (1,at&0xffffff,n,v,pc)
 return None
with tempfile.TemporaryDirectory(prefix='wf-records-aux-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 sources={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 lib=frozen/'aux.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_records_aux.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=frozen,check=True)
 native=ct.CDLL(str(lib));native.wf_r18_records_aux_call.argtypes=[ct.POINTER(Context),ct.c_uint32]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 def original(m,owner,regs,stop_at=None,stop_pc=None):
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(owner)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  events=[];pc_hits=0
  for _ in range(200000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   e=event(cpu,mem)
   if stop_pc is not None:
    target,nth=stop_pc if isinstance(stop_pc,tuple)else(stop_pc,1)
    if pc==target:
     pc_hits+=1
     if pc_hits==nth:break
   if e:
    if stop_at is not None and len(events)==stop_at:break
    events.append(e)
   cpu.execute(1)
  else:raise AssertionError(('ROM runaway',hex(cpu.r_pc())))
  return events
 def actual(m,owner,regs,fail_at=None):
  raw=ct.create_string_buffer(bytes(m[RAM:RAM+0x4000]));fg=bytearray(m[0xc0000:0xc4000]);shadow=Shadow((ct.c_uint32*8)(*regs[:8]),1);machine=Machine(ct.cast(raw,ct.POINTER(ct.c_uint8)),ct.pointer(shadow),(ct.c_uint32*7)(*regs[8:]),None,None,0);events=[]
  @Read
  def read(_,at,dest,n):
   idx=len(events);value=int.from_bytes(rom[at:at+n],'big');events.append((0,at,n,value))
   if idx==fail_at:return 1
   if at+n>len(rom):return 1
   ct.memmove(dest,rom[at:at+n],n);return 0
  @Store
  def store(_,at,value,n):
   idx=len(events);events.append((1,at,n,value))
   if idx==fail_at:return 1
   if at<0xc0000 or at+n>0xc4000:return 1
   fg[at-0xc0000:at-0xc0000+n]=value.to_bytes(n,'big');return 0
  c=Context(ct.pointer(machine),read,None,store,0,0);status=native.wf_r18_records_aux_call(ct.byref(c),owner)
  return status,raw.raw[:0x4000],bytes(fg),list(shadow.d)+list(machine.a),events,c.blocked_dependency,c.blocked_address,machine.dependency
 def compare(m,owner,regs,fail_at=None,stop_pc=None):
  events=original(m,owner,regs,fail_at,stop_pc);result=actual(m,owner,regs,fail_at)
  if fail_at is None and stop_pc is None:assert result[0]==0,('native blocked',hex(owner),result[5:]);assert result[4]==[e[:4]for e in events],('ordered bus effects',hex(owner))
  else:
   assert result[0]==1 and result[5]==cpu.r_pc()and result[7]==result[5],('faultPC',result[5:],hex(cpu.r_pc()))
   e=event(cpu,mem);assert result[6]==e[1],('fault address',result[6],e)
   expected=[v[:4]for v in events]
   if fail_at is not None:expected.append(e[:4])
   assert result[4]==expected,('ordered prefix effects',hex(owner),fail_at,stop_pc)
  assert result[1]==bytes(mem.r_block(RAM,0x4000)),('RAM',hex(owner),fail_at)
  assert result[2]==bytes(mem.r_block(0xc0000,0x4000)),('FG',hex(owner),fail_at,[(hex(0xc0000+i),a,b)for i,(a,b)in enumerate(zip(result[2],mem.r_block(0xc0000,0x4000)))if a!=b][:8])
  assert result[3]==[cpu.r_reg(i)for i in range(15)],('D/A',hex(owner),fail_at,list(map(hex,result[3])),[hex(cpu.r_reg(i))for i in range(15)])
  return events
 for name,owner,m in fixtures():
  regs=initial_regs(owner,m[RAM+0x161]);compare(m,owner,regs);count+=1;groups[name]=groups.get(name,0)+1
 faults=0;bounds=0;api_guards=0
 if not args.smoke:
  for owner,mode in itertools.product((0xa4d4,0x7874),(0,1)):
   m=seed(mode);base=RAM+(0x104 if mode else 0xc2)
   for row in range(11):m[base+row*6+4]=10 if row>2 else 0;m[base+row*6+5]=11
   regs=initial_regs(owner,mode);events=original(m,owner,regs)
   # Fail every actual bus operation, including the second glyph under the
   # nested D4/A3 MOVEM at7918, every row and both LONG separator writes.
   for index in range(len(events)):compare(m,owner,regs,index);faults+=1
  for mode,row,bad in itertools.product((0,1),(0,3),(12,13,0xff)):
   if row==3 and bad==12:continue
   m=seed(mode);base=RAM+(0x104 if mode else 0xc2);m[base+row*6+4]=bad
   compare(m,0xa4d4,initial_regs(0xa4d4,mode),stop_pc=0x77e4 if row==0 else 0xa5e6 if mode else 0xa5ea);bounds+=1
  for mode,high in itertools.product((0,2,0x80),(1,0x7f,0x80,0xff)):
   m=seed(mode);regs=initial_regs(0xa4d4,mode);regs[2]|=high<<8
   compare(m,0xa4d4,regs,stop_pc=(0x77e4,2));bounds+=1
  for mode,which,bad in itertools.product((0,2),(0,1),(12,13,0xff)):
   m=seed(mode);m[RAM+0xc2+4]=0;m[RAM+0xc2+5]=bad
   if which:
    # Row3 second badge consumes the high WORD of D2, not a fresh BYTE ID.
    m[RAM+0xc2+5]=0;m[RAM+0xc2+3*6+4]=0;m[RAM+0xc2+3*6+5]=bad
    if bad==12:continue
   compare(m,0xa4d4,initial_regs(0xa4d4,mode),stop_pc=(0xa5ea,2)if which else(0x77e4,2));bounds+=1
  for mode in(0,1):
   m=seed(mode);base=RAM+(0x104 if mode else 0xc2)
   for row in range(3,11):m[base+row*6+4]=12
   compare(m,0xa4d4,initial_regs(0xa4d4,mode));count+=1;groups['supported-badge12-with-Royal-common12']=groups.get('supported-badge12-with-Royal-common12',0)+1
  for owner in(0,0xa4d5,0x7875,0xffffffff):
   m=seed();regs=initial_regs(0xa4d4,0);result=actual(m,owner,regs)
   assert result==(1,bytes(m[RAM:RAM+0x4000]),bytes(m[0xc0000:0xc4000]),regs,[],owner,0,owner)
   api_guards+=1
  invalid_calls=[]
  @Read
  def forbidden_read(*_):invalid_calls.append('ROM');return 1
  @Store
  def forbidden_store(*_):invalid_calls.append('FG');return 1
  for bad in('null-machine','null-ram','null-regs','invalid-regs','null-ROM','null-bus'):
   raw=ct.create_string_buffer(rng.randbytes(0x4000));shadow=Shadow((ct.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]),1)
   ma=Machine(ct.cast(raw,ct.POINTER(ct.c_uint8)),ct.pointer(shadow),(ct.c_uint32*7)(*[rng.getrandbits(32)for _ in range(7)]),None,None,0x123456)
   c=Context(ct.pointer(ma),forbidden_read,None,forbidden_store,0xabcdef,0xfedcba)
   if bad=='null-machine':c.machine=ct.POINTER(Machine)()
   elif bad=='null-ram':ma.ram=ct.POINTER(ct.c_uint8)()
   elif bad=='null-regs':ma.regs=ct.POINTER(Shadow)()
   elif bad=='invalid-regs':shadow.valid=0
   elif bad=='null-ROM':c.read_rom=Read()
   else:c.store_bus=Store()
   before=(bytes(c),bytes(ma),bytes(shadow),raw.raw)
   assert native.wf_r18_records_aux_call(ct.byref(c),0xa4d4)==2
   assert before==(bytes(c),bytes(ma),bytes(shadow),raw.raw)and not invalid_calls
   api_guards+=1
  assert native.wf_r18_records_aux_call(None,0xa4d4)==2;api_guards+=1
 required=set()
 from capstone import Cs,CS_ARCH_M68K,CS_MODE_BIG_ENDIAN,CS_MODE_M68K_000
 disasm=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
 for start,end in((0xa4d4,0xa608),(0x77d8,0x7814),(0x7874,0x7956)):
  required.update(i.address for i in disasm.disasm(rom[start:end],start))
 if not args.smoke:assert required<=coverage,('uncovered original instructions',list(map(hex,sorted(required-coverage))))
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 freeze_sha=None
 if args.freeze:
  args.freeze.parent.mkdir(parents=True,exist_ok=True)
  with zipfile.ZipFile(args.freeze,'w',zipfile.ZIP_DEFLATED)as archive:
   for name in('include','generated'):
    for f in sorted((frozen/name).rglob('*')):
     if f.is_file():archive.write(f,f.relative_to(frozen).as_posix())
   for name in('src/wf_r18_records_aux.c','src/wf_r17_m68k_shadow.c'):archive.write(frozen/name,name)
   archive.write(__file__,'tests/r18/records_aux_verify_rom.py')
  freeze_sha=hashlib.sha256(args.freeze.read_bytes()).hexdigest()
report={'status':'PASS','seed':hex(args.seed),'cases':count,'groups':groups,'callback_fault_prefixes':faults,'table_bound_prefixes':bounds,'API_guards':api_guards,'sources_sha256':sources,'test_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'frozen_sources_sha256':freeze_sha,'ROM_sha1':hashlib.sha1(rom).hexdigest(),'compared':['D0-D7','A0-A6','rawRAM16KiB','FG16KiB including padding','ordered real ROM-read and bus-store effects'],'ROM_intercepts':[],'coverage_pcs':list(map(hex,sorted(coverage))),'required_pcs':list(map(hex,sorted(required))),'limits':['MOVEM temporary stack/CCR outside machine ABI','Bounded portrait0..11 and badge0..12 tables; other identities retain read guards','TAG second identity retains original D2.W high byte; it is not masked or cleared','No hardware timing or whole-ranking/trajectory claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'whole ROM calls,',faults,'callback prefixes,',bounds,'table-bound prefixes,',api_guards,'API guards')
