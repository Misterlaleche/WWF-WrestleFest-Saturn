"""Natural Royal1563 state4/21 callback1BD6C versus complete original1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'state4_1bd6c_snapshot_full_flow_1563.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(1563,8,1,0x1c03e,886,1)
rng=random.Random(0x1bd6c);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;descriptors=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot]
 put(m,o+0x20,0x8004);put(m,o+0x1c,0x8004);put(m,o+0x64,0x21);put(m,o+0x36,0);put(m,RAM+0x161,0,1);put(m,RAM+0x7e,0)
 put(m,o+6,0x280);put(m,o+10,0x160);put(m,o+14,0x180);put(m,o+0x32,0);put(m,o+0x40,0);put(m,o+0x42,0)
 return m,o
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-Royal1563',m,8,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,latch,phase,timer,face,highsub in itertools.product((0,4,8),(4,0x8004),(0,1,2,0xfe,0x100,0x1fe,0xffff),(0,1,0xff00,0xffff),(0,0x8000,0xffff),(0,0xff00)):
  m,o=seed(slot);put(m,o+0x1c,latch);put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0x64,highsub|0x21)
  yield 'phase-and-descriptor',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for hp,maximum,face in itertools.product((0,1,0x10,0x3f,0x40,0x41,0x80,0x100,0x8000,0xffff),(0,1,4,0x40,0x100,0xfffc,0xffff),(0,0x8000)):
  m,o=seed();put(m,o+0x1c,4);put(m,o+0x66,hp);put(m,o+0x72,maximum);put(m,o+0x2e,face)
  yield 'init-recovery-health-quartiles',m,4,[rng.getrandbits(32)for _ in range(8)]
 for role,buttons,recovery,tier,contact in itertools.product((0,0x40,0x80,0xc0),(0,1,0x80),(0,1,2,0x4000,0xffff),(0,2,0x102),(0,0x10)):
  m,o=seed();put(m,o+0x56,role,1);put(m,o+0xa3,buttons,1);put(m,o+0xaa,recovery);put(m,o+0x70,tier);put(m,o+0x37,contact,1)
  yield 'recovery-input-before-landing-tierBYTE',m,4,[rng.getrandbits(32)for _ in range(8)]
 for scene,x,y,face,mode,side in itertools.product(range(7),(0,0x260,0x380,0xffff),(0x118,0x160,0x198),(0,0x8000),(0,2),(0,4)):
  m,o=seed();put(m,RAM+0x7e,scene);put(m,RAM+0x161,mode,1);put(m,o+6,x);put(m,o+10,y);put(m,o+0x2e,face);put(m,o+0x32,side)
  yield 'clearance-restores-original-contact',m,4,[rng.getrandbits(32)for _ in range(8)]
 for mode,kind,bits,left,right in itertools.product(range(4),(0,1,3,5,0xff),(0,1,2,3,0x10,0x13,0xff),(0,0x100,1),(0,0x100,1)):
  m,o=seed();put(m,RAM+0x161,mode,1);put(m,o+0x36,kind,1);put(m,o+0x37,bits,1)
  put(m,RAM+0x1150,left);put(m,RAM+0x11b0,right)
  yield 'edge-landing-mode-priority',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-state4-1bd6c-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/state4_1bd6c_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest()
 manifest={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
 lib=frozen/'state.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.state4_1bd6c_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000)
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(2000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptors.add(cpu.r_reg(9))
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);st=native.state4_1bd6c_oracle(b,d,slot)
  assert st==0,(name,'native blocked',hex(-st))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',[hex(x)for x in d],[hex(cpu.r_reg(i))for i in range(8)])
  count+=1
 guards=0
 if not a.natural_only:
  for subtype,dependency in ((0x1a,0x1bbc6),(43,0x1afd4),(0xff,0x1afd4)):
   m,o=seed();put(m,o+0x64,subtype);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values)
   assert native.state4_1bd6c_oracle(b,d,4)==-dependency
   assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values;guards+=1
  m,o=seed();put(m,RAM+0x7e,7);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();assert native.state4_1bd6c_oracle(b,d,4)==-0x280dc;guards+=1
  for pc in (0x1bd6c,0x1bd74,0x1bd8e,0x1bdac,0x1bdc0,0x1bdc8,0x1bdd0,0x10c60,0x10d04,0x10b62,0x280dc,0x10fc6,0x1110e,0x2052,0x1c0c8):
   assert pc in coverage,('missing ROM branch',hex(pc))
  assert descriptors=={0x1bd66,0x1b1b8},descriptors
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'sources_sha256':manifest,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','unchangedRNG','rope/status globals'],'intercepts':[],'descriptors':[hex(pc)for pc in sorted(descriptors)],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend Royal1563 projected capture; no physical SaturnRAM/timing claim','Only1BD6C callback admitted; shared helpers unchanged','Temporary stack outside comparedRAM8KB','Native sound output remains silent, IOcommands outside state comparison','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'state4/21 whole ROM calls without intercepts and',guards,'retained guards')
