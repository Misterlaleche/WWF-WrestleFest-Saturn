"""Royal human target selection: full original20958 RAM effects and loop registers.

Legacy phase has no complete D/A ABI; this oracle explicitly compares RAM16KB
and its retained D5/D6 loop-register contract, not missing scratch-register ABI.
"""
import argparse,ctypes as ct,hashlib,json,os,random,re,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');a=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
sources=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();rng=random.Random(0x20a20);count=0
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
with tempfile.TemporaryDirectory(prefix='wf-royal-target-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'target.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/royal_target_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.royal_target_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32)]
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
 for case in range(1 if a.smoke else 1536):
  mem=bytearray(0x200000);mem[:len(rom)]=rom;put(mem,0x1c0161,1,1)
  active=list(range(1+case%4))+list(range(4,9))
  for slot in active:
   at=0x1c05b0+slot*0x10c;put(mem,at,0x8001);put(mem,at+0x56,(0 if slot<4 else 0x8000)|slot)
   put(mem,at+6,rng.randrange(0x1c0,0x330));put(mem,at+10,rng.randrange(0x170,0x200));put(mem,at+0x33,3 if slot<4 else 1,1)
   put(mem,at+0x7a,0x1c05b0+((slot+1)%9)*0x10c,4)
  # Natural first human starts with no target, as Royal1086C produces.
  put(mem,0x1c062a,0,4)
  if case>=512:
   at=0x1c09e0;put(mem,at+6,0x280);put(mem,at+10,0x180);put(mem,0x1c05b6,0x280);put(mem,0x1c05ba,0x180)
   put(mem,at+0x20,5);put(mem,at+0x60,(0x4a if case<1024 else 0x48)|(0x2000 if case&1 else 0))
   put(mem,at+0x26,0x1c0aec,4);put(mem,at+0x9c,0x1c0bf8,4)
   put(mem,0x1c0aec+0x9c,0 if case%4==0 else 0x1c0d04,4);put(mem,0x1c0aec+0x60,0x2000 if case&2 else 0)
  d=[rng.getrandbits(32)for _ in range(8)];ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x20958)
  for i,v in enumerate(d):cpu.w_reg(i,v)
  for _ in range(100000):
   if cpu.r_pc()==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,('ROM run',case,hex(cpu.r_pc()))
  nb=ct.create_string_buffer(bytes(mem));nd=(ct.c_uint32*8)(*d);assert native.royal_target_oracle(nb,nd)==0
  got=nb.raw[0x1c0000:0x1c4000];want=bytes(ram.r_block(0x1c0000,0x4000))
  assert got==want,(case,[(hex(0x1c0000+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(nd)[5:7]==[cpu.r_reg(i)for i in(5,6)],('retained D5/D6',case,list(nd)[5:7],[cpu.r_reg(i)for i in(5,6)])
  count+=1
 m.cleanup();report={'status':'PASS','calls':count,'intercepts':[],'compared':['RAM16KB','legacy retained D5/D6'],'limits':'Legacy phase scratch D/A ABI is outside this regression; full-ROM RAM effects are compared.'}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
