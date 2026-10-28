"""Whole original movement and card-strip payload, no intercepted ROM calls."""
import argparse,ctypes as ct,hashlib,json,os,random,re,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
make=(root/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
f='src/wf_r18_match_aux.c'
if f not in sources:sources.append(f)
rng=random.Random(0x2208b29e);counts={}
with tempfile.TemporaryDirectory(prefix='wf-match-aux-',ignore_cleanup_errors=os.name=='nt')as t:
 lib=Path(t)/'aux.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/match_aux_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ct.CDLL(str(lib));native.match_aux_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32),ct.c_uint]
 assert native.match_aux_clear_guards()==0,'B2E4 rejects invalid context before changing video'
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=m.cpu,m.mem
 cases=[(0x2208,i)for i in range(3 if args.smoke else 4352)]+[(0xb29e,i)for i in range(1 if args.smoke else 64)]+[(0xb2e4,i)for i in range(1 if args.smoke else 128)]
 for owner,case in cases:
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c4000]=rng.randbytes(0x4000);mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
  d=[rng.getrandbits(32)for _ in range(8)];a=[rng.getrandbits(32)for _ in range(7)]
  if owner==0x2208:
   a[0]=0x1c14ce if case&1 else 0x1c05b0;at=a[0];mem[at+1]=case%3 if args.smoke else(1 if case<4096 else case%3)
   mem[at+0x2d]=case&255;mem[at+0x2b]=(case//256)*17&255
  ram.w_block(0,bytes(mem));cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
  for i,v in enumerate(d+a):cpu.w_reg(i,v)
  for _ in range(15000):
   if cpu.r_pc()==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(hex(owner),case,hex(cpu.r_pc()))
  nb=ct.create_string_buffer(bytes(mem));nd=(ct.c_uint32*8)(*d);na=(ct.c_uint32*7)(*a)
  assert native.match_aux_oracle(nb,nd,na,owner)==0,(hex(owner),case,'native')
  for at,n in((0x1c0000,0x4000),(0xc0000,0x4000)):
   want=bytes(ram.r_block(at,n));got=nb.raw[at:at+n];assert got==want,(hex(owner),case,[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:8])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(hex(owner),case,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  assert list(na)==[cpu.r_reg(i)for i in range(8,15)],(hex(owner),case,'A')
  counts[hex(owner)]=counts.get(hex(owner),0)+1
 m.cleanup();report={'status':'PASS','owners':counts,'guards':4,'intercepts':[],'compared':['full RAM16KB','FG16KB','D0-D7/A0-A6'],'sources_sha256':{s:hashlib.sha256((root/s).read_bytes()).hexdigest()for s in sources}}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
