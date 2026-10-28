"""EC34 link specialization through whole, intact DCDE (no intercepts)."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--natural-only',action='store_true');p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x21]==0xff and rom[0xec34:0xec40]==bytes.fromhex('246800260c2a00050021662a')
capture=HERE/'human_ec34_snapshot_host6037.bin';meta=json.loads((HERE/'human_ec34_manifest.json').read_text())
assert hashlib.sha256(capture.read_bytes()).hexdigest()==meta['snapshot_sha256']
rng=random.Random(0xec34);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();groups={};count=0;real_header_reads=0
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000);a=actors[slot];t=actors[(slot+1)%9]
 put(m,RAM+0x161,0,1);put(m,a+0x20,0x8004);put(m,a+0x65,9,1);put(m,a+0xaa,0x4000);put(m,a+0xfe,0)
 put(m,a+0xa3,3,1);put(m,a+0x32,1,1);put(m,a+0x34,0,1);put(m,a+0x7a,t,4);put(m,t+0x34,0,1);put(m,a+0x26,0,4)
 return m,a,t
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=capture.read_bytes()[32:]
 yield 'natural-HOST6037-actor1',m,1,list(struct.unpack_from('=8I',capture.read_bytes()))
 if args.natural_only:return
 for slot,tag in itertools.product(range(9),(0,1,0x7f,0x80,0xff)):
  m,a,t=seed(slot);put(m,a+0x26,tag<<24,4)
  yield 'ROM-header21-physical-zero-tagged',m,slot,None
 for slot,lslot,tag,lstate,sel,hi in itertools.product((0,1,8),range(9),(0,0x80,0xff),(4,5,0x85),(0x0e,0x0f,0x23,0x35,0x36),(0,0x10,0x80,0xff)):
  if slot==lslot:continue
  m,a,t=seed(slot);link=actors[lslot];put(m,a+0x26,(tag<<24)|link,4);put(m,link+0x20,0x8000|lstate);put(m,link+0x60,(hi<<8)|sel)
  yield 'actor-link-byte-state-selector-flag',m,slot,None
 for slot,tag,hi in itertools.product(range(9),(0,0x80,0xff),(0,0x10,0x80)):
  m,a,t=seed(slot);put(m,a+0x26,(tag<<24)|a,4);put(m,a+0x60,(hi<<8)|0x35)
  yield 'self-link-state4',m,slot,None
 for fresh,tag in itertools.product(range(256),(0,0xff)):
  m,a,t=seed();put(m,a+0xa3,fresh,1);put(m,a+0x26,tag<<24,4)
  yield 'fresh-low-two-bits-lazy',m,1,None
 for state,fe in itertools.product((4,0x8004),(0,1,0x80,0x8000,0xffff)):
  m,a,t=seed();put(m,a+0x20,state);put(m,a+0xfe,fe);put(m,a+0x7a,0xdeadbeef,4);put(m,a+0x26,0xdeadbeef,4)
  if state==0x8004 and fe==0:continue
  yield 'DCDE-inactive-FE-gates-lazy',m,1,None
 for mash,sub,flag,bad in itertools.product((0,1,0x3fff,0x4001,0xffff),(0,8,9),(0,0x10),(0xdeadbeef,0x1c05b1)):
  m,a,t=seed();put(m,a+0xaa,mash);put(m,a+0x65,sub,1);put(m,t+0x34,flag,1);put(m,a+0x26,bad,4)
  yield 'lazy-unconsumed-link',m,1,None
 for slot,sub,flag in itertools.product(range(9),(8,9,0x89),(0,0x10,0xff)):
  m,a,t=seed(slot);put(m,a+0x65,sub,1);put(m,t+0x34,flag,1);put(m,a+0x26,0,4)
  yield 'EBC4-branch-order-target-flags',m,slot,None
 for slot,sel,flag in itertools.product(range(9),(0x4a,0x51,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x52,0x53,0x64),(0,0x10)):
  m,a,t=seed(slot);put(m,a+0x20,0x8005);put(m,a+0x60,0x8000|sel);put(m,t+0x34,flag,1)
  yield 'EBC4-existing-completion-branches',m,slot,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-ec34-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'human_ec34_oracle_bridge.c';shutil.copy2(HERE/bridge.name,bridge)
 sources={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 source_sha=hashlib.sha256((frozen/'src/wf_r17_human_raw23.c').read_bytes()).hexdigest()
 lib=frozen/'ec34.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge.name,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.human_ec34_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 def run(m,slot,regs,stop=0x70000):
  global real_header_reads
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0xdcde)
  for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
  cpu.w_reg(8,actors[slot])
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0xec38 and (cpu.r_reg(10)&0xffffff)==0:
    assert mem.r8(0x21)==0xff;real_header_reads+=1
   if pc==stop:break
   cpu.execute(1)
  assert cpu.r_pc()==stop,('ROM runaway',hex(cpu.r_pc()),hex(stop))
 def compare(name,m,slot,regs,status=0,stop=0x70000):
  run(m,slot,regs,stop);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);stage=ctypes.c_uint32();got=native.human_ec34_oracle(buf,dr,slot,ctypes.byref(stage))
  assert got==status,(name,'native status',hex(got),hex(status))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   actual=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert actual==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(actual,want))if g!=w][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)])
 for name,m,slot,regs in fixtures():
  m[0xc0000:0xc4000]=rng.randbytes(0x4000);compare(name,m,slot,regs or[rng.getrandbits(32)for _ in range(8)]);count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for slot,bad in itertools.product((0,1,8),(1,0x100,0x7ffff,0x1bffff,0x1c05b1,0x1c2000,0xdeadbeef)):
   m,a,t=seed(slot);put(m,a+0x26,bad,4);compare('unsupported-link-prefix',m,slot,[rng.getrandbits(32)for _ in range(8)],-0xec34,0xec34);guards+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0xec34,0xec38,0xec40,0xec48,0xec50,0xec58,0xec60,0xec6a,0xedb0,0xedb6)
if not args.natural_only:assert all(x in coverage for x in required),('missing branches',[hex(x)for x in required if x not in coverage])
assert hashlib.sha256((ROOT/'src/wf_r17_human_raw23.c').read_bytes()).hexdigest()==source_sha
assert real_header_reads>0
report={'status':'PASS','cases':count,'groups':groups,'guards_and_ROM_prefixes':guards,'real_ROM21_read_instructions':real_header_reads,'source_sha256':source_sha,'sources_sha256':sources,'snapshot_sha256':meta['snapshot_sha256'],'compared':['RAM16KiB','FG16KiB','D0-D7','RNG and globals'],'intercepts':[],'required_ROM_pcs':list(map(hex,required)),'coverage_pcs':list(map(hex,sorted(coverage))),'limits':['Only EC34 LINK permits physical null as immutable ROM21=FF; other nonactor pointers retain EC34 guard','24-bit tagged actor/null links proven; stack/A/CCR outside typed ABI','Natural HOST blocked fixture; unrepresented arcade bytes zero, no SH2 trajectory or global ROM equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'whole intact DCDE calls,',guards,'ROM guard prefixes')
