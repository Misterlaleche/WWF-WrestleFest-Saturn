"""Original Royal retire7A submachine1A364, natural humanHP0 frame2276."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action7a_snapshot_full_flow_2276.bin').read_bytes();assert struct.unpack_from('=6I',capture)==(2276,0,1,0x1c03e,1213,1)
assert len(capture)==16472 and rom[0x12614+0x7a*4:0x12618+0x7a*4]==bytes.fromhex('0001a348')
rng=random.Random(0x1a364);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot];t=actors[(slot+1)%9]
 put(m,o+0x20,0x8005);put(m,o+0x1c,0x8005);put(m,o+0x60,0x7a);put(m,o+2,slot);put(m,o+0x7a,t,4)
 put(m,o+0x44,0);put(m,o+0x24,0);put(m,o+0x22,1);put(m,o+0x34,0,1);return m,o,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-Royal2276-humanHP0',m,0,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,sub,latch,phase,timer,face,selector in itertools.product((0,3,4,8),range(6),(5,0x8005),(0,1,0xfe,0x1fe),(0,1,40,0xff00,0xffff),(0,0x8000),(0x7a,0x407a)):
  m,o,t=seed(slot);put(m,o+0x44,sub);put(m,o+0x1c,latch);put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0x60,selector)
  put(m,o+6,0x280);put(m,o+10,0x160);put(m,o+0xbe,0x140);put(m,o+0xc0,0xb8)
  yield 'six-subowners-descriptor-latch',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for sub,role,flags33,rel,flags34,tag in itertools.product((1,4,5),(0,0x80),(0,0x20),(0,0x80),(0,0x40,0x80,0xc0),(0,0xff000000)):
  m,o,t=seed();put(m,o+0x44,sub);put(m,o+0x1c,5);put(m,o+0x56,role,1);put(m,o+0x33,flags33,1);put(m,o+0x74,rel,1);put(m,o+0x34,flags34,1);put(m,o+0x7a,t|tag,4)
  yield 'magnitude-clear-link-priority',m,0,[rng.getrandbits(32)for _ in range(8)]
 for x,contact,sub,latch in itertools.product((0x27f,0x280,0x281,0xffff),(0,1,0x10,0xff),(1,3),(5,0x8005)):
  m,o,t=seed();put(m,o+0x44,sub);put(m,o+6,x);put(m,o+0x37,contact,1);put(m,o+0x1c,latch)
  yield 'wall-contact-and-facing-boundary',m,0,[rng.getrandbits(32)for _ in range(8)]
 for slot,char,allocation,oldcount in itertools.product(range(9),range(12),(0,0x80,0xff),(0,0xff)):
  m,o,t=seed(slot);put(m,o+0x44,5);put(m,o+2,char);put(m,o+6,0x140);put(m,o+10,0xb8);put(m,o+0xbe,0x140);put(m,o+0xc0,0xb8)
  put(m,RAM+0x1610+char,allocation,1);put(m,RAM+0x16a7,oldcount,1)
  yield 'retire-clear-HUD-allocator-actor-and-tick',m,slot,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action7a-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action7a_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 manifest={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in frozen.rglob('*')if f.is_file()};source_hash=manifest['src\\wf_r17_actions_r11.c']if os.name=='nt'else manifest['src/wf_r17_actions_r11.c']
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action7a_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 native.action7a_oracle_guard.argtypes=[*native.action7a_oracle.argtypes,ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(4000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action7a_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native guard',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1
 guards=0
 if not args.natural_only:
  for kind,expected,slot,disabled in(('sub6',0x1a370,0,0),('subFFFF',0x1a370,0,0),('magnitude1',0x1174c,0,0),('magnitude5',0x1174c,0,0),('linked-target',0x1a4a6,0,0),('video',0x7744,0,1),('resource',0x2b58,4,2)):
   m,o,t=seed(slot)
   if kind.startswith('sub'):put(m,o+0x44,6 if kind=='sub6'else 0xffff)
   if kind.startswith('magnitude'):put(m,o+0x44,int(kind[-1]));put(m,o+0x1c,5);put(m,o+2,0x100)
   if kind=='linked-target':put(m,o+0x44,4);put(m,o+0x1c,5);put(m,o+0x34,0x40,1);put(m,o+0x7a,0xdeadbeef,4)
   if kind in('video','resource'):put(m,o+0x44,5);put(m,o+6,0x140);put(m,o+10,0xb8);put(m,o+0xbe,0x140);put(m,o+0xc0,0xb8)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();st=native.action7a_oracle_guard(b,d,slot,ctypes.byref(nd),disabled);assert st==-expected,(kind,hex(-st),hex(expected));guards+=1
  for pc in(0x1a38e,0x1a3bc,0x1a3e8,0x1a40a,0x1a430,0x1a464,0x1a494,0x1a4a6,0x1a4ac,0x1a4ca,0x1a504,0x1a50e,0x1a518,0x1a524,0x1174c,0x1f15c,0x20c8,0x7744,0x2b58,0x1c0c8):assert pc in coverage,('missing ROM path',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'sources_sha256':manifest,'rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'intercepts':[],'compared':['RAM8KB','FG16KB','D0-D7','RNG','descriptor','literal allocator1600/1610 callback'],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['TypedHOST projected snapshot; no physicalSaturn RAM/timing equivalence claim','Allocator writes delegated to existing resource2B58 callback; original ROM helper runs intact','Consumed actor/table indexes bounded; missing output callbacks retain guards','Temporary ROM stack outsideRAM8KB; A/CCR outside typed combat ABI']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw7A wholeROM calls')
