"""Beetle Royal147 raw43 plus owner boundaries against intact1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--snapshot-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
meta=json.loads((HERE/'action43_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12614+0x43*4:0x12618+0x43*4]==bytes.fromhex('00018518');assert rom[0x18518:0x1853c]==bytes.fromhex('0001853c00010007000e00080010000e0006ff00001401b201b301b401b501b601b701b8')
capture=(HERE/'action43_snapshot_beetle_frame147.bin').read_bytes();assert len(capture)==8248 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x1853c);coverage=set();count=0;names={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=8):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 put(m,o+0x1c,0x8005);put(m,o+0x20,0x8005);put(m,o+0x60,0x43);put(m,o+2,slot);put(m,o+0x26,t,4);put(m,o+0x24,0);put(m,o+0x22,1);put(m,o+0x2e,0);put(m,o+0x56,0,1);put(m,o+0x33,0,1);put(m,o+0xee,0)
 put(m,t+0x56,0,1);put(m,t+0x86,partner,4);put(m,partner+0x20,1);put(m,partner+0xae,1)
 put(m,RAM+0x161,1,1);put(m,RAM+0x162,0);put(m,RAM+0x7c,0x8000);return m,o,t,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:];yield 'natural-Beetle-Royal147',m,8,struct.unpack_from('=8I',capture,24)
 if args.snapshot_only:return
 for slot,latch,selector,index,timer,face in itertools.product((0,4,8),(5,0x8005),(0x43,0x4043,0x8043,0xff43),(0,1,2,3,5,6,0xfe,0x101,0x102,0x1fe),(0,1,0xff00,0xffff),(0,0x8000,0x0080)):
  m,o,t,partner=seed(slot);put(m,o+0x1c,latch);put(m,o+0x60,selector);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0x37,0x10,1)
  yield 'phase-width',m,slot,None
 for mode,progress,own56,target56,called,state in itertools.product(range(4),(0,9),(0,0x80),(0,0x80),(0,1,3,4,0xff,0xffff),(0,1,0x55555555,0xffffffff)):
  m,o,t,partner=seed();put(m,o+0x1c,5);put(m,o+0x56,own56,1);put(m,t+0x56,target56,1);put(m,o+0xee,called);put(m,RAM+0x161,mode,1);put(m,RAM+0x162,progress);put(m,RAM+0x5c,state,4)
  yield 'initial-live-RNG',m,8,None
 for slot,char,credit,flags,mode in itertools.product((0,4,8),range(12),(0,1,0x8000),(0,2,0x80,0xff),range(4)):
  m,o,t,partner=seed(slot);put(m,o+2,char);put(m,o+0x24,0x1fe);put(m,o+0x22,0xff00);put(m,o+0x33,flags,1);put(m,RAM+0x7c,credit);put(m,RAM+0x161,mode,1);put(m,o+0x2e,0x8001 if char&1 else 0x0080)
  yield 'terminal-double-landing',m,slot,None
 for slot,relation,phase in itertools.product((0,4,8),('self','peer','taggedself','taggedpeer'),(0,1,2,5,0xfe)):
  m,o,t,partner=seed(slot);link=o if 'self'in relation else t;tag=0x80000000 if 'tagged'in relation else 0
  put(m,o+0x26,link|tag,4);put(m,o+0x24,phase);put(m,o+0x22,0);put(m,o+0x1c,5 if phase==0 else 0x8005);put(m,link+0x86,partner|tag,4);put(m,o+0x56,0x80,1)
  for off,value in((6,0xfff8),(10,0),(14,0xffff),(8,0xa55a),(12,0x5aa5),(16,0x8001)):put(m,o+off,value)
  yield 'alias-word-wrap',m,slot,None
 for phase,bad in itertools.product((1,2,5),(0,0x80000000,0xdeadbeef)):
  m,o,t,partner=seed();put(m,o+0x24,phase);put(m,o+0x22,0);put(m,o+0x26,bad,4);put(m,o+0x37,0x10,1);yield 'unread-link',m,8,None
 for index,timer,contact in itertools.product((5,0x105),(0,1,0xff00),(0,1,2,0x10,0xff)):
  m,o,t,partner=seed();put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x37,contact,1);yield 'landing-contact-width',m,8,None
 for state,ae,relation in itertools.product((0,1,0x8001,2),(0,1,0xff01),('owner','target','peer','taggedpeer')):
  m,o,t,partner=seed();p=o if relation=='owner'else t if relation=='target'else partner
  put(m,t+0x86,p|(0x80000000 if relation=='taggedpeer'else 0),4);put(m,p+0x20,state);put(m,p+0xae,ae);put(m,o+0x20,0x8005);put(m,o+0x24,0xfe);put(m,RAM+0x161,0,1)
  yield 'TAG-partner-low-byte-alias',m,8,None
 for slot,tag,own56,target56 in itertools.product((0,4,8),(0,0x40000000,0x80000000,0xff000000),(0,0x80),(0,0x80)):
  m,o,t,partner=seed(slot);put(m,o+0x1c,5);put(m,o+0x26,t|tag,4);put(m,o+0x56,own56,1);put(m,t+0x56,target56,1)
  yield 'initial-RNG-24bit-alias',m,slot,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action43-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action43_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  start=data.index(b'/* $12614[$43]');end=data.index(b'/* $12614[$75]',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x43u:[^\n]*',data,re.M)[0]).hexdigest()
 raw43_hash=owner_hash(source)
 source_map={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action43_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot]);descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.wf_action43_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;names[name]=names.get(name,0)+1
 guards=0
 if not args.snapshot_only:
  for phase,bad in itertools.product((0,0xfe),(0,0x80000000,0xdeadbeef)):
   m,o,t,partner=seed();put(m,o+0x1c,5 if phase==0 else 0x8005);put(m,o+0x24,phase);put(m,o+0x26,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();st=native.wf_action43_oracle(b,d,8,ctypes.byref(nd));assert st==-(0x18548 if phase==0 else 0x18604);guards+=1
  for kind,dependency in(('RNG-progression',0x113cd),('TAG-partner',0x110ee),('unowned35',0x12614)):
   m,o,t,partner=seed()
   if kind.startswith('RNG'):put(m,o+0x1c,5)
   if kind=='RNG-progression':put(m,RAM+0x161,0,1);put(m,RAM+0x162,10);put(m,t+0x56,0x80,1)
   if kind=='TAG-partner':put(m,o+0x24,0xfe);put(m,RAM+0x161,0,1);put(m,t+0x86,0xdeadbeef,4)
   if kind=='unowned35':put(m,o+0x60,0x35)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();actual=native.wf_action43_oracle(b,d,8,ctypes.byref(nd));assert actual==-dependency,(kind,'guard',hex(-actual),hex(dependency));guards+=1
  required={0x18544,0x18566,0x1856e,0x1857e,0x185ae,0x185c2,0x185e2,0x185fc,0x18604,0x1862e,0x16be2,0x16bee,0x110e0,0x21b4,0x2052,0x258e,0x1c0c8};assert required<=coverage,('missing ROM branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==raw43_hash,'Raw43 changed while oracle ran'
 report={'status':'PASS','cases':count,'guards':guards,'groups':names,'source_sha256':source_hash,'raw43_owner_sha256':raw43_hash,'sources_sha256':source_map,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM8KiB','FG16KiB','D0-D7','RNG','tick descriptor'],'intercepts':[],'limits':['Beetle Royal147 typed projection, not physical Saturn RAM','Unknown consumed pointers and unsupported1129E table rows retain precise guards','Reversal raw80 selected by this callback is a separate pending owner','CCR/A registers and audio IO commands outside typed combat ABI','No whole-combat/hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
