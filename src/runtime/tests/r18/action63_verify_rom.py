#!/usr/bin/env python3
"""Raw63 native F8C1825 and full intact 1C03E held-victim oracle."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action63_snapshot_native_f8c_1825.bin').read_bytes();meta=json.loads((HERE/'action63_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+0x63*4:0x12618+0x63*4]==bytes.fromhex('00019530')
assert rom[0x19530:0x19536]==bytes.fromhex('000195360000')
assert rom[0x10cfc:0x10cff]==bytes((4,12,24))
rng=random.Random(0x19536);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in (0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x7a,actors[(i+3)%9],4);put(m,at+0xb5,0,1);put(m,at+0xb6,0,1);put(m,at+0x70,0);put(m,at+0x20,4);put(m,at+0x64,8)
 put(m,t+0x26,a,4);put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x63);put(m,a+0x26,t,4)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0x36,0);put(m,a+0x33,1,1);put(m,a+0xa3,1,1);put(m,a+0x58,0);put(m,a+0x5a,0)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x161,0,1);put(m,RAM+0x162,0);put(m,RAM+0x5c,rng.getrandbits(32),4)
 return m,a,t,p
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-nativeF8C-TAG1825-actor0',m,0,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,credit,sel,tier in itertools.product((0,4,8),range(4),(0,1,0x8000),(0x63,0x4063,0x8063,0xff63),range(3)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x60,sel);put(m,a+0x70,tier)
  yield 'init-10C60-argument7',m,slot,None
 for mode,credit,flags,cpu,pflags,locked,tier in itertools.product((0,2),(0,1,0x8000),(0,1),(0,1),(0,2),(0,0x80),range(3)):
  m,a,t,p=seed();put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x33,flags,1);put(m,p+0x32,cpu,1);put(m,p+0x33,pflags,1);put(m,p+0x74,locked,1);put(m,p+0x70,tier)
  yield '21732-TAG-producer',m,4,None
 for mode,role,timer,mash,flags in itertools.product(range(4),(0,0x40,0x80,0xc0,0xff),(0,1,2,0x4000,0xffff),(0,1,0xff),(0,0x40,0x80,0xc0,0xff)):
  m,a,t,p=seed();put(m,RAM+0x161,mode,1);put(m,a+0x56,role,1);put(m,a+0xaa,timer);put(m,a+0xa3,mash,1);put(m,p+0x34,flags,1)
  yield '11432-10D04-latched',m,4,None
 for bad,mode,credit,flags,latch,role in itertools.product((0,0x80000000,0x123456),(0,1),(0,1),(0,1),(5,0x8005),(0,0x80)):
  if latch==5 and not mode&1 and credit and flags:continue
  if latch==0x8005 and not mode&1 and not role&0x80:continue
  m,a,t,p=seed();put(m,a+0x1c,latch);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x33,flags,1);put(m,a+0x56,role,1);put(m,a+0x86,bad,4)
  yield 'unused-partner',m,4,None
 for tag,cx,cy,reject in itertools.product((0,0x40000000,0x80000000,0xff000000),(0x250,0x64f,0x650,0xffff),(0x150,0xffff),('none','disabled','b5-3','b5-7','flags32-4','other-team')):
  m,a,t,p=seed(0);put(m,a+0x1c,5);put(m,RAM+0x161,1,1)
  for c in actors[4:]:put(m,c,0)
  c=actors[4];put(m,c,0x8000);put(m,c+6,cx);put(m,c+10,cy);put(m,a+0x26,c|tag,4)
  if reject=='disabled':put(m,c,0)
  if reject=='b5-3':put(m,c+0xb5,8,1)
  if reject=='b5-7':put(m,c+0xb5,0x80,1)
  if reject=='flags32-4':put(m,c+0x32,0x10,1)
  if reject=='other-team':put(m,c+0x56,0x20,1)
  yield '217FE-exact-2187E-raw-link',m,0,None
 for tag,flags in itertools.product((0,0x40000000,0xff000000),(0,0x40,0x80,0xc0)):
  m,a,t,p=seed();put(m,a+0x86,p|tag,4);put(m,p+0x34,flags,1);put(m,p+0x7a,t|tag,4)
  yield 'latched-consumed-bus-aliases',m,4,None
 for slot,mode,tier in itertools.product((0,8),(0,1),range(3)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0x70,tier);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(26):
   yield 'sequence-held-mash',m,slot,regs
   if int.from_bytes(m[a+0xaa:a+0xac],'big')==0x4000:break
  else:raise AssertionError('raw63 recovery did not reach sentinel')
  assert m[a+0x22:a+0x24]==bytes.fromhex('ff00') and m[a+0x60:a+0x62]==bytes.fromhex('0063')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action63-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 p=frozen/'tests/r18/action63_oracle_bridge.c';p.parent.mkdir(parents=True);shutil.copy2(HERE/p.name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action63_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action63_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,x in enumerate(regs):cpu.w_reg(i,x)
  descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action63_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,'native blocked',hex(-status),slot);assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  assert d[0]==regs[0],(name,'successful dispatcher restores D0')
  if name=='sequence-held-mash':m[RAM:RAM+0x4000]=b.raw[RAM:RAM+0x4000];regs[:]=list(d)
  count+=1
 guards=0
 if not args.natural_only:
  for route,expected in (('init-tier',0x10ca8),('prepare-partner',0x21756),('prepare-tier',0x217c8),('latched-partner',0x11448),('latched-resume',0x11462),('selector',0x12614)):
   m,a,t,p=seed()
   if route=='init-tier':put(m,a+0x1c,5);put(m,a+0x70,3)
   if route=='prepare-partner':put(m,a+0x1c,5);put(m,a+0x86,0x123456,4)
   if route=='prepare-tier':put(m,a+0x1c,5);put(m,p+0x32,1,1);put(m,p+0x70,3)
   if route=='latched-partner':put(m,a+0x86,0x123456,4)
   if route=='latched-resume':put(m,p+0x34,0xc0,1);put(m,p+0x7a,0x123456,4)
   if route=='selector':put(m,a+0x60,0x35)
   initial=[rng.getrandbits(32)for _ in range(8)];b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*initial);nd=ctypes.c_uint32()
   assert native.action63_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,expected);guards+=1
   if route=='init-tier':
    # The fail-closed table bound preserves the caller prefix, before the
    # unsupported10C60 table lookup. No dispatcher return has restored D0.
    mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_reg(8,a);cpu.w_pc(0x1c03e)
    for i,x in enumerate(initial):cpu.w_reg(i,x)
    for _ in range(10000):
     if cpu.r_pc()==0x195e2:break
     cpu.execute(1)
    assert cpu.r_pc()==0x195e2
    assert b.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),('tier-guard-prefix','RAM16KB')
    assert list(d)==[cpu.r_reg(i)for i in range(8)],('tier-guard-prefix','D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
    assert int.from_bytes(b.raw[a+0xaa:a+0xac],'big')==0 and d[0]==((initial[0]&0xffff0000)|7)
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x19536,0x19540,0x195da,0x195ec,0x19600,0x21732,0x217fe,0x2187e,0x10c60,0x10d04,0x11432,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'guard_prefix_checks':0 if args.natural_only else 1,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals','tick descriptor'],'intercepts':[],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST verified-native-F8C TAG1825 actor0/id7; not physical timing proof','Only raw63 admitted; raw35 remains guarded','TAG21732/latched5F and exact public Royal2187E reused; explicit bounds retained','No full match or physical timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw63 cases,',guards,'guards; intact ROM')
