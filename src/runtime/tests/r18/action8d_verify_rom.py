"""Actual HOST7856 raw8D vs whole intact1C03E; fullRAM/FG/D8, no intercepts."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action8d_manifest.json').read_text());capture=(HERE/'action8d_snapshot_host7856.bin').read_bytes();assert len(capture)==16440 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
natural_fg=(Path(meta['origin'])/'blocked-fg.bin').read_bytes();assert len(natural_fg)==0x4000 and hashlib.sha256(natural_fg).hexdigest()==meta['dumps_sha256']['blocked-fg.bin']
assert rom[0x12614+0x8d*4:0x12618+0x8d*4]==bytes.fromhex('00017f9a');assert rom[0x17f9a:0x17fb2]==bytes.fromhex('00017fb2000100040006ff00ff00ff0000b200b3001f0012')
assert rom[0x25ca+12*6:0x25ca+13*6]==bytes.fromhex('ff0000000028') and rom[0x25ca+3*6:0x25ca+4*6]==bytes.fromhex('000003000038')
rng=random.Random(0x17fb2);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=0):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):
  put(m,at,0x8000);put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140)
  put(m,at+0x26,actors[(i+1)%9],4);put(m,at+0x2e,0);put(m,at+0x32,0);put(m,at+0x40,0);put(m,at+0x42,0)
 a=actors[slot];put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x8d);put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x36,0);put(m,RAM+0x161,0,1);put(m,RAM+0x7e,0)
 return m,a
def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:];yield 'natural-HOST7856-HP0-no-manipulation',m,0,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,mode,latch,phase,timer,face in itertools.product((0,1,8),(0,1,2,0xff),(5,0x8005,0x7f05),(0,1,2,0xfe,0x101,0x102),(0,1,0xff00),(0,0x8000)):
  m,a=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x2e,face);yield 'init-phase-lowBYTE-latch-highBYTE',m,slot,None
 for slot,mode,phase,contact,activity in itertools.product((0,1,8),(0,1,2,0xff),(0,1,2),(0x0100,0x0101,0x0102,0x0103,0x0511,0x0512,0x0310,0x0313),(0,1,0x100,0xff00)):
  m,a=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0x24,phase);put(m,a+0x36,contact)
  for off in(0x1150,0x1180,0x11b0,0x11e0):put(m,RAM+off,activity)
  yield '10FC6-carry-ignored-WORD-lowBYTE-rope-flags',m,slot,None
 for slot,world,side,face,phase,selector in itertools.product((0,4,8),range(7),(0,4),(0,0x8000),(1,0x101,2),(0x8d,0x808d)):
  m,a=seed(slot);put(m,RAM+0x7e,world);put(m,a+0x32,side);put(m,a+0x2e,face);put(m,a+0x24,phase);put(m,a+0x36,0x0010);put(m,a+0x60,selector)
  yield '10B62-all-owned-world-routes-contact-restored',m,slot,None
 for world,x,y,z,face in itertools.product((0,1,2,3,5,6),(0,0x250,0xffff),(0,0x150,0xffff),(0,0x140,0xffff),(0,0x8000)):
  m,a=seed();put(m,RAM+0x7e,world);put(m,a+6,x);put(m,a+10,y);put(m,a+14,z);put(m,a+0x2e,face);put(m,a+0x24,1);put(m,a+0x36,0x0010);yield 'resolver-WORD-coordinate-wrap',m,0,None
 for phase,contact,face,bad,world in itertools.product((0,1,2,0xfe,0x101,0x102),(0,0x10),(0,0x8000),(0,0x80000000,0xdeadbeef,0x1c05b1),(0,7)):
  if world==7 and phase in(1,0x101)and contact:continue
  m,a=seed();put(m,a+0x24,phase);put(m,a+0x36,contact);put(m,a+0x2e,face);put(m,a+0x26,bad,4);put(m,a+0x86,bad,4);put(m,RAM+0x7e,world);yield 'all-targets-and-team-links-unconsumed-lazy-world',m,0,None
 for world,face,flags,contact in itertools.product((7,0xfff7),(0,0x8000),(0x2000,0x2004,0xff04),(0x0010,0x0112,0x0310)):
  m,a=seed();put(m,RAM+0x7e,world);put(m,a+0x24,1);put(m,a+0x36,contact);put(m,a+0x32,flags);put(m,a+0x2e,face);yield 'resolver-BTST5-early-return-before-invalid-world',m,0,None
 for hp,result,phase,contact in itertools.product((0,1,0xffff),(0,0x4000,0x8001),(0,1,2),(0,0x10)):
  m,a=seed();put(m,a+0x66,hp);put(m,a+0xa0,result);put(m,a+0x24,phase);put(m,a+0x36,contact);yield 'HP-result-RNG-untouched',m,0,None
 for face,selector,phase,timer in itertools.product((0x80,0xff01,0x8080),(0x408d,0xff8d),(1,2,0x101,0x102,0xfffe),(0,0xff,0xff00,0xffff)):
  m,a=seed();put(m,a+0x2e,face);put(m,a+0x60,selector);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x36,0x0310);yield 'BYTE-facing-padding-selector-highbits-WORD-timer',m,0,None
 for slot,world,flags,face in itertools.product(range(9),(0x100,0x101,0x105,0xfffe),(0,4,0x2000),(0,0x8000)):
  m,a=seed(slot);put(m,RAM+0x7e,world);put(m,a+0x32,flags);put(m,a+0x2e,face);put(m,a+0x24,1);put(m,a+0x36,0x10);yield 'all-nine-slots-world-mask-WORD',m,slot,None
 for contact,own_activity,other_activity in itertools.product((0x0101,0x0102,0x0103),(0,1,0x100,0xff01),(0,1,0xdead)):
  m,a=seed();put(m,a+0x36,contact);put(m,a+0x24,1)
  put(m,RAM+0x1150,own_activity);put(m,RAM+0x11b0,own_activity);put(m,RAM+0x1180,other_activity);put(m,RAM+0x11e0,other_activity)
  yield 'rope-pair-WORD-write-only-when-LOW-BYTE-empty',m,0,None
 # Shared synthetic contact events are explicitly a callback-sequence test;
 # neither model runs physics or claims a natural trajectory here.
 for slot,face in itertools.product((0,4,8),(0,0x8000)):
  m,a=seed(slot);put(m,a+0x1c,5);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(20):
   phase=m[a+0x25];put(m,a+0x36,0x0010 if phase in(1,2)else 0)
   yield 'sequence-shared-contact-tape',m,slot,regs
   if phase==2:break
  else:raise AssertionError('raw8D did not reach landing phase2')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action8d-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action8d_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 source_hashes={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for directory in('src','include','generated')for item in sorted((frozen/directory).rglob('*'))if item.is_file()for name in[item.relative_to(frozen).as_posix()]}
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/action8d_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action8d_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 def start(m,slot,regs):
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
  for i,v in enumerate(regs):cpu.w_reg(i,v)
 def compare(name,buf,dr):
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)])
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=natural_fg if name.startswith('natural-')else rng.randbytes(0x4000);start(m,slot,regs);descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action8d_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor));compare(name,buf,dr)
  # Raw8D never owns HP/result/RNG and must not turn a knocked-out actor into a winner.
  a=actors[slot]
  for at,n in((a+0x66,2),(a+0xa0,2),(RAM+0x5c,4)):assert buf.raw[at:at+n]==m[at:at+n],(name,'unowned HP/result/RNG mutation')
  count+=1;groups[name]=groups.get(name,0)+1
  if name=='sequence-shared-contact-tape':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  if count%1000==0:print('verified',count,flush=True)
 guards=0;prefixes=0
 if not args.natural_only:
  for slot,world,face,contact in itertools.product((0,1,8),(7,0xfff7),(0,0x8000),(0x0010,0x0112,0x0310)):
   m,a=seed(slot);put(m,RAM+0x7e,world);put(m,a+0x24,0x101);put(m,a+0x36,contact);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();assert native.action8d_oracle(buf,dr,slot,ctypes.byref(desc))==-0x280dc,'resolver guard';guards+=1
   start(m,slot,regs)
   for _ in range(20000):
    if cpu.r_pc()==0x280dc:break
    cpu.execute(1)
   assert cpu.r_pc()==0x280dc;compare('retained280DC-entry-prefix',buf,dr);assert buf.raw[a+0x3e:a+0x40]==bytes.fromhex('0050');assert dr[0]&0xffff==contact;prefixes+=1
  m,a=seed();put(m,a+0x60,0xfe);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();assert native.action8d_oracle(buf,dr,0,ctypes.byref(desc))==-0x12614;guards+=1
 if not args.natural_only and not args.guards_only:
  assert {0x17fba,0x17fc8,0x17fde,0x17fee,0x17ffc,0x1800c,0x1801a,0x10ff0,0x11012,0x1102c,0x10b62,0x1c0c8}<=coverage,'missing raw8D branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'guard_prefix_cases':prefixes,'groups':groups,'source_sha256':source_sha,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','original tickdescriptor'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)],'frozen_sources_sha256':source_hashes}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
