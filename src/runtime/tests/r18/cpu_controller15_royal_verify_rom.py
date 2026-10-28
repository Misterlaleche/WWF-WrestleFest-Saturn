"""Royal controller15 natural400 and1E96E Y/weighted60-40 branch vs intact ROM."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'cpu_controller15_royal_snapshot_full_flow_400.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(400,8,0,0x1dfe2,258,1)
assert rom[0x1e9d4:0x1e9d6]==bytes([60,40])
rng=random.Random(0x1e96e);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=8):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 o=actors[slot];t=actors[(slot+1)%9];m[o:o+0x10c]=capture[56+0xe10:56+0xf1c]
 put(m,o+0x20,0x8005);put(m,o+0x60,0x4015);put(m,o+0xbc,1)
 put(m,o+0x7a,t,4);put(m,o+0x26,t,4);put(m,t+0x70,1)
 put(m,RAM+0x161,1,1);return m,o,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-Royal400',m,8,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,y,sample,b6 in itertools.product((0,4,8),(0,0x14f,0x150,0x15f,0x160,0xffff),range(16),(0,8,0xff)):
  m,o,t=seed(slot);put(m,o+10,y);put(m,o+0xb6,b6,1);put(m,RAM+0x5c,rng.getrandbits(32),4)
  yield 'Y-weighted-RNG-B6',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for mode,selector,timer,latch in itertools.product((1,3,0x81,0xff),(0x15,0x4015,0x8015,0xff15),(0,1,2,0xffff),(5,0x8005)):
  m,o,t=seed();put(m,RAM+0x161,mode,1);put(m,o+0x60,selector);put(m,o+0xbc,timer);put(m,o+0x20,latch)
  yield 'mode-selector-timer-edges',m,8,[rng.getrandbits(32)for _ in range(8)]
 for char,y,b5 in itertools.product(range(12),(0,0x14f,0x150,0xffff),(0,0x10,0x80,0xff)):
  m,o,t=seed();put(m,o+0x57,char,1);put(m,o+10,y);put(m,o+0xb5,b5,1)
  yield 'decision-bank9-character',m,8,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-controller15-royal-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/cpu_controller15_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_state5_exec.c').read_bytes()).hexdigest()
 lib=frozen/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_controller15_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1dfe2);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);st=native.wf_cpu_controller15_oracle(b,d,slot)
  assert st==0,(name,'native blocked',hex(-st))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',[hex(x)for x in d],[hex(cpu.r_reg(i))for i in range(8)])
  count+=1
 guards=0
 if not a.natural_only:
  for kind,dependency in [('decision-link',0x1f0f4),('decision-id',0x1f0ea)]:
   m,o,t=seed();put(m,o+10,0x150);put(m,o+0xb5,0,1)
   if kind=='decision-link':put(m,o+0x26,0x123456,4)
   if kind=='decision-id':put(m,o+0x57,12,1)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
   st=native.wf_cpu_controller15_oracle(b,d,8);assert st==-dependency,(kind,hex(-st),hex(dependency));guards+=1
  for pc in (0x1e96e,0x1e976,0x1e988,0x1e9a6,0x1ea2e,0x24cc,0x24e4,0x24e8,0x24f2):
   assert pc in coverage,('missing ROM branch',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG','B6.bit3/controller16/decision-bank9'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend Royal400; no physical SaturnRAM/timing claim','Only controller15 Royal1E96E newly admitted; no match setup reset or fixed selection','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'Royal controller15 whole ROM calls without intercepts')
