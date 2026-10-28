"""Raw62 versus whole intact1C03E/19536/21732/10C60/11432/10D04."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'action62_manifest.json').read_text())
assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']=='a726c46f350723ea73158bc78055d17d418b14e2'
assert int.from_bytes(rom[0x12614+0x62*4:0x12618+0x62*4],'big')==0x19530
assert rom[0x19530:0x19536]==bytes.fromhex('000195360000') and rom[0x10cfc:0x10cff]==bytes((4,12,24))
rng=random.Random(0x195c2);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();groups={};count=0;sequence_quanta=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+0x20,0x8004);put(m,at+0x64,8)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0xb5):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x7a,actors[(i+2)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0)
 put(m,a+0x1c,5);put(m,a+0x20,0x8005);put(m,a+0x60,0x62);put(m,a+0x26,t,4);put(m,a+0x33,1,1);put(m,a+0xa3,1,1);put(m,a+0xaa,0x1234)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x161,0,1)
 return m,a,t,partner
def fixtures():
 for cap in meta['captures']:
  b=(HERE/cap['file']).read_bytes();assert hashlib.sha256(b).hexdigest()==cap['snapshot_sha256'];m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=b[32:]
  yield f'natural-HOST-policy{cap["policy"]:03}-frame{cap["frame"]}',m,cap['slot'],list(struct.unpack_from('=8I',b))
 if args.natural_only:return
 for slot,mode,credit,sel,tier in itertools.product((0,4,8),range(4),(0,0x8000),(0x62,0x4062,0x8062,0xff62),range(3)):
  m,a,t,pa=seed(slot);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x60,sel);put(m,a+0x70,tier)
  yield 'init-selector-lowBYTE-mode-credit-tier',m,slot,None
 for mode,credit,flag,cpu,pflags,rel,tier,tag in itertools.product((0,2),(0,1),(0,1),(0,1),(0,2),(0,0x80),range(3),(0,0x80,0xff)):
  m,a,t,pa=seed();put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x33,flag,1);put(m,pa+0x32,cpu,1);put(m,pa+0x33,pflags,1);put(m,pa+0x74,rel,1);put(m,pa+0x70,tier);put(m,a+0x86,pa|(tag<<24),4)
  yield '21732-TAG-literal-partner',m,0,None
 for mode,role,aa,fresh,flags in itertools.product(range(4),(0,0x40,0x80,0xc0,0xff),(0,1,2,0x4000,0x8000,0xffff),(0,1,0xff),(0,0x40,0x80,0xc0)):
  m,a,t,pa=seed();put(m,a+0x1c,0x8005);put(m,RAM+0x161,mode,1);put(m,a+0x56,role,1);put(m,a+0xaa,aa);put(m,a+0xa3,fresh,1);put(m,pa+0x34,flags,1)
  yield 'latched-11432-10D04-BYTE-WORD',m,0,None
 for fresh,health in itertools.product(range(256),(0,0xffff)):
  m,a,t,pa=seed();put(m,a+0x1c,0xff05);put(m,a+0xa3,fresh,1);put(m,a+0xaa,1);put(m,a+0x66,health)
  yield 'fresh-BYTE-any-nonzero-HP-no-5E-exception',m,0,None
 for mode,latch,role,credit,flag in itertools.product(range(4),(5,0x8005),(0,0x40,0x80),(0,1),(0,1)):
  if latch==5 and not(mode&1)and credit and flag:continue
  if latch==0x8005 and not(mode&1)and not(role&0x80):continue
  m,a,t,pa=seed();put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x1c,latch);put(m,a+0x56,role,1);put(m,a+0x33,flag,1);put(m,a+0x86,0xdeadbeef,4)
  yield 'lazy-partner-address',m,0,None
 for tag,dx,reject in itertools.product((0,0x40,0x80,0xff),(0,1,1023,1024,0xffff),('none','disabled','b5-3','b5-7','flags32-4','other-team')):
  m,a,t,pa=seed();put(m,RAM+0x161,1,1)
  for c in actors[4:]:put(m,c,0)
  c=actors[4];put(m,c,0x8000);put(m,c+6,0x250+dx);put(m,a+0x26,c|(tag<<24),4)
  if reject=='disabled':put(m,c,0)
  if reject=='b5-3':put(m,c+0xb5,8,1)
  if reject=='b5-7':put(m,c+0xb5,0x80,1)
  if reject=='flags32-4':put(m,c+0x32,0x10,1)
  if reject=='other-team':put(m,c+0x56,0x20,1)
  yield 'Royal2187E-rawLONG-exclusion-nearest',m,0,None
 for slot,tag,latch,mode,cpu in itertools.product(range(9),(0,0x80,0xff),(5,0x8005),(0,1),(0,1)):
  m,a,t,pa=seed(slot);put(m,a+0x1c,latch);put(m,RAM+0x161,mode,1);put(m,a+0x86,a|(tag<<24),4);put(m,a+0x32,cpu,1);put(m,a+0x34,0xc0,1);put(m,a+0x7a,a|(tag<<24),4)
  yield 'allslots-selfpartner-selfresume-tagged',m,slot,None
 for mode,role in itertools.product((0,1),range(256)):
  m,a,t,pa=seed();put(m,a+0x1c,0x8005);put(m,RAM+0x161,mode,1);put(m,a+0x56,role,1);put(m,a+0xaa,1);put(m,a+0x66,0)
  yield 'allROLE-BYTE-no-health-exception',m,0,None
 for mode,tier in itertools.product(range(4),range(3)):
  m,a,t,pa=seed();put(m,RAM+0x161,mode,1);put(m,a+0x70,tier);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(30):yield 'sequence-live-recovery',m,0,regs
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-action62-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'action62_oracle_bridge.c';shutil.copy2(HERE/bridge.name,bridge)
 sources={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()};source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 inc=frozen/'src/wf_r18_action62.inc';owner_sha=hashlib.sha256(inc.read_bytes()).hexdigest()if inc.exists()else None
 lib=frozen/'action.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge.name,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action62_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 def compare(name,m,slot,regs,status=0,stop=0x70000):
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot]);descriptor=0
  for _ in range(30000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==stop:break
   cpu.execute(1)
  assert cpu.r_pc()==stop,(name,'ROM runaway',hex(cpu.r_pc()),hex(stop))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();got=native.action62_oracle(b,d,slot,ctypes.byref(nd));assert got==status,(name,count,'native status',hex(got),hex(status))
  if not status:assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   actual=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert actual==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(actual,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  return b.raw,list(d)
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);result,after=compare(name,m,slot,regs or[rng.getrandbits(32)for _ in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
  if name=='sequence-live-recovery':m[RAM:RAM+0x4000]=result[RAM:RAM+0x4000];regs[:]=after;sequence_quanta+=1
 guards=0
 if not args.natural_only:
  for bad in(3,0xff,0x100,0x8000,0xffff):
   m,a,t,pa=seed();put(m,a+0x70,bad);compare('10C60-table-prefix',m,0,[rng.getrandbits(32)for _ in range(8)],-0x10ca8,0x10ca8);guards+=1
   m,a,t,pa=seed();put(m,pa+0x32,1,1);put(m,pa+0x70,bad);compare('217C8-partner-table-prefix',m,0,[rng.getrandbits(32)for _ in range(8)],-0x217c8,0x217c8);guards+=1
  for bad in(0,0x80000000,0xdeadbeef,0x1c05b1):
   for route,pc in(('init',0x21756),('latched',0x11448),('resume',0x11462)):
    m,a,t,pa=seed()
    if route!='init':put(m,a+0x1c,0x8005)
    if route=='resume':put(m,pa+0x34,0xc0,1);put(m,pa+0x7a,bad,4)
    else:put(m,a+0x86,bad,4)
    compare(route+'-pointer-prefix',m,0,[rng.getrandbits(32)for _ in range(8)],-pc,pc);guards+=1
  m,a,t,pa=seed();put(m,a+0x60,0x35);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action62_oracle(b,d,0,ctypes.byref(nd))==-0x12614;guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x19536,0x19540,0x195c2,0x195ca,0x195ec,0x19600,0x21732,0x21756,0x2179c,0x217c8,0x217fe,0x2187e,0x10c60,0x10ca8,0x11432,0x11462,0x10d04,0x10d2e,0x1c0c8)
if not args.natural_only:assert all(x in coverage for x in required),('missing branches',[hex(x)for x in required if x not in coverage])
if owner_sha:assert hashlib.sha256((ROOT/'src/wf_r18_action62.inc').read_bytes()).hexdigest()==owner_sha
report={'status':'PASS','cases':count,'groups':groups,'guards':guards,'ROM_prefix_checks':max(0,guards-1),'sequence_quanta':sequence_quanta,'source_sha256':source_sha,'raw62_include_sha256':owner_sha,'sources_sha256':sources,'capture_manifest_sha256':hashlib.sha256((HERE/'action62_manifest.json').read_bytes()).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7','RNG/globals','descriptor19530'],'intercepts':[],'required_ROM_pcs':list(map(hex,required)),'coverage_pcs':list(map(hex,sorted(coverage))),'limits':['Three exact HOST raw62 guard fixtures, all9actors/D8; unrepresented arcadeRAMzero','New include/raw62 admission only; unsupported selectors/pointers/tables retain guards','Original stack/A/CCR, physical timing and complete trajectories outside typed ABI']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'whole intact1C03E calls,',guards,'guards,',sequence_quanta,'sequence quanta')
