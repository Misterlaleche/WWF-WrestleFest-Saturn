"""Natural TAG punch9693 action1B vs intact full1C03E, raw16KB/Dregs/FG."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action1b_manifest.json').read_text());capture=(HERE/'action1b_snapshot_punch_9693.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x12614+0x1b*4:0x12618+0x1b*4]==bytes.fromhex('00015384');assert rom[0x15384:0x153a0]==bytes.fromhex('000153bc00010005000800180018ff00001000840085015b015c015c')
rng=random.Random(0x153bc);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,i%3);put(m,at+0x20,1);put(m,at+0x64,1)
 a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x1b);put(m,a+0x26,t,4);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0x44,2);put(m,a+0x37,0,1)
 put(m,RAM+0x161,0,1);put(m,RAM+0x7c,0x8000);return m,a,t,partner
def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-TAG-punch9693',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,latch,phase,timer,face,route,contact in itertools.product((0,4,8),range(4),(5,0x8005),(0,1,2,3,4,0xfe,0x103,0x1fe),(0,1,0xff00),(0,0x8000),(0,1,2),(0,0x10)):
  # Full initial cross-products repeat identical route selection; retain a
  # smaller independent constructor matrix below for those branches.
  if latch==5 and(phase!=0 or timer!=0 or route!=0 or contact!=0):continue
  m,a,t,partner=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x2e,face);put(m,a+0x44,route);put(m,a+0x37,contact,1)
  yield 'phase-WORD-BYTE-contact-mode',m,slot,None
 for role,state,selector,face,x in itertools.product((0,8),(1,5),(0x53,0x153,0x52),(0,0x80,0x8000,0xff01),(0x230,0x250,0x270)):
  m,a,t,partner=seed();put(m,a+0x1c,5);put(m,a+0x34,role,1);put(m,t+0x20,state);put(m,t+0x60,selector);put(m,a+0x2e,face);put(m,a+6,x);yield 'init-route-and-facing',m,4,None
 for dx,dy,selector in itertools.product((-280,-279,-256,-255,-24,-23,-1,0,1,23,24,255,256,257,279,280),(-276,-275,-256,-255,-20,-19,0,19,20,255,256,275,276),(0x53,0x153)):
  m,a,t,partner=seed();put(m,a+0x24,3);put(m,a+0x37,0x10,1);put(m,a+0x44,1);put(m,t+0x20,5);put(m,t+0x60,selector);put(m,a+6,0x250+dx);put(m,a+10,0x150+dy);yield 'route1-BYTE-distance-unsigned-borrow',m,4,None
 for mode,state,sub,dx,dy,flags in itertools.product(range(4),(0,1,4,5),(1,0x101),(-49,-48,-47,0,47,48,49),(-49,-48,-47,0,47,48,49),(0,4)):
  if state!=4 and sub!=1:continue
  m,a,t,partner=seed();put(m,RAM+0x161,mode,1);put(m,a+0x24,3);put(m,a+0x37,0x10,1);put(m,a+0x44,2);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0x33,flags,1);put(m,a+6,0x250+dx);put(m,a+10,0x150+dy);yield 'route2-WORD-box-state-mode',m,4,None
 for face,ptr,tag in itertools.product((0,0x80,0x8000,0xff01),(actors[0],actors[5],0x1c0f1c,0x1c1028),(0,0x40000000,0xff000000)):
  m,a,t,partner=seed();put(m,a+0x24,3);put(m,a+0x37,0x10,1);put(m,a+0x44,2);put(m,a+0x2e,face);put(m,t+0x74,0x80,1);put(m,t+0x76,ptr|tag,4);put(m,t+0xc7,0xff,1);yield 'route2-object-release-aliases',m,4,None
 for phase,timer,bad in itertools.product((0,1,2,3,4,0xfe,0x103,0x1fe),(0,1,0xff00),(0,0x80000000,0xdeadbeef)):
  m,a,t,partner=seed();put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x26,bad,4);yield 'unconsumed-target',m,4,None
 for flags,firsttag,secondtag,tier in itertools.product((0,2,0x80,0x82),(0,0x40000000),(0,0xff000000),range(3)):
  m,a,t,partner=seed();linked=actors[7];second=actors[0];put(m,a+0x24,3);put(m,a+0x37,0x10,1);put(m,a+0x44,0);put(m,a+0x86,partner|firsttag,4);put(m,partner+0x33,flags,1);put(m,partner+0x26,linked,4);put(m,linked+0x86,second|secondtag,4);put(m,second+0x33,flags^0x80,1);put(m,second+0x70,tier);yield 'route0-21424-live-team-pointers',m,4,None
 for target,tag,latched in itertools.product((0,1),(0,0x40000000,0xff000000),(0,1)):
  m,a,t,partner=seed();put(m,a+0x26,(a if target==0 else t)|tag,4);put(m,a+0x1c,0x8005 if latched else 5);put(m,a+0x24,1 if latched else 0);yield 'init-self-and-tagged-links',m,4,None
 for route in(1,2):
  m,a,t,partner=seed();put(m,a+0x1c,5);put(m,a+0x34,0,1);put(m,t+0x20,5 if route==1 else 1);put(m,t+0x60,0x53 if route==1 else 0);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(160):
   if int.from_bytes(m[a+0x24:a+0x26],'big')==3:put(m,a+0x37,m[a+0x37]|0x10,1)
   yield 'sequence-init-to-FE',m,4,regs
   if int.from_bytes(m[a+0x20:a+0x22],'big')==0:break
  else:raise AssertionError('raw1B sequence did not terminate')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action1b-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action1b_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/action1b_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action1b_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
   if pc==0x2052:cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action1b_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)]);count+=1
  if name=='sequence-init-to-FE':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
 guards=0
 if not args.natural_only:
  for route,expected in(('init',0x153f2),('init-tag-jump',0x15444),('route0',0x154bc),('route1',0x154f4),('route2',0x15562),('other',0x15540),('object',0x15614),('handover-first',0x2142c),('handover-link',0x2144c),('handover-second',0x2145c),('tier',0x21486),('index',0x154a4),('selector',0x12614)):
   m,a,t,partner=seed();put(m,a+0x24,3);put(m,a+0x37,0x10,1)
   if route.startswith('init'):put(m,a+0x1c,5);put(m,a+0x26,0x123456,4);put(m,a+0x34,8 if route=='init-tag-jump'else 0,1)
   elif route in('route0','route1','route2'):put(m,a+0x44,int(route[-1]));put(m,a+0x26,0x123456,4)
   elif route=='other':put(m,a+0x44,1);put(m,t+0x20,5);put(m,t+0x60,0x53);put(m,t+0x26,0x123456,4)
   elif route=='object':put(m,t+0x74,0x80,1);put(m,t+0x76,0x123456,4)
   elif route.startswith('handover')or route=='tier':
    put(m,a+0x44,0);linked=actors[7];second=actors[0];put(m,partner+0x26,linked,4);put(m,linked+0x86,second,4)
    if route=='handover-first':put(m,a+0x86,0x123456,4)
    if route=='handover-link':put(m,partner+0x26,0x123456,4)
    if route=='handover-second':put(m,linked+0x86,0x123456,4)
    if route=='tier':put(m,second+0x70,3)
   elif route=='index':put(m,a+0x44,3)
   else:put(m,a+0x60,0xfe)
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();status=native.action1b_oracle(buf,dr,4,ctypes.byref(desc));assert status==-expected,(route,hex(-status),hex(expected));guards+=1
   if route=='selector':continue
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,a)
   for i in range(8):cpu.w_reg(i,i)
   for _ in range(20000):
    pc=cpu.r_pc()
    if pc==expected:break
    if pc==0x2052:cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
    cpu.execute(1)
   assert cpu.r_pc()==expected,(route,'partial ROM did not reach first unsupported read',hex(cpu.r_pc()),hex(expected))
   got=buf.raw[RAM:RAM+0x4000];want=bytes(mem.r_block(RAM,0x4000));assert got==want,(route,'guard RAM prefix',[(hex(RAM+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:10])
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],(route,'guard Dregs prefix',list(dr),[cpu.r_reg(i)for i in range(8)])
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','original tickdescriptor'],'intercepts':['2052 explicit silent output only'],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
