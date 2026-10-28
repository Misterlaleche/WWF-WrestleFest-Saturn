"""Forced raw79 CPU callback1E27A versus intact1DFE2/10C60/10F56."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'cpu_raw79_manifest.json').read_text());capture=(HERE/'cpu_raw79_snapshot_basic4551.bin').read_bytes()
assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1'] and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'] and list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x1e018+0x79*4:0x1e01c+0x79*4]==bytes.fromhex('8001e27a')
assert rom[0xb455:0xb458]==bytes.fromhex('006004')
rng=random.Random(0x1e27a);actors=[RAM+0x5b0+i*0x10c for i in range(9)];count=0;coverage=set();groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 a=actors[slot];t=actors[(slot+1)%9]
 for at in actors:
  put(m,at,0x8000);put(m,at+0x20,0x8005);put(m,at+0x70,0);put(m,at+0x56,0,1);put(m,at+0x33,0,1)
 put(m,a+0x60,0x8079);put(m,a+0x7a,t,4);put(m,a+0xb5,0x80,1);put(m,a+0x34,0xff,1)
 return m,a,t
def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-TAG-basic4551',m,1,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,state,b5,b9,tag in itertools.product((0,1,4,8),range(4),(5,0x8005,0xff05),(0,0x80,0xff),(0,2,0xff),(0,0x40000000,0xff000000)):
  m,a,t=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x20,state);put(m,a+0xb5,b5,1);put(m,a+0xb9,b9,1);put(m,a+0x7a,t|tag,4)
  yield 'forced-call-state-mode-B5-B9-bus24',m,slot,None
 for tier,health,maxhealth,flags,role,aa,angle in itertools.product(range(3),(0,1,67,0xffff),(0,1,100,0xffff),(0,4,0x20,0x24),(0,0x80),(0,1,0x4000,0x8000,0xffff),(0,0x5555)):
  m,a,t=seed();put(m,t+0x70,tier);put(m,t+0x66,health);put(m,t+0x72,maxhealth);put(m,t+0x33,flags,1);put(m,t+0x56,role,1);put(m,t+0xaa,aa);put(m,t+0x9a,angle)
  yield 'recovery-live-callback-D0-and10F56',m,1,None
 for tag,tier,flags,role in itertools.product((0,0x40000000,0xff000000),range(3),(0,4,0x20),(0,0x80)):
  m,a,t=seed();put(m,a+0x7a,a|tag,4);put(m,a+0x70,tier);put(m,a+0x33,flags,1);put(m,a+0x56,role,1)
  yield 'self-target-order-and-24bitalias',m,1,None
 for bad in(0,0x80000000,0xdeadbeef):
  m,a,t=seed();put(m,a+0xb5,0,1);put(m,a+0x7a,bad,4);yield 'unconsumed-invalid-target',m,1,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-cpu-raw79-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/cpu_raw79_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_state5_exec.c').read_bytes()).hexdigest();lib=frozen/'cpu.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/cpu_raw79_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.cpu_raw79_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1dfe2)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot])
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);status=native.cpu_raw79_oracle(buf,dr,slot);assert status==0,(name,count,'native blocked',hex(-status))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)])
  count+=1;groups[name]=groups.get(name,0)+1
  if count%1000==0:print('verified',count,flush=True)
 guards=0
 if not args.natural_only:
  for kind,bad,expected in [('target',x,0x1e292)for x in(0,0x80000000,0xdeadbeef,0x1c0f1c)]+[('tier',x,0x10ca8)for x in(3,0xff,0x8000,0xffff)]:
   m,a,t=seed()
   if kind=='target':put(m,a+0x7a,bad,4)
   else:put(m,t+0x70,bad)
   buf=ctypes.create_string_buffer(bytes(m));regs=[rng.getrandbits(32)for _ in range(8)];dr=(ctypes.c_uint32*8)(*regs);status=native.cpu_raw79_oracle(buf,dr,1);assert status==-expected,(kind,bad,'guard',hex(-status));guards+=1
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1dfe2);cpu.w_reg(8,a)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   for _ in range(20000):
    if cpu.r_pc()==expected:break
    cpu.execute(1)
   assert cpu.r_pc()==expected,(kind,'guard ROM prefix runaway',hex(cpu.r_pc()))
   assert buf.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),(kind,bad,'guard RAM prefix')
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],(kind,bad,'guard Dregs prefix',list(dr),[cpu.r_reg(i)for i in range(8)])
 if not args.natural_only and not args.guards_only:
  assert {0x1e27a,0x1e282,0x1e292,0x10c60,0x10ca8,0x10f56,0x10f7c,0x10f8c,0x10f94}<=coverage,'missing branches'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'guards':guards,'groups':groups,'source_sha256':source_sha,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM16KB','FG16KB','D0-D7/RNG/globals'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)],'limits':['Only forced callback1E27A/raw79 added; other CPU guards unchanged','10C60 data address derives from actualD0=1E27A, not a guessed argument','Tier0..2 supported; other table indexes explicit10CA8 guard with original prefix','A/CCR and physical timing outside typed combat ABI']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
