"""Stage8b real raw7F vs whole intact1C03E; raw16KB/FG/D0-D7/descriptor."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action7f_manifest.json').read_text());capture=(HERE/'action7f_snapshot_beetle_stage8b_frame9016.bin').read_bytes();assert len(capture)==16440 and hashlib.sha256(capture).hexdigest()==meta['sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x12614+0x7f*4:0x12618+0x7f*4]==bytes.fromhex('0001a704');assert rom[0x1a704:0x1a718]==bytes.fromhex('0001a718000100030014001c000c011b011c011d')
rng=random.Random(0x1a718);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x20,0x8005)
 a=actors[slot];t=actors[(slot+1)%9]
 put(m,a+0x1c,0x8005);put(m,a+0x60,0x7f);put(m,a+0x26,t,4);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x44,2);put(m,a+0x2e,0);return m,a,t

def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-Beetle-TAG9016-HP0',m,1,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,latch,phase,timer,count44,face in itertools.product((0,1,8),(0,1),(5,0x8005),(0,2,0xfe,0x1fe,0xfffe),(0,1,0xff00),(0,1,2,0xffff),(0,0x8000)):
  m,a,t=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x44,count44);put(m,a+0x2e,face);yield 'latch-lowBYTE-phase-WORD-counter',m,slot,None
 for slot,other,tag,face,selector in itertools.product((0,1,8),range(9),(0,0x40000000,0xff000000),(0,0x8000,0x80,0xff01),(0x7f,0x407f,0x807f,0xff7f)):
  m,a,t=seed(slot);put(m,a+0x24,0xfe);put(m,a+0x44,1);put(m,a+0x2e,face);put(m,a+0x60,selector);put(m,a+0x26,actors[other]|tag,4);yield 'release-all-targets-self-tags-facing-padding',m,slot,None
 for phase,timer,count44,bad in itertools.product((0,2,0xfe,0x1fe),(0,0xff00),(0,2,0xff01),(0,0x80000000,0xdeadbeef,0x1c05b1)):
  m,a,t=seed();put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x44,count44);put(m,a+0x26,bad,4);yield 'unconsumed-invalid-link',m,1,None
 for latch,phase,bad in itertools.product((5,0x7f05),(0,0xfe,0x1fe),(0,0x80000000,0xdeadbeef)):
  m,a,t=seed();put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x26,bad,4);yield 'initial-counter-reset-before-FE-test',m,1,None
 for hp,otherhp,result,otherresult,counter in itertools.product((0,1,0xffff),(0,1),(0,0x4000,0x8001),(0,0x4000,0x8001),(1,2)):
  m,a,t=seed();put(m,a+0x24,0xfe);put(m,a+0x44,counter);put(m,a+0x66,hp);put(m,t+0x66,otherhp);put(m,a+0xa0,result);put(m,t+0xa0,otherresult);yield 'HP-and-results-never-modified',m,1,None
 for slot,face,selftarget,hp in itertools.product((0,1,8),(0,0x8000),(0,1),(0,37)):
  m,a,t=seed(slot);put(m,a+0x1c,5);put(m,a+0x2e,face);put(m,a+0x66,hp);put(m,a+0x26,a if selftarget else t,4);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(200):
   yield 'sequence-two-original-animation-cycles',m,slot,regs
   if m[a+0x61]!=0x7f:break
  else:raise AssertionError('raw7F did not release its pair')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action7f-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action7f_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/action7f_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action7f_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action7f_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)]);count+=1
  if name=='sequence-two-original-animation-cycles':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  if count%1000==0:print('verified',count,flush=True)
 guards=0
 if not args.natural_only:
  for slot,bad,face in itertools.product((0,1,8),(0,0x80000000,0xdeadbeef,0x1c0f1c,0x1c05b1),(0,0xff01)):
   m,a,t=seed(slot);put(m,a+0x24,0xfe);put(m,a+0x44,1);put(m,a+0x26,bad,4);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action7f_oracle(buf,dr,slot,ctypes.byref(desc));assert status==-0x1a758,(slot,bad,'guard',hex(-status));guards+=1
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,a)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   for _ in range(20000):
    if cpu.r_pc()==0x1a758:break
    cpu.execute(1)
   assert cpu.r_pc()==0x1a758,'ROM guard prefix not reached'
   got=buf.raw[RAM:RAM+0x4000];want=bytes(mem.r_block(RAM,0x4000));assert got==want,('guard RAM prefix',[(hex(RAM+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],('guard Dregs prefix',list(dr),[cpu.r_reg(i)for i in range(8)])
  m,a,t=seed();put(m,a+0x60,0xfe);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();assert native.action7f_oracle(buf,dr,1,ctypes.byref(desc))==-0x12614;guards+=1
 if not args.natural_only and not args.guards_only:
  assert {0x1a720,0x1a72e,0x1a734,0x1a73e,0x1a758,0x1a776,0x1a782,0x1a78e,0x1c0c8}<=coverage,'missing raw7F branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','original tickdescriptor'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
