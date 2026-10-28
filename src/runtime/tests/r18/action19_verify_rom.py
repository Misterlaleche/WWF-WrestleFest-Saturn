"""Raw19 naturalRoyal1500, throw/RNG/rope/descriptor owner15006 vs whole1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action19_snapshot_full_flow_1500.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(1500,6,1,0x1c03e,866,1)
assert rom[0x12614+0x19*4:0x12618+0x19*4]==bytes.fromhex('00014fb6')
rng=random.Random(0x15006);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=6):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];o=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 m[o:o+0x10c]=capture[56+0xbf8:56+0xd04]
 put(m,o+0x20,0x8005);put(m,o+0x1c,0x8005);put(m,o+0x60,0x19);put(m,o+0x26,t,4);put(m,o+0x7a,t,4);put(m,o+2,6)
 put(m,o+0x24,0);put(m,o+0x22,1);put(m,o+0x37,0,1);put(m,o+0x56,0x80,1)
 put(m,t+0x86,partner,4);put(m,partner+0x20,1);put(m,partner+0xae,1)
 put(m,RAM+0x161,1,1);put(m,RAM+0x162,0);put(m,RAM+0x7e,0);return m,o,t,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-Royal1500',m,6,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,selector,latch,phase,timer,face in itertools.product((0,4,8),(0x19,0x4019,0x8019,0xff19),(5,0x8005),(0,1,2,3,4,5,6,7,0xfe,0x103,0x105,0x1fe),(0,1,0xff00),(0,0x8000)):
  m,o,t,partner=seed(slot);put(m,o+0x60,selector);put(m,o+0x1c,latch);put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2e,face)
  yield 'phase-latch-descriptor',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for mode,counter,ownrole,linkedrole,progress in itertools.product(range(4),(0,1,3,4,0xff,0x100,0xffff),(0,0x80),(0,0x80),range(10)):
  m,o,t,partner=seed();put(m,o+0x1c,5);put(m,o+0xf4,counter);put(m,o+0x56,ownrole,1);put(m,t+0x56,linkedrole,1);put(m,RAM+0x161,mode,1);put(m,RAM+0x162,progress)
  yield 'init1129E-live-RNG-counter',m,6,[rng.getrandbits(32)for _ in range(8)]
 for scene,x,y,face,mode in itertools.product(range(7),(0,0x180,0x220,0x2e0,0x400,0xffff),(0x118,0x160,0x198),(0,0x8000),(0,2)):
  m,o,t,partner=seed();put(m,o+0x24,3);put(m,o+0x22,0);put(m,o+6,x);put(m,o+10,y);put(m,o+14,0x140);put(m,o+0x2e,face);put(m,o+0x32,0)
  put(m,RAM+0x7e,scene);put(m,RAM+0x161,mode,1)
  yield 'resolver-wall-lock-rope',m,6,[rng.getrandbits(32)for _ in range(8)]
 for phase,credited,char,f33,tagged in itertools.product((5,0xfe),(0,1,0x8000),range(12),(0,2),(False,True)):
  m,o,t,partner=seed();put(m,o+0x24,phase);put(m,o+0x22,0);put(m,o+2,char);put(m,o+0x33,f33,1);put(m,RAM+0x7c,credited);put(m,o+0x60,0x8019 if tagged else 0x19)
  yield 'release-finish-RNG',m,6,[rng.getrandbits(32)for _ in range(8)]
 for tag,phase in itertools.product((0,0x40000000,0x80000000,0xff000000),(0,5,0xfe)):
  m,o,t,partner=seed();put(m,o+0x26,t|tag,4);put(m,o+0x24,phase);put(m,o+0x22,0)
  if phase==0:put(m,o+0x1c,5);put(m,o+0x56,0,1)
  yield '24-bit-link-alias',m,6,[rng.getrandbits(32)for _ in range(8)]
 for phase,timer,contact,selector,mode in itertools.product((2,3,5,6,0xfe,0x105,0x1fe),(0,1,0xff00),(0,0x10,0xff),(0x19,0x8019),(0,1)):
  m,o,t,partner=seed();put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x37,contact,1);put(m,o+0x60,selector);put(m,RAM+0x161,mode,1)
  yield 'landing-clears-timer-before-release',m,6,[rng.getrandbits(32)for _ in range(8)]
 for link,phase in itertools.product((0,0x80000000,0xdeadbeef),(1,6,0xfe)):
  m,o,t,partner=seed();put(m,o+0x26,link,4);put(m,o+0x24,phase);put(m,o+0x22,1)
  yield 'unused-link-never-blocks',m,6,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action19-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action19_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 frozen_sources={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action19_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000)
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action19_oracle(b,d,slot,ctypes.byref(nd))
  assert st==0,(name,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',[hex(x)for x in d],[hex(cpu.r_reg(i))for i in range(8)])
  count+=1
 guards=0
 if not a.natural_only:
  for kind,dependency in [('init-link',0x15012),('wall-link',0x150dc),('release-link',0x1514e),('release-partner',0x110ee),('landing-character',0x110b6),('rng-progress',0x113cd),('scene7',0x280dc),('selector',0x12614)]:
   m,o,t,partner=seed();put(m,o+0x22,0)
   if kind=='init-link':put(m,o+0x1c,5);put(m,o+0x26,0x40123456,4)
   if kind in ('wall-link','scene7'):
    put(m,o+0x24,3);put(m,RAM+0x161,0,1);put(m,RAM+0x7e,7 if kind=='scene7' else 2);put(m,o+0x32,0);put(m,o+6,0);put(m,o+10,0x160)
   if kind=='wall-link':put(m,o+0x26,0x80123456,4)
   if kind in ('release-link','release-partner'):put(m,o+0x24,5)
   if kind=='release-link':put(m,o+0x26,0xff123456,4)
   if kind=='release-partner':put(m,RAM+0x161,0,1);put(m,t+0x86,0x40123456,4)
   if kind=='landing-character':put(m,o+0x24,0xfe);put(m,o+2,0x100);put(m,RAM+0x7c,1);put(m,o+0x33,0,1)
   if kind=='rng-progress':put(m,o+0x1c,5);put(m,o+0x56,0,1);put(m,t+0x56,0x80,1);put(m,RAM+0x161,0,1);put(m,RAM+0x162,10)
   if kind=='selector':put(m,o+0x60,0xff)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
   st=native.action19_oracle(b,d,6,ctypes.byref(nd));assert st==-dependency,(kind,hex(-st),hex(dependency));guards+=1
  for pc in (0x15012,0x15038,0x15048,0x15076,0x1508a,0x150b6,0x150dc,0x15122,0x1514e,0x15186,0x151c2,0x151dc,0x151fc,0x1129e,0x11058,0x1108c,0x110e0,0x1110e,0x21b4,0x2052,0x1c0c8):assert pc in coverage,('missing ROM path',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'sources_sha256':frozen_sources,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG','tick descriptor','rope/shake/audio-selector globals'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend Royal1500; no physicalSaturnRAM/timing claim','Raw19 only, sound MMIO outside native output comparison','Consumed pointers resolve through24-bit bus; invalid consumed links and table indexes retain guards; unused links never block','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw19 whole ROM calls without intercepts')
