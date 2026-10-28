"""Royal1775 raw01 jump128FA versus complete unchanged1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action01_snapshot_full_flow_1775.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(1775,7,1,0x1c03e,1004,1)
assert rom[0x128de:0x128fa]==bytes.fromhex('000128fa0001000500040008ff00ff000010008500ab00ac017a017a')
rng=random.Random(0x128fa);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot];t=actors[(slot+1)%9]
 put(m,o+0x20,0x8005);put(m,o+0x1c,0x8005);put(m,o+0x60,1);put(m,o+0x26,t,4);put(m,o+0x32,0,1)
 put(m,o+0x24,0);put(m,o+0x22,1);put(m,o+0x2e,0);put(m,o+0x36,0);put(m,RAM+0x161,1,1)
 for actor in(o,t):put(m,actor+6,0x280);put(m,actor+10,0x160)
 return m,o,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-Royal1775',m,7,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,selector,latch,phase,timer,face,flags32 in itertools.product((0,4,8),(1,0x4001,0x8001,0xff01),(5,0x8005),(0,1,2,3,4,0xfe,0x102,0x103,0x1fe),(0,1,0xff00,0xffff),(0,0x8000),(0,2)):
  m,o,t=seed(slot);put(m,o+0x60,selector);put(m,o+0x1c,latch);put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0x32,flags32,1)
  yield 'phase-width-latch-selector',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for previous,face in itertools.product((0,1,2,3,0xff,0x102),(0,0x8000)):
  m,o,t=seed();put(m,o+0x1c,5);put(m,o+0x1e,previous);put(m,o+0x2e,face)
  yield 'preset27-or28-priorBYTE',m,4,[rng.getrandbits(32)for _ in range(8)]
 for slot,relation,tag,x,tx,y,ty in itertools.product((0,4,8),('self','peer'),(0,0x40000000,0xff000000),(0,0x500),(0,0x280,0x800),(0,0x300),(0,0x160,0x500)):
  m,o,t=seed(slot);t=o if relation=='self'else t;put(m,o+0x1c,5);put(m,o+0x32,2,1);put(m,o+0x26,t|tag,4)
  put(m,o+6,x);put(m,o+10,y);put(m,t+6,tx);put(m,t+10,ty)
  yield 'trajectory26AE-alias-clamp',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for phase,height,f33,face,x,timer in itertools.product((2,0x102),(0x10f,0x110,0x111,0x14f,0x150,0x151,0xffff),(0,4),(0,0x8000),(0,0xffff),(0,0xff00)):
  m,o,t=seed();put(m,o+0x24,phase);put(m,o+14,height);put(m,o+0x33,f33,1);put(m,o+0x2e,face);put(m,o+6,x);put(m,o+0x22,timer)
  yield 'elevation-profile-offset-WORD-wrap',m,4,[rng.getrandbits(32)for _ in range(8)]
 for mode,contact,bits,phase,timer in itertools.product((0,1,2,3,0x82),(0,1,3,5),(0,1,2,3,0x10,0x13),(2,3,0xfe),(0,0xff00)):
  m,o,t=seed();put(m,RAM+0x161,mode,1);put(m,o+0x36,contact,1);put(m,o+0x37,bits,1);put(m,o+0x24,phase);put(m,o+0x22,timer)
  yield 'rope-before-profile-landing-priority',m,4,[rng.getrandbits(32)for _ in range(8)]
 for bad,phase,latch in itertools.product((0,0x80000000,0xdeadbeef),(0,2,3,0xfe),(5,0x8005)):
  m,o,t=seed();put(m,o+0x26,bad,4);put(m,o+0x24,phase);put(m,o+0x1c,latch)
  yield 'unused-link-never-blocks',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action01-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action01_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();manifest={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action01_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action01_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1
 guards=0
 if not args.natural_only:
  for bad in(0,0x80000000,0xdeadbeef):
   m,o,t=seed();put(m,o+0x1c,5);put(m,o+0x32,2,1);put(m,o+0x26,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.action01_oracle(b,d,4,ctypes.byref(nd))==-0x26ae;guards+=1
  m,o,t=seed();put(m,o+0x60,0xff);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action01_oracle(b,d,4,ctypes.byref(nd))==-0x12614;guards+=1
  m,o,t=seed();put(m,o+0x1c,5);put(m,o+0x32,2,1);put(m,o+6,0xffff);put(m,t+6,0);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action01_oracle(b,d,4,ctypes.byref(nd))==-0x26ae;guards+=1
  for pc in(0x128fa,0x1290a,0x12916,0x1292a,0x1293e,0x12952,0x1295a,0x1297a,0x12986,0x129a0,0x129aa,0x129cc,0x129e2,0x258e,0x26ae,0x10fc6,0x1110e,0x2052,0x1c0c8):assert pc in coverage,('missing ROM path',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'sources_sha256':manifest,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','unchangedRNG','rope globals','1670'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST projected Royal1775; no physicalSaturn RAM/timing equivalence claim','Existing26AE helper remains bounded to nonoverflow signed division; no helper altered','Consumed invalid trajectory links retain guard; unused links never block','Temporary ROM stack outsideRAM8KB; CCR/A registers and audioIO outside typed combat ABI']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw01 wholeROM calls without intercepts and',guards,'retained guards')
