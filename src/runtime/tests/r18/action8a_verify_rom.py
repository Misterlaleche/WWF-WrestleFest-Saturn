"""NativeF8C HOST Royal3537 raw8a and boundaries against intact1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--snapshot-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
meta=json.loads((HERE/'action8a_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12614+0x8a*4:0x12618+0x8a*4]==bytes.fromhex('0001ac62');assert rom[0x1ac62:0x1ac72]==bytes.fromhex('0001ac72000100020020003001270000')
capture=(HERE/'action8a_snapshot_native_f8c_royal_3537.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x1ac72);coverage=set();count=0;names={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=8):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);o=actors[slot];t=actors[(slot+1)%9]
 put(m,o+0x1c,0x8005);put(m,o+0x20,0x8005);put(m,o+0x60,0x8a);put(m,o+0x26,t,4);put(m,o+0x24,0);put(m,o+0x22,1);put(m,o+0x2e,0);put(m,RAM+0x161,1,1);return m,o,t

def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-nativeF8C-HOST-Royal3537',m,8,struct.unpack_from('=8I',capture,24)
 if args.snapshot_only:return
 for slot,latch,selector,index,timer,face,mode in itertools.product((0,4,8),(5,0x8005),(0x8a,0x408a,0x808a,0xff8a),(0,1,2,0xfe,0x100,0x1fe,0xffff),(0,1,0xff00),(0,0x8000,0x0080),range(4)):
  m,o,t=seed(slot);put(m,o+0x1c,latch);put(m,o+0x60,selector);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,RAM+0x161,mode,1);yield 'phase-BYTE-WORD-modebit',m,slot,None
 for slot,relation,mode,face in itertools.product((0,4,8),('self','peer','taggedself','taggedpeer'),range(4),(0,0x8001,0x0080,0xff01)):
  m,o,t=seed(slot);link=o if 'self'in relation else t;tag=0xff000000 if 'tagged'in relation else 0;put(m,o+0x26,link|tag,4);put(m,o+0x24,0x100);put(m,o+0x22,0);put(m,o+0x2e,face);put(m,RAM+0x161,mode,1)
  for off,value in((6,0xfff8),(10,0),(14,0xffff),(8,0xa55a),(12,0x5aa5),(16,0x8001)):put(m,o+off,value)
  yield 'alias-position-word-wrap',m,slot,None
 for phase,timer,bad,latch in itertools.product((0,1,2,0xfe,0x1fe),(0,1,0xff00),(0,0x80000000,0xdeadbeef),(5,0x8005)):
  if phase==0 and timer==0 and latch==0x8005:continue
  m,o,t=seed();put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x26,bad,4);put(m,o+0x1c,latch);yield 'unused-link',m,8,None
 for char,flags32,own35,target35,mode in itertools.product((0,11,0x1234,0xffff),(0,0x10,0xff),(0,2,0xff),(0,4,0xff),(1,3)):
  m,o,t=seed();put(m,o+0x22,0);put(m,t+2,char);put(m,t+0x32,flags32,1);put(m,o+0x35,own35,1);put(m,t+0x35,target35,1);put(m,RAM+0x161,mode,1);yield 'Royal-target-BYTE-effect-flags',m,8,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action8a-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action8a_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  if b'/* $12614[$8A]'not in data:return None
  start=data.index(b'/* $12614[$8A]');end=data.index(b'/* $12614[$60]',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x8au:[^\n]*',data,re.M)[0]).hexdigest()
 raw8a_hash=owner_hash(source)
 source_map={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action8a_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.wf_action8a_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;names[name]=names.get(name,0)+1
 guards=0
 if not args.snapshot_only:
  for mode,bad in itertools.product((0,1,2,3),(0,0x80000000,0xdeadbeef)):
   m,o,t=seed();put(m,o+0x22,0);put(m,o+0x26,bad,4);put(m,RAM+0x161,mode,1);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();st=native.wf_action8a_oracle(b,d,8,ctypes.byref(nd));assert st==-(0x1aca0 if mode&1 else 0x1acc8),(mode,bad,hex(-st));guards+=1
  m,o,t=seed();put(m,o+0x60,0x35);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();assert native.wf_action8a_oracle(b,d,8,ctypes.byref(nd))==-0x12614;guards+=1
  required={0x1ac7c,0x1ac96,0x1aca0,0x1acc2,0x1acc8,0x1acf2,0x1acfc,0x10b9a,0x1c0c8};assert required<=coverage,('missing ROM branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==raw8a_hash,'Raw8A changed while oracle ran'
 report={'status':'PASS','cases':count,'guards':guards,'groups':names,'source_sha256':source_hash,'raw8a_owner_sha256':raw8a_hash,'sources_sha256':source_map,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM16KiB','FG16KiB','D0-D7','RNG','tick descriptor'],'intercepts':[],'limits':['HOST Royal3537 exploration starts from exact nativeF8C capture; later HOST/SH2 trajectories not claimed identical','Unknown consumed target pointers retain precise guards; no target pointer consumed on initial/terminal/nonimpact branches','Only raw8A added; existing8C not changed without a literal route dependency','CCR/A registers and physical input timing outside typed combat ABI','No whole-combat/hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
