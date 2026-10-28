"""Independent real backend2B58 callback comparison, raw7A retirement seam."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
make=(ROOT/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
rng=random.Random(0x2b58);count=0
with tempfile.TemporaryDirectory(prefix='wf-7a-palette-',ignore_cleanup_errors=os.name=='nt')as tmp:
 f=Path(tmp)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,f/folder)
 bridge='tests/r18/action7a_palette_oracle_bridge.c';(f/'tests/r18').mkdir(parents=True);shutil.copy2(ROOT/bridge,f/bridge)
 manifest={str(x.relative_to(f)):hashlib.sha256(x.read_bytes()).hexdigest()for x in f.rglob('*')if x.is_file()}
 lib=f/'palette.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,bridge,'-o',str(lib)],cwd=f,check=True)
 native=ctypes.CDLL(str(lib));native.action7a_palette_oracle.argtypes=[ctypes.c_void_p,ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
 for actor_id,value in itertools.product((*range(12),15,128,255),range(256)):
  initial=bytearray(rng.randbytes(0x2000));initial[0x1610+actor_id]=value;mem.w_block(0x1c0000,bytes(initial));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x2b58)
  regs=[rng.getrandbits(32)for _ in range(8)];regs[0]=(regs[0]&0xffff0000)|actor_id
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(100):
   if cpu.r_pc()==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000 and[cpu.r_reg(i)for i in range(8)]==regs
  m=bytearray(0x200000);m[0x1c0000:0x1c2000]=initial;b=ctypes.create_string_buffer(bytes(m));assert native.action7a_palette_oracle(b,actor_id)==0
  got=b.raw[0x1c0000:0x1c2000];want=bytes(mem.r_block(0x1c0000,0x2000));assert got==want,(actor_id,value,[(hex(0x1c0000+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:10]);count+=1
 machine.cleanup()
report={'status':'PASS','cases':count,'sources_sha256':manifest,'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','original D0-D7 preservation'],'intercepts':[],'limits':['Backend callback ids0..255 supported; typed character tables in gameplay remain bounded to0..11','Callback writes final allocator state; no asynchronous observer between independent byte writes','No physicalSaturn timing equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'real backend2B58 callback versus intact ROM')
