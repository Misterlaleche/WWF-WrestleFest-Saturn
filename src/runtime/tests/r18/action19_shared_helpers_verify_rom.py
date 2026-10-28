"""Direct unchanged-ROM regressions for consumed bus aliases and Y==0160."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--legacy',choices=['resolver','bus']);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action19_snapshot_full_flow_1500.bin').read_bytes();rng=random.Random(0x284801129e)
actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();counts={0x1129e:0,0x280dc:0}
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def seed():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
 return m,actors[4],actors[8]
def fixtures():
 for mode,tag,ownrole,peerrole,cabinet in itertools.product(range(4),(0,0x40000000,0x80000000,0xff000000),(0,0x80),(0,0x80),(0,1)):
  for tier in ((0,1,3,4,0xff,0x100,0xffff) if mode==1 else range(3)):
   m,o,t=seed();put(m,o+0x26,t|tag,4);put(m,o+0x7a,t|tag,4);put(m,o+0x56,ownrole,1);put(m,t+0x56,peerrole,1);put(m,o+0x70,tier)
   put(m,RAM+0x161,cabinet,1);put(m,RAM+0x162,(tier+tag)%10);d=[rng.getrandbits(32)for _ in range(8)]
   d[1]=(d[1]&0xffff0000)|mode;d[2]=(d[2]&0xffff0000)|tier
   yield f'1129E mode={mode} tag={tag:x} roles={ownrole:x}/{peerrole:x} cabinet={cabinet} tier={tier:x}',m,d,0x1129e
 for scene,side,y,contact,face in itertools.product((2,6,0x82),(0,4),(0x15f,0x160,0x161),(0,4,0xff),(0,0x8000)):
  m,o,t=seed();put(m,RAM+0x7e,scene);put(m,o+0x32,side);put(m,o+0x36,contact,1);put(m,o+0x37,contact,1)
  put(m,o+6,0x260);put(m,o+10,y);put(m,o+14,0x140);put(m,o+0x40,0);put(m,o+0x42,0);put(m,o+0x3e,0);put(m,o+0x2e,face)
  yield f'28480 scene={scene:x} side={side} y={y:x} contact={contact:x} face={face:x}',m,[rng.getrandbits(32)for _ in range(8)],0x280dc
 for y,x in itertools.product((0x13f,0x140,0x141),(0x293,0x294,0x38b,0x38c)):
  m,o,t=seed();put(m,RAM+0x7e,2);put(m,o+0x32,4);put(m,o+0x36,0)
  put(m,o+6,x);put(m,o+10,y);put(m,o+14,0x100);put(m,o+0x40,0);put(m,o+0x42,0);put(m,o+0x3e,0)
  yield f'2851E y={y:x} x={x:x}',m,[rng.getrandbits(32)for _ in range(8)],0x280dc
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action19-shared-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 bridge='tests/r18/action19_oracle_bridge.c';dest=frozen/bridge;dest.parent.mkdir(parents=True);shutil.copy2(ROOT/bridge,dest)
 if args.legacy=='resolver':
  target=frozen/'src/wf_r17_resolver_280dc.c';s=target.read_text();old='if(y!=0x0160u)corr_y(a,y,0x0160u,1u,2u);';assert s.count(old)==1;target.write_text(s.replace(old,old+'\n    setbit(a,WF_A_CONTACT+1u,2u);'))
 if args.legacy=='bus':
  target=frozen/'src/wf_r17_rng1129e.c';s=target.read_text();old='    p&=0x00ffffffu; /* 68000 dereference bus; the stored LONG remains intact. */\n';assert s.count(old)==1;target.write_text(s.replace(old,''))
 manifest={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
 lib=frozen/'helpers.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));fn=native.action19_helper_oracle;fn.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_int)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,d,helper in fixtures():
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(helper);cpu.w_reg(8,actors[4]);cpu.w_sr(0x2000)
  for i,v in enumerate(d):cpu.w_reg(i,v)
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));nd=(ctypes.c_uint32*8)(*d);carry=ctypes.c_int();st=fn(b,nd,4,helper,ctypes.byref(carry))
  assert st==0,(name,'native guard',hex(-st))
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7')
  if helper==0x1129e:assert carry.value==(cpu.r_sr()&1),(name,'carry',carry.value,cpu.r_sr()&1)
  got=b.raw[RAM:RAM+0x2000];want=bytes(mem.r_block(RAM,0x2000));assert got==want,(name,'RAM',[(hex(RAM+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10])
  counts[helper]+=1
 guards=0
 for mode,expected in ((0,0x112c8),(1,0x1130c),(2,0x11376)):
  m,o,t=seed();put(m,o+0x56,0,1);put(m,o+0x26,0x40123456,4);put(m,o+0x7a,0x80123456,4);put(m,o+0x70,0)
  d=(ctypes.c_uint32*8)(0,mode,0,0,0,0,0,0);b=ctypes.create_string_buffer(bytes(m));c=ctypes.c_int();assert fn(b,d,4,0x1129e,ctypes.byref(c))==-expected;guards+=1
 for pc in (0x112c8,0x1130c,0x11376,0x11388,0x113a2,0x28488,0x2849c,0x284a2):assert pc in coverage,('missing ROM path',hex(pc))
 machine.cleanup()
report={'status':'PASS','cases':sum(counts.values()),'counts':{hex(k):v for k,v in counts.items()},'retained_guards':guards,'rom_sha1':hashlib.sha1(rom).hexdigest(),'sources_sha256':manifest,'compared':['RAM8KB','D0-D7','1129E carry','RNG'],'intercepts':[],'coverage':[hex(pc)for pc in sorted(coverage)],'limits':['Only typed consumed actor links resolve through24-bit bus; stored LONG untouched','Resolver modes2/6 and high-bit alias82, both sides, Y015F/0160/0161 and route2851E Y013F/0140/0141 with contact-X boundaries','Temporary stack near1D0000 is outside gameplay RAM8KB; A/otherCCR outside typed ABI']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',sum(counts.values()),'direct shared helper ROM calls and',guards,'retained guards')
