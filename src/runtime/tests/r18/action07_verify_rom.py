"""Raw07 natural1200 and original13116 descriptor substitution versus ROM."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action07_snapshot_full_flow_1200.bin').read_bytes();assert len(capture)==16472
assert struct.unpack_from('=6I',capture)==(1200,1,1,0x1c03e,408,1)
assert rom[0x12614+7*4:0x12618+7*4]==bytes.fromhex('000130e6')
assert rom[0x130e6:0x13116]==bytes.fromhex('0001311600010004000400080014000400bf00c000c100c00001311600010004000400040014000400bf00c000c100c0')
rng=random.Random(0x13116);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 yield 'natural-full-flow1200',m,1,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,selector,latch,phase,timer,char in itertools.product((0,4,8),(7,0x4007,0x8007,0xff07),(5,0x8005),(0,1,2,3,0xfe,0x101,0x1fe),(0,1,0xff00,0xffff),(0,7,11,0x1007)):
  m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot]
  put(m,o+0x20,0x8005);put(m,o+0x1c,latch);put(m,o+0x60,selector);put(m,o+2,char);put(m,o+0x24,phase);put(m,o+0x22,timer)
  yield 'phase-character-descriptor',m,slot,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action07-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action07_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action07_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action07_oracle(b,d,slot,ctypes.byref(nd))
  assert st==0,(name,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7')
  count+=1
 guards=0
 if not a.natural_only:
  # $35 is now implemented; $8F is the first ID beyond the ROM table.
  for selector in (0x8f,0x408f,0xff8f):
   m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];o=actors[1];put(m,o+0x60,selector)
   b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values);nd=ctypes.c_uint32()
   assert native.action07_oracle(b,d,1,ctypes.byref(nd))==-0x12614
   assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values;guards+=1
  for pc in (0x13116,0x1311e,0x13124,0x13136,0x13142,0x13150,0x13164,0x1c0c8):
   assert pc in coverage,('missing ROM branch',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_hash,'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'bridge_sha256':hashlib.sha256((ROOT/bridge).read_bytes()).hexdigest(),'snapshot_sha256':hashlib.sha256(capture).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','unchanged RNG','tick descriptor'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['NaturalHOST full frontend frame1200; no physical Saturn RAM/timing claim','Raw07 only; invalid pointers are never dereferenced by this owner','CCR/A registers remain outside typed combat ABI']}
if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw07 whole ROM calls without intercepts')
