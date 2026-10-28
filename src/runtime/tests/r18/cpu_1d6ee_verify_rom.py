"""Natural pressure3632 CPUraw70 and exact object-reservation branch1D6EE."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'cpu_1d6ee_manifest.json').read_text());assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1'];capture=(HERE/'cpu_1d6ee_snapshot_pressure3632.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
assert list(struct.unpack_from('=6I',capture))==meta['header'];assert rom[0x1d6ee:0x1d716]==bytes.fromhex('266800760c2b0003001d66084a6b00766602601408a8000500b508a8000500b7317c000000204e75')
actors=[RAM+0x5b0+i*0x10c for i in range(9)];objects=[RAM+0xf1c,RAM+0x1028];rng=random.Random(0x1d6ee);coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];o=objects[0]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x140);put(m,at+0x20,0x8001)
  for off in(0x32,0x33,0x34,0x35,0x56,0xb4,0xb5,0xb6,0xb7,0xb9):put(m,at+off,0,1)
  put(m,at+0x7a,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x26,0,4)
 put(m,a+0xae,10);put(m,a+0x60,0x70);put(m,a+0x76,o,4);put(m,a+0xbe,0x300);put(m,a+0xc0,0x150);put(m,o+0x1c,3);put(m,o+0x76,0);put(m,RAM+0x161,2,1);put(m,RAM+0x7e,0);put(m,RAM+0x7c,0x8000);return m,a,t,o
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-pressure3632-slot5',m,5,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,selector,latch,busy,tag,delta in itertools.product((0,4,8),(0x70,0x4070,0x8070,0xff70),(2,3,0x8003,0xff03),(0,1,0x8000),(0,0x80000000,0xff000000),(-8,-7,0,7,8)):
  m,a,t,o=seed(slot);put(m,a+0x60,selector);put(m,o+0x1c,latch);put(m,o+0x76,busy);put(m,a+0x76,o|tag,4);put(m,a+6,0x300+delta);put(m,a+10,0x150);yield 'object-BYTEstate-WORDreservation-arrival-boundary',m,slot,None
 for slot,object_index,mode,flags,dx,dy in itertools.product((0,5,8),(0,1),range(4),(0,0x20,0xff),(-8,-7,0,7,8),(-8,-7,0,7,8)):
  m,a,t,o=seed(slot);o=objects[object_index];put(m,o+0x1c,3);put(m,o+0x76,0);put(m,o+0x78,0xa55a);put(m,a+0x76,o,4);put(m,a+0xb5,flags,1);put(m,a+0xb7,flags,1);put(m,a+6,0x300+dx);put(m,a+10,0x150+dy);put(m,RAM+0x161,mode,1);yield 'both-fixed-objects-XY-flags-mode',m,slot,None
 for slot,relation,tag in itertools.product((0,4,8),('self','target','peer'),(0,0x80000000,0xff000000)):
  m,a,t,o=seed(slot);o=a if relation=='self'else t if relation=='target'else actors[(slot+2)%9];put(m,o+0x1c,3)
  if relation!='self':put(m,o+0x76,0)
  put(m,a+0x76,o|tag,4);put(m,a+6,0x300);put(m,a+10,0x150);yield 'actor-object-alias-canonical-reservation',m,slot,None
 for state,sub,selector,facing,delta in itertools.product((0,4,5),(0,8,9),(0,0x22,0x52),(0,0x8000,0xff01),(-1,0,1)):
  m,a,t,o=seed();put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0x60,selector);put(m,t+0x2e,facing);put(m,t+6,0x250+delta);yield 'facing-target-state-offset',m,5,None
 for x,y,gx,gy in itertools.product((0,1,0x7fff,0x8000,0xffff),(0,0x8000,0xffff),(0,0xffff),(0,0xffff)):
  m,a,t,o=seed();put(m,a+6,x);put(m,a+10,y);put(m,a+0xbe,gx);put(m,a+0xc0,gy);yield 'arrival-direction-saturated-wrap',m,5,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-cpu1d6ee-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/cpu_1d6ee_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_cpu_runtime.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  start=data.index(b'wf_status_t wf_cpu_pursuit_1d5a2(wf_runtime_t*rt');end=data.index(b'/* $1F5B2:',start);return hashlib.sha256(data[start:end]).hexdigest()
 owner=owner_hash(source);hashes={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_1d6ee_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c8f4)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot])
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);status=native.wf_cpu_1d6ee_oracle(b,d,slot);assert status==0,(name,count,'native blocked',hex(-status))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for bad in(0,0x80000000,0xdeadbeef,RAM+0xf1d):
   m,a,t,o=seed();put(m,a+0x76,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));assert native.wf_cpu_1d6ee_oracle(b,d,5)==-0x1d6f2;guards+=1
  required={0x1c8f4,0x1d5a2,0x1d6ee,0x1d6f2,0x1d6fa,0x1d702,0x1d716,0x1d724,0x1d728,0x1dfe2,0x1f15c,0x20c8};assert required<=coverage,('missing branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup();assert owner_hash((ROOT/'src/wf_cpu_runtime.c').read_bytes())==owner,'Pursuit source changed during oracle'
 report={'status':'PASS','cases':count,'guards':guards,'groups':groups,'source_sha256':source_hash,'pursuit_owner_sha256':owner,'sources_sha256':hashes,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM16KiB','FG16KiB','D0-D7','RNG','fixed0/1 reservation'],'intercepts':[],'limits':['Natural HOST pressure3632; no forced win/health/seed and no HOST/SH2 trajectory identity claim','Only raw70 continuation1D6EE added; unrelated pursuit dependencies remain explicit','CCR/A registers and physical timing outside typed combat ABI','No full-combat/hardware/global ROM equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
