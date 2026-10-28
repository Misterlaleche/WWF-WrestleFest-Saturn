"""Raw12 native-F8C TAG1524 and full jump/contact callback, intact 1C03E oracle."""
import argparse,ctypes as ct,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
snapshot=(here/'action12_snapshot_native_f8c_1524.bin').read_bytes();meta=json.loads((here/'action12_manifest.json').read_text());assert hashlib.sha256(snapshot).hexdigest()==meta['sha256']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x14826);coverage=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x7e,0,4);put(m,at+0x2e,0)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(m,at+off,0,1)
  put(m,at+0xfe,0);put(m,at+0x70,i%3);put(m,at+0x66,10);put(m,at+0x72,100);put(m,at+0x20,4);put(m,at+0x64,1)
 a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9];op=actors[(slot+3)%9]
 put(m,a+0x20,5);put(m,a+0x1c,0x8005);put(m,a+0x60,0x12);put(m,a+0x24,0);put(m,a+0x22,0);put(m,a+0x44,0)
 put(m,0x1c007c,1);put(m,0x1c0161,0,1);put(m,0x1c17e6,0);put(m,0x1c17ee,0)
 for k in range(6):put(m,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
 return m,a,t,partner,op
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
 yield 'native-F8C-HOST1524',m,4,struct.unpack_from('=8I',snapshot,24)
 if args.natural_only:return
 for slot,latch,frame,timer,role,alt in itertools.product((0,4,8),(5,0x8005),(0,1,3,5,0xfe,0x101,0x103,0x105,0x1fe),(0,1,0x100,0xff00),(0,0x80),(0,2)):
  m,a,t,partner,op=seed(slot);put(m,a+0x1c,latch);put(m,a+0x24,frame);put(m,a+0x22,timer);put(m,a+0x56,role,1);put(m,a+0x45,alt,1);put(m,a+0x37,0,1)
  yield 'phase/high-timer/descriptor',m,slot,None
 for state,sub,selector,face,dx,dy in itertools.product((4,5),(8,9,0x108),(0,0x61),(0,0x8000),(-105,-104,-8,0,8,104,105),(-73,-72,0,24,25)):
  m,a,t,partner,op=seed();put(m,a+0x24,5);put(m,a+0x37,0x10,1);put(m,a+0x2e,face);put(m,t+0x2e,face);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0x60,selector);put(m,t+0x32,0,1)
  put(m,a+6,0x250+dx);put(m,a+10,0x150+dy)
  yield '11152/11192/miss-boxes',m,0,None
 for phase,bad in itertools.product((0,1,2,4,0xfe),(0,0x80000000,0xdeadbeef)):
  m,a,t,partner,op=seed();put(m,a+0x24,phase);put(m,a+0x22,0x100);put(m,a+0x26,bad,4)
  yield 'unconsumed-link',m,0,None
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();count=0
with tempfile.TemporaryDirectory(prefix='wf-raw53-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'victim.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action12_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.wf_action12_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.POINTER(ct.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);regs=regs or[rng.getrandbits(32)for _ in range(8)]
  ram.w_block(0,bytes(m));cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM',hex(cpu.r_pc()))
  nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*regs);desc=ct.c_uint32();status=native.wf_action12_oracle(nb,nd,slot,ct.byref(desc))
  assert status==0,(name,'native',hex(-status));assert desc.value==descriptor,(name,'descriptor')
  for at,n in((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,(name,count,[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:10])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(name,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  count+=1
 guards=0
 if not args.natural_only:
  for phase,dep in ((3,0x148a2),(5,0x148e2)):
   m,a,t,partner,op=seed();put(m,a+0x24,phase);put(m,a+0x22,0);put(m,a+0x37,0x10,1);put(m,a+0x26,0xdeadbeef,4)
   nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*range(8));desc=ct.c_uint32();assert native.wf_action12_oracle(nb,nd,0,ct.byref(desc))==-dep;guards+=1
 required=(0x14826,0x14856,0x1486a,0x148e8,0x1492a,0x14934,0x14978,0x14986,0x149da,0x1c0c8)
 if not args.natural_only:assert all(x in coverage for x in required),[hex(x)for x in required if x not in coverage]
 machine.cleanup();report={'status':'PASS','calls':count,'retained_guards':guards,'source_sha256':hashlib.sha256((root/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'intercepts':[],'snapshot_sha256':meta['sha256'],'compared':['RAM8KB','FG16KB','D0-D7','RNG','compact queues'],'required_rom_pcs':[hex(x)for x in required],'limits':['Silent sound device; no full combat or hardware equivalence claim.']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
