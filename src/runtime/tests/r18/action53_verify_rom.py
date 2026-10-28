"""Raw53 held victim, literal 21114 TAG transaction, intact 1C03E oracle."""
import argparse,ctypes as ct,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
snapshot=(here/'action53_snapshot_full_flow_1054.bin').read_bytes();meta=json.loads((here/'action53_manifest.json').read_text());assert hashlib.sha256(snapshot).hexdigest()==meta['sha256']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x18e64);coverage=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x7e,0,4);put(m,at+0x2e,0)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(m,at+off,0,1)
  put(m,at+0xfe,0);put(m,at+0x70,i%3);put(m,at+0x66,10);put(m,at+0x72,100);put(m,at+0x20,4);put(m,at+0x64,1)
 a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9];op=actors[(slot+3)%9]
 put(m,a+0x20,5);put(m,a+0x1c,0x8005);put(m,a+0x60,0x53);put(m,a+0x24,0);put(m,a+0x22,0);put(m,a+0x44,0)
 put(m,0x1c007c,1);put(m,0x1c0161,0,1);put(m,0x1c17e6,0);put(m,0x1c17ee,0)
 for k in range(6):put(m,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
 return m,a,t,partner,op
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
 yield 'natural1054',m,0,struct.unpack_from('=8I',snapshot,24)
 if args.natural_only:return
 for slot,phase,latch,frame,timer in itertools.product((0,5,8),(0,1,2,4,5,6),(False,True),(0,1,0xfe,0x1fe),(0,1,0xff00)):
  m,a,t,partner,op=seed(slot);put(m,a+0x1c,0x8005 if latch else 5);put(m,a+0x44,phase);put(m,a+0x24,frame);put(m,a+0x22,timer)
  # Avoid the deliberately unowned Royal branch while stressing both phases.
  yield 'phase/descriptor',m,slot,None
 for graphic,face,tier,recovery in itertools.product(range(12),(0,0x8001),range(3),(0,1,0x4000,0x8000,0xffff)):
  m,a,t,partner,op=seed();put(m,a+0x1c,5);put(m,a+2,graphic);put(m,t+0x2e,face);put(m,a+0x70,tier);put(m,a+0xaa,recovery)
  yield 'initial-position/recovery',m,0,None
 for role,mash,recovery,frame,tagged in itertools.product((0,0x40,0x80,0xc0),(0,1,0xff),(0,1,2,0x4000,0xffff),(0,0xfe),(False,True)):
  m,a,t,partner,op=seed();put(m,a+0x56,role,1);put(m,a+0xa3,mash,1);put(m,a+0xaa,recovery);put(m,a+0x24,frame);put(m,a+0x60,0x8053 if tagged else 0x53)
  yield 'mash/end',m,0,None
 for distance,opcpu,pflags,locked,tier,tag in itertools.product((0,79,80,81,0xffff),(0,1),(0,2),(0,0x80),range(3),(0,0x80000000)):
  m,a,t,partner,op=seed();put(m,a+0x24,0xfe);put(m,a+0x33,1,1);put(m,t+0x86,op|tag,4);put(m,a+0x86,partner|tag,4)
  put(m,op+0x32,opcpu,1);put(m,op+6,(0x250+distance)&0xffff);put(m,partner+0x33,pflags,1);put(m,partner+0x74,locked,1);put(m,partner+0x70,tier)
  yield 'TAG21114',m,0,None
 for mode,credit,ownerflag,phase,tagged in itertools.product((0,1,2,3),(0,1),(0,1),(0,1,2),(False,True)):
  m,a,t,partner,op=seed();put(m,0x1c0161,mode,1);put(m,0x1c007c,credit);put(m,a+0x33,ownerflag,1);put(m,a+0x24,0xfe);put(m,a+0x44,phase);put(m,a+0x60,0x8053 if tagged else 0x53)
  yield 'gates',m,0,None
 for bad,distance in itertools.product((0,0x80000000,0xdeadbeef),(0,79)):
  m,a,t,partner,op=seed();put(m,a+0x24,0xfe);put(m,a+0x33,1,1);put(m,a+0x86,bad,4)
  put(m,op+0x32,1,1);put(m,op+6,0x250+distance)
  yield 'near-CPU-does-not-consume-partner',m,0,None
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();count=0
with tempfile.TemporaryDirectory(prefix='wf-raw53-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'victim.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action53_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.wf_action53_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);regs=regs or[rng.getrandbits(32)for _ in range(8)]
  ram.w_block(0,bytes(m));cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM',hex(cpu.r_pc()))
  nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*regs);status=native.wf_action53_oracle(nb,nd,slot)
  assert status==0,(name,'native',hex(-status))
  for at,n in((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,(name,count,[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:10])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(name,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  count+=1
 guards=0
 if not args.natural_only:
  for kind,expected in(('bad-graphics',0x18ebe),('bad-tier',0x10ca8),('bad-partner-tier',0x211ac),('bad-partner',0x2113c),('bad-target',0x18e9c),('phase3',0x18e7a)):
   m,a,t,partner,op=seed();put(m,a+0x24,0xfe);put(m,a+0x33,1,1)
   if kind=='royal':put(m,0x1c0161,1,1)
   if kind=='bad-graphics':put(m,a+0x1c,5);put(m,a+2,12)
   if kind=='bad-tier':put(m,a+0x1c,5);put(m,a+0xaa,0);put(m,a+0x70,3)
   if kind=='bad-partner-tier':put(m,op+0x32,1,1);put(m,op+6,0x350);put(m,partner+0x70,3)
   if kind=='bad-partner':put(m,a+0x86,0x123456,4)
   if kind=='bad-target':put(m,a+0x1c,5);put(m,a+0x26,0x123456,4)
   if kind=='phase3':put(m,a+0x44,3)
   nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
   assert native.wf_action53_oracle(nb,nd,0)==-expected,(kind,'guard');guards+=1
 required=(0x18e86,0x18e8e,0x18eac,0x18ede,0x18ef2,0x18f00,0x18f0e,0x18f38,0x21114,0x21168,0x2119e,0x211da)
 if not args.natural_only:assert all(x in coverage for x in required),[hex(x)for x in required if x not in coverage]
 machine.cleanup();report={'status':'PASS','calls':count,'retained_guards':guards,'source_sha256':hashlib.sha256((root/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'intercepts':[],'snapshot_sha256':meta['sha256'],'compared':['RAM8KB','FG16KB','D0-D7','RNG','compact queues'],'required_rom_pcs':[hex(x)for x in required],'limits':['Royal21114/2187E remains explicit.','Phase3 table entry and consumed graphics/pointers/tier remain guarded.']}
 report['limits']=['Phase3 table entry and consumed graphics/pointers/tier remain guarded.','Royal21114/2187E caller compared through intact1C03E.']
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
