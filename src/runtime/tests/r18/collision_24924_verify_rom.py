"""Natural collision970 and facing-mismatch24924 through full original2435A."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2';assert rom[0x243dc:0x243e0]==bytes.fromhex('00024924')
meta=json.loads((HERE/'collision_24924_manifest.json').read_text());capture=(HERE/'collision_24924_snapshot_native_f8c_tag_970.bin').read_bytes();assert len(capture)==8248 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x2497c);coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(ss=5,ts=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);s=actors[ss];t=actors[ts]
 put(m,s+0x2e,0);put(m,t+0x2e,0x8000);put(m,t+0x4c,10);put(m,t+0x33,0,1);put(m,t+0x52,0);put(m,t+0x26,actors[(ts+1)%9],4);return m,s,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:];yield 'natural970-pair',m,5,4,7,10,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for ss,ts,sface,tface,profile,flags,timer in itertools.product((0,5,8),(1,4,7),(0,1,0x80,0xff),(0,1,0x80,0xff),(0x000a,0x400a,0x800a,0xc00a),(0,0x10,0xff),(0,2,3)):
  if sface==tface:continue
  m,s,t=seed(ss,ts);put(m,s+0x2e,(sface<<8)|rng.randrange(256));put(m,t+0x2e,(tface<<8)|rng.randrange(256));put(m,t+0x4c,profile);put(m,t+0x33,flags,1);put(m,t+0x52,timer)
  yield 'full-pair-first1-second2',m,ss,ts,7,10,None
 for subtype,state,link,face in itertools.product((0,1,2,10,0x8000,0xffff),(0,4,0x8005),(0,0x80000000,0x1c05b0,0xff1c09e0,0xdeadbeef),(1,0x80,0xff)):
  m,s,t=seed();put(m,t+0x64,subtype);put(m,t+0x20,state);put(m,t+0x26,link,4);put(m,t+0x2e,face<<8)
  yield 'callback-via-noop-first',m,5,4,0,10,None
 for ps,current in itertools.product((7,25),(0,7,25,0xff07)):
  m,s,t=seed();put(m,s+0x4c,current);yield 'retained-source-row',m,5,4,ps,10,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-collision24924-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/collision_24924_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_collision_dispatch.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 lib=frozen/'collision.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.collision_24924_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,ss,ts,ps,pt,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x2435a)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  for i,v in((8,actors[ss]),(9,actors[ts]),(10,0x24ef6+ps*7),(11,0x24ef6+pt*7)):cpu.w_reg(i,v)
  for _ in range(4000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  expected=0 if cpu.r_sr()&1 else 1
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);result=native.collision_24924_oracle(b,d,ss,ts,ps,pt);assert result==expected,(name,count,'status',result,expected)
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 if not args.natural_only:assert{0x2435a,0x24408,0x24462,0x2484c,0x24924,0x2497c,0x2052,0x24d7a,0x24ddc}<=coverage
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'groups':groups,'source_sha256':source_hash,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM8KiB','FG16KiB','D0-D7','RNG','dispatch carry/result'],'intercepts':[],'limits':['Full2435A pair dispatch and complete24924 mismatch branch; equal-facing branch unchanged and outside this proof','Fixture is exact retained natural HOST970 failed pair from nativeF8C import; original phase24062 and HOST/SH2 trajectories not claimed globally identical','Unused links are not dereferenced by the mismatch branch','CCR after dispatch and A context outside C API; dispatch carry is compared']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
