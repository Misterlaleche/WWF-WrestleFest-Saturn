"""Raw3C native-F8C TAG915 exploration and intact1C03E boundaries."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--snapshot-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
meta=json.loads((HERE/'action3c_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1'];assert rom[0x12614+0x3c*4:0x12618+0x3c*4]==bytes.fromhex('00018022');assert rom[0x18022:0x18028]==bytes.fromhex('000180280000')
capture=(HERE/'action3c_snapshot_native_f8c_tag_915.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x18028);coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);o=actors[slot]
 put(m,o+0x1c,0x8005);put(m,o+0x20,0x8005);put(m,o+0x60,0x3c);put(m,o+2,slot);put(m,o+0x24,0);put(m,o+0x22,1)
 for off in(0x32,0x33,0x56,0x74,0x75):put(m,o+off,0,1)
 put(m,o+0xfe,0);return m,o
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-native-F8C-HOST915',m,5,struct.unpack_from('=8I',capture,24)
 if args.snapshot_only:return
 for slot,selector,latch,index,timer,fe in itertools.product((0,4,8),(0x3c,0x403c,0x803c,0xff3c),(5,0x8005),(0,1,2,0xfe,0x101,0x1fe,0xffff),(0,1,0xff00,0xffff),(0,0x80,0x8000,0xffff)):
  m,o=seed(slot);put(m,o+0x60,selector);put(m,o+0x1c,latch);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0xfe,fe);yield 'phase-width',m,slot,None
 for char,f32,f56,f33,obj in itertools.product(range(12),(0,8,0xff),(0,0x80),(0,4,0x20,0x24,0xff),(0,1,0x80,0x81)):
  m,o=seed();put(m,o+0x1c,5);put(m,o+2,char);put(m,o+0x32,f32,1);put(m,o+0x56,f56,1);put(m,o+0x33,f33,1);put(m,o+0x74,obj,1);put(m,o+0x75,obj,1);yield 'magnitude-descriptor',m,5,None
 for y,f33,obj in itertools.product((0,0x127,0x128,0x129,0x14f,0x150,0x151,0x8150,0xffff),(0,4),(0,0x80,0x81)):
  m,o=seed();put(m,o+0x1c,5);put(m,o+10,y);put(m,o+0x33,f33,1);put(m,o+0x74,obj,1);put(m,o+0x75,obj,1);yield 'Y-unsigned-threshold',m,5,None
 for selector,index,timer,badchar,badlink in itertools.product((0x3c,0x803c),(0,1,0x101),(0,1),(12,0xffff),(0,0x80000000,0xdeadbeef)):
  m,o=seed();put(m,o+0x60,selector);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+2,badchar);put(m,o+0x26,badlink,4);yield 'unused-graphics-link',m,5,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action3c-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action3c_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action3c_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(2000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.wf_action3c_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.snapshot_only:
  for char in(12,0xffff):
   m,o=seed();put(m,o+0x1c,5);put(m,o+2,char);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.wf_action3c_oracle(b,d,5,ctypes.byref(nd))==-0x1176a;guards+=1
  required={0x18038,0x18050,0x1805e,0x18074,0x1808e,0x18096,0x180a4,0x11710,0x11726,0x1172e,0x11736,0x11744,0x11780,0x1178e,0x1c0c8};assert required<=coverage,('missing branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 report={'status':'PASS','cases':count,'guards':guards,'groups':groups,'source_sha256':source_hash,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM8KiB','FG16KiB','D0-D7','RNG','tick descriptor'],'intercepts':[],'limits':['HOST915 exploration starts from exact nativeF8C capture; later HOST/SH2 trajectories are not claimed identical','Unknown initial graphics retain1176A; unused graphics and actor links never consumed','CCR/A and physical input timing outside ABI','No whole-combat/hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
