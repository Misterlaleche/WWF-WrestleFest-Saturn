"""Exact6CFA/6D76 glyph bus writes and failed-prefix D/A against intact ROM."""
from pathlib import Path
import argparse,ctypes as ct,hashlib,itertools,json,os,random,shutil,subprocess,tempfile
import machine68k as mus
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--seed',type=lambda x:int(x,0),default=0x6cfa);p.add_argument('--smoke',action='store_true');args=p.parse_args()
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(args.seed);coverage=set();counts={};prefix_count=0
sources=['src/wf_r18_ranking_glyph.c','src/wf_r17_m68k_shadow.c','tests/r18/ranking_glyph_oracle_bridge.c']
writes={0x6d04:(1,1,2),0x6d12:(0,4,None),0x6d1a:(4,4,None),0x6d28:(0,4,None),0x6d30:(4,4,None),0x6d76:(1,1,3),0x6d7c:(3,1,3)}
with tempfile.TemporaryDirectory(prefix='wf-rank-glyph-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 shutil.copytree(root/'include',frozen/'include')
 for name in sources:
  path=frozen/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(root/name,path)
 hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 lib=frozen/'glyph.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'-o',str(lib)],cwd=frozen,check=True)
 c=ct.CDLL(str(lib));c.ranking_glyph_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32),ct.c_uint,ct.c_int,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32)]
 assert c.ranking_glyph_bad_context()==0
 assert c.ranking_glyph_missing_dependencies()==0
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 def check(owner,mode,glyph,base,fail=-1):
  global prefix_count
  raw=bytearray(0x200000);raw[:len(rom)]=rom;raw[0xc0000:0xc4000]=rng.randbytes(0x4000);raw[0x1c0000:0x1c4000]=rng.randbytes(0x4000)
  dr=[rng.getrandbits(32)for _ in range(8)];ar=[rng.getrandbits(32)for _ in range(7)]
  dr[0]=(dr[0]&0xffff0000)|mode;dr[2]=(dr[2]&0xffff0000)|glyph;ar[2]=base
  mem.w_block(0,bytes(raw));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(owner)
  for i,v in enumerate(dr+ar):cpu.w_reg(i,v)
  events=[];failed_pc=failed_at=0
  for _ in range(200):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   if pc in writes:
    offset,width,reg=writes[pc];at=(cpu.r_reg(10)+offset)&0xffffff;value=0 if reg is None else cpu.r_reg(reg)&((1<<(width*8))-1)
    events.append((at,value,width))
    if len(events)-1==fail:failed_pc,failed_at=pc,at;break
   cpu.execute(1)
  else:raise AssertionError(('ROM runaway',hex(cpu.r_pc())))
  assert cpu.r_pc()==0x70000 or failed_pc,(hex(cpu.r_pc()),owner,mode,glyph)
  nb=ct.create_string_buffer(bytes(raw));nd=(ct.c_uint32*8)(*dr);na=(ct.c_uint32*7)(*ar);diag=(ct.c_uint32*4)();ev=(ct.c_uint32*48)()
  status=c.ranking_glyph_oracle(nb,nd,na,owner,fail,diag,ev)
  assert status==(1 if failed_pc else 0),(hex(owner),mode,glyph,fail,status,list(diag))
  assert list(diag)==[failed_pc,failed_at,len(events),failed_pc],(hex(owner),mode,glyph,fail,'diag',list(diag),failed_pc,failed_at,len(events))
  assert [tuple(ev[i*3:i*3+3])for i in range(diag[2])]==events,('write order/width/value',mode,glyph,fail)
  for at,n in ((0xc0000,0x4000),(0x1c0000,0x4000)):
   want=bytes(mem.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,(hex(owner),mode,glyph,fail,'memory',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:8])
  assert list(nd)==[cpu.r_reg(i)for i in range(8)],(hex(owner),mode,glyph,fail,'D',list(nd),[cpu.r_reg(i)for i in range(8)])
  assert list(na)==[cpu.r_reg(i)for i in range(8,15)],(hex(owner),mode,glyph,fail,'A')
  counts[hex(owner)]=counts.get(hex(owner),0)+1
  if failed_pc:prefix_count+=1
 modes=(0,1,2,3,4,0x7fff,0x8000,0xffff);glyphs=(0,0x30,0x31,0x32,0x4a,0x4b,0x4c,0x7fff,0x8000,0xffff)
 for mode,glyph,tag in itertools.product(modes,glyphs,(0,0x40000000,0xff000000)):
  check(0x6cfa,mode,glyph,tag|(0xc0000+2*rng.randrange(0x1e00)))
  if args.smoke:break
 if not args.smoke:
  for _ in range(1024):check(0x6cfa,rng.randrange(65536),rng.randrange(65536),0xc0000+2*rng.randrange(0x1e00))
  for _ in range(512):check(0x6d76,rng.randrange(65536),rng.randrange(65536),rng.choice((0,0xff000000))|(0xc0000+rng.randrange(0x3ffc)))
  for mode,glyph,amount in ((0,0x31,8),(2,0xffff,8),(0,0x4b,4),(3,0x4b,1),(0xffff,0,1)):
   for fail,tag in itertools.product(range(amount),(0,0xff000000)):check(0x6cfa,mode,glyph,tag|0xc0120,fail)
  for fail in range(2):check(0x6d76,0,0,0xff0c0120,fail)
 machine.cleanup()
 report={'status':'PASS','owners':counts,'failed_bus_prefixes':prefix_count,'bad_context_guards':5,'missing_dependency_guards':2,'rom_sha1':hashlib.sha1(rom).hexdigest(),'rom_owner_sha256':hashlib.sha256(rom[0x6cfa:0x6d82]).hexdigest(),'seed':args.seed,'coverage':sorted(coverage),'sources_sha256':hashes,'intercepts':[],'compared':['RAM16KB','FG16KB including padding','D0-D7','A0-A6 including high aliases','ordered bus address/value/width','every failure prefix before corresponding original store'],'limits':['CCR/stack and physical timing excluded','No ROM callback/helper interception']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report[k]for k in ('status','owners','failed_bus_prefixes','bad_context_guards','seed')}))
