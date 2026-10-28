"""Linked victim1B7A2 and alternate descriptor, intact1C03E/tail stack context."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1afd4+0x14*4:0x1afd8+0x14*4]==bytes.fromhex('0001b78a');assert rom[0x1b78a:0x1b7a2]==bytes.fromhex('0001b7a200010001000c01950001b7a200010001000c0196')
meta=json.loads((HERE/'state4_1b7a2_manifest.json').read_text());capture=(HERE/'state4_1b7a2_snapshot_full_flow_427.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
rng=random.Random(0x1b7a2);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);m[0xc0000:0xc4000]=rng.randbytes(0x4000);o=actors[slot]
 put(m,o+0x1c,0x8004);put(m,o+0x20,0x8004);put(m,o+0x64,0x14);put(m,o+0x92,actors[(slot+4)%9],4);return m,o
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural427',m,0,0,struct.unpack_from('=8I',capture,24)
 if a.natural_only:return
 for slot,latch,index,timer,face,prior,sub in itertools.product((0,4,8),(4,0x8004),(0,0xfe,0x1fe,0xffff),(0,1,12,0xff00,0xffff),(0,0x8000,0x80,0xffff),(0,2,0x8002,3),(0x14,0x8014)):
  m,o=seed(slot);put(m,o+0x1c,latch);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0x1e,prior);put(m,o+0x64,sub)
  yield 'dispatch-widths',m,slot,0,None
 # Callback seam has the original JSR/MOVEM return stack and runs the intact
 # common tail. It exposes descriptor1B796 even when no dispatch row selects it.
 for slot,sub,latch,index,timer,face,prior in itertools.product((0,4,8),(0x13,0x14,0x8014,0xff02),(4,0x8004),(0,0xfe,0x1fe,0xffff),(0,1,0xff00),(0,0x8000),(0,2,0xff02)):
  m,o=seed(slot);put(m,o+0x64,sub);put(m,o+0x1c,latch);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0x1e,prior)
  yield 'callback-descriptor',m,slot,1,None
 for slot,ptrkind in itertools.product((0,4,8),('self','taggedself','peer','taggedpeer','bytealias')):
  m,o=seed(slot);target=o if 'self'in ptrkind else actors[(slot+4)%9];ptr=target|(0x80000000 if 'tagged'in ptrkind else 0xff000000 if ptrkind=='bytealias'else 0)
  put(m,o+0x92,ptr,4);put(m,o+0x1c,4);yield f'alias-{slot}-{ptrkind}',m,slot,0,None
 for prior,face,xyz in itertools.product((0,2),(0,0x8000),(0,1,0xfff8,0xffff)):
  m,o=seed();put(m,o+0x1e,prior);put(m,o+0x2e,face);put(m,o+0x24,0x1fe);put(m,o+0x22,0xff00)
  for off in(6,10,14):put(m,o+off,xyz)
  yield 'terminal-wrap',m,0,0,None
 for ptr in(0,0x80000000,0x1c0001,0xdeadbeef):
  m,o=seed();put(m,o+0x92,ptr,4);put(m,o+0x24,0);put(m,o+0x22,1);yield 'unread-invalid-relation',m,0,0,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_r17_dispatch_1c03e.c')
with tempfile.TemporaryDirectory(prefix='wf-state4-1b7a2-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/state4_1b7a2_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest()
 lib=frozen/'state.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_state4_1b7a2_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,direct,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);cpu.w_reg(8,actors[slot]);cpu.w_reg(9,0x1b78a);cpu.w_reg(12,0x1b7a2)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  if direct:
   for off,value in((0,0x1c0ac),(4,regs[0]),(8,0),(12,0),(16,0x70000)):mem.w32(0x1d0000+off,value)
   cpu.w_pc(0x1b7a2)
  else:mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  descriptor=0
  for _ in range(2000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();st=native.wf_state4_1b7a2_oracle(b,d,slot,direct,ctypes.byref(desc))
  assert st==0,(name,'native blocked',hex(-st));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);assert b.raw[RAM+0x5c:RAM+0x60]==m[RAM+0x5c:RAM+0x60];count+=1
 guards=0
 if not a.natural_only:
  for ptr in(0,0x80000000,0x1c0001,0x1c0f1c,0xdeadbeef):
   m,o=seed();put(m,o+0x1c,4);put(m,o+0x92,ptr,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();st=native.wf_state4_1b7a2_oracle(b,d,0,0,ctypes.byref(desc));assert st==-0x1b7b6,(hex(ptr),hex(-st));guards+=1
  assert{0x1b7aa,0x1b7de,0x1b7e6,0x1b80c,0x1b83e,0x1b846,0x10bd8,0x1c0c8}<=coverage
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guards':guards,'source_sha256':source_hash,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM8KiB/FG16KiB','D0-D7/RNG','Selected tick descriptor'],'intercepts':[],'limits':['Natural HOST427 plus constructed callback/tail contexts; no full combat/hardware equality','Unknown initial relation pointers retain precise guard','CCR/A registers remain outside typed combat ABI']}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
