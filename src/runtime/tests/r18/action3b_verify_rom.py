"""Natural TAG basic5026 raw3B vs whole intact1C03E; raw16KB/FG/D0-D7/descriptor."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action3b_manifest.json').read_text());capture=(HERE/'action3b_snapshot_basic5026.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x12614+0x3b*4:0x12618+0x3b*4]==bytes.fromhex('00017cf6');assert rom[0x17cf6:0x17d12]==bytes.fromhex('00017d12000100050008000c002000180018019b019c019d019e019f')
rng=random.Random(0x17d12);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in(0x32,0x33,0x34,0x35,0x56,0x74,0x75):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0);put(m,at+0x20,5);put(m,at+0x66,100);put(m,at+0x72,150);put(m,at+0xfe,0);put(m,at+0x7e,0,4)
 a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x203b);put(m,a+0x26,t,4);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x44,0);put(m,a+0x2e,0);put(m,a+0x37,0,1);put(m,t+0x60,0x8000);put(m,RAM+0x161,0,1);put(m,RAM+0x7c,0x8000);put(m,RAM+0x66,0);return m,a,t,partner

def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-TAG-basic5026',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,sub,latch,phase,timer,face,selector in itertools.product((0,4,8),(0,1),range(3),(5,0x8005),(0,2,0xfe,0x102),(0,1,0xff00),(0,0x8000),(0x203b,0xa03b)):
  m,a,t,partner=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x44,sub);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x2e,face);put(m,a+0x60,selector);yield 'three-subowners-and-BYTE-frames',m,slot,None
 for ident,slow,role,flags,objectflag,objectside,face in itertools.product(range(12),(0,8),(0,0x80),(0,0x20),(0,0x80),(0,1),(0,0x8000)):
  m,a,t,partner=seed();put(m,a+0x44,1);put(m,a+0x1c,5);put(m,a+2,ident);put(m,a+0x32,slow,1);put(m,a+0x56,role,1);put(m,a+0x33,flags,1);put(m,a+0x74,objectflag,1);put(m,a+0x75,objectside,1);put(m,t+0x2e,face);yield '1174C-speed-and11710-descriptors',m,4,None
 for sub,frame,targetflag,field44,dx,dy,tag,selftarget in itertools.product((0,1,2),(0,2,0xfe),(0,0x8000),(0,0x8000),(-8,-7,7,8),(-8,-7,7,8),(0,0x40000000),(0,1)):
  if sub!=1 and(field44 or dx!=-8 or dy!=-8):continue
  m,a,t,partner=seed();put(m,a+0x44,field44|sub);put(m,a+0x24,frame);put(m,a+0x26,(a if selftarget else t)|tag,4);put(m,t+0x60,targetflag);put(m,a+0xbe,int.from_bytes(m[a+6:a+8],'big')+dx);put(m,a+0xc0,int.from_bytes(m[a+10:a+12],'big')+dy);yield 'phase1-arrival-strict8-targetflag-selfalias',m,4,None
 for mode,credit,ownflags,targetflags,health,selector,done in itertools.product(range(4),(0,0x8000),(0,1,5),(0,1),(0,1),(0x203b,0x283b,0xa03b,0xa83b),(0,0x8000)):
  m,a,t,partner=seed();put(m,a+0x44,2);put(m,a+0x24,0xfe);put(m,a+0x60,selector);put(m,a+0xfe,done);put(m,a+0x33,ownflags,1);put(m,t+0x33,targetflags,1);put(m,t+0x66,health);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);yield 'hold-result-and6F6E-original-handover',m,4,None
 for cpu,ownflags,pflags,objectflag,tier in itertools.product((0,1),(1,3),(0,2),(0,0x80),range(3)):
  m,a,t,partner=seed();put(m,a+0x44,2);put(m,a+0x24,0xfe);put(m,a+0x33,ownflags,1);put(m,partner+0x32,cpu,1);put(m,partner+0x33,pflags,1);put(m,partner+0x74,objectflag,1);put(m,partner+0x70,tier);yield '215B6-team-human-cpu-delay',m,4,None
 for sub,latch,frame in itertools.product((0,2),(5,0x8005),(0,1,3,0xfe)):
  if (sub==0 and(latch==5 or frame==0xfe))or(sub==2 and(latch==5 or frame==0xfe)):continue
  m,a,t,partner=seed();put(m,a+0x44,sub);put(m,a+0x1c,latch);put(m,a+0x24,frame);put(m,a+0x26,0xdeadbeef,4);yield 'unconsumed-invalid-target',m,4,None
 for face in(0,0x8000):
  m,a,t,partner=seed();put(m,a+0x60,0x3b);put(m,a+0x1c,5);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(300):
   sub=m[a+0x45]
   if sub==1:put(m,t+0x60,0x8000);put(m,a+6,int.from_bytes(m[a+0xbe:a+0xc0],'big'));put(m,a+10,int.from_bytes(m[a+0xc0:a+0xc2],'big'))
   if sub==2 and m[a+0x25]==0xfe:put(m,a+0xfe,0x8000)
   yield 'sequence-setup-wait-walk-capture-release',m,4,regs
   if m[a+0x61]!=0x3b:break
  else:raise AssertionError('raw3B sequence did not release')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action3b-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action3b_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/action3b_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action3b_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
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
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action3b_oracle(buf,dr,slot,ctypes.byref(desc),1);assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)]);count+=1
  if name=='sequence-setup-wait-walk-capture-release':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  if count%1000==0:print('verified',count,flush=True)
 guards=0
 if not args.natural_only:
  for kind,dependency,checkpoint in(('init',0x17d4a,0x17d4a),('impact',0x17d70,0x17d70),('walk',0x17dee,0x17dee),('speed',0x1176a,0x1176a),('capture',0x17e5c,0x17e5c),('damage',0x17ece,0x17ece),('release',0x17f1c,0x17f1c),('index',0x17d2c,0x17d2c),('ownpartner',0x1123c,0x11240),('otherpartner',0x1124c,0x11250),('video',0x2503c,0x1127c),('reserve-tag',0x21368,0x2136c),('reserve-royal',0x21368,0x2136c),('partner',0x215e2,0x215e6),('tier',0x2172c,0x21682),('selector',0x12614,0)):
   m,a,t,partner=seed();video_enabled=1
   if kind in('init','impact','walk','capture','damage','release'):put(m,a+0x26,0x123456,4)
   if kind=='init':put(m,a+0x1c,5)
   if kind=='impact':put(m,a+0x24,2);put(m,a+0x22,0)
   if kind=='walk':put(m,a+0x44,1);put(m,a+0x1c,5)
   if kind=='speed':put(m,a+0x44,1);put(m,a+2,12);put(m,a+0x1c,5)
   if kind=='capture':put(m,a+0x44,2);put(m,a+0x1c,5)
   if kind in('damage','release'):put(m,a+0x44,2);put(m,a+0x24,0xfe);put(m,a+0xfe,0x8000 if kind=='release'else 0)
   if kind=='index':put(m,a+0x44,3)
   if kind in('ownpartner','otherpartner','video','reserve-tag','reserve-royal'):
    put(m,a+0x44,2);put(m,a+0x24,0xfe);put(m,a+0x60,0x283b);put(m,a+0x33,1,1);put(m,t+0x33,1,1);put(m,t+0x66,0)
    if kind=='ownpartner':put(m,a+0x86,0x123456,4)
    if kind=='otherpartner':put(m,t+0x86,0x123456,4)
    if kind=='video':video_enabled=0
    if kind.startswith('reserve-'):put(m,a+0x7e,0x123456,4);put(m,a+0x56,0x80,1);put(m,RAM+0x161,1 if kind=='reserve-royal'else 0,1)
   if kind in('partner','tier'):
    put(m,a+0x44,2);put(m,a+0x24,0xfe);put(m,a+0x33,1,1)
    if kind=='partner':put(m,a+0x86,0x123456,4)
    else:put(m,partner+0x32,1,1);put(m,partner+0x70,3)
   if kind=='selector':put(m,a+0x60,0xfe)
   buf=ctypes.create_string_buffer(bytes(m));regs=[rng.getrandbits(32)for _ in range(8)];dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action3b_oracle(buf,dr,4,ctypes.byref(desc),video_enabled);assert status==-dependency,(kind,hex(-status),hex(dependency));guards+=1
   if not checkpoint:continue
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,a)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   for _ in range(20000):
    pc=cpu.r_pc()
    if pc==checkpoint:break
    if pc==0x2052:cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
    cpu.execute(1)
   assert cpu.r_pc()==checkpoint,(kind,'prefix not reached',hex(cpu.r_pc()))
   got=buf.raw[RAM:RAM+0x4000];want=bytes(mem.r_block(RAM,0x4000));assert got==want,(kind,'RAM guard prefix',[(hex(RAM+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],(kind,'Dregs guard prefix',list(dr),[cpu.r_reg(i)for i in range(8)])
 if not args.natural_only and not args.guards_only:
  assert {0x17d3e,0x17d70,0x17db4,0x17ddc,0x17e04,0x17e26,0x17e36,0x17e48,0x17e50,0x17ece,0x17f04,0x17f42,0x17f78}<=coverage,'missing raw3B branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','original tickdescriptor'],'intercepts':['2052 explicit silent output only'],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
