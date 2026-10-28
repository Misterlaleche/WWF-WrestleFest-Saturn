"""Raw54 against intact whole1C03E/10B9A/2052; no ROM interceptions."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus
from capstone import Cs,CS_ARCH_M68K,CS_MODE_BIG_ENDIAN,CS_MODE_M68K_000
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x54*4:0x12618+0x54*4]==bytes.fromhex('00018f62')
assert rom[0x18f62:0x18f82]==bytes.fromhex('00018f8200010002ff00002001c001c000018f8200010002ff00002002200220')
rng=random.Random(0x18f82);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0;groups={}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=1):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 for i,at in enumerate(actors):put(m,at,0x8000);put(m,at+2,i%12);put(m,at+0x26,actors[(i+1)%9],4)
 a=actors[slot];t=actors[(slot+1)%9]
 put(m,a+0x20,0x8005);put(m,a+0x60,0x54);put(m,a+0x1c,0x8005)
 put(m,a+0x24,0);put(m,a+0x22,1);put(m,a+0x2e,0);put(m,a+0x64,9);put(m,t+0x20,0x8004)
 return m,a,t
def fixtures():
 data=(HERE/'action54_snapshot_host6724.bin').read_bytes();meta=json.loads((HERE/'action54_manifest.json').read_text())
 assert len(data)==16440 and hashlib.sha256(data).hexdigest()==meta['snapshot_sha256']
 assert list(struct.unpack_from('=6I',data))==meta['header']
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x4000]=data[56:]
 yield 'natural-HOST6724',m,1,struct.unpack_from('=8I',data,24)
 if args.natural_only:return
 for slot,latch,phase,timer,sub in itertools.product((0,1,8),(5,0x8005,0xff05),(0,1,0xfe,0x100,0x1fe,0xffff),(0,1,0xff00),(8,9,0x109,0xff09)):
  m,a,t=seed(slot);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x64,sub)
  yield 'init-latch-BYTEfe-WORDphase-BYTEsub-descriptors',m,slot,None
 for slot,other,tag,face,state in itertools.product((0,1,8),range(9),(0,0x40000000,0xff000000),(0,0x8000),(4,0x8004,0x0084,0x8005)):
  m,a,t=seed(slot);t=actors[other];put(m,a+0x26,t|tag,4);put(m,t+0x20,state);put(m,a+0x2e,face)
  yield 'target-BYTEstate-allactors-self-alias-face',m,slot,None
 for slot,selector,face,x,y,z in itertools.product((0,8),(0x54,0x4054,0xff54),(0,0x8000),(0,0xffc0,0xffff),(0,0xffff),(0,0xffec,0xffff)):
  m,a,t=seed(slot);put(m,a+0x60,selector);put(m,a+0x2e,face);put(m,a+6,x);put(m,a+10,y);put(m,a+14,z)
  yield 'position-WORD-wrap-fraction-preservation-selector-high',m,slot,None
 for link,latch,phase,sub in itertools.product((0,0x40000000,0xff000000,0xdeadbeef,0x1c05b1,0x1c0f1c),(5,0x8005),(1,0xfe,0x100),(8,9)):
  m,a,t=seed();put(m,a+0x26,link,4);put(m,a+0x1c,latch);put(m,a+0x24,phase);put(m,a+0x64,sub)
  yield 'unconsumed-invalid-links',m,1,None
 for link in(0,0x40000000,0xff000000):
  m,a,t=seed();put(m,a+0x26,link,4)
  yield 'literal-null-dummy-or-tagged-zero-ROMstate-not4',m,1,None
 for slot,state,flags,timer in itertools.product((0,1,8),(0,4,5,7,0x8005),(0,0x80,0xff),(0,1,0xff00)):
  m,a,t=seed(slot);put(m,a+0x20,state);put(m,a+0x18,flags<<8|0x12);put(m,a+0x22,timer);put(m,a+0x24,0xfe)
  yield 'terminal-old-descriptor-tail-clear-flags',m,slot,None
 for slot,state in itertools.product(range(9),range(256)):
  m,a,t=seed(slot);put(m,t+0x20,0xa500|state);put(m,a+0x64,0x8a09 if state&1 else 0x8a08)
  yield 'all9-slots-all256-target-state-BYTEvalues',m,slot,None
 for latch in range(256):
  m,a,t=seed();put(m,a+0x1c,latch<<8|5);put(m,a+0x64,0xff09)
  yield 'all256-latch-BYTEvalues',m,1,None
 for slot,credit,config in itertools.product((0,1,8),(0,1,0xffff),(0,0x20,0xff)):
  m,a,t=seed(slot);put(m,RAM+0x7c,credit);put(m,RAM+0x67,config,1)
  yield 'sound2052-full-real-ROM-branches',m,slot,None
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action54-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge='tests/r18/action54_oracle_bridge.c';dest=frozen/bridge;dest.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,dest)
 # Private test composition until inventory adds the include/case to shared code.
 actions=frozen/'src/wf_r17_actions_r11.c';text=actions.read_text();wiring=False
 if(frozen/'src/wf_r18_action54.inc').exists()and '#include "wf_r18_action54.inc"'not in text:
  text=text.replace('wf_status_t wf_r17_dispatch_1c03e_r11(', '#include "wf_r18_action54.inc"\n\nwf_status_t wf_r17_dispatch_1c03e_r11(',1)
  text=text.replace('        default:return block(tr,0x00012614u);','        case 0x54u:tr->callback_rom=0x18f82u;tr->descriptor_rom=0x18f62u;s=raw54(rt,a,&d,tr);break;\n        default:return block(tr,0x00012614u);',1)
  actions.write_text(text);wiring=True
 hashes={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in frozen.rglob('*')if f.is_file()}
 lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c',bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action54_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for page in range(0,len(rom),0x10000):
  for width in range(3):
   mem.set_special_range_write_func(page,width,lambda address,value:None)
   mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 def run(m,slot,regs,stop=0x70000):
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot]);descriptor=0
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==stop:return descriptor
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   cpu.execute(1)
  raise AssertionError(('ROM runaway',hex(cpu.r_pc())))
 for name,m,slot,regs in fixtures():
  regs=regs or[rng.getrandbits(32)for _ in range(8)];m[0xc0000:0xc4000]=rng.randbytes(0x4000);descriptor=run(m,slot,regs)
  buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action54_oracle(buf,dr,slot,ctypes.byref(desc))
  assert status==0,(name,'native blocked',hex(-status));assert desc.value==descriptor,(name,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in((RAM,0x4000),(0xc0000,0x4000)):
   got=buf.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(dr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(dr),[cpu.r_reg(i)for i in range(8)])
  count+=1;groups[name]=groups.get(name,0)+1
 guards=0
 if not args.natural_only:
  for slot,bad,sub in itertools.product(range(9),(0xdeadbeef,0x1c05b1,0x1c0f1c,0xff1c0f1c),(8,9)):
   m,a,t=seed(slot);put(m,a+0x26,bad,4);put(m,a+0x64,sub);regs=[rng.getrandbits(32)for _ in range(8)]
   buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32();status=native.action54_oracle(buf,dr,slot,ctypes.byref(desc))
   assert status==-0x18fb0,(slot,hex(bad),'guard',hex(-status));run(m,slot,regs,0x18fb0)
   assert buf.raw[RAM:RAM+0x4000]==bytes(mem.r_block(RAM,0x4000)),'guard rawRAM prefix'
   assert list(dr)==[cpu.r_reg(i)for i in range(8)],('guard D0-D7 prefix',slot,hex(bad));guards+=1
  m,a,t=seed();put(m,a+0x60,0x35);buf=ctypes.create_string_buffer(bytes(m));dr=(ctypes.c_uint32*8)(*range(8));desc=ctypes.c_uint32()
  assert native.action54_oracle(buf,dr,1,ctypes.byref(desc))==-0x12614;guards+=1
  decoder=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
  expected_callback={i.address for i in decoder.disasm(rom[0x18f82:0x18fee],0x18f82)}
  expected_position={i.address for i in decoder.disasm(rom[0x10b9a:0x10bd0],0x10b9a)}
  assert expected_callback<=coverage,('raw54 missing callback PC coverage',sorted(expected_callback-coverage))
  assert expected_position<=coverage,('raw54 missing 10B9A PC coverage',sorted(expected_position-coverage))
  assert {0x2052,0x205e,0x2066,0x206c,0x1c0c8}<=coverage,'raw54 missing sound/tick branch coverage'
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
report={'status':'PASS','cases':count,'groups':groups,'retained_guards':guards,'sources_sha256':hashes,'private_test_wiring':wiring,'snapshot_sha256':hashlib.sha256((HERE/'action54_snapshot_host6724.bin').read_bytes()).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['rawRAM16KB','FG16KB','D0-D7/RNG/globals','tickdescriptor'],'intercepts':[],'coverage_pcs':[hex(x)for x in sorted(coverage)]}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='sources_sha256'}))
