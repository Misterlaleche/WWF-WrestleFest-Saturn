"""Native Stage7b raw3E versus intact1C03E,1174C,11710 and common animation."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();meta=json.loads((HERE/'action3e_manifest.json').read_text());capture=(HERE/'action3e_snapshot_beetle_frame1230.bin').read_bytes()
assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1'] and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'] and len(capture)==16440
assert list(struct.unpack_from('=6I',capture))==meta['header']==[1230,1,1,0x1c03e,0,1]
assert int.from_bytes(rom[0x12614+0x3e*4:0x12618+0x3e*4],'big')==0x18314 and rom[0x18314:0x1831a]==bytes.fromhex('0001831a0000')
assert rom[0x116ae:0x116ba]==bytes((27,29,28,25,30,27,24,30,22,27,28,27)) and rom[0x116ba:0x116c6]==bytes([12]*12)
for at,durations,poses in((0x115f2,[10]*4,[1,2,3,4]),(0x1160a,[12]*4,[5,6,7,8]),(0x11622,[20,16,20,16],[0x7c,0x7d,0x7e,0x7f]),(0x1163a,[20,16,20,16],[0x80,0x81,0x82,0x83])):
 assert struct.unpack_from('>10H',rom,at+4)==tuple([2,4]+durations+poses)
rng=random.Random(0x1831a);actors=[RAM+0x5b0+i*0x10c for i in range(9)];count=0;coverage=set();groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot]
 put(m,a+2,7);put(m,a+0x20,0x8005);put(m,a+0x1c,0x8005);put(m,a+0x60,0x3e);put(m,a+0x22,1);put(m,a+0x24,0);put(m,a+0x2e,0);put(m,a+0x32,0,1);put(m,a+0x33,0,1);put(m,a+0x56,0,1);put(m,a+0x74,0);put(m,a+0xfe,0)
 return m,a
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture[56:]
 yield 'natural-Beetle-Stage7b-1230',m,1,struct.unpack_from('=8I',capture,24)
 if args.natural_only:return
 for slot,id,f32,role,f33,rel,latch,face in itertools.product((0,4,8),range(12),(0,8),(0,0x80),(0,0x20),(0,0x8000,0x8001),(5,0x8005),(0,0x8000)):
  m,a=seed(slot);put(m,a+2,id);put(m,a+0x32,f32,1);put(m,a+0x56,role,1);put(m,a+0x33,f33,1);put(m,a+0x74,rel);put(m,a+0x1c,latch);put(m,a+0x2e,face)
  yield '1174C-every-character-speed-modifier-init-latch',m,slot,None
 for f32,rel,face,phase,timer,fe in itertools.product((0,8),(0,0x8000,0x8001),(0,0x8000,0x80,0xffff),(0,1,3,4,0xfe,0x100,0xffff),(0,1,0xff00,0xffff),(0,0x80,0x8000)):
  m,a=seed();put(m,a+0x32,f32,1);put(m,a+0x74,rel);put(m,a+0x2e,face);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0xfe,fe)
  yield '11710-four-descriptors-WORD-counter-FE-BYTE',m,1,None
 for slot,selector,fe in itertools.product(range(9),(0x3e,0x403e,0x803e,0xff3e),(0,0x80,0x8000)):
  m,a=seed(slot);put(m,a+0x60,selector);put(m,a+0xfe,fe)
  yield 'selector-lowBYTE-allslots-terminal',m,slot,None
 for latch,fe,bad in itertools.product((5,0x8005),(0,0x8000),(0,0x80000000,0xdeadbeef,0x1c05b1)):
  m,a=seed();put(m,a+0x1c,latch);put(m,a+0xfe,fe)
  for off in(0x26,0x76,0x7a,0x86):put(m,a+off,bad,4)
  yield 'no-pointer-consumed',m,1,None
 for f32,rel,face in itertools.product((0,8),(0,0x8000,0x8001),(0,0x8000)):
  m,a=seed();put(m,a+0x1c,5);put(m,a+0x32,f32,1);put(m,a+0x74,rel);put(m,a+0x2e,face);regs=[rng.getrandbits(32)for _ in range(8)]
  for tick in range(80):yield 'sequence-loop-live-D-RNG',m,1,regs
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
def owner_hash(data):
 if b'/* $12614[$3E]'not in data:return None
 start=data.index(b'/* $12614[$3E]');end=data.index(b'/* $12614[$3C]',start)
 return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x3eu:[^\n]*',data,re.M)[0]).hexdigest()
with tempfile.TemporaryDirectory(prefix='wf-action3e-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/action3e_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(HERE/bridge.name,bridge)
 source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_sha=hashlib.sha256(source).hexdigest();owner_sha=owner_hash(source)
 sources={str(path.relative_to(frozen)):hashlib.sha256(path.read_bytes()).hexdigest()for path in frozen.rglob('*')if path.is_file()}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action3e_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action3e_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot]);descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action3e_oracle(buf,dr,slot,ctypes.byref(desc));assert status==0,(name,count,'native blocked',hex(-status));assert desc.value==descriptor,(name,'tick descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,count,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,count,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)])
  if name=='sequence-loop-live-D-RNG':m[RAM:RAM+0x4000]=buf.raw[RAM:RAM+0x4000];regs[:]=list(dr)
  count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for id,latch,f32 in itertools.product((12,0xff,0x100,0x8000,0xffff),(5,0x8005),(0,8)):
   m,a=seed();put(m,a+2,id);put(m,a+0x1c,latch);put(m,a+0x32,f32,1);regs=[rng.getrandbits(32)for _ in range(8)]
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();assert native.action3e_oracle(buf,dr,1,ctypes.byref(desc))==-0x1176a
   mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
   for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
   cpu.w_reg(8,a)
   for _ in range(20000):
    if cpu.r_pc()==0x1176a:break
    cpu.execute(1)
   assert cpu.r_pc()==0x1176a
   for at,n in((RAM,0x4000),(0xc0000,0x4000)):assert buf.raw[at:at+n]==bytes(mem.r_block(at,n)),('guard prefix RAM/FG',id,latch,f32)
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],('guard prefix D0-D7',id,latch,f32,list(dr),[cpu.r_reg(i)for i in range(8)])
   guards+=1
  m,a=seed();put(m,a+0x60,0x35);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)();desc=ctypes.c_uint32();assert native.action3e_oracle(buf,dr,1,ctypes.byref(desc))==-0x12614;guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1831a,0x18322,0x18336,0x18350,0x1174c,0x11754,0x1175c,0x11780,0x1178e,0x11710,0x11718,0x11726,0x1172e,0x11736,0x11744,0x1c0c8,0x1c0fa,0x1c11e)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing ROM branches',[hex(pc)for pc in required if pc not in coverage])
assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==owner_sha,'Raw3E owner changed during oracle'
report={'status':'PASS','cases':count,'groups':groups,'retained_guards':guards,'ROM_prefix_checks':guards-1 if guards else 0,'source_sha256':source_sha,'raw3e_owner_sha256':owner_sha,'sources_sha256':sources,'snapshot_sha256':meta['snapshot_sha256'],'rom_sha1':meta['rom_sha1'],'compared':['RAM16KiB','FG16KiB','D0-D7','unchanged RNG/globals','four tick descriptors'],'intercepts':[],'required_rom_pcs':[hex(pc)for pc in required],'coverage_pcs':[hex(pc)for pc in sorted(coverage)],'limits':['Native Beetle Stage7b projected actors/D8/scalar globals; unrepresented arcadeRAM zero','Character WORD0..11 supported; other table entries retain1176A guard with literal prefix','Original stack/A/CCR outside typed combat ABI; no global ROM or hardware timing equivalence claim','Only raw3E added; existing walk descriptor helper reused without changes']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw3E intactROM calls,',guards,'guards')
