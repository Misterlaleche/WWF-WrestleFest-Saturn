"""Literal2187E scan and Royal CPU1D connection; no ROM intercepts."""
import argparse,ctypes as ct,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
snapshot=(here/'royal_candidate_snapshot_native_f8c_437.bin').read_bytes();meta=json.loads((here/'royal_candidate_manifest.json').read_text());assert hashlib.sha256(snapshot).hexdigest()==meta['snapshot_sha256']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x2187e);coverage=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 for i,at in enumerate(actors):
  put(m,at,0);put(m,at+0xb5,0,1);put(m,at+0xb6,0xff,1);put(m,at+0x32,0,1);put(m,at+0x56,0,1)
  put(m,at+6,0x250);put(m,at+10,0x150)
 a=actors[slot];t=actors[(slot+1)%9];put(m,a+0x26,t,4);put(m,a+0x60,0x1d)
 return m,a,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
 yield 'natural-native-F8C-HOST437',m,8,0,struct.unpack_from('=8I',snapshot,24)
 if args.natural_only:return
 for sel,ownrole,otherrole,linktag,flags in itertools.product((0x1d,0x48,0x148,0xff48),(0,0x20),(0,0x20),(0,0x80000000),range(32)):
  slot=flags%9;m,a,t=seed(slot);c=actors[4+flags%5]
  put(m,a+0x60,sel);put(m,a+0x56,ownrole,1);put(m,t+0x56,ownrole^0x20,1);put(m,a+0x26,t|linktag,4)
  put(m,c,0x8000 if flags&1 else 0);put(m,c+0xb5,(8 if flags&2 else 0)|(0x80 if flags&4 else 0),1);put(m,c+0x32,0x10 if flags&8 else 0,1);put(m,c+0x56,otherrole,1)
  yield 'qualification/word-byte/rawlink',m,slot,1,None
 for slot,sel,xy in itertools.product((0,4,8),(0x1d,0x48),((0,0),(1,0),(0x3ff,0),(0x400,0),(0xffff,0),(0xffff,2),(0x8000,0x8001))):
  m,a,t=seed(slot);put(m,a+0x60,sel)
  for i,c in enumerate(actors[4:]):
   put(m,c,0x8000);put(m,c+0x56,0x20 if sel==0x48 else 0,1);put(m,c+6,xy[0]);put(m,c+10,xy[1])
  put(m,a+6,0);put(m,a+10,0);put(m,t+6,0);put(m,t+10,0);put(m,t+0x56,0,1)
  yield 'ties/distance/wrap',m,slot,1,None
 for kind in ('all-inactive','self','canonical-target','tagged-target','tagged-self'):
  m,a,t=seed(4);put(m,a+0x60,0x48 if kind=='all-inactive'else 0x1d)
  if kind=='all-inactive':put(m,a+0x26,0xdeadbeef,4)
  else:
   c=a if 'self'in kind else t;put(m,c,0x8000);put(m,a+0x26,c|(0x80000000 if 'tagged'in kind else 0),4)
  yield 'unconsumed/raw-LONG-exclusion',m,4,1,None
 for tag in (0,0x80000000,0xff000000):
  m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
  a=actors[8];t=int.from_bytes(m[a+0x7a:a+0x7e],'big');put(m,t+0x26,actors[5]|tag,4)
  yield 'CPU-already-linked-target-returns-before-scan',m,8,0,None
 for tag,linked,already in itertools.product((0,0x80000000,0xff000000),(False,True),(False,True)):
  m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
  a=actors[8];t=int.from_bytes(m[a+0x7a:a+0x7e],'big');put(m,a+0x20,5);put(m,a+0x60,0x401d if already else 0x1d);put(m,a+0x7a,t|tag,4)
  if linked:put(m,t+0x26,actors[5],4)
  yield 'controller1D-raw-pointer-and-early-return',m,8,2,None
 for bad in (0,0x80000000,0xdeadbeef):
  m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
  a=actors[8];put(m,a+0x20,5);put(m,a+0x60,0x401d);put(m,a+0x7a,bad,4)
  yield 'controller1D-set-bit6-does-not-consume-target',m,8,2,None
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();count=0
with tempfile.TemporaryDirectory(prefix='wf-royal-candidate-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'scan.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/royal_candidate_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.wf_royal_candidate_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.c_uint,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_int)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=machine.cpu,machine.mem
 for name,m,slot,direct,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);regs=regs or[rng.getrandbits(32)for _ in range(8)]
  ram.w_block(0,bytes(m));cpu.w_sr(0x2000);cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x2187e if direct==1 else 0x1dfe2 if direct==2 else 0x1c8f4)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM',hex(cpu.r_pc()))
  nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*regs);candidate=ct.c_uint32();found=ct.c_int(-1)
  status=native.wf_royal_candidate_oracle(nb,nd,slot,direct,ct.byref(candidate),ct.byref(found));assert status==0,(name,'native',hex(-status))
  for at,n in((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,(name,count,[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:10])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(name,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  if direct==1:assert candidate.value==cpu.r_reg(9) and found.value==bool(cpu.r_sr()&4),(name,'A1/Z',candidate.value,found.value,cpu.r_reg(9),hex(cpu.r_sr()))
  count+=1
 guards=0
 if not args.natural_only:
  for bad in (0,0x80000000,0xdeadbeef):
   m,a,t=seed();put(m,a+0x60,0x48);put(m,a+0x26,bad,4);put(m,actors[4],0x8000)
   nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*range(8));candidate=ct.c_uint32();found=ct.c_int()
   assert native.wf_royal_candidate_oracle(nb,nd,0,1,ct.byref(candidate),ct.byref(found))==-0x218ce;guards+=1
  assert {0x2187e,0x218ce,0x21900,0x2192c,0x21936,0x2195c,0x21970,0x1e81c,0x1e824,0x1e838}<=coverage
 machine.cleanup();report={'status':'PASS','calls':count,'retained_guards':guards,'source_sha256':hashlib.sha256((root/'src/wf_state5_exec.c').read_bytes()).hexdigest(),'intercepts':[],'snapshot_sha256':meta['snapshot_sha256'],'compared':['RAM8KB','FG16KB','D0-D7','RNG','helper A1/Z'],'limits':['Only consumed selector48 nonactor links retain218CE guard.','HOST trajectory from read-only native F8C; no whole SH2 timing equivalence.']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
