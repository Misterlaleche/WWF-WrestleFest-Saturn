"""Raw2A run/edge/countdown observed in native SH2 image, intact 1C03E oracle."""
import argparse,ctypes as ct,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
snapshot=(here/'action2a_snapshot_beetle_frame327.bin').read_bytes();meta=json.loads((here/'action2a_manifest.json').read_text());assert hashlib.sha256(snapshot).hexdigest()==meta['snapshot_sha256']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x1662c);coverage=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x7e,0,4);put(m,at+0x2e,0)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(m,at+off,0,1)
  put(m,at+0xfe,0);put(m,at+0x70,i%3);put(m,at+0x66,10);put(m,at+0x72,100);put(m,at+0x20,4);put(m,at+0x64,1)
 a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9];op=actors[(slot+3)%9]
 put(m,a+0x20,5);put(m,a+0x1c,0x8005);put(m,a+0x60,0x2a);put(m,a+0x24,0);put(m,a+0x22,0);put(m,a+0x44,0)
 put(m,0x1c007c,1);put(m,0x1c0161,0,1);put(m,0x1c17e6,0);put(m,0x1c17ee,0)
 for k in range(6):put(m,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
 return m,a,t,partner,op
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=snapshot[56:8248]
 yield 'Beetle327',m,4,struct.unpack_from('=8I',snapshot,24)
 if args.natural_only:return
 for slot,latch,frame,timer,count in itertools.product((0,4,8),(5,0x8005),(0,1,2,3,0xfe,0x103),(0,1,0xff00),(0,2,3,4,0x7fff,0x8000,0xffff)):
  m,a,t,partner,op=seed(slot);put(m,a+0x1c,latch);put(m,a+0x24,frame);put(m,a+0x22,timer);put(m,a+0x2a,count);put(m,a+0x36,0)
  yield 'phase/countdown',m,slot,None
 for mode,contact,bits in itertools.product(range(4),(0,1,3,5,0x81,0xff),range(256)):
  m,a,t,partner,op=seed();put(m,0x1c0161,mode,1);put(m,a+0x36,contact,1);put(m,a+0x37,bits,1)
  yield 'edge-original-carry',m,0,None
 for side,left,right in itertools.product((1,2,3),(0,1,0x100,0xffff),(0,1,0x100,0xffff)):
  m,a,t,partner,op=seed();put(m,a+0x36,1,1);put(m,a+0x37,side,1);put(m,0x1c1150,left);put(m,0x1c11b0,right)
  yield 'rope-lowbyte-priority',m,0,None
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();count=0
with tempfile.TemporaryDirectory(prefix='wf-raw53-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'victim.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action2a_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.wf_action2a_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.POINTER(ct.c_uint32)]
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
  nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*regs);desc=ct.c_uint32();status=native.wf_action2a_oracle(nb,nd,slot,ct.byref(desc))
  assert status==0,(name,'native',hex(-status));assert desc.value==descriptor,(name,'descriptor')
  for at,n in((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n));assert got==want,(name,count,[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:10])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(name,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  count+=1
 guards=0
 if not args.natural_only:
  m,a,t,partner,op=seed();put(m,a+0x60,0x35);nb=ct.create_string_buffer(bytes(m));nd=(ct.c_uint32*8)(*range(8));desc=ct.c_uint32()
  assert native.wf_action2a_oracle(nb,nd,0,ct.byref(desc))==-0x12614;guards=1
 required=(0x1662c,0x16636,0x16642,0x1664e,0x1665e,0x16664,0x16678,0x10fc6,0x1c0c8)
 if not args.natural_only:assert all(x in coverage for x in required),[hex(x)for x in required if x not in coverage]
 machine.cleanup();report={'status':'PASS','calls':count,'retained_guards':guards,'source_sha256':hashlib.sha256((root/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'intercepts':[],'snapshot_sha256':meta['snapshot_sha256'],'compared':['RAM8KB','FG16KB','D0-D7','RNG','compact queues'],'required_rom_pcs':[hex(x)for x in required],'limits':['Silent sound device; no full combat or hardware equivalence claim.']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
