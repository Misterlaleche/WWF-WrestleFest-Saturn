"""Natural AE10 request prefix through intact1C8F4/1D5A2/1C5FA."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'cpu_pursuit_tag_manifest.json').read_text());capture=(HERE/'cpu_pursuit_tag_snapshot_full_flow_1148.bin').read_bytes()
assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x1d5aa);coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 put(m,o+0x20,0x8001);put(m,o+0xae,10);put(m,o+0x60,0x1c);put(m,o+0x34,2,1);put(m,o+0x33,1,1)
 put(m,o+0x7a,t,4);put(m,o+0x86,partner,4);put(m,o+0xb5,0x20,1);put(m,o+0xb7,0,1);put(m,o+0xbc,0x20);put(m,o+0xb0,0x80)
 put(m,o+6,0x280);put(m,o+10,0x160);put(m,o+0xbe,0x310);put(m,o+0xc0,0x13f);put(m,o+0x2e,0)
 put(m,t+0x20,4);put(m,t+0x64,1);put(m,t+0x33,0,1);put(m,t+0x2e,0);put(m,t+6,0x290);put(m,t+10,0x160)
 return m,o,t,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural1148',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,f33,f34,ae,selector,mode in itertools.product((0,4,8),(0,1,0x80,0x81,0xff),(0,2,0x82,0xff),(10,0x200a,0x800a),(0x1c,0x69),(0,1,2,0xfe)):
  m,o,t,partner=seed(slot);put(m,o+0x33,f33,1);put(m,o+0x34,f34,1);put(m,o+0xae,ae);put(m,o+0x60,selector);put(m,RAM+0x161,mode,1)
  yield 'request-widths',m,slot,None
 for slot,relation,side in itertools.product((0,4,8),('self','taggedself','peer','taggedpeer'),(1,0x81)):
  m,o,t,partner=seed(slot);link=o if 'self'in relation else partner;link|=0x80000000 if 'tagged'in relation else 0
  put(m,o+0x86,link,4);put(m,o+0x33,side,1);yield 'request-alias',m,slot,None
 for slot,bad,f33 in itertools.product((0,4,8),(0,0x80000000,0xdeadbeef),(0,0x80)):
  m,o,t,partner=seed(slot);put(m,o+0x33,f33,1);put(m,o+0x7a,bad,4);put(m,o+0x86,bad,4);yield 'reserve-unread-pointers',m,slot,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-pursuit-tag-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/cpu_pursuit_tag_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source_hash=hashlib.sha256((frozen/'src/wf_cpu_runtime.c').read_bytes()).hexdigest()
 lib=frozen/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_pursuit_tag_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_reg(8,actors[slot]);cpu.w_pc(0x1c8f4)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);st=native.wf_cpu_pursuit_tag_oracle(b,d,slot);assert st==0,(name,'native blocked',hex(-st))
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1
 guards=0
 if not args.natural_only:
  for kind,bad in itertools.product(('partner','target'),(0,0x80000000,0xdeadbeef)):
   m,o,t,partner=seed();put(m,o+(0x86 if kind=='partner'else 0x7a),bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));st=native.wf_cpu_pursuit_tag_oracle(b,d,4)
   assert st==-(0x1c608 if kind=='partner'else 0x1d5b2),(kind,hex(bad),hex(-st));guards+=1
  assert{0x1d5aa,0x1d5b0,0x1d5b2,0x1c5fa,0x1c602,0x1c60c,0x1c61a,0x1c622,0x1c62e,0x10be8,0x20c8}<=coverage
 assert 0x21b4 not in coverage and bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guards':guards,'source_sha256':source_hash,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM8KiB','FG16KiB','D0-D7','RNG'],'intercepts':[],'limits':['1D5A2 request connection only; other pending caller guards remain','Natural HOST1148 plus constructed request/continuation boundaries; no hardware RAM/timing claim','CCR/A registers outside typed combat ABI']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
