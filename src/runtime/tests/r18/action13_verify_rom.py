"""Stage6 real raw13 vs whole intact1C03E; raw16KB/FG/D0-D7/descriptor."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action13_manifest.json').read_text());capture=(HERE/'action13_snapshot_beetle_stage6_frame4483.bin').read_bytes();assert len(capture)==16440 and hashlib.sha256(capture).hexdigest()==meta['sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x12614+0x13*4:0x12618+0x13*4]==bytes.fromhex('000149e2');assert rom[0x149e2:0x149f2]==bytes.fromhex('000149f2000100020020ff0000ed00ee')
rng=random.Random(0x149f2);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x70,i%3);put(m,at+0x20,4);put(m,at+0x64,8);put(m,at+0x2e,0)
 a=actors[slot];t=actors[(slot+1)%9]
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x13);put(m,a+0x26,t,4);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x37,0,1);put(m,RAM+0x161,0,1)
 return m,a,t
def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-Beetle-TAG-pressure4483',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,latch,phase,timer,face,selector,contact in itertools.product((0,4,8),range(4),(5,0x8005),(0,1,2,0xfe,0x101,0x1fe),(0,1,0xff00),(0,0x8000),(0x13,0x8013),(0,0x10)):
  if latch==5 and(phase!=0 or timer!=0 or selector!=0x13 or contact!=0):continue
  m,a,t=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x2e,face);put(m,a+0x60,selector);put(m,a+0x37,contact,1)
  yield 'phase-WORD-selector-highbit-contact-mode',m,slot,None
 for face,otherface,x,otherx,tag,target in itertools.product((0,0x80,0x8000,0xff01),(0,0x80,0x8000),(0,0x250,0xfff0),(0,0x250,0xfff0),(0,0x40000000,0xff000000),(0,1)):
  aim=(otherx+(-56 if otherface&0x8000 else 56))&65535
  end=(aim+(-16 if aim>=x else 16))&65535
  # The existing DIVS-bounded trajectory deliberately rejects the ROM's
  # overflow-result domain. Verify that retained guard separately below.
  if target and abs(otherx-end)>2000:continue
  m,a,t=seed();put(m,a+0x1c,5);put(m,a+0x2e,face);put(m,t+0x2e,otherface);put(m,a+6,x);put(m,t+6,otherx);put(m,a+0x26,(a if target==0 else t)|tag,4)
  yield 'init-coordinate-wrap-facing-self-24bitalias',m,4,None
 for state,sub,face,dx,dy in itertools.product((4,0x104,3),(8,9,0x108,7),(0,0x8000),(-105,-104,-103,-9,-8,-7,0,7,8,9,103,104,105),(-73,-72,-71,0,23,24,25)):
  m,a,t=seed();put(m,a+0x24,1);put(m,a+0x37,0x10,1);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0x2e,face);put(m,a+6,0x250+dx);put(m,a+10,0x150+dy);put(m,t+0xd6,0xffff);put(m,t+0xc7,0xff,1)
  yield '11152-strict-lower-inclusive-upper-lowbytes-wrapcounters',m,4,None
 for phase,selector,contact,tag in itertools.product((0,1,2,0xfe,0x101,0x1fe),(0x13,0x8013),(0,0x10),(0,0x80000000,0xdeadbeef)):
  if phase==1 and selector==0x13 and contact:continue
  m,a,t=seed();put(m,a+0x24,phase);put(m,a+0x60,selector);put(m,a+0x37,contact,1);put(m,a+0x26,tag,4)
  yield 'unconsumed-invalid-target',m,4,None
 for target,tag,contact,face in itertools.product((0,1),(0,0x40000000,0xff000000),(0,0x10),(0,0x8000)):
  m,a,t=seed();put(m,a+0x24,1);put(m,a+0x37,contact,1);put(m,a+0x2e,face);put(m,a+0x26,(a if not target else t)|tag,4)
  yield 'impact-self-and-tagged-link',m,4,None
 for hit in(0,1):
  m,a,t=seed();put(m,a+0x1c,5);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(160):
   if int.from_bytes(m[a+0x24:a+0x26],'big')==1:
    put(m,a+0x37,0x10,1);put(m,a+6,int.from_bytes(m[t+6:t+8],'big')+(0 if hit else 1000))
   yield 'sequence-init-to-terminal',m,4,regs
   if int.from_bytes(m[a+0x20:a+0x22],'big')!=0x8005:break
  else:raise AssertionError('raw13 sequence did not terminate')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action13-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action13_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/action13_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action13_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action13_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)]);count+=1
  if name=='sequence-init-to-terminal':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  if count%1000==0:print('verified',count,flush=True)
 guards=0
 if not args.natural_only:
  for kind,expected in(('init',0x14a06),('impact',0x11156),('trajectory',0x26ae),('selector',0x12614)):
   m,a,t=seed();put(m,a+0x26,0x123456,4)
   if kind=='init':put(m,a+0x1c,5)
   elif kind=='impact':put(m,a+0x24,1);put(m,a+0x37,0x10,1)
   elif kind=='trajectory':put(m,a+0x26,t,4);put(m,a+0x1c,5);put(m,t+6,0);put(m,t+0x2e,0x8000)
   else:put(m,a+0x60,0xfe)
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();status=native.action13_oracle(buf,dr,4,ctypes.byref(desc));assert status==-expected,(kind,hex(-status),hex(expected));guards+=1
   if kind=='selector':continue
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,a)
   for i in range(8):cpu.w_reg(i,i)
   for _ in range(20000):
    pc=cpu.r_pc()
    if pc==expected:break
    if pc==0x2052:cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
    cpu.execute(1)
   assert cpu.r_pc()==expected,(kind,'prefix not reached',hex(cpu.r_pc()))
   assert buf.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),(kind,'RAM guard prefix')
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],(kind,'Dregs guard prefix',list(dr),[cpu.r_reg(i)for i in range(8)])
 if not args.natural_only and not args.guards_only:
  assert {0x14a06,0x14a8a,0x14acc,0x14ad4,0x14afc,0x11152,0x14b40,0x1c126}<=coverage,'missing raw13 branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','original tickdescriptor'],'intercepts':['2052 explicit silent output only'],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
