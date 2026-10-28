"""Natural full frontend TAG frame319 controller15 against untouched ROM."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'cpu_controller15_snapshot_full_flow_319.bin').read_bytes()
assert len(capture)==16472 and struct.unpack_from('=6I',capture)==(319,4,0,0x1dfe2,79,1)
rng=random.Random(0x1e878);actors=[0x1c05b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=capture[56:8248]
 own=actors[slot];target=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 m[own:own+0x10c]=capture[56+0x9e0:56+0xaec]
 put(m,own+0x20,0x8005);put(m,own+0x60,0x4015);put(m,own+0xbc,1)
 put(m,own+0x7a,target,4);put(m,own+0x26,target,4);put(m,own+0x86,partner,4)
 put(m,partner+0x32,1,1);put(m,partner+0x34,0,1)
 put(m,0x1c007c,0x8000);put(m,0x1c0161,0,1);put(m,0x1c0162,0)
 return m,own,target,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=capture[56:8248]
 yield 'natural-full-frontend319',m,4,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,side,position,progress,tier in itertools.product((0,4,8),(0,0x80),range(12),range(10),range(3)):
  m,o,t,partner=seed(slot);put(m,o+0x57,6,1);put(m,o+0x33,side,1);put(m,o+0x31,position,1)
  put(m,0x1c0162,progress);put(m,o+0x70,(rng.randrange(256)<<8)|tier)
  yield f'bank-{slot}-{side}-{position}-{progress}-{tier}',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for slot,side,x,tier,timer in itertools.product((0,4,8),(0,0x80),(0,0x16c,0x16d,0x200,0x293,0x294,0x2d0,0x363,0x364,0xffff),range(3),(0,15,16,0xffff)):
  m,o,t,partner=seed(slot);put(m,o+0x57,5,1);put(m,o+0x33,side,1);put(m,o+6,x)
  put(m,t+0x70,tier);put(m,0x1c16a0,timer)
  yield f'id5-{slot}-{side}-{x}-{tier}-{timer}',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for selector,timer in itertools.product((0x15,0x4015,0x8015,0xc015,0xff15),(0,1,2,0xffff)):
  m,o,t,partner=seed(4);put(m,o+0x60,selector);put(m,o+0xbc,timer)
  yield f'edge-{selector}-{timer}',m,4,[rng.getrandbits(32)for _ in range(8)]
 for flags32,flags34,mode,credited in itertools.product((0,1,2,3,0xff),(0,1,2,0xff),(0,2),(0x8000,0x80ff)):
  m,o,t,partner=seed(4);put(m,partner+0x32,flags32,1);put(m,partner+0x34,flags34,1)
  put(m,0x1c0161,mode,1);put(m,0x1c007c,credited)
  yield 'partner-gates',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-controller15-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_controller15_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_controller15_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1dfe2);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);st=native.wf_cpu_controller15_oracle(b,d,slot)
  assert st==0,(name,'native blocked',hex(-st))
  for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',[hex(x)for x in d],[hex(cpu.r_reg(i))for i in range(8)])
  count+=1
 guards=0
 if not a.natural_only:
  for kind,dependency in [('partner',0x1e878),('link',0x1e8bc),('special-tier',0x1e8d8),('progression',0x1e93e),('normal-tier',0x1e94c)]:
   m,o,t,partner=seed(4)
   if kind=='partner':put(m,o+0x86,0x123456,4)
   if kind in ('link','special-tier'):
    put(m,o+0x57,5,1);put(m,o+6,0x2d0);put(m,o+0x33,0x80,1)
    put(m,0x1c16a0,16)
   if kind=='link':put(m,o+0x26,0x123456,4)
   if kind=='special-tier':put(m,t+0x70,3)
   if kind=='progression':put(m,0x1c0162,10)
   if kind=='normal-tier':put(m,o+0x71,0xff,1)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
   st=native.wf_cpu_controller15_oracle(b,d,4);assert st==-dependency,(kind,hex(-st),hex(dependency))
   guards+=1
  for pc in (0x1e878,0x1e886,0x1e898,0x1e8a4,0x1e8ae,0x1e8ca,0x1e8d8,0x1e8e4,0x1e8fa,0x1e91a,0x1e92a,0x1e930,0x1e958,0x1e9a6,0x24e4,0x24e8,0x24f2):
   assert pc in coverage,('missing ROM branch',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'state5_source_sha256':hashlib.sha256((ROOT/'src/wf_state5_exec.c').read_bytes()).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['Natural HOST frontend selection0/7, opponents6/3; not physical Saturn RAM','Credited TAG controller15; Royal1E96E qualified in cpu_controller15_royal_verify_rom.py','Progression index0..9 and in-bounds two-byte ROM rows only; unsupported rows fail at original table read','CCR/A registers are outside existing typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'controller15 whole ROM calls, no intercepts')
