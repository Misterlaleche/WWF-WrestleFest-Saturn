"""Raw02 natural full-flow810 and isolated action calls against intact ROM."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action02_snapshot_full_flow_810.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(810,1,1,0x1c03e,247,1)
assert rom[0x12614+2*4:0x12618+2*4]==bytes.fromhex('000129f0')
assert rom[0x129f0:0x12a08]==bytes.fromhex('00012a0800010004000c000cff0000080142014300ac017a')
rng=random.Random(0x12a08);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 o=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 m[o:o+0x10c]=capture[56+0x6bc:56+0x7c8]
 put(m,o+0x20,0x8005);put(m,o+0x1c,0x8005);put(m,o+0x60,2);put(m,o+2,7)
 put(m,o+0x26,t,4);put(m,t+0x86,partner,4);put(m,partner+0x20,1);put(m,partner+0xae,1)
 put(m,RAM+0x161,0,1);put(m,RAM+0x7c,0x8000)
 return m,o,t,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-full-flow810',m,1,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,selector,latch,phase,timer,contact in itertools.product((0,4,8),(2,0x4002,0x8002,0xff02),(0,1),(0,1,2,3,0xfe,0x100,0x101,0x1fe),(0,1,0xff00,0xffff),(0,0x10)):
  m,o,t,partner=seed(slot);put(m,o+0x60,selector);put(m,o+0x1c,0x8005 if latch else 5)
  put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x37,contact,1)
  yield 'phase-contact-priority',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for slot,face,mode,partner_state,partner_ae in itertools.product((0,4,8),(0,0x8000,0xffff),range(4),(0,1,2,0x8001),(0,1,2,0x1001)):
  m,o,t,partner=seed(slot);put(m,o+0x24,1);put(m,o+0x22,0);put(m,o+0x2e,face)
  put(m,RAM+0x161,mode,1);put(m,partner+0x20,partner_state);put(m,partner+0xae,partner_ae)
  yield 'impact-partner-mode',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for slot,char,credited,f33 in itertools.product((0,4,8),range(12),(0,1,0x8000),(0,2,4,0xff)):
  m,o,t,partner=seed(slot);put(m,o+0x24,0xfe);put(m,o+0x22,0);put(m,o+0x37,0,1)
  put(m,o+2,char);put(m,RAM+0x7c,credited);put(m,o+0x33,f33,1)
  yield 'finish-rng',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for slot,face,x,y in itertools.product((0,4,8),(0,0x8000),(0,0x180,0x200,0x2f8,0x400,0xffff),(0,0x120,0x160,0x1a0,0xffff)):
  m,o,t,partner=seed(slot);put(m,o+0x1c,5);put(m,o+0x2e,face);put(m,o+6,x);put(m,o+10,y)
  yield 'init-collision-offset',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for tag,phase in itertools.product((0,0x40000000,0x80000000,0xff000000),(0,1,2,0xfe)):
  m,o,t,partner=seed(4);put(m,o+0x26,t|tag,4);put(m,o+0x24,phase);put(m,o+0x22,0)
  put(m,o+0x1c,5 if phase==0 else 0x8005);put(m,o+0x37,0,1)
  yield 'tagged-link',m,4,[rng.getrandbits(32)for _ in range(8)]
 for link,phase in itertools.product((0,0x80000000,0xdeadbeef),(2,0xfe)):
  m,o,t,partner=seed(4);put(m,o+0x26,link,4);put(m,o+0x24,phase);put(m,o+0x22,1);put(m,o+0x37,0,1)
  yield 'unconsumed-link',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action02-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action02_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 frozen_sources={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action02_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action02_oracle(b,d,slot,ctypes.byref(nd))
  assert st==0,(name,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',[hex(x)for x in d],[hex(cpu.r_reg(i))for i in range(8)])
  count+=1
 guards=0
 if not a.natural_only:
  for kind,dependency in [('init-link',0x12a14),('impact-link',0x10ba2),('impact-partner',0x110ee),('unowned35',0x12614)]:
   m,o,t,partner=seed(4);put(m,o+0x37,0,1)
   if kind=='init-link':put(m,o+0x1c,5);put(m,o+0x26,0x123456,4)
   if kind in ('impact-link','impact-partner'):
    put(m,o+0x24,1);put(m,o+0x22,0)
   if kind=='impact-link':put(m,o+0x26,0x123456,4)
   if kind=='impact-partner':put(m,t+0x86,0x123456,4)
   if kind=='unowned35':put(m,o+0x60,0x4035)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
   st=native.action02_oracle(b,d,4,ctypes.byref(nd));assert st==-dependency,(kind,hex(-st),hex(dependency));guards+=1
  for pc in (0x12a08,0x12a10,0x12a3a,0x12a76,0x12a9e,0x12aa6,0x12ac4,0x12acc,0x12ae0,0x10b62,0x10b9a,0x10bd0,0x1108c,0x110e0,0x1110e,0x258e,0x2052,0x1c0c8):
   assert pc in coverage,('missing ROM path',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'sources_sha256':frozen_sources,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG','tick descriptor'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend manualTAG0/7, frame810 partnerID7 raw02','Valid consumed actor links only; invalid consumed links fail at original dereference PC, unused links never block','Sound2052 original ROM executes; native existing sound output device is silent, IO command output outside comparison','CCR/A registers remain outside typed combat ABI','No physical Saturn timing/RAM claim']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw02 whole ROM calls without intercepts')
