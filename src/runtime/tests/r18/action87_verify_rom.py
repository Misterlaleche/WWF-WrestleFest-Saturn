"""Raw87 pair reversal against whole intact1C03E; no ROM interception."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x87*4:0x12618+0x87*4]==bytes.fromhex('0001aa8a')
assert rom[0x1aa8a:0x1aa9a]==bytes.fromhex('0001aa9a000200020012000a01440145')
rng=random.Random(0x1aa9a);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;groups={}

def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+0x26,actors[(i+1)%9],4)
 a=actors[slot];t=actors[(slot+1)%9]
 put(m,a+0x20,0x8005);put(m,a+0x60,0x87);put(m,a+0x1c,0x8005)
 put(m,a+0x44,2);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0)
 return m,a,t

def fixtures():
 capture=HERE/'action87_snapshot_host7493.bin'
 meta=json.loads((HERE/'action87_manifest.json').read_text());data=capture.read_bytes()
 assert len(data)==16440 and hashlib.sha256(data).hexdigest()==meta['snapshot_sha256']
 assert list(struct.unpack_from('=6I',data))==meta['header']
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=data[56:]
 yield 'natural-HOST7493',m,1,struct.unpack_from('=8I',data,24)
 if args.natural_only:return
 for slot,latch,counter,phase,timer in itertools.product((0,1,8),(5,0x8005,0x7f05),(0,1,2,71,72,0x100,0x8000,0xffff),(0,1,0xfe,0x100,0x1fe),(0,1,0xff00)):
  m,a,t=seed(slot);put(m,a+0x1c,latch);put(m,a+0x44,counter);put(m,a+0x24,phase);put(m,a+0x22,timer)
  yield 'initial-WORD-countdown-independent-of-animation',m,slot,None
 for slot,other,tag,face,z in itertools.product((0,1,8),range(9),(0,0x40000000,0xff000000),(0,0x8000,0xff01),(0,0xffc0,0xffff)):
  m,a,t=seed(slot);t=actors[other];put(m,a+0x26,t|tag,4);put(m,a+0x44,1);put(m,a+0x2e,face);put(m,a+14,z)
  yield 'terminal-all-targets-self-tagged-alias-Z-wrap',m,slot,None
 for latch,bad,face in itertools.product((5,0x8005),(0,0x80000000,0xdeadbeef,0x1c05b1),(0,0x8000)):
  m,a,t=seed();put(m,a+0x1c,latch);put(m,a+0x26,bad,4);put(m,a+0x2e,face)
  yield 'unconsumed-invalid-link',m,1,None
 for selector,x,y,z in itertools.product((0x87,0x4087,0x8087,0xff87),(0,0xffff,0x8000),(0,0xffff),(0,0xffff)):
  m,a,t=seed();put(m,a+0x60,selector);put(m,a+0x1c,5);put(m,a+6,x);put(m,a+10,y);put(m,a+14,z)
  yield 'selector-high-WORD-translation-carries',m,1,None
 for slot,face in itertools.product((0,4,8),(0,0x8000)):
  m,a,t=seed(slot);put(m,a+0x1c,5);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(72):yield 'continuous72-physical-countdown',m,slot,regs
  assert m[a+0x21]==4 and m[t+0x21]==5 and m[t+0x61]==6

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action87-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge='tests/r18/action87_oracle_bridge.c';dest=frozen/bridge;dest.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,dest)
 hashes={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in frozen.rglob('*')if f.is_file()}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c',bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action87_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 def run(m,slot,regs,stop=0x70000):
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot]);descriptor=0
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==stop:return descriptor
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   cpu.execute(1)
  raise AssertionError(('ROM runaway',hex(cpu.r_pc())))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);descriptor=run(m,slot,regs)
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action87_oracle(buf,dr,slot,ctypes.byref(desc))
  assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)])
  count+=1;groups[name]=groups.get(name,0)+1
  if name=='continuous72-physical-countdown':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
 guards=0
 if not args.natural_only:
  for slot,bad,face in itertools.product((0,1,8),(0,0x80000000,0xdeadbeef,0x1c0f1c,0x1c05b1),(0,0xff01)):
   m,a,t=seed(slot);put(m,a+0x44,1);put(m,a+0x26,bad,4);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action87_oracle(buf,dr,slot,ctypes.byref(desc))
   assert status==-0x1aaca,(slot,bad,'guard',hex(-status));run(m,slot,regs,0x1aaca)
   assert buf.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),'guard rawRAM prefix'
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],('guard D0-D7 prefix',slot,hex(bad),list(dr),[cpu.r_reg(i)for i in range(8)]);guards+=1
  m,a,t=seed();put(m,a+0x60,0xfe);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32()
  assert native.action87_oracle(buf,dr,1,ctypes.byref(desc))==-0x12614;guards+=1
  assert {0x1aaa2,0x10bd0,0x1aab6,0x1aabc,0x1aaca,0x1ab06,0x1ab0e,0x1ab14,0x1ab30,0x1c0c8}<=coverage,'raw87 missing branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'groups':groups,'retained_guards':guards,'sources_sha256':hashes,'snapshot_sha256':hashlib.sha256((HERE/'action87_snapshot_host7493.bin').read_bytes()).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','tickdescriptor'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='sources_sha256'}))
