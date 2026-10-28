#!/usr/bin/env python3
"""Raw6E native F8C2083 and full intact 1C03E result-victim timer oracle."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action6e_snapshot_native_f8c_2083.bin').read_bytes();meta=json.loads((HERE/'action6e_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
beetle=(HERE/'action6e_snapshot_beetle_stage4_frame1724.bin').read_bytes();beetle_meta=json.loads((HERE/'action6e_beetle_stage4_manifest.json').read_text());assert hashlib.sha256(beetle).hexdigest()==beetle_meta['sha256'];assert struct.unpack_from('=6I',beetle)==tuple(beetle_meta['header'])
assert rom[0x12614+0x6e*4:0x12618+0x6e*4]==bytes.fromhex('00019d7c')
assert rom[0x19d7c:0x19d82]==bytes.fromhex('00019d820000')
rng=random.Random(0x19d82);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  for off in (0x32,0x33,0x34,0x35,0x56,0x74):put(m,at+off,0,1)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x86,actors[(i+2)%9],4);put(m,at+0x70,0);put(m,at+0x20,4);put(m,at+0x64,8)
 put(m,t+0x26,a,4);put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x6e);put(m,a+0x26,t,4)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0x36,0);put(m,a+0x58,0);put(m,a+0x5a,0)
 put(m,RAM+0x7c,0x8000);put(m,RAM+0x161,0,1);put(m,RAM+0x162,0);put(m,RAM+0x5c,rng.getrandbits(32),4)
 return m,a,t,p
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-nativeF8C-TAG2083-actor0',m,0,struct.unpack_from('=8I',capture,24)
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=beetle[56:16440];yield 'actual-Beetle-stage4-TAG1724-actor0',m,0,struct.unpack_from('=8I',beetle,24)
 if args.natural_only:return
 for slot,mode,sel,latch,timer,result,face in itertools.product((0,4,8),range(4),(0x6e,0xff6e),(5,0x8005),(0,1,0x80,0xffff,0xff00),(0,1,0x8000,0x8001,0x8002,0xffff),(0,0x8000,0xffff)):
  m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x60,sel);put(m,a+0x1c,latch);put(m,a+0x22,timer);put(m,a+0xfe,result);put(m,a+0x2e,face)
  yield 'init-mask-countdown-mode',m,slot,None
 for ptr,latch,mode in itertools.product((0,0x80000000,0x123456,0xdeadbeef),(5,0x8005),range(4)):
  m,a,t,p=seed();put(m,a+0x26,ptr,4);put(m,a+0x1c,latch);put(m,RAM+0x161,mode,1);put(m,a+0xfe,0);yield 'target-unconsumed',m,4,None
 for slot,mode,result in itertools.product((0,8),(1,3),(0,0x8001)):
  m,a,t,p=seed(slot);put(m,a+0x1c,5);put(m,RAM+0x161,mode,1);put(m,a+0xfe,result);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(129):yield 'sequence-Royal-128',m,slot,regs
  assert m[a+0x20:a+0x22]==bytes.fromhex('0005') and m[a+0x60:a+0x62]==bytes.fromhex('007a') and m[a+0x22:a+0x24]==bytes.fromhex('0000')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action6e-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 p=frozen/'tests/r18/action6e_oracle_bridge.c';p.parent.mkdir(parents=True);shutil.copy2(HERE/p.name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action6e_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action6e_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action6e_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,'native blocked',hex(-status),slot);assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  if name=='sequence-Royal-128':m[RAM:RAM+0x4000]=b.raw[RAM:RAM+0x4000];regs[:]=list(d)
  count+=1
 guards=0
 if not args.natural_only:
  m,a,t,p=seed();put(m,a+0x60,0x35);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32()
  assert native.action6e_oracle(b,d,4,ctypes.byref(nd))==-0x12614;guards=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x19d82,0x19d8a,0x19db2,0x19db8,0x19dc6,0x19dd6,0x19dde,0x19e06,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'beetle_stage4_snapshot_sha256':beetle_meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals','tick descriptor'],'intercepts':[],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['Native-F8C HOST2083 +actualBeetle-stage4 TAG1724 actor0/id7; projected state, not physical timing proof','Only raw6E admitted; raw35 remains guarded','Pure owner with no external dependency; descriptor kind0 and exact result mask','No full match or physical timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw6E cases,',guards,'guards; intact ROM')
