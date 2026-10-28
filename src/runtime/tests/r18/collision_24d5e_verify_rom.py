"""Natural Royal2398 and complete24D5E through intact original2435A."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--equal-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2';assert rom[0x24404:0x24408]==bytes.fromhex('00024d5e')
meta=json.loads((HERE/'collision_24d5e_manifest.json').read_text());capture=(HERE/'collision_24d5e_snapshot_native_f8c_royal_2398.bin').read_bytes();assert len(capture)==8248 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x24d5e);coverage=set();count=0;groups={};guards=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(ss=5,ts=8):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);s=actors[ss];t=actors[ts]
 put(m,s+0x2e,0);put(m,t+0x2e,0);put(m,t+0x4c,4);put(m,t+0x33,0,1);put(m,t+0x52,0);put(m,t+0x26,actors[(ts+1)%9],4);return m,s,t

def fixtures():
 if args.equal_only:
  m,s,t=seed();put(m,t+0x64,10);yield 'equal-0A-normalization',m,5,8,0,4,None;return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:];yield 'natural2398-pair',m,5,8,3,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,low,high,sface,tface in itertools.product((0,4,8),range(256),(0,0x8000),(0,0x80),(0,0x80)):
  ts=(slot+1)%9;m,s,t=seed(slot,ts);put(m,t+0x64,high|low);put(m,s+0x2e,(sface<<8)|0x12);put(m,t+0x2e,(tface<<8)|0xab);yield 'all-subtype-BYTEs',m,slot,ts,0,4,None
 for ss,ts,sface,tface,profile,state in itertools.product((0,5,8),(1,4,7),(0,1,0x80,0xff),(0,1,0x80,0xff),(0x0004,0x4004,0x8004,0xff04),(0,1,2,0x8002,5,0xffff)):
  m,s,t=seed(ss,ts);put(m,s+0x2e,(sface<<8)|rng.randrange(256));put(m,t+0x2e,(tface<<8)|rng.randrange(256));put(m,t+0x4c,profile);put(m,t+0x20,state);yield 'full-pair-first2-second12',m,ss,ts,3,4,None
 for sface,tface,profile,flags,timer in itertools.product((0,1,0x80,0x81,0xff),(0,1,0x80,0x81,0xff),(0x0004,0x4004,0x8004),(0,0x10,0xff),(0,3)):
  m,s,t=seed();put(m,s+0x2e,sface<<8);put(m,t+0x2e,tface<<8);put(m,t+0x4c,profile);put(m,t+0x33,flags,1);put(m,t+0x52,timer);yield 'full-pair-first8-second12',m,5,8,17,4,None
 for subtype,state,link,sface,tface in itertools.product((0xa,0x10a,0x800a,0xff0a),(0,4,0x8005),(0,0x80000000,0x1c05b0,0x1c0aec,0x1c0e10,0xff1c0e10,0xdeadbeef),(0,1,0x80,0xff),(0,1,0x80,0xff)):
  m,s,t=seed();put(m,t+0x64,subtype);put(m,t+0x20,state);put(m,t+0x26,link,4);put(m,s+0x2e,(sface<<8)|0x12);put(m,t+0x2e,(tface<<8)|0xab);yield 'equal-BYTE-0A-raw-high-links',m,5,8,0,4,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-collision24d5e-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/collision_24d5e_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_collision_dispatch.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 compiled_sources={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in[*core,bridge]}
 lib=frozen/'collision.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.collision_24d5e_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint];native.collision_24d5e_norm.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);result=native.collision_24d5e_oracle(b,d,ss,ts,ps,pt);assert result==expected,(name,count,'status',result,expected)
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 if not args.natural_only and not args.equal_only:
  assert{0x2435a,0x24468,0x24470,0x24482,0x2449a,0x244ac,0x244ce,0x244da,0x2484c,0x246ea,0x2470c,0x2471e,0x24724,0x24728,0x2472c,0x24d5e,0x24d66,0x24d72,0x24d78,0x24d7a,0x24da8,0x2052}<=coverage
  for subtype,high,sface,tface in itertools.product((0,2,10,0x23,0x24,0x25,0xfe,0xff),(0,0x8000),(0,1,0x80,0x81,0xff),(0,1,0x80,0x81,0xff)):
   m,s,t=seed();put(m,t+0x64,subtype|high);put(m,s+0x2e,sface<<8);put(m,t+0x2e,tface<<8);regs=[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x24d7a)
   for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
   cpu.w_reg(8,s);cpu.w_reg(9,t)
   for _ in range(1000):
    pc=cpu.r_pc();coverage.add(pc)
    if pc==0x70000:break
    cpu.execute(1)
   assert cpu.r_pc()==0x70000,'norm runaway';b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);native.collision_24d5e_norm(b,d,5,8)
   for at,n in((RAM,0x2000),(0xc0000,0x4000)):assert b.raw[at:at+n]==bytes(mem.r_block(at,n)),('norm bounds',hex(subtype|high),sface,tface)
   assert list(d)==[cpu.r_reg(i)for i in range(8)],'norm D0-D7';count+=1;groups['direct-norm-boundaries']=groups.get('direct-norm-boundaries',0)+1
  assert 0x24db2 in coverage
  for subtype,link in itertools.product((0,10),(0,0x1c05b0,0xdeadbeef)):
   m,s,t=seed();put(m,t+0x2e,0x8000);put(m,t+0x64,subtype);put(m,t+0x26,link,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
   assert native.collision_24d5e_oracle(b,d,5,8,0,12)==2,'second3 mismatch low-timer guard removed';assert b.raw[:len(m)]==bytes(m),'blocked pair changed RAM';guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guards':guards,'groups':groups,'source_sha256':source_hash,'compiled_sources_sha256':compiled_sources,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM8KiB','FG16KiB','D0-D7','RNG','dispatch carry/result'],'intercepts':[],'limits':['Complete24D5E callback through intact2435A pair dispatch, including exact BYTE0A condition and both norm-facing directions; direct24D7A boundaries also compared','Fixture is retained natural HOST Royal2398 failed pair from exact nativeF8C import; full24062 and HOST/SH2 trajectories not globally identical','No24D5E branch consumes actor links; invalid/source/self/tagged fixtures verify preservation','CCR/A outside C API; dispatch carry/result compared; unrelated second3 mismatch guards retained']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
