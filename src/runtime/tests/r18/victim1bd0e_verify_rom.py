"""Raw54 victim1BD0E naturalHOST6733 vs whole intact1C03E; raw16KB/FG/D0-D7/descriptor."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--guards-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'victim1bd0e_manifest.json').read_text());capture=(HERE/'victim1bd0e_snapshot_host6733.bin').read_bytes();assert len(capture)==16440 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
assert rom[0x1afd4+0x1e*4:0x1afd8+0x1e*4]==bytes.fromhex('0001bcf6');assert rom[0x1bcf6:0x1bd0e]==bytes.fromhex('0001bd0e00010001001001720001bd0e00010001001001ae')
rng=random.Random(0x1bd0e);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=5):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):put(m,at+2,i%12);put(m,at+6,0x250);put(m,at+10,0x150);put(m,at+14,0x140);put(m,at+0x2e,0);put(m,at+0x26,at,4)
 a=actors[slot];p=actors[(slot+1)%9]
 put(m,a+0x1c,0x8004);put(m,a+0x20,0x8004);put(m,a+0x64,0x1e);put(m,a+0x60,0x54);put(m,a+0x26,p,4);put(m,p+0x26,a,4);put(m,a+0x24,0);put(m,a+0x22,1);return m,a,p

def fixtures():
 if args.guards_only:return
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:];yield 'natural-HOST6733-HP0',m,5,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,latch,phase,timer,selector,high in itertools.product((0,5,8),(4,0x8004,0x7f04),(0,0xfe,0x1fe,0xfffe,0xffff),(0,1,0xff00,0xffff),(0xf,0x10f,0x54,0x8054),(0,0xff)):
  m,a,pa=seed(slot);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x60,selector);put(m,a+0x64,(high<<8)|0x1e);yield 'BYTE-latch-FE-selector-WORD-timer-subflags',m,slot,None
 for slot,partner,recipient,tag,face in itertools.product((0,5,8),range(9),range(9),(0,0x40000000,0xff000000),(0,0x8000)):
  m,a,pa=seed(slot);pa=actors[partner];put(m,a+0x1c,4);put(m,a+0x26,pa|tag,4);put(m,pa+0x26,actors[recipient]|tag,4);put(m,pa+0x2e,face)
  yield 'init-two-hop-all-actors-self-alias-tags',m,slot,None
 for phase,timer,link,selector in itertools.product((0,1,0xfe,0x1fe,0xffff),(0,0xff00,0xffff),(0,0x80000000,0xdeadbeef,0x1c05b1),(0xf,0x54)):
  m,a,pa=seed();put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x26,link,4);put(m,a+0x60,selector);yield 'latched-invalid-links-never-read',m,5,None
 for face,x,y,z,selector in itertools.product((0x80,0xff01,0x8080),(0,0xffff),(0,0xffff),(0,0xffff),(0xf,0x54)):
  m,a,pa=seed();put(m,a+0x1c,4);put(m,pa+0x2e,face);put(m,pa+6,x);put(m,pa+10,y);put(m,pa+14,z);put(m,a+0x60,selector);yield 'init-WORD-coordinate-wrap-BYTE-facing-padding',m,5,None
 for slot,latch,selector,flags in itertools.product(range(9),(4,0x8004),(0xf,0xff0f,0x54),(0,0x80,0x8000,0xffff)):
  m,a,pa=seed(slot);put(m,a+0x1c,latch);put(m,a+0x60,selector);put(m,a+0x18,flags);put(m,a+0x1a,0xabcd);put(m,a+0x24,0xfe);put(m,a+0x66,0);put(m,a+0xa0,0x8001);yield 'all9-tail-HP-result-preservation',m,slot,None
 for slot,selector in itertools.product((0,5,8),(0xf,0x54)):
  m,a,pa=seed(slot);put(m,a+0x1c,4);put(m,a+0x60,selector);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(20):
   yield 'sequence-original16tick-victim-animation',m,slot,regs
   if m[a+0x65]!=0x1e:break

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-victim1bd0e-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/victim1bd0e_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source_sha=hashlib.sha256((frozen/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest();lib=frozen/'action.so'
 sources={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 # Observe the owner's actual alternate argument, even when1C0C8 does not
 # load a pose. One isolated host trace write; production/ROM untouched.
 copied_source=frozen/'src/wf_r17_dispatch_1c03e.c';copied_text=copied_source.read_text()
 marker='static wf_status_t state4_1bd0e_tick(wf_actor_raw_t*a,const wf_anim_desc_t*d,unsigned alternate){'
 assert copied_text.count(marker)==1
 observer='extern uint32_t wf_test_victim1bd0e_descriptor;\n'+marker+'\n    wf_test_victim1bd0e_descriptor=alternate?0x1bd02u:d->descriptor_rom;'
 copied_source.write_text(copied_text.replace(marker,observer));observer_sha=hashlib.sha256(copied_source.read_bytes()).hexdigest()
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','tests/r18/victim1bd0e_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.victim1bd0e_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.victim1bd0e_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'selected descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)]);count+=1
  for actor in actors:
   for off in(0x66,0xa0):assert buf.raw[actor+off:actor+off+2]==m[actor+off:actor+off+2],(name,'unowned HP/result mutation')
  assert buf.raw[RAM+0x5c:RAM+0x60]==m[RAM+0x5c:RAM+0x60],(name,'unowned RNG mutation')
  if name=='sequence-original16tick-victim-animation':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  if count%1000==0:print('verified',count,flush=True)
 guards=0;prefixes=0
 if not args.natural_only:
  for slot,stage,bad,face in itertools.product((0,5,8),(0,1),(0,0x80000000,0xdeadbeef,0x1c0f1c,0x1c05b1),(0,0x8000)):
   m,a,pa=seed(slot);put(m,a+0x1c,4);put(m,pa+0x2e,face);put(m,(a if stage==0 else pa)+0x26,bad,4);regs=[rng.getrandbits(32)for _ in range(8)];pc=0x10b9e if stage==0 else 0x10ba2
   b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.victim1bd0e_oracle(b,d,slot,ctypes.byref(desc));assert status==-pc,(slot,stage,hex(bad),'guard',hex(-status));guards+=1
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,a)
   for i,v in enumerate(regs):cpu.w_reg(i,v)
   for _ in range(20000):
    if cpu.r_pc()==pc:break
    cpu.execute(1)
   assert cpu.r_pc()==pc,'ROM guard prefix missing';want=bytes(mem.r_block(RAM,0x4000));got=b.raw[RAM:RAM+0x4000];assert got==want,('guard RAM',[(hex(RAM+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10]);assert list(d)==[cpu.r_reg(i)for i in range(8)],'guardD8';prefixes+=1
  for sub in(43,127,255):
   m,a,pa=seed();put(m,a+0x64,sub);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32();assert native.victim1bd0e_oracle(b,d,5,ctypes.byref(desc))==-0x1afd4;guards+=1
 if not args.natural_only and not args.guards_only:
  assert {0x1bd0e,0x1bd16,0x1bd28,0x1bd3c,0x1bd44,0x1bd50,0x1bd58,0x10b9a,0x10bbc,0x1c0c8}<=coverage,'missing victim branches'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'retained_guards':guards,'guard_prefix_cases':prefixes,'source_sha256':source_sha,'sources_sha256':sources,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','actual selected descriptor token and both original poses/periods'],'intercepts':[],'native_trace_observer':'One isolated write in copied state4_1bd0e_tick records its actual alternate argument; production/ROM unchanged','instrumented_native_source_sha256':observer_sha,'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
