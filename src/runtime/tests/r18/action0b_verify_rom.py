"""Stage9 real raw0B vs whole intact1C03E; raw16KB/FG/D0-D7/descriptor."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action0b_manifest.json').read_text());capture=(HERE/'action0b_snapshot_beetle_stage9_frame8798.bin').read_bytes();assert len(capture)==16440 and hashlib.sha256(capture).hexdigest()==meta['sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x12614+0x0b*4:0x12618+0x0b*4]==bytes.fromhex('00013a1e');assert rom[0x13a1e:0x13a2e]==bytes.fromhex('00013a2e00010002ff00001000f100f2')
assert rom[0x25ca+35*6:0x25ca+36*6]==bytes.fromhex('fe0004000058')
rng=random.Random(0x13a2e);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140);put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x2e,0);put(m,at+0x20,0x8004);put(m,at+0x64,8)
 a=actors[slot];t=actors[(slot+1)%9]
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x400b);put(m,a+0x26,t,4);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x36,0);return m,a,t

def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-Beetle-stage9-TAG8798',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,latch,sub,face,targetface,x,targetx in itertools.product((0,4,8),(5,0x7f05),(8,9,0x109,0xff09),(0,0x8000),(0,0x8000),(0,0x250,0xffff),(0,0x250,0xffff)):
  m,a,t=seed(slot);put(m,a+0x1c,latch);put(m,t+0x64,sub);put(m,a+0x2e,face);put(m,t+0x2e,targetface);put(m,a+6,x);put(m,t+6,targetx);yield 'init-target-subBYTE-facing-WORD-position-wrap',m,slot,None
 for slot,phase,timer,contact,selector in itertools.product((0,4,8),(0,1,0xfe,0x100,0x1fe),(0,1,0xff00,0xffff),(0,0x10),(0x0b,0x400b,0x800b,0xff0b)):
  m,a,t=seed(slot);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x36,contact);put(m,a+0x60,selector);yield 'contact-priority-lowBYTE-phase-WORD-timer-selector',m,slot,None
 for state,sub,count in itertools.product((4,0x104,0x804,0x84,5),(8,9,0x108,0x109,7,0xff09),(0,0xff,0xffff)):
  m,a,t=seed();put(m,a+0x36,0x10);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0xd4,count);yield 'grounded-BYTE-state-sub8-9-WORD-D4-wrap',m,4,None
 for face,dx,dy in itertools.product((0,0x8000),(-105,-104,-103,-9,-8,-7,0,7,8,9,103,104,105),(-73,-72,-71,0,23,24,25)):
  m,a,t=seed();put(m,a+0x36,0x10);put(m,t+0x20,0x8005);put(m,t+0x60,0x8061);put(m,t+0x2e,face);put(m,a+6,0x250+dx);put(m,a+10,0x150+dy);put(m,t+0xc7,0xff,1);yield '11192-open-lower-inclusive-upper-hit-box',m,4,None
 for slot,other,tag,latch,sub in itertools.product((0,4,8),range(9),(0,0x40000000,0xff000000),(5,0x8005),(8,9)):
  m,a,t=seed(slot);t=actors[other];put(m,a+0x26,t|tag,4);put(m,a+0x1c,latch);put(m,t+0x64,sub);put(m,a+0x36,0x10);yield 'all-targets-self-alias-raw-tag-write-order',m,slot,None
 for phase,selector,bad in itertools.product((0,1,0xfe,0x1fe),(0x0b,0x800b),(0,0x80000000,0xdeadbeef,0x1c05b1)):
  m,a,t=seed();put(m,a+0x24,phase);put(m,a+0x60,selector);put(m,a+0x26,bad,4);yield 'no-contact-invalid-link-unused',m,4,None
 for slot,targetface,face,y,z in itertools.product((0,4,8),(0x80,0xff01,0x8080),(0x80,0xff01),(0,0xffff),(0,0xffff)):
  m,a,t=seed(slot);put(m,a+0x1c,5);put(m,t+0x64,9);put(m,t+0x2e,targetface);put(m,a+0x2e,face);put(m,a+10,y);put(m,a+14,z);yield 'init-facing-BYTE-padding-Y-Z-wrap',m,slot,None
 for hp,result,state,sub in itertools.product((0,1,0xffff),(0,0x4000,0x8001),(4,5),(8,9)):
  m,a,t=seed();put(m,a+0x36,0x10);put(m,a+0x66,hp);put(m,t+0x66,hp);put(m,a+0xa0,result);put(m,t+0xa0,result);put(m,t+0x20,state);put(m,t+0x64,sub);yield 'HP-results-RNG-preserved',m,4,None
 for slot,state,selector,contact in itertools.product(range(9),(0x8005,0x8085),(0x61,0x161,0x8061),(0,0x10)):
  m,a,t=seed(slot);put(m,t+0x20,state);put(m,t+0x60,selector);put(m,a+0x36,contact);put(m,t+0xc7,0xff,1);yield 'all-nine-slots-target-BYTE-state-selector',m,slot,None
 for targetx,targety,face,dx,dy in itertools.product((0,0xffff),(0,0xffff),(0,0x8000),(-104,-8,0,8,104),(-72,0,24)):
  m,a,t=seed();put(m,t+6,targetx);put(m,t+10,targety);put(m,t+0x2e,face);put(m,a+6,targetx+dx);put(m,a+10,targety+dy);put(m,t+0x20,5);put(m,t+0x60,0x61);put(m,a+0x36,0x10);yield '11192-WORD-window-wrap',m,4,None
 for slot,face in itertools.product((0,4,8),(0,0x8000)):
  m,a,t=seed(slot);put(m,a+0x1c,5);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
  # This is an explicit shared callback-event sequence, not a physics run.
  yield 'sequence-shared-contact-tape',m,slot,regs
  put(m,a+0x36,0x10)
  for tick in range(19):
   yield 'sequence-shared-contact-tape',m,slot,regs
   if m[a+0x21]!=5:break

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action0b-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action0b_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 sources={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/action0b_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action0b_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action0b_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)]);count+=1
  for actor in actors:
   for off in(0x66,0xa0):assert buf.raw[actor+off:actor+off+2]==m[actor+off:actor+off+2],(name,'unowned HP/result mutation')
  assert buf.raw[RAM+0x5c:RAM+0x60]==m[RAM+0x5c:RAM+0x60],(name,'unowned RNG mutation')
  if name=='sequence-shared-contact-tape':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  if count%1000==0:print('verified',count,flush=True)
 guards=0;prefixes=0
 if not args.natural_only:
  for slot,latch,bad,face in itertools.product((0,4,8),(5,0x8005),(0,0x80000000,0xdeadbeef,0x1c0f1c,0x1c05b1),(0,0xff01)):
   m,a,t=seed(slot);put(m,a+0x1c,latch);put(m,a+0x36,0x10);put(m,a+0x26,bad,4);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)];pc=0x13a38 if latch==5 else 0x13aba
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action0b_oracle(buf,dr,slot,ctypes.byref(desc));assert status==-pc,(slot,bad,'guard',hex(-status));guards+=1
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,a)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   for _ in range(20000):
    if cpu.r_pc()==pc:break
    cpu.execute(1)
   assert cpu.r_pc()==pc,'ROM guard prefix not reached'
   got=buf.raw[RAM:RAM+0x4000];want=bytes(mem.r_block(RAM,0x4000));assert got==want,('guard RAM prefix',[(hex(RAM+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],('guard Dregs prefix',list(dr),[cpu.r_reg(i)for i in range(8)]);prefixes+=1
  m,a,t=seed();put(m,a+0x60,0xfe);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();assert native.action0b_oracle(buf,dr,4,ctypes.byref(desc))==-0x12614;guards+=1
 if not args.natural_only and not args.guards_only:
  assert {0x13a40,0x13a62,0x13aa8,0x13ad2,0x13b16,0x13b4a,0x13b82,0x13b8a,0x11192,0x13b46,0x14422,0x1c0c8}<=coverage,'missing raw0B branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'guard_prefix_cases':prefixes,'source_sha256':source_sha,'sources_sha256':sources,'snapshot_sha256':meta['sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','original tickdescriptor'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
