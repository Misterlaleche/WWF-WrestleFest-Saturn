"""State4 subtype18/29 callback1BB08, whole original1C03E comparison."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'state4_1bb08_snapshot_full_flow_836.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(836,4,1,0x1c03e,253,1)
rng=random.Random(0x1bb08);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-full-flow836',m,4,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,subtype,latch,phase,timer,face,state in itertools.product((0,4,8),(0x29,0x18,0x8029,0xff18),(0x0004,0x8004,0xc004),(0,1,0xfe,0x100,0x1fe),(0,1,0xff00,0xffff),(0,0x8000),(4,0x8004,0x80ff)):
  m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000)
  o=actors[slot];put(m,o+0x20,state);put(m,o+0x1c,latch);put(m,o+0x64,subtype)
  put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2e,face)
  yield 'callback-and-tick',m,slot,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-state4-1bb08-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/state4_1bb08_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest()
 lib=frozen/'state.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.state4_1bb08_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(2000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);st=native.state4_1bb08_oracle(b,d,slot)
  assert st==0,(name,'native blocked',hex(-st))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7')
  count+=1
 guards=0
 if not a.natural_only:
  for subtype,dependency in ((0x1a,0x1bbc6),(43,0x1afd4),(0xff,0x1afd4)):
   m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];o=actors[4]
   put(m,o+0x64,subtype);put(m,o+0x20,0x8004);put(m,o+0x1c,0x8004)
   b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values)
   st=native.state4_1bb08_oracle(b,d,4);assert st==-dependency,(subtype,hex(-st),hex(dependency))
   assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values
   guards+=1
  for pc in (0x1bb08,0x1bb10,0x1bb1c,0x1bb24,0x1bb32,0x1c0c8,0x1c0b2):
   assert pc in coverage,('missing ROM branch',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','unchanged RNG'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend frame836; no physical SaturnRAM/timing claim','Only state4 callbacks1BB08 admitted; other missing callbacks retain guards','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'state4/18-29 whole ROM calls without intercepts')
