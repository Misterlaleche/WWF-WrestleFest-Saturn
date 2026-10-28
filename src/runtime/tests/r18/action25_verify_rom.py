"""NativeF8C HOST Royal3209 raw25 and boundaries against intact1C03E."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--snapshot-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
meta=json.loads((HERE/'action25_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12614+0x25*4:0x12618+0x25*4]==bytes.fromhex('00016116');assert rom[0x16116:0x1612e]==bytes.fromhex('0001612e00010004000c000c0010001800f8012401250126')
capture=(HERE/'action25_snapshot_native_f8c_royal_3209.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[RAM+0x5b0+i*0x10c for i in range(9)];rng=random.Random(0x1612e);coverage=set();count=0;names={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=8):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);o=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140);put(m,at+0x2e,0)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x7e,0,4);put(m,at+0x70,0);put(m,at+0x66,10);put(m,at+0x72,100);put(m,at+0xfe,0)
 put(m,o+0x1c,0x8005);put(m,o+0x20,0x8005);put(m,o+0x60,0x25);put(m,o+0x26,t,4);put(m,o+0x24,0);put(m,o+0x22,1)
 for off in(0x36,0x3e,0x40,0x42):put(m,o+off,0)
 put(m,RAM+0x161,1,1);put(m,RAM+0x162,0);put(m,RAM+0x7c,0x8000);put(m,RAM+0x7e,3);return m,o,t,partner

def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-nativeF8C-HOST-Royal3209',m,8,struct.unpack_from('=8I',capture,24)
 if args.snapshot_only:return
 for slot,latch,selector,index,timer,face,done in itertools.product((0,4,8),(5,0x8005),(0x25,0x0825,0x8025,0xff25),(0,1,3,0xfe,0x100,0x1fe),(0,1,0xff00),(0,0x8000,0x0080),(0,0x80,0x8000)):
  m,o,t,partner=seed(slot);put(m,o+0x1c,latch);put(m,o+0x60,selector);put(m,o+0x24,index);put(m,o+0x22,timer);put(m,o+0x2e,face);put(m,o+0xfe,done);yield 'phase-width-selector-done',m,slot,None
 for scene,x,y,z,face in itertools.product(range(7),(0,0x1a0,0x250,0x3d0,0xffff),(0x118,0x160,0x198),(0x100,0x180),(0,0x8000)):
  m,o,t,partner=seed();put(m,o+0x1c,5);put(m,RAM+0x7e,scene);put(m,o+6,x);put(m,o+10,y);put(m,o+14,z);put(m,o+0x2e,face);yield 'initial-wall-resolver',m,8,None
 for credit,mode,flags,tflags,health,prior,role,config in itertools.product((0,1),range(4),(0,1,3,5),(0,1),(0,1),(0,8,0x80,0x88),(0,0x80),(0,0x2000,0x4000,0x6000)):
  m,o,t,partner=seed();put(m,o+0x24,0xfe);put(m,RAM+0x7c,credit);put(m,RAM+0x161,mode,1);put(m,o+0x33,flags,1);put(m,t+0x33,tflags,1);put(m,t+0x66,health);put(m,o+0x60,(prior<<8)|0x25);put(m,o+0x56,role,1);put(m,RAM+0x66,config)
  put(m,o+0x7e,partner,4);put(m,partner+0x66,0xfff8);put(m,partner+0x6a,0xffff);yield 'result111C8-gates-referee-health-message',m,8,None
 for credit,mode,cpu,flags,pcpu,phuman,apron,tier in itertools.product((0,1),(0,1),(0,1),(1,3),(0,1),(0,2),(0,0x80),(0,1,2)):
  m,o,t,partner=seed();put(m,o+0x24,0xfe);put(m,RAM+0x7c,credit);put(m,RAM+0x161,mode,1);put(m,o+0x32,cpu,1);put(m,o+0x33,flags,1);put(m,partner+0x32,pcpu,1);put(m,partner+0x33,phuman,1);put(m,partner+0x74,apron,1);put(m,partner+0x70,tier);yield 'partner215B6-routing',m,8,None
 for slot,relation,phase,done in itertools.product((0,4,8),('self','peer','taggedself','taggedpeer'),(0,0xfe),(0,0x8000)):
  m,o,t,partner=seed(slot);link=o if 'self'in relation else t;tag=0xff000000 if 'tagged'in relation else 0;put(m,o+0x26,link|tag,4);put(m,o+0x24,phase);put(m,o+0x22,0);put(m,o+0x1c,5 if phase==0 else 0x8005);put(m,o+0xfe,done);put(m,o+0x2e,0x8001)
  for off,value in((6,0xfff8),(10,0),(14,0xffff),(8,0xa55a),(12,0x5aa5),(16,0x8001)):put(m,o+off,value)
  yield 'alias-position-word-wrap',m,slot,None
 for phase,timer,bad in itertools.product((0,1,3,0x100),(0,1,0xff00),(0,0x80000000,0xdeadbeef)):
  m,o,t,partner=seed();put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+0x26,bad,4);yield 'unused-link',m,8,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action25-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action25_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest()
 def owner_hash(data):
  if b'/* $12614[$25]'not in data:return None
  start=data.index(b'/* $12614[$25]');end=data.index(b'/* $12614[$28]',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x25u:[^\n]*',data,re.M)[0]).hexdigest()
 raw25_hash=owner_hash(source)
 source_map={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action25_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.wf_action25_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)]);count+=1;names[name]=names.get(name,0)+1
 guards=0
 if not args.snapshot_only:
  for phase,done,bad in itertools.product((0,0xfe),(0,0x8000),(0,0x80000000,0xdeadbeef)):
   if phase==0 and done:continue
   m,o,t,partner=seed();put(m,o+0x1c,5 if phase==0 else 0x8005);put(m,o+0x24,phase);put(m,o+0x22,0);put(m,o+0xfe,done);put(m,o+0x26,bad,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();st=native.wf_action25_oracle(b,d,8,ctypes.byref(nd));expected=0x16142 if phase==0 else 0x161a0 if done else 0x161de;assert st==-expected,(phase,done,bad,hex(-st));guards+=1
  for kind,dependency in(('scene7',0x10b62),('TAG-own-partner',0x1123c),('TAG-target-partner',0x1124c),('partner',0x215e2),('partner-tier3',0x2172c),('cleanup-pointer',0x21368),('unowned35',0x12614)):
   m,o,t,partner=seed();put(m,o+0x24,0xfe);put(m,o+0x22,0);put(m,RAM+0x161,0,1);put(m,o+0x33,1,1);put(m,t+0x33,1,1)
   if kind=='scene7':put(m,o+0x1c,5);put(m,RAM+0x7e,7)
   if kind.startswith('TAG-')or kind=='cleanup-pointer':put(m,t+0x66,0);put(m,o+0x60,0x0825);put(m,o+0x56,0x80,1)
   if kind=='TAG-own-partner':put(m,o+0x86,0xdeadbeef,4)
   if kind=='TAG-target-partner':put(m,t+0x86,0xdeadbeef,4)
   if kind=='partner':put(m,o+0x86,0xdeadbeef,4)
   if kind=='partner-tier3':put(m,partner+0x32,1,1);put(m,partner+0x70,3)
   if kind=='cleanup-pointer':put(m,RAM+0x161,1,1);put(m,o+0x7e,0xdeadbeef,4)
   if kind=='unowned35':put(m,o+0x60,0x35)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();actual=native.wf_action25_oracle(b,d,8,ctypes.byref(nd));assert actual==-dependency,(kind,'guard',hex(-actual),hex(dependency));guards+=1
  required={0x16136,0x16142,0x16168,0x16176,0x1618e,0x161a0,0x161d4,0x161ec,0x161fe,0x16206,0x111c8,0x215b6,0x21358,0x2503c,0x2052,0x280dc,0x1c0c8};assert required<=coverage,('missing ROM branches',[hex(x)for x in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==raw25_hash,'Raw25 changed while oracle ran'
 report={'status':'PASS','cases':count,'guards':guards,'groups':names,'source_sha256':source_hash,'raw25_owner_sha256':raw25_hash,'sources_sha256':source_map,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['All RAM16KiB','FG16KiB','D0-D7','RNG','tick descriptor'],'intercepts':[],'limits':['HOST Royal3209 exploration starts from exact nativeF8C capture; later HOST/SH2 trajectories not claimed identical','Unknown consumed links, TAG partners, tier3 and resolver scene7 retain precise guards','Result transition raw8A has its own subsequent proof; unowned35 remains guarded','CCR/A registers and audio IO commands outside typed combat ABI','No whole-combat/hardware equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
