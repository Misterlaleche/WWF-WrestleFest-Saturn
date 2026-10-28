#!/usr/bin/env python3
"""Raw2B natural A6005369 and full intact 1C03E jump/rope/landing oracle."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action2b_snapshot_01349.bin').read_bytes();meta=json.loads((HERE/'action2b_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+0x2b*4:0x12618+0x2b*4]==bytes.fromhex('0001667a')
assert rom[0x1667a:0x1669a]==bytes.fromhex('0001669a0001000600100008000a000aff00001c011b011c011d011e011f0120')
assert rom[0x267e:0x2684]==bytes.fromhex('ff0001800058')
rng=random.Random(0x1669a);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in (0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0);put(m,at+0x20,4);put(m,at+0x64,8)
 put(m,t+0x26,a,4);put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x2b);put(m,a+0x26,t,4)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0x36,0);put(m,a+0x3e,0);put(m,a+0x40,0);put(m,a+0x42,0);put(m,a+0x46,0);put(m,a+0x58,0);put(m,a+0x5a,0)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x161,0,1);put(m,RAM+0x162,0);put(m,RAM+0x7e,3);put(m,RAM+0x5c,rng.getrandbits(32),4)
 return m,a,t,p
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-TAG5369-actor1',m,1,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,face,repetition,role,trole,progression,turn,tag in itertools.product((0,8),range(4),(0,0x8000),(0,0xffff),(0,0x80),(0,0x80),(0,9),(0,1),(0,0x40000000)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0x2e,face);put(m,a+0xec,repetition);put(m,a+0x56,role,1);put(m,t+0x56,trole,1);put(m,RAM+0x162,progression);put(m,a+0x46,turn);put(m,a+0x26,t|tag,4)
  yield 'init-live-RNG-turn',m,slot,None
 for slot,mode,sel,phase,timer,contact,turn in itertools.product((0,8),range(4),(0x2b,0x802b),(0,3,4,5,0xfe,0x104,0x1fe),(0,1,0xff00),(0,0x10,0x13),(0,1)):
  m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x60,sel);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x37,contact,1);put(m,a+0x46,turn)
  yield 'phase-contact-priority',m,slot,None
 for scene,face,sel,x,y,z,turn in itertools.product(range(7),(0,0x8000),(0x2b,0x802b),(0,0x1a0,0x250,0x350,0xffff),(0x118,0x160),(0x13f,0x180),(0,1)):
  m,a,t,p=seed();put(m,RAM+0x7e,scene);put(m,a+0x2e,face);put(m,a+0x60,sel);put(m,a+0x24,4);put(m,a+6,x);put(m,a+10,y);put(m,a+14,z);put(m,a+0x46,turn)
  yield 'wall-resolver-ROM-scenes',m,4,None
 for mode,credit,ownflags,char,partner,tag in itertools.product(range(4),(0,1,0x8000),(0,2),(0,5,11),(False,True),(0,0xff000000)):
  m,a,t,p=seed();put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x33,ownflags,1);put(m,a+2,char);put(m,p+0x20,1);put(m,p+0xae,1 if partner else 0);put(m,a+0x24,4);put(m,a+0x37,0x10,1);put(m,a+0x26,t|tag,4)
  yield 'landing-RNG-partner',m,4,None
 for backtag,linktag in itertools.product((0x40000000,0x80000000,0xff000000),(0,0x40000000)):
  m,a,t,p=seed();put(m,a+0x1c,5);put(m,t+0x26,a|backtag,4);put(m,a+0x26,t|linktag,4)
  yield 'literal-reciprocal-miss',m,4,None
 for ptr,phase,sel in itertools.product((0,0x80000000,0x123456),(0,3,5,0xfe),(0x2b,0x802b)):
  m,a,t,p=seed();put(m,a+0x26,ptr,4);put(m,a+0x24,phase)
  if phase==0 and ptr!=0x123456:put(m,a+0x1c,5)
  put(m,a+0x60,sel);yield 'unconsumed-link-or-ROM-init-miss',m,4,None
 for mode,ptr in itertools.product((1,3),(0,0x123456,0xdeadbeef)):
  m,a,t,p=seed();put(m,RAM+0x161,mode,1);put(m,a+0x60,0x802b);put(m,a+0x24,4);put(m,a+0x37,0x10,1);put(m,a+0x26,ptr,4)
  yield 'Royal-already-captured-unused-target',m,4,None
 for ptr in (0,0x123456,0xdeadbeef):
  m,a,t,p=seed();put(m,RAM+0x7e,2);put(m,a+0x60,0x802b);put(m,a+0x24,4);put(m,a+6,0);put(m,a+10,0x160);put(m,a+0x26,ptr,4)
  yield 'wall-already-captured-unused-target',m,4,None
 for mode,face,turn in itertools.product((0,1),(0,0x8000),(0,1)):
  m,a,t,p=seed();put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0x2e,face);put(m,a+0x46,turn);put(m,a+0x56,0x80,1);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(110):
   if int.from_bytes(m[a+0x24:a+0x26],'big')==4:put(m,a+0x37,0x10,1)
   yield 'sequence-linked-jump',m,4,regs
   if int.from_bytes(m[a+0x20:a+0x22],'big')==7:break
  else:raise AssertionError('raw2B sequence did not finish')
  assert m[t+0x64:t+0x66]==bytes.fromhex('2024')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action2b-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 p=frozen/'tests/r18/action2b_oracle_bridge.c';p.parent.mkdir(parents=True);shutil.copy2(HERE/p.name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action2b_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action2b_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action2b_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,'native blocked',hex(-status),slot);assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  if name=='sequence-linked-jump':m[RAM:RAM+0x4000]=b.raw[RAM:RAM+0x4000];regs[:]=list(d)
  count+=1
 guards=0
 if not args.natural_only:
  for route,expected in (('init',0x1141a),('wall',0x167aa),('release',0x1683c),('partner',0x110ee),('scene7',0x280dc),('RNG-progression',0x113cd),('landing-id',0x110b6),('selector',0x12614)):
   m,a,t,p=seed()
   if route=='init':put(m,a+0x1c,5);put(m,a+0x26,0x123456,4)
   if route=='wall':put(m,a+0x24,4);put(m,RAM+0x7e,2);put(m,a+6,0);put(m,a+10,0x160);put(m,a+0x26,0x123456,4)
   if route in ('release','partner','landing-id'):put(m,a+0x24,4);put(m,a+0x37,0x10,1)
   if route=='release':put(m,RAM+0x161,1,1);put(m,a+0x26,0x123456,4)
   if route=='partner':put(m,t+0x86,0x123456,4)
   if route=='scene7':put(m,a+0x24,4);put(m,RAM+0x7e,7)
   if route=='RNG-progression':put(m,a+0x1c,5);put(m,a+0x56,0,1);put(m,t+0x56,0x80,1);put(m,RAM+0x162,10)
   if route=='landing-id':put(m,a+2,12)
   if route=='selector':put(m,a+0x60,0x35)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
   assert native.action2b_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,expected);guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1669a,0x166a4,0x166b2,0x166e8,0x166f0,0x16738,0x1675c,0x1677c,0x1679e,0x16814,0x1683c,0x1689e,0x258e,0x280dc,0x11058,0x1108c,0x110e0,0x1129e,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals','tick descriptor'],'intercepts':['2052 silent sound device only'],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST TAG A6005369 actor1; not physical Saturn RAM','Only raw2B admitted; raw35 remains guarded','Closed RNG/resolver/landing helpers and their explicit bounds reused','No full match or physical timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw2B cases,',guards,'guards; intact ROM')
