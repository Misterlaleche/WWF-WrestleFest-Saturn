#!/usr/bin/env python3
"""Raw46 natural A6003098 and full intact 1C03E landing callback oracle."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action46_snapshot_00819.bin').read_bytes();meta=json.loads((HERE/'action46_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+0x46*4:0x12618+0x46*4]==bytes.fromhex('00014424')
assert rom[0x14424:0x14440]==bytes.fromhex('00014458000100050008000c0010ff0000080084008501b001b101b1')
rng=random.Random(0x14458);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in (0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0);put(m,at+0x20,4);put(m,at+0x64,8)
 put(m,t+0x26,a,4);put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x46);put(m,a+0x26,t,4)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0x36,0);put(m,a+0x58,0);put(m,a+0x5a,0)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x161,0,1);put(m,RAM+0x162,0);put(m,RAM+0x5c,rng.getrandbits(32),4)
 return m,a,t,p
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-TAG3098-actor1',m,1,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,face,targetface,targetstate,targetsel in itertools.product((0,4,8),range(4),(0,0x8000),(0,0x8000),(4,5),(0x61,0x6061,0x60)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0x2e,face);put(m,t+0x2e,targetface);put(m,t+0x20,targetstate);put(m,t+0x60,targetsel);put(m,a+6,0x230);put(m,a+10,0x170)
  yield 'init-trajectories',m,slot,None
 for slot,mode,sel,phase,timer,contact in itertools.product((0,4,8),range(4),(0x46,0x4046,0x8046,0xff46),(0,1,2,3,4,0xfe,0x100,0x101,0x1fe),(0,1,0xff00),(0,0x10)):
  m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x60,sel);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x37,contact,1)
  yield 'phase-timer-contact',m,slot,None
 for face,targetstate,sub,dx,dy,backtag in itertools.product((0,0x8000),(4,5),(8,9,0x61,0x60),(-105,-104,-9,-8,0,8,9,104,105),(-73,-72,0,24,25),(0,0x40000000)):
  m,a,t,p=seed();put(m,a+0x24,2);put(m,a+0x37,0x10,1);put(m,t+0x2e,face);put(m,t+0x20,targetstate);put(m,t+0x64,sub);put(m,t+0x60,sub);put(m,a+6,0x250+dx);put(m,a+10,0x150+dy);put(m,t+0x26,a|backtag,4)
  yield 'impact-box-reciprocal',m,4,None
 for tag,sel,latch,phase,bad in itertools.product((0,0x40000000,0x80000000,0xff000000),(0x46,0x8046),(5,0x8005),(0,0xfe),(False,True)):
  if bad and sel==0x46 and latch==5:continue
  m,a,t,p=seed();put(m,a+0x26,0x123456 if bad else t|tag,4);put(m,a+0x60,sel);put(m,a+0x1c,latch);put(m,a+0x24,phase)
  yield 'bus-alias-or-unused-link',m,4,None
 for state,sub,back in itertools.product((4,5),(8,9,0x61),(0,1)):
  m,a,t,p=seed();put(m,a+0x24,2);put(m,a+0x37,0x10,1);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0x60,sub);put(m,t+0x26,a if back else p,4);put(m,a+0x26,t|0x40000000,4)
  yield 'live-impact-alias',m,4,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action46-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 p=frozen/'tests/r18/action46_oracle_bridge.c';p.parent.mkdir(parents=True);shutil.copy2(HERE/p.name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action46_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action46_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action46_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,'native blocked',hex(-status),slot);assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  count+=1
 guards=0
 if not args.natural_only:
  for route,expected in (('init',0x1447e),('impact',0x11156),('selector',0x12614)):
   m,a,t,p=seed()
   if route=='init':put(m,a+0x1c,5);put(m,a+0x26,0x123456,4)
   if route=='impact':put(m,a+0x37,0x10,1);put(m,a+0x24,2);put(m,a+0x26,0x123456,4)
   if route=='selector':put(m,a+0x60,0x35)
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
   assert native.action46_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,expected);guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x14458,0x1446a,0x1448e,0x144ac,0x144b8,0x144d0,0x144fa,0x1451e,0x14578,0x14582,0x145cc,0x145d6,0x14606,0x26ae,0x11152,0x11412,0x11192,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals','tick descriptor'],'intercepts':['2052 silent sound device only'],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST TAG A6003098 actor1; not physical Saturn RAM','Only raw46 admitted; raw35 remains guarded','26AE signed division bounds retained; invalid consumed pointers fail closed','No full match or physical timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw46 cases,',guards,'guards; intact ROM')
