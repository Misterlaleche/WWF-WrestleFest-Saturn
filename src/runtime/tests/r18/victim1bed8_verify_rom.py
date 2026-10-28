#!/usr/bin/env python3
"""NaturalTAG5427 victim24/1BED8 vs intact whole1C03E, no intercepts."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'victim1bed8_snapshot_01359.bin').read_bytes();meta=json.loads((HERE/'victim1bed8_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x1afd4+0x24*4:0x1afd8+0x24*4]==bytes.fromhex('0001becc')
assert rom[0x1becc:0x1bed8]==bytes.fromhex('0001bed80001000100100122')
rng=random.Random(0x1bed8);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot]
 put(m,a+0x1c,0x8004);put(m,a+0x20,0x8004);put(m,a+0x64,0x24);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x36,0);put(m,a+0x18,0);put(m,a+0x2e,0);put(m,RAM+0x161,1,1)
 return m,a
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:16440];yield 'natural-TAG5427-actor5',m,5,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,latch,phase,timer,face,subhigh,tail in itertools.product((0,4,8),range(4),(4,0x8004),(0,0xfe,0x1fe,0xffff),(0,1,0xff00,0xffff),(0,0x8000,0xffff),(0,0x20,0xff),(0,0x8000)):
  m,a=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x2e,face);put(m,a+0x64,(subhigh<<8)|0x24);put(m,a+0x18,tail);put(m,a+0x1a,0xabcd)
  yield 'init-FE-tail-byte-preservation',m,slot,[rng.getrandbits(32)for _ in range(8)]
 for mode,slot in itertools.product((0,1,3),(0,8)):
  m,a=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x1c,4);put(m,a+0x2e,0x8000);put(m,a+0x64,0xa024);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(24):
   yield 'sequence-pose-through-FE',m,slot,regs
   if m[a+0x65]==5:break
  else:raise AssertionError('1BED8 descriptor sequence did not finish')
  assert m[a+0x64:a+0x66]==bytes.fromhex('a005') and m[a+0x20:a+0x22]==bytes.fromhex('0004')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-victim1bed8-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 for name in ('victim1bed8_oracle_bridge.c','victim1b684_oracle_bridge.c'):
  p=frozen/'tests/r18'/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(HERE/name,p)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest()
 lib=frozen/'victim.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/victim1bed8_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.victim1bed8_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,x in enumerate(regs):cpu.w_reg(i,x)
  for _ in range(10000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);status=native.victim1bed8_oracle(b,d,slot)
  assert status==0,(name,'native blocked',hex(-status),slot)
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
  if name=='sequence-pose-through-FE':m[RAM:RAM+0x4000]=b.raw[RAM:RAM+0x4000];regs[:]=list(d)
  count+=1
 guards=0
 if not args.natural_only:
  for subtype in (43,127,255):
   m,a=seed(4);put(m,a+0x64,subtype);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values)
   assert native.victim1bed8_oracle(b,d,4)==-0x1afd4 and b.raw[RAM:RAM+0x4000]==m[RAM:RAM+0x4000]and list(d)==values;guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1bed8,0x1bee0,0x1bee6,0x1beee,0x1bf06,0x1c0c8,0x1c0ba)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(x)for x in required if x not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM16KiB','FG16KiB','D0-D7'],'intercepts':[],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST TAG full-flow5427 actor5; not physical Saturn RAM','Only victim1BED8 admitted; other dispatch guards unchanged','Original kind1 descriptor tick retained after FE subtype change; no external callees','Typed combat ABI excludes A/CCR; no full match equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'victim1BED8 cases,',guards,'guards; intact ROM, no intercepts')
