"""Raw1F release callback versus complete intact $1C03E; no intercepts."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'action1f_manifest.json').read_bytes());assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12690:0x12694]==bytes.fromhex('000159f8')
assert rom[0x1268c:0x12690]==rom[0x12690:0x12694]
assert rom[0x159f8:0x15a20]==bytes.fromhex('00015a200001000300080008000800740075007600015a20000100030008000800080079007a007b')
capture=(HERE/'action1f_snapshot_tag_pressure_3747.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
alias_capture=(HERE/'action1f_snapshot_tag_pressure_4189_raw1e.bin').read_bytes();assert hashlib.sha256(alias_capture).hexdigest()==meta['alias1e']['snapshot_sha256']
rng=random.Random(0x15a20);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9]
 for i,o in enumerate(actors):put(m,o+2,i);put(m,o+0x26,actors[(i+1)%9],4)
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x1f);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x74,0);put(m,a+0x76,RAM+0xf1c,4);put(m,a+0x92,0,4)
 return m,a,t
def base_fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-TAG-pressure3747-actor5',m,5,struct.unpack_from('=8I',capture,24)
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=alias_capture[56:8248]
 yield 'natural-TAG-pressure4189-actor1-raw1E',m,1,struct.unpack_from('=8I',alias_capture,24)
 if args.natural_only:return
 for ident,alternate,frame,face in itertools.product(range(12),(0,1),(0,1,2),(0,0x8000)):
  slot=ident%9;m,a,t=seed(slot);put(m,a+2,ident);put(m,a+0x75,alternate,1)
  put(m,a+0x24,frame);put(m,a+0x22,1);put(m,a+0x2e,face)
  yield 'all-character-descriptor',m,slot,None
 for slot,flags,face,selector in itertools.product((0,5,8),(0,1,0x80,0x8000,0x8001,0xffff),(0,0x8000,0xff,0xffff),(0x1f,0x401f,0x801f,0xff1f)):
  m,a,t=seed(slot);put(m,a+0x1c,5);put(m,a+0x74,flags);put(m,a+0x2e,face);put(m,a+0x60,selector);put(m,a+0x76,0xdeadbeef,4)
  yield 'init-descriptor-BYTE75-vs-WORD74',m,slot,None
 for slot,phase,timer,flags,pending in itertools.product((0,5,8),(0,1,2,0xfe,0xff,0x100,0x101,0x1fe,0xff01),(0,1,0xff00,0xffff),(0,1,0x80,0x100,0x8000,0xffff),(0,0x80000000)):
  m,a,t=seed(slot);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x74,flags);put(m,a+0x92,pending,4)
  yield 'phase-LOW-timer-LONG92-WORD74',m,slot,None
 for slot,route,relation,tag,face,xyz in itertools.product((0,5,8),('phase1','terminal'),('object0','object1','peer','self'),(0,0x40000000,0x80000000,0xff000000),(0,0x8000,0xff,0xffff),((0,0,0),(0xffff,0xffff,0xffff),(0xffa0,0x1234,0xffc0),(0x60,0xfff0,0xfff0))):
  m,a,t=seed(slot);o={'object0':RAM+0xf1c,'object1':RAM+0x1028,'peer':t,'self':a}[relation]
  put(m,a+0x76,o|tag,4);put(m,a+0x24,1 if route=='phase1'else 0x1fe);put(m,a+0x22,0);put(m,a+0x74,0x8001);put(m,a+0x92,0x12345678,4);put(m,a+0x2e,face)
  for at,v in zip((6,10,14),xyz):put(m,a+at,v)
  put(m,a+0x26,t|0x40000000,4)
  yield 'release-object-actor-self-alias-WORD-wrap',m,slot,None
 for bad,latch,phase,timer,flags,pending in itertools.product((0,0x80000000,0xdeadbeef),(5,0x8005),(0,1,0xfe),(0,1),(0,1),(0,1)):
  if latch==0x8005 and((phase==1 and timer==0 and pending)or(phase==0xfe and flags)):continue
  m,a,t=seed();put(m,a+0x76,bad,4);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x74,flags);put(m,a+0x92,pending,4)
  yield 'unused-invalid-object-link',m,5,None
 for slot,relation,flags,signal in itertools.product((0,8),('object0','object1','peer','self'),(0,1),(False,True)):
  m,a,t=seed(slot);put(m,a+0x1c,5);put(m,a+0x76,{'object0':RAM+0xf1c,'object1':RAM+0x1028,'peer':t,'self':a}[relation],4);put(m,a+0x74,flags)
  regs=[rng.getrandbits(32)for _ in range(8)]
  for step in range(40):
   if signal and step==8:put(m,a+0x92,1,4)
   yield 'continuous-init-phase1-terminal40calls',m,slot,regs
   m=bytearray(b.raw[:0x200000]);regs=list(d)
   # The dispatcher takes this callback only while the caller's state is5.
   # Stop at the owner transition rather than inventing a new state0 caller.
   if int.from_bytes(m[a+0x20:a+0x22],'big')&255!=5:break
def fixtures():
 for name,m,slot,regs in base_fixtures():
  if name.startswith('natural-'):yield name,m,slot,regs;continue
  for sel in(0x1f,0x1e):
   clone=bytearray(m);at=actors[slot]+0x61;clone[at]=sel
   yield name+'-raw'+format(sel,'02X'),clone,slot,regs
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action1f-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge='tests/r18/action1f_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  if b'/* $12614[$1F]'not in data:return None
  start=data.index(b'/* $12614[$1F]');end=data.index(b'static const desc_t D_15B24=',start)
  return hashlib.sha256(data[start:end]+b''.join(re.findall(rb'^[ \t]*case 0x1[ef]u:[^\n]*',data,re.M))).hexdigest()
 own=owner_hash(source);source_map={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action1f_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot]);descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,count,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action1f_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,count,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D8');count+=1;groups[name]=groups.get(name,0)+1
 # Continue each real TAG-pressure capture through release and return to idle,
 # carrying RAM/foreground/D-registers forward after every owner tick.
 def continue_natural(data,slot,label):
  global count
  m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=data[56:8248]
  m[0xc0000:0xc4000]=rng.randbytes(0x4000)
  a=actors[slot];regs=list(struct.unpack_from('=8I',data,24));steps=0
  for tick in range(80):
   name=f'natural-continuation-{label}-{tick}';mem.w_block(0,bytes(m))
   for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
   cpu.w_reg(8,a);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
   descriptor=0
   for _ in range(20000):
    pc=cpu.r_pc();coverage.add(pc)
    if pc==0x1c0c8:descriptor=cpu.r_reg(9)
    if pc==0x70000:break
    cpu.execute(1)
   assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32()
   st=native.action1f_oracle(b,d,slot,ctypes.byref(nd))
   assert st==0 and nd.value==descriptor,(name,'native status/descriptor',st,hex(nd.value),hex(descriptor))
   for at,n in((RAM,0x4000),(0xc0000,0x4000)):
    expected=bytes(mem.r_block(at,n));assert b.raw[at:at+n]==expected,(name,'RAM/FG',hex(at))
    m[at:at+n]=expected
   regs=[cpu.r_reg(i)for i in range(8)]
   assert list(d)==regs,(name,'D0-D7')
   count+=1;steps+=1
   if int.from_bytes(m[a+0x20:a+0x22],'big')&0xff!=5:break
  assert steps==29 and (int.from_bytes(m[a+0x20:a+0x22],'big')&0xff)==0,(label,steps)
  groups[f'natural-continuation-{label}']=steps
  return steps
 continuations=continue_natural(capture,5,'raw1F')+continue_natural(alias_capture,1,'raw1E')
 guards=0;guard_prefixes=0
 if not args.natural_only:
  for selector,route,bad in itertools.product((0x1e,0x1f),('phase1','terminal'),(0,0x80000000,0xdeadbeef,RAM+0x5b1)):
   m,a,t=seed();put(m,a+0x60,selector);put(m,a+0x24,1 if route=='phase1'else 0xfe);put(m,a+0x22,0);put(m,a+0x92,1,4);put(m,a+0x74,0x8001);put(m,a+0x76,bad,4);boundary=0x15a7e if route=='phase1'else 0x15ac6
   mem.w_block(0,bytes(m));cpu.w_pc(0x1c03e);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000)
   for i in range(15):cpu.w_reg(i,i if i<8 else 0)
   cpu.w_reg(8,a)
   for _ in range(2000):
    if cpu.r_pc()==boundary:break
    cpu.execute(1)
   assert cpu.r_pc()==boundary,'Original guard boundary not reached'
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.action1f_oracle(b,d,5,ctypes.byref(nd))==-boundary;guards+=1
   assert b.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),'Guard lost writes before consumed pointer';guard_prefixes+=1
  # $35 is now implemented; use selector $8F (outside the 143-entry table)
  # to keep this bounded-dispatch guard meaningful.
  for sel in(0x8f,0xff8f):
   m,a,t=seed();put(m,a+0x60,sel);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action1f_oracle(b,d,5,ctypes.byref(nd))==-0x12614;guards+=1
  required={0x15a20,0x15a28,0x15a3a,0x15a46,0x15a5a,0x15a7e,0x15a96,0x15a98,0x15abe,0x15ac6,0x15ae2,0x15ae4,0x15b0c,0x15b14,0x15b1c,0x15b22,0x1c0c8}
  assert required<=coverage,('missing original sites',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup();assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==own,'Raw1F changed during oracle'
 report={'status':'PASS','cases':count,'guards':guards,'natural_continuations':continuations,'RAM_guard_prefix_checks':guard_prefixes,'groups':groups,'raw1f_owner_sha256':own,'source_sha256':source_hash,'sources_sha256':source_map,'snapshot_sha256':meta['snapshot_sha256'],'alias1e_snapshot_sha256':meta['alias1e']['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'intercepts':[],'coverage':[hex(x)for x in sorted(coverage)],'compared':['RAM16KB including both fixed objects','FG16KB','D0-D7','RNG unchanged','descriptor'],'limits':['HOST pressure fixtures projected from preRAM8KB only; unrepresented originalRAM zero','Consumed +76 outside aligned represented actors/fixedobjects retains precise guard and original RAM prefix','Raw1E shares the exact same ROM table entry, descriptors and callback; no shared helpers changed','CCR/A-register/soundIO outside typed ABI; temporary stack outsideRAM16KB','No full combat or physical timing equivalence claim']}
 report['verifier_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS',count,'raw1F intact1C03E calls and',guards,'guards; no intercepts')
 if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
