"""CPU related-object distance owner1D032/1CD2C versus intact1C8F4."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'cpu_1d032_manifest.json').read_text());assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1'];capture=(HERE/'cpu_1d032_snapshot_pressure3674.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x1d066:0x1d068]==bytes.fromhex('3232')
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x1d032);coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x140);put(m,at+0x20,0x8001)
  for off in(0x32,0x33,0x34,0x35,0x56,0xb4,0xb5,0xb6,0xb7,0xb9):put(m,at+off,0,1)
  put(m,at+0x7a,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x26,0,4)
 put(m,a+0xae,0);put(m,a+0xb4,2,1);put(m,a+0x74,0x80,1);put(m,a+0xb0,128);put(m,a+0xb2,0);put(m,t+0x20,0x8000);put(m,RAM+0x161,2,1);put(m,RAM+0x7e,0);put(m,RAM+0x7c,0x8000);return m,a,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-pressure3674-slot5',m,5,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,distance,b5,tag in itertools.product((0,4,8),(0,103,104,105,143,144,145,0x8000,0xffff),(0,1,0xfe,0xff),(0,0x80000000,0xff000000)):
  m,a,t=seed(slot);put(m,a+0xb0,distance);put(m,a+0xb5,b5,1);put(m,a+0x7a,t|tag,4);yield 'distance-WORD-103-104-143-144-directionBYTE',m,slot,None
 for state in(0,1,0xffffffff,0x12345678,0x80000000):
  for delta,mode,frame in itertools.product((104,120,143),range(4),(0,1,0xffffffff)):
   m,a,t=seed();put(m,a+0xb0,delta);put(m,RAM+0x161,mode,1);put(m,RAM+0x5c,state,4);put(m,RAM+0x80,frame,4);yield 'middle-50-50-live-RNG-D-registers',m,5,None
 for slot,cls,distance,targetstate,sub,b7 in itertools.product((0,5,8),(0,1,2),(0,103),(0,1,4,0x8004),(0,1,8,9,0x108,0x109),(0,0x40,0xff)):
  m,a,t=seed(slot);put(m,a+0xb4,cls,1);put(m,a+0xb0,distance);put(m,t+0x20,targetstate);put(m,t+0x64,sub);put(m,a+0xb7,b7,1);yield 'shared1CD2C-BYTE-state-subtype-B7bit6',m,slot,None
 for targetstate,sub,selector,facing in itertools.product((0,4,5),(0,8,9),(0,0x22,0x52),(0,0x8000,0xff01)):
  m,a,t=seed();put(m,a+0xb0,144);put(m,t+0x20,targetstate);put(m,t+0x64,sub);put(m,t+0x60,selector);put(m,t+0x2e,facing);put(m,t+6,0x240);yield 'initial-facing-special-offset-preserved',m,5,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-cpu1d032-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/cpu_1d032_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_cpu_runtime.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  start=data.index(b'/* $1CD2C:'if b'/* $1CD2C:'in data else b'/* $1CE38:');end=data.index(b'/* ROM $1D398',start);return hashlib.sha256(data[start:end]).hexdigest()
 owner=owner_hash(source);hashes={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_1d032_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);status=native.wf_cpu_1d032_oracle(b,d,slot);assert status==0,(name,count,'native blocked',hex(-status))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for bad in(0,0x80000000,0xdeadbeef):
   m,a,t=seed();put(m,a+0x7a,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));assert native.wf_cpu_1d032_oracle(b,d,5)==-0x10be8;guards+=1
  m,a,t=seed();put(m,a+0xae,13);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));assert native.wf_cpu_1d032_oracle(b,d,5)==-0x1c8f4;guards+=1
  required={0x1c8f4,0x1d032,0x1d03e,0x1d046,0x1d058,0x1d01a,0x1d028,0x1cd2c,0x1cd44,0x1cd4e,0x24cc,0x21b4};assert required<=coverage,('missing branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup();assert owner_hash((ROOT/'src/wf_cpu_runtime.c').read_bytes())==owner,'Aligned source changed during oracle'
 report={'status':'PASS','cases':count,'guards':guards,'groups':groups,'source_sha256':source_hash,'aligned_owner_sha256':owner,'sources_sha256':hashes,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM16KiB','FG16KiB','D0-D7','RNG'],'intercepts':[],'limits':['Natural HOST pressure3674; no forced win/health/seed or HOST/SH2 trajectory identity claim','Only related-object1D032/1CD2C branches added; other CPU guards remain','CCR/A registers and physical timing outside typed combat ABI','No full-combat/hardware/global ROM equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
