"""Verified Stage4 Royal1840 raw04 against intact original 1C03E, no intercepts."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
meta=json.loads((HERE/'action04_manifest.json').read_bytes());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12624:0x12628]==bytes.fromhex('00012bda')
assert rom[0x12bda:0x12c22]==bytes.fromhex('00012c220001000400080010ff00002000a400a500a6017a00012c220001000400020004ff00002000a200a300a6017a00012c220001000400040008ff00002001cb01cc00a6017a')
capture=(HERE/'action04_snapshot_beetle_frame1840.bin').read_bytes();assert len(capture)==16440 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
rng=random.Random(0x12c22);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=6):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);o=actors[slot];t=actors[(slot+1)%9];q=actors[(slot+2)%9]
 for i,a in enumerate(actors):
  put(m,a+2,i%12);put(m,a+0x26,actors[(i+1)%9],4);put(m,a+0x86,actors[(i+2)%9],4);put(m,a+0x92,0,4)
  put(m,a+6,0x280);put(m,a+10,0x160);put(m,a+14,0x140);put(m,a+0x2e,0);put(m,a+0x20,4);put(m,a+0x64,0x12)
  for off in(0x32,0x33,0x34,0x36,0x37):put(m,a+off,0,1)
 put(m,o+0x1c,0x8005);put(m,o+0x20,0x8005);put(m,o+0x60,4);put(m,o+0x24,0);put(m,o+0x22,1)
 put(m,RAM+0x161,1,1);put(m,RAM+0x7c,0x8000);return m,o,t,q
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:]
 yield 'natural-Beetle-Stage4-Royal1840',m,6,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,directed,face,flags33,z in itertools.product((0,4,8),(0,2),(0,0x8000,0x0080,0xffff),(0,4,0x80,0x94),(0,0xfff0)):
  m,o,t,q=seed(slot);put(m,o+0x1c,5);put(m,o+0x32,0x11|directed,1);put(m,o+0x33,flags33,1);put(m,o+0x2e,face);put(m,o+14,z)
  yield 'init-preset10-or-directed-sign',m,slot,None
 for slot,selector,phase,timer,z,f33 in itertools.product((0,4,8),(4,0x4004,0x8004,0xff04),(0,1,2,3,0xfe,0x100,0x101,0x102,0x1fe),(0,1,0xff00),(0x120,0x121,0x160,0x161,0xffff),(0,4)):
  m,o,t,q=seed(slot);put(m,o+0x60,selector);put(m,o+0x24,phase);put(m,o+0x22,timer);put(m,o+14,z);put(m,o+0x33,f33,1)
  yield 'BYTE-phase-WORD-timer-height-profile',m,slot,None
 for mode,credit,relation,tag,back,flags34,skip,id in itertools.product((0,1,2,3),(0,1,0x8000),('self','peer'),(0,0x40000000,0xff000000),(0,0x80000000),(0,8),(0,2),(0,11)):
  m,o,t,q=seed();c=o if relation=='self'else t;put(m,o+0x92,c|tag,4);put(m,c+0x92,o|back,4);put(m,c+0x64,0xab12);put(m,c+0x86,q,4);put(m,q+0x20,0x8001);put(m,q+0xae,0xcd01)
  # Self alias must retain a reciprocal canonical raw pointer when back==0.
  if c==o:put(m,o+0x92,o|tag|back,4)
  put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,o+0x34,flags34,1);put(m,o+0x33,skip,1);put(m,o+2,id);put(m,o+14,0x161);put(m,o+0x24,0x101)
  yield 'capture-reciprocal-rawLONG-partner-RNG',m,6,None
 for slot,relation,tag,third,flags34,face in itertools.product((0,4,8),('self','peer'),(0,0x80000000,0xff000000),('self','target','third'),(0,8),(0,0x8000)):
  m,o,t,q=seed(slot);t=o if relation=='self'else t;put(m,o+0x26,t|tag,4);put(m,t+0x26,{'self':o,'target':t,'third':q}[third]|tag,4);put(m,o+0x60,0x8004);put(m,o+0x24,0x101);put(m,o+0x22,0);put(m,o+0x34,flags34,1);put(m,o+0x2e,face)
  for a in(o,t,q):
   for off,val in((6,0xfff8),(10,0),(14,0xffff),(8,0x8a5a),(12,0x5aa5),(16,0x8001)):put(m,a+off,val)
  yield 'linked-release-third-actor-alias-order-wrap',m,slot,None
 for mode,phase,contact,bits,face in itertools.product((0,1,2,3),(0,1,2,3,0xfe,0x102),(0,1,3,5,0xff),(0,1,2,3,0x10,0x13),(0,0x8000)):
  m,o,t,q=seed();put(m,RAM+0x161,mode,1);put(m,o+0x24,phase);put(m,o+0x36,contact,1);put(m,o+0x37,bits,1);put(m,o+0x2e,face);put(m,o+14,0x170)
  for at in(0x1150,0x1180,0x11b0,0x11e0):put(m,RAM+at,0x8000)
  yield 'landing-rope-terminal-priority',m,6,None
 for bad,latch,phase,z in itertools.product((0,0x80000000,0xdeadbeef),(5,0x8005),(0,2,3,0xfe),(0,0xffff)):
  m,o,t,q=seed();put(m,o+0x26,bad,4);put(m,o+0x1c,latch);put(m,o+0x24,phase);put(m,o+14,z)
  yield 'unused-link-never-blocks',m,6,None
 for mode,third,face in itertools.product((0,1),(0,8),(0,0x8000)):
  m,o,t,q=seed();put(m,o+0x1c,5);put(m,o+0x2e,face);put(m,RAM+0x161,mode,1)
  regs=[rng.getrandbits(32)for _ in range(8)];finished=False
  for step in range(128):
   if step==1:
    put(m,o+0x92,t,4);put(m,t+0x92,o,4);put(m,o+14,0x170);put(m,o+0x34,third,1);put(m,q+0x20,1);put(m,q+0xae,1)
   if step>=40:put(m,o+0x37,0x10,1)
   yield 'sequence-live-RNG-capture-impact-landing',m,6,regs
   m=bytearray(b.raw[:0x200000]);regs=list(d)
   if (int.from_bytes(m[o+0x20:o+0x22],'big')&255)!=5:finished=True;break
  assert finished,('raw04 sequence did not finish',mode,third,face)
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action04-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action04_oracle_bridge.c';target=frozen/bridge;target.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,target)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(source).hexdigest();sources={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+[bridge]}
 def owner_hash(data):
  if b'/* $12614[$04]'not in data:return None
  start=data.index(b'/* $12614[$04]');end=data.index(b'/* $12614[$05]',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x04u:[^\n]*',data,re.M)[0]).hexdigest()
 raw04_hash=owner_hash(source)
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action04_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  assert cpu.r_pc()==0x70000,(name,count,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();st=native.action04_oracle(b,d,slot,ctypes.byref(nd));assert st==0,(name,count,'native blocked',hex(-st));assert nd.value==descriptor,(name,count,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for bad in(0,0x80000000,0xdeadbeef):
   for route in('candidate','impact','third'):
    if route=='candidate'and not bad:continue
    m,o,t,q=seed();put(m,o+0x24,1);put(m,o+0x22,0)
    if route=='candidate':put(m,o+14,0x161);put(m,o+0x92,bad,4);dep=0x12cdc
    else:
     put(m,o+0x60,0x8004)
     if route=='impact':put(m,o+0x26,bad,4);dep=0x12d7a
     else:put(m,o+0x34,8,1);put(m,t+0x26,bad,4);dep=0x12db6
    b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32();st=native.action04_oracle(b,d,6,ctypes.byref(nd));assert st==-dep,(route,bad,hex(-st),hex(dep));guards+=1
  m,o,t,q=seed();put(m,o+0x92,t,4);put(m,t+0x92,o,4);put(m,o+14,0x161);put(m,RAM+0x161,0,1);put(m,t+0x86,0xdeadbeef,4)
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action04_oracle(b,d,6,ctypes.byref(nd))==-0x110ee;guards+=1
  m,o,t,q=seed();put(m,o+0x92,t,4);put(m,t+0x92,o,4);put(m,o+14,0x161);put(m,o+2,0x100)
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action04_oracle(b,d,6,ctypes.byref(nd))==-0x110b6;guards+=1
  m,o,t,q=seed();put(m,o+0x60,0x8004);put(m,o+0x24,1);put(m,o+0x22,0);put(m,o+0x34,8,1);put(m,q+0x26,0xdeadbeef,4)
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action04_oracle(b,d,6,ctypes.byref(nd))==-0x10b9e;guards+=1
  m,o,t,q=seed();put(m,o+0x60,0xff);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action04_oracle(b,d,6,ctypes.byref(nd))==-0x12614;guards+=1
  required={0x12c22,0x12c48,0x12c56,0x12cc0,0x12cfa,0x12d12,0x12d22,0x12d6e,0x12dae,0x12dfe,0x12e24,0x12e50,0x12e8e,0x12eb8,0x12ef2,0x12f00,0x110e0,0x1108c,0x10b9a,0x21b4,0x1c0c8};assert required<=coverage,('missing ROM coverage',[hex(pc)for pc in required-coverage])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==raw04_hash,'Raw04 changed during oracle'
 report={'status':'PASS','cases':count,'guards':guards,'groups':groups,'source_sha256':source_hash,'raw04_owner_sha256':raw04_hash,'sources_sha256':sources,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM16KB','FG16KB','D0-D7','RNG','selected tick descriptor'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['Native Stage4 capture projected to supported raw RAM; unrepresented bytes zero','Consumed unknown pointers retain guards; unused pointers do not block','CCR/A registers and sound IO outside typed combat ABI; original helpers execute fully in oracle','No physical Saturn or full-match/timing equivalence claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS',count,'raw04 whole-ROM calls and',guards,'retained guards')
