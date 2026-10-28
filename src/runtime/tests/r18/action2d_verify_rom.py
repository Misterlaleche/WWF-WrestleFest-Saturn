"""Raw2D16A16 natural3614 and full carry/signed-speed/tick ROM boundary matrix."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action2d_snapshot_full_flow_3614.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(3614,4,1,0x1c03e,1248,1)
assert rom[0x12614+0x2d*4:0x12618+0x2d*4]==bytes.fromhex('000169fa')
assert rom[0x169fa:0x16a06]==bytes.fromhex('00016a1600010001000a0095')
rng=random.Random(0x16a16);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot]
 put(m,o+0x20,0x8005);put(m,o+0x1c,0x8005);put(m,o+0x60,0x2d);put(m,o+0x24,0);put(m,o+0x22,1)
 put(m,o+0x36,0);put(m,RAM+0x161,0,1);return m,o
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-full-flow3614',m,4,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,selector,latch,speed,timer,face in itertools.product((0,4,8),(0x2d,0x402d,0x802d,0xff2d),(5,0x8005),(0,1,2,3,0x7fff,0x8000,0x8001,0xffff),(0,1,10,0xff00,0xffff),(0,0x8000,0xffff)):
  m,o=seed(slot);put(m,o+0x60,selector);put(m,o+0x1c,latch);put(m,o+0x2a,speed);put(m,o+0x22,timer);put(m,o+0x2e,face)
  yield 'signed-speed-init-tick',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for mode,kind,bits,speed in itertools.product(range(4),(0,1,2,3,5,0x81,0xff),(0,1,2,3,0x10,0x11,0x12,0x13,0xff),(0,2,0x8000)):
  m,o=seed();put(m,RAM+0x161,mode,1);put(m,o+0x36,kind,1);put(m,o+0x37,bits,1);put(m,o+0x2a,speed)
  yield 'rope-contact-priority',m,4,[rng.getrandbits(32)for _ in range(8)]
 for mode,kind,bits,left,right in itertools.product(range(4),(1,5),(1,2,3),(0,1,0x100,0x101,0xff00,0xffff),(0,1,0x100,0x101,0xff00,0xffff)):
  m,o=seed();put(m,RAM+0x161,mode,1);put(m,o+0x36,kind,1);put(m,o+0x37,bits,1)
  put(m,RAM+0x1150,left);put(m,RAM+0x11b0,right)
  yield 'rope-low-byte-gate',m,4,[rng.getrandbits(32)for _ in range(8)]
 for phase,timer in itertools.product((1,0xfe,0x100,0x1fe,0xffff),(0,1,0xff00,0xffff)):
  m,o=seed();put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x2a,2)
  yield 'tick-index-word-edges',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action2d-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action2d_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action2d_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(2000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action2d_oracle(b,d,slot,ctypes.byref(nd))
  assert st==0,(name,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7')
  count+=1
 guards=0
 if not a.natural_only:
  for selector in (0x8f,0x408f,0xff8f):
   m,o=seed();put(m,o+0x60,selector);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values);nd=ctypes.c_uint32()
   assert native.action2d_oracle(b,d,4,ctypes.byref(nd))==-0x12614
   assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values;guards+=1
  for pc in (0x16a16,0x16a1c,0x16a24,0x16a2c,0x16a3a,0x16a48,0x10ff0,0x11012,0x1102c,0x2052,0x1c0c8):
   assert pc in coverage,('missing ROM branch',hex(pc))
 machine.cleanup()
 if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','unchanged RNG','tick descriptor','rope-global low-byte gates'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend frame3614; no physical SaturnRAM/timing claim','Raw2D only; shared10FC6 reused without changing other owners','Sound2052 executes in ROM; silent native output device outside IO comparison','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw2D whole ROM calls without intercepts')
