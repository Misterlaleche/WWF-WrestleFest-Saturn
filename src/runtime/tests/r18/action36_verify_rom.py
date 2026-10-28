#!/usr/bin/env python3
"""Raw36 native F8C1755 and full intact 1C03E submission/result oracle."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action36_snapshot_native_f8c_1755.bin').read_bytes();meta=json.loads((HERE/'action36_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+0x36*4:0x12618+0x36*4]==bytes.fromhex('00017912')
assert rom[0x17912:0x1792a]==bytes.fromhex('0001792a00010004000800100014001400f8012b012c012d')
rng=random.Random(0x1792a);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in (0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0);put(m,at+0x20,4);put(m,at+0x64,8)
 put(m,t+0x26,a,4);put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x36);put(m,a+0x26,t,4)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0xfe,0);put(m,a+0x36,0);put(m,a+0x7e,p,4);put(m,t+0x7e,p,4);put(m,a+0x33,1,1);put(m,t+0x33,1,1);put(m,t+0x60,0x7b);put(m,a+0x86,p,4);put(m,a+0x58,0);put(m,a+0x5a,0)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x161,0,1);put(m,RAM+0x162,0);put(m,RAM+0x5c,rng.getrandbits(32),4)
 return m,a,t,p
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-native-F8C-TAG1755-actor4',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,face,tag,backtag,targetsel,targetlatch in itertools.product((0,4,8),range(4),(0,0x8000),(0,0x80000000),(0,0x40000000),(0x7b,0x407b,0x7a),(0,0xff)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0x2e,face);put(m,a+0x26,t|tag,4);put(m,t+0x26,a|backtag,4);put(m,t+0x60,targetsel);put(m,t+0x1c,targetlatch)
  yield 'init-reciprocal-selector-latch',m,slot,None
 for slot,mode,sel,phase,timer,locked,health in itertools.product((0,8),range(4),(0x36,0x4036,0x8036,0x8836,0x0836),(0,0xfe,0x1fe,0xffff),(0,1,0xff00),(0,0x8000),(0,5)):
  m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x60,sel);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0xfe,locked);put(m,t+0x66,health)
  yield 'FE-result-lock-priority',m,slot,None
 for mode,credit,role,flags,tflags,health,sel,config in itertools.product(range(4),(0,1,0x8000),(0,0x80),(0,1,3,5),(0,1),(0,1),(0x0836,0x8836),(0,0x2000,0x4000,0x6000)):
  m,a,t,p=seed();put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,RAM+0x66,config);put(m,a+0x56,role,1);put(m,a+0x33,flags,1);put(m,t+0x33,tflags,1);put(m,t+0x66,health);put(m,a+0x60,sel);put(m,a+0x24,0xfe)
  yield '111C8-result-and-bonus',m,4,None
 for mode,cpu,flags,pcpu,pflags,locked,tier in itertools.product(range(4),(0,1),(0,1,3),(0,1),(0,2),(0,0x80),range(3)):
  m,a,t,p=seed();put(m,RAM+0x161,mode,1);put(m,a+0x32,cpu,1);put(m,a+0x33,flags,1);put(m,p+0x32,pcpu,1);put(m,p+0x33,pflags,1);put(m,p+0x74,locked,1);put(m,p+0x70,tier);put(m,a+0x24,0xfe);put(m,t+0x66,5)
  yield '215B6-partner-producer',m,4,None
 for ptr,mode,phase,sel in itertools.product((0,0x80000000,0x123456),range(4),(0,1),(0x36,0x8036)):
  m,a,t,p=seed();put(m,a+0x26,ptr,4);put(m,RAM+0x161,mode,1);put(m,a+0x24,phase);put(m,a+0x60,sel);yield 'unused-target',m,4,None
 for ptr in (0,0x80000000,0xff000000):
  m,a,t,p=seed();put(m,a+0x1c,5);put(m,a+0x26,ptr,4);yield 'ROM-reciprocal-init-miss',m,4,None
 for mode,role,face in itertools.product((0,1),(0,0x80),(0,0x8000)):
  m,a,t,p=seed();put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0x56,role,1);put(m,a+0x2e,face);put(m,t+0x66,5);regs=[rng.getrandbits(32)for _ in range(8)];ends=0
  for tick in range(210):
   if int.from_bytes(m[a+0x24:a+0x26],'big')==0xfe:
    ends+=1
    if ends>=2:put(m,t+0x66,0)
   yield 'sequence-submission-result',m,4,regs
   if m[a+0x61]==0x6f:break
  else:raise AssertionError('raw36 sequence did not finish')
  assert m[t+0x60:t+0x62]==bytes.fromhex('006e')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action36-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 p=frozen/'tests/r18/action36_oracle_bridge.c';p.parent.mkdir(parents=True);shutil.copy2(HERE/p.name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action36_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action36_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
   if pc==0x2052:cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action36_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,'native blocked',hex(-status),slot);assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  if name=='sequence-submission-result':m[RAM:RAM+0x4000]=b.raw[RAM:RAM+0x4000];regs[:]=list(d)
  count+=1
 guards=0
 if not args.natural_only:
  for route,expected in (('init',0x1141a),('FE-target',0x179c0),('locked-target',0x17a32),('result-partner',0x1123c),('cleanup-partner',0x212de),('Royal-candidate',0x21368),('producer-partner',0x215e2),('producer-tier',0x2172c),('selector',0x12614)):
   m,a,t,p=seed();put(m,a+0x24,0xfe)
   if route=='init':put(m,a+0x1c,5);put(m,a+0x26,0x123456,4)
   if route=='FE-target':put(m,a+0x26,0x123456,4)
   if route=='locked-target':put(m,a+0xfe,0x8000);put(m,a+0x26,0x123456,4)
   if route=='result-partner':put(m,a+0x60,0x0836);put(m,t+0x66,0);put(m,a+0x86,0x123456,4)
   if route=='cleanup-partner':put(m,a+0xfe,0x8000);put(m,a+0x86,0x123456,4)
   if route=='Royal-candidate':put(m,RAM+0x161,1,1);put(m,a+0xfe,0x8000);put(m,a+0x7e,0x123456,4)
   if route=='producer-partner':put(m,t+0x66,5);put(m,a+0x86,0x123456,4)
   if route=='producer-tier':put(m,t+0x66,5);put(m,p+0x32,1,1);put(m,p+0x70,3)
   if route=='selector':put(m,a+0x60,0x35)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
   assert native.action36_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,expected);guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1792a,0x1794a,0x1797a,0x17990,0x179b6,0x179ce,0x179f4,0x179fc,0x17a26,0x111c8,0x212a0,0x215b6,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals','tick descriptor'],'intercepts':['2052 silent sound device only'],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST native-F8C TAG1755 actor4/id9; not physical Saturn RAM','Only raw36 admitted; raw35 remains guarded','Closed111C8/212A0/215B6 and bounds reused; no omitted result side effects','No full match or physical timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw36 cases,',guards,'guards; intact ROM')
