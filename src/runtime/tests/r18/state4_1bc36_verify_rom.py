"""Natural basic5100 state4/1B callback1BC36 versus complete original1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'state4_1bc36_snapshot_full_flow_5100.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(5100,1,1,0x1c03e,1788,1)
rng=random.Random(0x1bc36);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot]
 put(m,o+0x20,0x8004);put(m,o+0x1c,0x8004);put(m,o+0x64,0x1b);put(m,o+0x36,0);put(m,RAM+0x161,0,1)
 return m,o
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-full-flow5100',m,1,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,latch,phase,timer,face in itertools.product((0,4,8),(4,0x8004),(0,1,2,0xfe,0x100,0x1fe,0xffff),(0,1,0xff00,0xffff),(0,0x8000,0xffff)):
  m,o=seed(slot);put(m,o+0x1c,latch);put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2e,face)
  yield 'phase-and-descriptor',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for slot,y,face in itertools.product((0,4,8),(0,0xf7,0xf8,0x100,0x117,0x118,0x178,0x197,0x198,0xffe0,0xffff),(0,0x8000)):
  m,o=seed(slot);put(m,o+0x1c,4);put(m,o+10,y);put(m,o+0x2e,face)
  yield 'launch-row20-Y-boundary',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for mode,kind,bits,left,right in itertools.product(range(4),(0,1,3,5,0xff),(0,1,2,3,0x10,0x13,0xff),(0,0x100,1),(0,0x100,1)):
  m,o=seed();put(m,RAM+0x161,mode,1);put(m,o+0x36,kind,1);put(m,o+0x37,bits,1)
  put(m,RAM+0x1150,left);put(m,RAM+0x11b0,right)
  yield 'edge-landing-mode-priority',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-state4-1bc36-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/state4_1bc36_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest()
 lib=frozen/'state.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.state4_1bc36_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(2000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);st=native.state4_1bc36_oracle(b,d,slot)
  assert st==0,(name,'native blocked',hex(-st))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',[hex(x)for x in d],[hex(cpu.r_reg(i))for i in range(8)])
  count+=1
 guards=0
 if not a.natural_only:
  for subtype,dependency in ((0x1a,0x1bbc6),(43,0x1afd4),(0xff,0x1afd4)):
   m,o=seed();put(m,o+0x64,subtype);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values)
   assert native.state4_1bc36_oracle(b,d,4)==-dependency
   assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values;guards+=1
  for pc in (0x1bc36,0x1bc40,0x1bc58,0x1bc5e,0x1bc72,0x10faa,0x10fc0,0x1110e,0x2052,0x1c0c8):
   assert pc in coverage,('missing ROM branch',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','unchangedRNG','rope globals'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend basic5100, no physical SaturnRAM/timing claim','Only1BC36 callback admitted; shared10FC6 unchanged','Native sound output remains silent, IOcommands outside state comparison','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'state4/1B whole ROM calls without intercepts')
