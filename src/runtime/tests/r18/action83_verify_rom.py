"""Raw83 timer reversal against complete intact1C03E, no intercepts."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'action83_manifest.json').read_bytes());assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12820:0x12824]==bytes.fromhex('0001a8a6')and rom[0x1a8a6:0x1a8b6]==bytes.fromhex('0001a8b6000200020018001001f701f8')
capture=(HERE/'action83_snapshot_tag_pressure_3434.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
rng=random.Random(0x1a8b6);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9]
 for i,o in enumerate(actors):put(m,o+2,i);put(m,o+0x2e,0);put(m,o+0x26,actors[(i+1)%9],4)
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x83);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x44,2)
 return m,a,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-TAG-pressure3434',m,0,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,latch,counter,phase,timer,selector in itertools.product((0,4,8),(5,0x8005),(0,1,2,79,80,0xffff,0x100,0x8000),(0,1,0xfe,0x100,0x101),(0,1,0xff00,0xffff),(0x83,0x4083,0x8083,0xff83)):
  m,a,t=seed(slot);put(m,a+0x1c,latch);put(m,a+0x44,counter);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x60,selector)
  yield 'init-and-WORD-counter-vs-animation',m,slot,None
 for slot,relation,tag,y,z,face in itertools.product((0,4,8),('self','peer'),(0,0x40000000,0x80000000,0xff000000),(0,0xffff),(0,0xffc0,0xffff),(0,0x8000,0x00ff,0xffff)):
  m,a,t=seed(slot);t=a if relation=='self'else t;put(m,a+0x26,t|tag,4);put(m,a+0x44,1);put(m,a+0x2e,face);put(m,a+14,z);put(m,t+10,y)
  put(m,a+0x33,0xff,1);put(m,t+0x33,0xff,1);put(m,a+8,0x85a5);put(m,t+12,0xa55a);put(m,a+16,0x1234)
  yield 'terminal-self-peer-alias-order-WORD-wrap',m,slot,None
 for bad,latch,counter,selector in itertools.product((0,0x80000000,0xdeadbeef),(5,0x8005),(0,1,2),(0x83,0xff83)):
  if latch==0x8005 and counter==1:continue
  m,a,t=seed();put(m,a+0x26,bad,4);put(m,a+0x1c,latch);put(m,a+0x44,counter);put(m,a+0x60,selector)
  yield 'unused-invalid-link',m,0,None
 for slot,relation,tag in itertools.product((0,8),('self','peer'),(0,0xff000000)):
  m,a,t=seed(slot);t=a if relation=='self'else t;put(m,a+0x26,t|tag,4);put(m,a+0x1c,5)
  regs=[rng.getrandbits(32)for _ in range(8)]
  for step in range(80):
   yield 'continuous80-call-reversal',m,slot,regs
   m=bytearray(b.raw[:0x200000]);regs=list(d)
  assert int.from_bytes(m[a+0x44:a+0x46],'big')==0 and m[RAM+0x15d2:RAM+0x15d4]==bytes.fromhex('0f1a')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action83-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge='tests/r18/action83_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  if b'/* $12614[$83]'not in data:return None
  start=data.index(b'/* $12614[$83]');end=data.index(b'/* $1A950:',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x83u:[^\n]*',data,re.M)[0]).hexdigest()
 own=owner_hash(source);source_map={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action83_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action83_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,count,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D8');count+=1;groups[name]=groups.get(name,0)+1
 guards=0;guard_prefixes=0
 if not args.natural_only:
  for bad in(0,0x80000000,0xdeadbeef):
   m,a,t=seed();put(m,a+0x44,1);put(m,a+0x26,bad,4)
   mem.w_block(0,bytes(m));cpu.w_pc(0x1c03e);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000)
   for i in range(15):cpu.w_reg(i,i if i<8 else 0)
   cpu.w_reg(8,a)
   for _ in range(2000):
    if cpu.r_pc()==0x1a8ea:break
    cpu.execute(1)
   assert cpu.r_pc()==0x1a8ea,'Original guard boundary not reached'
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.action83_oracle(b,d,0,ctypes.byref(nd))==-0x1a8ea;guards+=1
   assert b.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),'Guard lost original writes before consumed pointer'
   guard_prefixes+=1
  m,a,t=seed();put(m,a+0x60,0x35);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action83_oracle(b,d,0,ctypes.byref(nd))==-0x12614;guards+=1
  assert {0x1a8b6,0x1a8be,0x1a8c4,0x1a8ca,0x1a91a,0x1a93e,0x1c0c8}<=coverage
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup();assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==own,'Raw83 changed during oracle'
 report={'status':'PASS','cases':count,'guards':guards,'RAM_guard_prefix_checks':guard_prefixes,'groups':groups,'raw83_owner_sha256':own,'source_sha256':source_hash,'sources_sha256':source_map,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'intercepts':[],'coverage':[hex(x)for x in sorted(coverage)],'compared':['RAM16KB','FG16KB','D0-D7','RNG unchanged','descriptor'],'limits':['HOST pressure fixture projected from preRAM8KB only; unrepresented originalRAM zero','Consumed target outside represented actors retains precise guard, including ROM-domain pointers; RAM prefix compared before1A8EA but not unsupported target write','CCR/A-register/soundIO outside typed ABI; temporary stack outsideRAM16KB','No full combat or physical timing equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS',count,'raw83 intact1C03E calls and',guards,'guards; no intercepts')
