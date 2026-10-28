"""Linked raw1D hold/effects and TAG/Royal partner handoff against intact1C03E."""
import argparse,ctypes as ct,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
snapshot=(here/'action1d_snapshot_full_flow_1040.bin').read_bytes();meta=json.loads((here/'action1d_manifest.json').read_text());assert hashlib.sha256(snapshot).hexdigest()==meta['sha256']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x15870);coverage=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x7e,0,4);put(m,at+0x2e,0)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(m,at+off,0,1)
  put(m,at+0xfe,0);put(m,at+0x70,i%3);put(m,at+0x66,10);put(m,at+0x72,100);put(m,at+0x20,4);put(m,at+0x64,1)
 a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9];put(m,t+0x26,a,4)
 put(m,a+0x20,5);put(m,a+0x1c,0x8005);put(m,a+0x60,0x1d);put(m,a+0x24,0);put(m,a+0x22,0)
 put(m,0x1c007c,1);put(m,0x1c0161,0,1);put(m,0x1c17e6,0);put(m,0x1c17ee,0)
 for k in range(6):put(m,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
 for i in range(11):m[0x1c1258+i*0x2a]|=0x80
 return m,a,t,partner
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
 yield 'natural1040',m,5,struct.unpack_from('=8I',snapshot,24)
 if args.natural_only:return
 for slot,sel,latch,frame,timer in itertools.product((0,5,8),(0x1d,0x401d,0x801d),(False,True),(0,1,0x100,0x101,0x1fe,0xfe),(0,1,0xff00)):
  m,a,t,partner=seed(slot);put(m,a+0x1c,0x8005 if latch else 5);put(m,a+0x60,sel);put(m,a+0x24,frame);put(m,a+0x22,timer)
  yield 'phase/capture',m,slot,None
 for graphic,phase,face,mode in itertools.product(range(12),(0,1,0xfe),(0,0x8000),(0,1,2,3)):
  m,a,t,partner=seed();put(m,t+2,graphic);put(m,a+0x24,phase);put(m,a+0x2e,face);put(m,0x1c0161,mode,1)
  # Both original Royal20F04 branches are now exercised: CPU immediate
  # return and the human2187E selection scan.
  if mode&1:put(m,a+0x56,0x80 if graphic&1 else 0,1)
  yield 'graphics/royal-cpu',m,5,None
 for distance,pflags,oflags,p32,tier,already in itertools.product((0,79,80,119,120,121,160,0xffff),(0,2),(1,3),(0,1),range(3),(False,True)):
  m,a,t,partner=seed();put(m,a+0x24,0xfe);put(m,a+0x45,int(already),1);put(m,a+0x33,oflags,1);put(m,partner+0x33,pflags,1)
  put(m,partner+0x32,p32,1);put(m,partner+6,(0x250+distance)&0xffff);put(m,partner+0x70,tier)
  yield 'TAG20F04partner',m,5,None
 for phase,timer,free in itertools.product((0,1,0x100,0xfe),(0,1),range(12)):
  m,a,t,partner=seed();put(m,a+0x24,phase);put(m,a+0x22,timer)
  if free<11:put(m,0x1c1258+free*0x2a,0,1)
  yield 'effects8/9',m,5,None
 for state,sub,locked,abort in itertools.product((0,4,5,0x104),(1,2,0x101),(0,1),(0,0x80)):
  m,a,t,partner=seed();put(m,t+0x20,state);put(m,t+0x64,sub);put(m,a+0xfe,abort,1);put(m,t+0x54,locked)
  yield 'qualification',m,5,None
 for phase,timer,abort,tag in itertools.product((1,0xfe,0x101),(0,1),(0x80,0xff),(0,0x80000000,0xff000000)):
  m,a,t,partner=seed();put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0xfe,abort,1);put(m,a+0x26,t|tag,4)
  yield 'abort-byte-and-tagged-link',m,5,None
 for tag,backtag in itertools.product((0x12000000,0x80000000,0xff000000),(0,0x80000000)):
  m,a,t,partner=seed();put(m,a+0x26,t|tag,4);put(m,t+0x26,a|backtag,4)
  yield 'bus-alias-with-raw-LONG-reciprocal-compare',m,5,None
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();count=0
with tempfile.TemporaryDirectory(prefix='wf-raw1d-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'hold.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action1d_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.wf_action1d_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint]
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
  nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*regs);status=native.wf_action1d_oracle(nb,nd,slot)
  assert status==0,(name,'native',hex(-status))
  for at,n in((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,(name,count,[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:10])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(name,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  count+=1
 guards=0
 if not args.natural_only:
  for kind,expected in(('bad-graphics',0x159a2),('bad-tier',0x20fba),('bad-partner',0x1595a),('bad-capture',0x15884),('unowned-selector',0x12614)):
   m,a,t,partner=seed();put(m,a+0x24,0xfe);put(m,a+0x45,0,1)
   if kind=='royal-human':put(m,0x1c0161,1,1)
   if kind=='bad-graphics':put(m,t+2,0x100)
   if kind=='bad-tier':put(m,a+0x33,1,1);put(m,partner+0x32,1,1);put(m,partner+6,0x350);put(m,partner+0x70,3)
   if kind=='bad-partner':put(m,a+0x86,0x123456,4)
   if kind=='bad-capture':put(m,a+0x1c,5);put(m,a+0x26,0x123456,4)
   if kind=='unowned-selector':put(m,a+0x60,0x35)
   nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
   assert native.wf_action1d_oracle(nb,nd,5)==-expected,(kind,'guard');guards+=1
 required=(0x15870,0x15884,0x15890,0x158a8,0x158c2,0x158da,0x15900,0x15910,0x15928,0x15946,0x15950,0x1596c,0x15980,0x159c0,0x159c8,0x159da,0x159ec,0x20f04,0x20f50,0x20fac)
 if not args.natural_only:assert all(x in coverage for x in required),[hex(x)for x in required if x not in coverage]
 machine.cleanup();report={'status':'PASS','calls':count,'retained_guards':guards,'source_sha256':hashlib.sha256((root/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'intercepts':[],'snapshot_sha256':meta['sha256'],'compared':['RAM8KB','FG16KB','D0-D7','RNG','compact effects/queues'],'required_rom_pcs':[hex(x)for x in required],'limits':['Royal human20F04/2187E remains an explicit dependency.','Consumed actor pointers and graphics0..11/tier0..2 bounded.']}
 report['limits']=['Consumed actor pointers and graphics0..11/tier0..2 bounded.','Royal20F04 human2187E and CPU return compared through intact1C03E.']
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
