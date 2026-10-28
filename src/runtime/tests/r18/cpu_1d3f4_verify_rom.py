"""Physical-null LINK in CPU1D398 versus whole intact1C8F4, with prefix guards."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'cpu_1d3f4_manifest.json').read_text());capture=(HERE/'cpu_1d3f4_snapshot_host284.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
assert rom[0x21]==0xff
weights=[[25,15,60],[30,20,50],[30,20,50],[35,25,40]]+[[40,30,30]]*5+[[45,35,20]]
assert [list(rom[0x1d4fc+i*4:0x1d500+i*4])for i in range(10)]==[x+[0]for x in weights]
rng=random.Random(0x1d3f4);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();groups={};count=0;header_reads=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];partner=actors[(slot+1)%9];target=actors[(slot+2)%9]
 put(m,a+0x20,0x8001);put(m,a+0xaf,1,1);put(m,a+0x86,partner,4);put(m,partner+0x26,0,4);put(m,partner+0x7a,target,4)
 put(m,a+6,0x200);put(m,a+10,0x140);put(m,partner+10,0x141);put(m,target+6,0x280);put(m,target+10,0x140);put(m,target+0x20,0x8000)
 put(m,a+0x33,0,1);put(m,a+0x34,0,1);put(m,a+0xb7,0,1);put(m,RAM+0x7c,0x8000);put(m,RAM+0x162,0);put(m,RAM+0x161,0,1)
 return m,a,partner,target
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[32:]
 yield 'natural-HOST284-actor1-tagged-null',m,1,list(struct.unpack_from('=8I',capture))
 if args.natural_only:return
 for slot,tag,side,dy in itertools.product(range(9),(0,1,0x7f,0x80,0xff),(0,0x80),(0,1,7,8,255,256,263,264,0xffff)):
  m,a,pa,t=seed(slot);put(m,pa+0x26,tag<<24,4);put(m,pa+10,0x140+dy);put(m,a+0x33,side,1)
  yield 'null24bit-Y-lowBYTE-threshold-side',m,slot,None
 for state,sel,dx,tag in itertools.product((4,5,0x85),(0x52,0x53,0x153,0x8053),(0,63,64,65,0xffff),(0,0x80,0xff)):
  m,a,pa,t=seed();put(m,pa+0x26,t|(tag<<24),4);put(m,t+0x20,0x8000|state);put(m,t+0x60,sel);put(m,t+6,0x200+dx)
  yield 'linked-state-selector-BYTE-X-WORD',m,1,None
 for ay,py,side,credit,flag in itertools.product((0,1,0x8000,0xffff),(0,1,0x8000,0xffff),(0,0x80),(0,1),(0,1)):
  m,a,pa,t=seed();put(m,a+10,ay);put(m,pa+10,py);put(m,a+0x33,side,1);put(m,a+0x34,flag,1);put(m,RAM+0x7c,credit);put(m,pa+0x26,0xff000000,4)
  yield 'absolute-WORD-wrap-unsigned-and-BYTE-compare',m,1,None
 for state,sub,dy,dx,b7 in itertools.product((0,1,4,5,0x80,0x81,0x84),(0,1,10,11,0x109),(0,15,16),(0,63,64),(2,0xff)):
  m,a,pa,t=seed();put(m,t+0x20,0x8000|state);put(m,t+0x64,sub);put(m,t+10,0x140+dy);put(m,t+6,0x200+dx);put(m,a+0xb7,b7,1)
  yield 'target-pursuit-BYTE-state-subtype-latched-B7',m,1,None
 for prog,seedvalue,tag in itertools.product(range(10),(0,1,0x80000000,0xffffffff,0x12345678),(0,0x80,0xff)):
  m,a,pa,t=seed();put(m,pa+0x7a,t|(tag<<24),4);put(m,t+6,0x210);put(m,t+0x26,actors[8],4);put(m,RAM+0x162,prog);put(m,RAM+0x5c,seedvalue,4)
  # All weighted outcomes are executed; a linked target returns choice1
  # before its separately owned state5 callback, while choice0 calls39.
  yield 'live-weighted-RNG-table-and-choice0-state5',m,1,None
 for prog,seedvalue,linkword,tag in itertools.product(range(10),(0,1,0xffffffff),(0,0x80000000),(0,0x80,0xff)):
  m,a,pa,t=seed();put(m,a+0x7a,t|(tag<<24),4);put(m,t+6,0x210);put(m,t+0x26,linkword,4);put(m,RAM+0x162,prog);put(m,RAM+0x5c,seedvalue,4)
  yield 'choice1-original-LONG-null-test-and-state5-1C',m,1,None
 for tag in range(256):
  m,a,pa,t=seed();put(m,a+0x86,pa|(tag<<24),4);put(m,pa+0x26,tag<<24,4)
  yield 'all256-partner-and-null-tags',m,1,None
 for high in range(128):
  m,a,pa,t=seed();put(m,a+0x20,(high<<8)|1);put(m,a+0x86,0xdeadbeef,4)
  yield 'owner-highBYTE-bit7-inactive-lazy',m,1,None
 for dy,flag,credit in itertools.product((0,7,8,256),(0,1),(0,1)):
  m,a,pa,t=seed();put(m,pa+10,0x140+dy);put(m,a+0x34,flag,1);put(m,RAM+0x7c,credit)
  if dy in(0,7,256)and flag==0:put(m,pa+0x26,0,4)
  else:put(m,pa+0x26,0xdeadbeef,4)
  if not credit:put(m,pa+0x7a,0xdeadbeef,4)
  yield 'lazy-link-target-after-Y-flag-credit-gates',m,1,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-cpu1d3f4-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'cpu_1d3f4_oracle_bridge.c';shutil.copy2(HERE/bridge.name,bridge)
 sources={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 source_sha=hashlib.sha256((frozen/'src/wf_cpu_runtime.c').read_bytes()).hexdigest()
 lib=frozen/'cpu.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge.name,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.cpu_1d3f4_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 def compare(name,m,slot,regs,status=0,stop=0x70000,valid=3):
  global header_reads
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c8f4)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot])
  for _ in range(40000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1d3f8 and(cpu.r_reg(10)&0xffffff)==0:assert mem.r8(0x21)==0xff;header_reads+=1
   if pc==stop:break
   cpu.execute(1)
  assert cpu.r_pc()==stop,(name,'ROM runaway',hex(cpu.r_pc()),hex(stop))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);got=native.cpu_1d3f4_oracle(b,d,slot,valid);assert got==status,(name,count,'native status',hex(got),hex(status))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   actual=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert actual==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(actual,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);compare(name,m,slot,regs or[rng.getrandbits(32)for _ in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for bad in(1,0x100,0x7ffff,0x1bffff,0x1c05b1,0x1c2000,0xdeadbeef):
   m,a,pa,t=seed();put(m,pa+0x26,bad,4);compare('unsupported-LINK-prefix',m,1,[rng.getrandbits(32)for _ in range(8)],-0x1d3f4,0x1d3f4);guards+=1
  for field,pc in((0x86,0x1d3aa),(0x7a,0x1d436)):
   for bad in(0,0x80000000,0xdeadbeef):
    m,a,pa,t=seed();put(m,(a if field==0x86 else pa)+field,bad,4);compare('existing-partner-target-prefix',m,1,[rng.getrandbits(32)for _ in range(8)],-pc,pc);guards+=1
  for bad in(10,0xffff):
   m,a,pa,t=seed();put(m,t+6,0x210);put(m,RAM+0x162,bad);compare('progression-table-prefix',m,1,[rng.getrandbits(32)for _ in range(8)],-0x1d4a0,0x1d4a0);guards+=1
  for prog in range(10):
   m,a,pa,t=seed();put(m,t+6,0x210);put(m,RAM+0x162,prog);compare('missing-RNG-prefix',m,1,[rng.getrandbits(32)for _ in range(8)],-0x24cc,0x24cc,1);guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1d398,0x1d3a0,0x1d3a2,0x1d3bc,0x1d3d6,0x1d3de,0x1d3f4,0x1d3f8,0x1d400,0x1d41a,0x1d428,0x1d436,0x1d46c,0x1d47e,0x1d490,0x1d4aa,0x1d4be,0x1d4d2,0x1d4d8,0x1d4f4,0x1e792,0x24cc,0x21b4)
if not args.natural_only:assert all(x in coverage for x in required),('missing branches',[hex(x)for x in required if x not in coverage])
assert hashlib.sha256((ROOT/'src/wf_cpu_runtime.c').read_bytes()).hexdigest()==source_sha
report={'status':'PASS','cases':count,'groups':groups,'guards_and_ROM_prefixes':guards,'real_ROM21_reads':header_reads,'source_sha256':source_sha,'sources_sha256':sources,'snapshot_sha256':meta['snapshot_sha256'],'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals'],'intercepts':[],'required_ROM_pcs':list(map(hex,required)),'coverage_pcs':list(map(hex,sorted(coverage))),'limits':['Natural HOST284 guard-state replay via whole1C8F4, not reconstructed earlier PC state','Only physical null/actor links supported; other LINK addresses retain1D3F4','Other unrepresented partner/target/table dependencies remain guarded','Stack/A/CCR, native timing and complete trajectory equivalence outside typed ABI']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'whole intact1C8F4 calls,',guards,'ROM guard prefixes')
