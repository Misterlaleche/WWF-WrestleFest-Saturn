#!/usr/bin/env python3
"""Raw30/16E48 linked throw: naturalRoyal309 and full original1C03E.

Catches omitted action; wrong capture/RNG/reversal; discardedD125C0 miss
descriptor; phase BYTE aliases; target damage/position and final landing RNG.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action30_snapshot_00214.bin').read_bytes();meta=json.loads((HERE/'action30_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+0x30*4:0x12618+0x30*4]==bytes.fromhex('00016e28')
assert rom[0x16e28:0x16e48]==bytes.fromhex('00016e480001000600100008000600120016000e010d010e010f01f001f101f2')
rng=random.Random(0x16e48);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;descriptors=set()
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250+8*i);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in (0x32,0x33,0x34,0x35,0x56):put(m,at+off,0,1)
  put(m,at+0x40,0);put(m,at+0x42,0);put(m,at+0x70,0)
 put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x30);put(m,a+0x26,t,4);put(m,a+0x7a,t,4);put(m,t+0x26,a,4);put(m,t+0x86,p,4);put(m,p+0x20,1);put(m,p+0xae,1)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x36,0);put(m,a+0x46,0);put(m,a+0xea,0)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x7e,0);put(m,RAM+0x161,1,1);put(m,RAM+0x162,0);put(m,RAM+0x5c,rng.getrandbits(32),4)
 return m,a,t,p
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-Royal309-slot4',m,4,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,ident,face,repeat,turn,role,reciprocal in itertools.product((0,4,8),(0,1),(0,5,11),(0,0x8000),(0,4,0xffff),(0,1),(0,0x80),(False,True)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+2,ident);put(m,a+0x2e,face);put(m,a+0xea,repeat);put(m,a+0x46,turn);put(m,a+0x56,role,1)
  if not reciprocal:put(m,t+0x26,p,4)
  yield 'init-capture-rng-turn',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for slot,mode,ident,face,phase,timer in itertools.product((0,4,8),(0,1),(0,5,11),(0,0x8000),(0,2,3,4,5,0xfe,0x102,0x104,0x1fe),(0,1,0xff00,0xffff)):
  m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+2,ident);put(m,a+0x2e,face);put(m,a+0x24,phase);put(m,a+0x22,timer)
  yield 'phase-timer-damage-terminal',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for mode,credit,flags,ident,tag in itertools.product(range(4),(0,1,0x8000),(0,2),(0,5,11),(0,0x80000000,0xff000000)):
  m,a,t,p=seed(4);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit);put(m,a+0x33,flags,1);put(m,a+2,ident);put(m,a+0x24,0xfe);put(m,a+0x22,0);put(m,a+0x26,t|tag,4)
  yield 'terminal-landing-rng',m,4,[rng.getrandbits(32)for _ in range(8)]
 for mode,tag,partner_state,partner_sub in itertools.product(range(4),(0,0x80000000,0xff000000),(0,1,0x101),(0,1,0x101)):
  m,a,t,p=seed(4);put(m,RAM+0x161,mode,1);put(m,a+0x24,4);put(m,a+0x22,0);put(m,a+0x26,t|tag,4);put(m,p+0x20,partner_state);put(m,p+0xae,partner_sub)
  yield 'partner-marking-byte-tests',m,4,[rng.getrandbits(32)for _ in range(8)]
 for alias,back in itertools.product((0,0x80000000,0xff000000),(0,0x80000000,0xff000000)):
  m,a,t,p=seed(4);put(m,a+0x1c,5);put(m,a+0x56,0x80,1);put(m,a+0x26,t|alias,4);put(m,t+0x26,a|back,4)
  yield 'reciprocal-raw-long-comparison',m,4,[rng.getrandbits(32)for _ in range(8)]
 for alias,mode,target_role,progression in itertools.product((0x40000000,0x80000000,0xff000000),(0,1),(0,0x80),(0,9)):
  m,a,t,p=seed(4);put(m,a+0x1c,5);put(m,a+0x26,t|alias,4);put(m,RAM+0x161,mode,1);put(m,t+0x56,target_role,1);put(m,RAM+0x162,progression)
  yield 'init-alias-with-live1129E',m,4,[rng.getrandbits(32)for _ in range(8)]
 for link,phase in itertools.product((0,0x80000000,0x123456),(0,2,3,0xfe)):
  m,a,t,p=seed(4);put(m,a+0x26,link,4);put(m,a+0x24,phase);put(m,a+0x22,0)
  yield 'unconsumed-link',m,4,[rng.getrandbits(32)for _ in range(8)]
 for link in (0,0x80000000):
  m,a,t,p=seed(4);put(m,a+0x1c,5);put(m,a+0x26,link,4)
  yield 'null-reciprocity-miss',m,4,[rng.getrandbits(32)for _ in range(8)]
 for sel,phase,timer in itertools.product((0x30,0x4030,0x8030,0xff30),(0,4,0xfe),(0,1)):
  m,a,t,p=seed(4);put(m,a+0x60,sel);put(m,a+0x24,phase);put(m,a+0x22,timer)
  yield 'selector-upper-flags',m,4,[rng.getrandbits(32)for _ in range(8)]
 for ident,face in itertools.product((0,5,11),(0,0x8000)):
  m,a,t,p=seed(4);put(m,a+2,ident);put(m,a+0x2e,face);put(m,a+0x1c,5);put(m,a+0x56,0x80,1);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(240):
   yield 'sequence-throw',m,4,regs
   if int.from_bytes(m[a+0x20:a+0x22],'big')==0:break
  else:raise AssertionError('raw30 sequence did not finish')
  assert m[t+0x20:t+0x22]==bytes.fromhex('0004')
  assert int.from_bytes(m[t+0x68:t+0x6a],'big')==(19 if ident in(0,5)else 17)
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action30-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 p=frozen/'tests/r18/action30_oracle_bridge.c';p.parent.mkdir(parents=True);shutil.copy2(HERE/p.name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action30_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action30_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,x in enumerate(regs):cpu.w_reg(i,x)
  descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   if pc==0x2052:cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action30_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,'native blocked',hex(-status),slot);assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  if name=='sequence-throw':m[RAM:RAM+0x2000]=b.raw[RAM:RAM+0x2000];regs[:]=list(d)
  count+=1;descriptors.add(descriptor)
 guards=0
 if not args.natural_only:
  for route,expected in (('capture',0x1141a),('release',0x16f38),('partner',0x110ee),('selector',0x12614),('RNG-progression',0x113cd),('landing-character',0x110b6)):
   m,a,t,p=seed(4)
   if route=='capture':put(m,a+0x1c,5);put(m,a+0x26,0x123456,4)
   if route=='release':put(m,a+0x24,4);put(m,a+0x22,0);put(m,a+0x26,0x123456,4)
   if route=='partner':put(m,RAM+0x161,0,1);put(m,a+0x24,4);put(m,a+0x22,0);put(m,t+0x86,0x123456,4)
   if route=='selector':put(m,a+0x60,0x35)
   if route=='RNG-progression':put(m,a+0x1c,5);put(m,RAM+0x161,0,1);put(m,t+0x56,0x80,1);put(m,RAM+0x162,10)
   if route=='landing-character':put(m,a+0x24,0xfe);put(m,a+2,0x100)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
   assert native.action30_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,expected);guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x16e48,0x16e52,0x16e62,0x16e7c,0x16e8c,0x16e94,0x16ecc,0x16ee0,0x16ef6,0x16f04,0x16f18,0x16f32,0x16f6e,0x16f74,0x16fa4,0x11412,0x1129e,0x10b62,0x1108c,0x110e0,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KiB','FG16KiB','D0-D7','RNG and globals','tick descriptor'],'intercepts':['2052 silent sound device only'],'tick_descriptors':[hex(x)for x in sorted(descriptors)],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST Royal full-flow309 actor4; not physical Saturn RAM','Only raw30 admitted; all unrelated guards remain','Original1129E bounds and invalid consumed pointer/partner guards retained','No full match or physical timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw30 cases,',guards,'guards; intact ROM')
