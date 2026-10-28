"""Raw20/16A16 against whole intact1C03E, including unlatched NOP path."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'action20_manifest.json').read_text())
capture=(HERE/'action20_snapshot_host5845.bin').read_bytes()
assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
assert len(capture)==16440 and list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x12614+0x20*4:0x12618+0x20*4]==bytes.fromhex('00016a06')
assert rom[0x16a06:0x16a16]==bytes.fromhex('00016a16000100020006000400ad00ae')
assert rom[0x16a1c:0x16a1e]==bytes.fromhex('4e71')
rng=random.Random(0x16a16);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();groups={};count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):put(m,at+2,i);put(m,at+0x26,0xdeadbeef,4)
 a=actors[slot];put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x20);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x36,0);put(m,a+0x2e,0);put(m,a+0x2a,4);put(m,RAM+0x161,0,1)
 return m,a
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:]
 yield 'natural-HOST5845',m,5,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,latch,counter,phase,face in itertools.product((0,5,8),(5,0x8005,0x7f05),(0,1,2,3,4,0x7fff,0x8000,0x8001,0xfffe,0xffff),(0,1,0xfe),(0,0x8000,0x80,0xff01)):
  m,a=seed(slot);put(m,a+0x1c,latch);put(m,a+0x2a,counter);put(m,a+0x24,phase);put(m,a+0x2e,face)
  yield 'NOP-first-entry-WORD-subtraction-N-flag',m,slot,None
 for slot,mode,contact,activity,counter in itertools.product((0,4,8),(0,1,2,0xff),(0x0101,0x0102,0x0103,0x0300,0x0310,0x0511,0x0512),(0,1,0x100,0xff01),(0,2,0x8000)):
  m,a=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x36,contact);put(m,a+0x2a,counter)
  for off in(0x1150,0x1180,0x11b0,0x11e0):put(m,RAM+off,activity)
  yield 'real10FC6-carry-contacts-modes-activity',m,slot,None
 for slot,selector,link,x,y,z,face in itertools.product((0,5,8),(0x20,0x4020,0x8020,0xff20),(0,0x80000000,0xdeadbeef),(0,0xffff),(0xffff,),(0,0xffff),(0,0x8000)):
  m,a=seed(slot);put(m,a+0x60,selector);put(m,a+0x26,link,4);put(m,a+6,x);put(m,a+10,y);put(m,a+14,z);put(m,a+0x2e,face);put(m,a+0x2a,1)
  yield 'no-linked-actor-read-facing-wrap-high-selector',m,slot,None
 for slot in range(9):
  m,a=seed(slot);put(m,a+0x1c,5);put(m,a+0x2a,8);regs=[rng.getrandbits(32)for _ in range(8)]
  for step in range(5):yield 'continuous-five-call-negative-transition',m,slot,regs
  assert m[a+0x21]==0 and m[a+0x26]&128
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action20-',ignore_cleanup_errors=os.name=='nt')as td:
 t=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,t/name)
 bridge='tests/r18/action20_oracle_bridge.c';dest=t/bridge;dest.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,dest)
 hashes={str(f.relative_to(t)):hashlib.sha256(f.read_bytes()).hexdigest()for f in t.rglob('*')if f.is_file()}
 lib=t/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c',bridge,'-o',str(lib)],cwd=t,check=True)
 n=ctypes.CDLL(str(lib));n.action20_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot]);descriptor=0
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=n.action20_oracle(buf,d,slot,ctypes.byref(desc))
  assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor')
  for at,size in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+size];want=bytes(mem.r_block(at,size));assert got==want,(name,'RAM/FG',[(hex(at+j),g,w)for j,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7');count+=1;groups[name]=groups.get(name,0)+1
  if name=='continuous-five-call-negative-transition':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(d)
 guards=0
 if not args.natural_only:
  m,a=seed();put(m,a+0x60,0xfe);buf=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();assert n.action20_oracle(buf,d,5,ctypes.byref(desc))==-0x12614;guards+=1
  assert {0x16a1c,0x16a24,0x16a2c,0x16a3a,0x16a48,0x16a4e,0x10fc6,0x10bd0,0x1c0c8}<=coverage,'raw20 missing branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'groups':groups,'retained_guards':guards,'snapshot_sha256':meta['snapshot_sha256'],'sources_sha256':hashes,'compared':['RAM16KB','FG16KB','D0-D7/globals/RNG','tickdescriptor'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='sources_sha256'}))
