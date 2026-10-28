"""Natural nativeF8C TAG558 raw2C against intact original dispatch1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--snapshot-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
meta=json.loads((HERE/'action2c_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12614+0x2c*4:0x12618+0x2c*4]==bytes.fromhex('000168d8');assert rom[0x168d8:0x168f8]==bytes.fromhex('000168f80001000600080008000c0014000c0010010701080114011501160117')
capture=(HERE/'action2c_snapshot_native_f8c_tag_558.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x168f8);coverage=set();count=0;names={};feedback={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);o=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0)
 put(m,o+0x1c,0x8005);put(m,o+0x20,0x8005);put(m,o+0x60,0x2c);put(m,o+0x26,t,4);put(m,o+0x24,0);put(m,o+0x22,1);put(m,o+0x2e,0)
 for off in(0x36,0x3e,0x40,0x42):put(m,o+off,0)
 put(m,partner+0x20,1);put(m,partner+0xae,1);put(m,RAM+0x161,0,1);put(m,RAM+0x162,0);put(m,RAM+0x7c,0x8000);put(m,RAM+0x7e,3);return m,o,t,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-nativeF8C-HOST-TAG558',m,4,struct.unpack_from('=8I',capture,24)
 if args.snapshot_only:return
 for slot,latch,selector,index,timer,face in itertools.product((0,4,8),(5,0x8005),(0x2c,0x402c,0x802c,0xff2c),(0,1,2,3,4,5,0xfe,0x101,0x104,0x1fe,0xffff),(0,1,0xff00),(0,0x8000,0x0080)):
  m,o,t,partner=seed(slot);put(m,o+0x1c,latch);put(m,o+0x60,selector);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x2e,face);yield 'phase-BYTE-WORD-init-priority',m,slot,None
 for scene,x,y,z,face in itertools.product(range(7),(0,0x1a0,0x250,0x3d0,0xffff),(0x118,0x160,0x198),(0x100,0x180),(0,0x8000)):
  m,o,t,partner=seed();put(m,o+0x1c,5);put(m,RAM+0x7e,scene);put(m,o+6,x);put(m,o+10,y);put(m,o+14,z);put(m,o+0x2e,face);yield 'initial-resolver-borders',m,4,None
 for char,credit,flags,mode,state in itertools.product(range(12),(0,1,0x8000),(0,2),(0,1),(0,1,0xffffffff)):
  m,o,t,partner=seed();put(m,o+2,char);put(m,o+0x24,0x1fe);put(m,o+0x33,flags,1);put(m,RAM+0x7c,credit);put(m,RAM+0x161,mode,1);put(m,RAM+0x5c,state,4);yield 'terminal-char9-RNG-partner',m,4,None
 for slot,relation,phase,face in itertools.product((0,4,8),('self','peer','taggedself','taggedpeer'),('initial','impact'),(0,0x8001,0xff01)):
  m,o,t,partner=seed(slot);link=o if 'self'in relation else t;tag=0xff000000 if 'tagged'in relation else 0;put(m,o+0x26,link|tag,4);put(m,o+0x24,0 if phase=='initial'else 0x104);put(m,o+0x22,0);put(m,o+0x1c,5 if phase=='initial'else 0x8005);put(m,o+0x2e,face)
  for off,value in((6,0xfff8),(10,0),(14,0xffff),(8,0xa55a),(12,0x5aa5),(16,0x8001)):put(m,o+off,value)
  yield 'alias-order-face-coordinatewrap',m,slot,None
 for index,timer,bad in itertools.product((0,1,2,3,4,5,0xfe,0x104),(0,1),(0,0x80000000,0xdeadbeef)):
  if index&255==4 and timer==0:continue
  m,o,t,partner=seed();put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x26,bad,4);put(m,o+2,9);yield 'unused-link',m,4,None
 for state,ae,relation,mode in itertools.product((0,1,0x8001,2),(0,1,0xff01),('self','target','peer','taggedpeer'),range(4)):
  m,o,t,partner=seed();linked=o if relation=='self'else t if relation=='target'else partner;put(m,t+0x86,linked|(0xff000000 if relation=='taggedpeer'else 0),4);put(m,linked+0x20,state);put(m,linked+0xae,ae);put(m,o+0x24,0xfe);put(m,o+2,8);put(m,RAM+0x161,mode,1);yield 'terminal-TAG-partner-alias-modebit',m,4,None
 for c7,flags,face in itertools.product((0,0xfc,0xff),(0,0x40,0xff),(0,0x8000,0x0080,0xff01)):
  m,o,t,partner=seed();put(m,o+0x24,4);put(m,o+0x22,0);put(m,t+0xc7,c7,1);put(m,o+0x33,flags,1);put(m,t+0x33,flags,1);put(m,o+0x2e,face);yield 'impact-C7wrap-hold-flags-faceBYTE',m,4,None
 for char,credit,flags,mode in itertools.product((0x109,0xff09,0x100,0x8008),(0,1),(0,2),range(4)):
  if char&255!=9 and credit and not flags:continue
  m,o,t,partner=seed();put(m,o+2,char);put(m,o+0x24,0xfe);put(m,o+0x33,flags,1);put(m,RAM+0x7c,credit);put(m,RAM+0x161,mode,1);yield 'terminal-graphicsBYTE-unconsumed-tableWORD',m,4,None
 for mode,bad in itertools.product((1,3),(0,0x80000000,0xdeadbeef)):
  m,o,t,partner=seed();put(m,o+0x24,0xfe);put(m,o+2,8);put(m,o+0x26,bad,4);put(m,RAM+0x161,mode,1);yield 'Royal-terminal-unused-link',m,4,None
 # Feed the verified result, including live RNG/D registers, into the next call.
 for slot,mode,face in itertools.product((0,4,8),(0,1),(0,0x8000)):
  m,o,t,partner=seed(slot);put(m,o+0x1c,5);put(m,o+0x2e,face);put(m,RAM+0x161,mode,1);regs=None
  for quantum in range(120):
   yield 'sequence-to-release',m,slot,regs
   m=bytearray(feedback['image']);regs=feedback['regs']
   if m[o+0x21]!=5:assert quantum>70;break
  else:raise AssertionError('sequence did not release')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action2c-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action2c_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  if b'/* $12614[$2C]'not in data:return None
  start=data.index(b'/* $12614[$2C]');end=data.index(b'wf_status_t wf_r17_dispatch_1c03e_r11(',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x2cu:[^\n]*',data,re.M)[0]).hexdigest()
 raw2c_hash=owner_hash(source)
 source_map={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action2c_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.wf_action2c_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;names[name]=names.get(name,0)+1;feedback['image']=b.raw[:0x200000];feedback['regs']=list(d)
 guards=0
 if not args.snapshot_only:
  for latch,bad in itertools.product((5,0x8005),(0,0x80000000,0xdeadbeef)):
   m,o,t,partner=seed();put(m,o+0x1c,latch);put(m,o+0x24,4);put(m,o+0x22,0);put(m,o+0x26,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();st=native.wf_action2c_oracle(b,d,4,ctypes.byref(nd));assert st==-(0x16904 if latch==5 else 0x16966),(latch,bad,hex(-st));guards+=1
  for bad in(0,0x80000000,0xdeadbeef):
   m,o,t,partner=seed();put(m,o+0x24,0xfe);put(m,o+2,8);put(m,t+0x86,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.wf_action2c_oracle(b,d,4,ctypes.byref(nd))==-0x110f2;guards+=1
   put(m,o+0x26,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));assert native.wf_action2c_oracle(b,d,4,ctypes.byref(nd))==-0x110ee;guards+=1
  m,o,t,partner=seed();put(m,o+0x24,0xfe);put(m,o+2,12);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.wf_action2c_oracle(b,d,4,ctypes.byref(nd))==-0x110b6;guards+=1
  m,o,t,partner=seed();put(m,o+0x60,0x35);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.wf_action2c_oracle(b,d,4,ctypes.byref(nd))==-0x12614;guards+=1
  required={0x16900,0x16928,0x16936,0x1694e,0x16966,0x169b2,0x169ba,0x169ce,0x169d8,0x169f8,0x10b62,0x10b9a,0x1108c,0x110e0,0x10bd0,0x2052,0x1c0c8};assert required<=coverage,('missing ROM branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==raw2c_hash,'Raw2C changed while oracle ran'
 report={'status':'PASS','cases':count,'guards':guards,'groups':names,'source_sha256':source_hash,'raw2c_owner_sha256':raw2c_hash,'sources_sha256':source_map,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM16KiB','FG16KiB','D0-D7','RNG','tick descriptor'],'intercepts':[],'limits':['Exact nativeF8C capture then HOST558; later HOST/SH2 trajectory equality not claimed','Consumed unknown links/partners and unsupported character RNG table retain exact guards','Audio commands outside combat ABI; original2052 instructions execute in ROM oracle','CCR/A registers and physical input timing outside typed combat ABI','No whole-combat/hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
