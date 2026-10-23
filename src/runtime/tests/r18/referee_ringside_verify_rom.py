#!/usr/bin/env python3
"""Referee RINGSIDE owner from a literal frame1343 pre-$1F914 observation.
The ROM executes its complete primary/action chain through boundary $1F934.
All movement, count display, result, bonus, text and sound selectors are real.
"""
import argparse,ctypes,hashlib,json,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--report',type=Path);a=p.parse_args()
here=Path(__file__).resolve().parent;root=here.parents[1];rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();snap=(here/'referee_ringside_snapshot_f88acad.bin').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2';assert hashlib.sha256(snap).hexdigest()=='2acb956695715e3add046dee477cb3450726800cd667541112498d58631e25e5'
rng=random.Random(0x1fd5e);original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snap[56:8248];regs=struct.unpack_from('=8I',snap,24)
def w(m,p,v,n=2):m[p:p+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def add(label,changes):
 m=bytearray(original)
 for at,v,n in changes:w(m,at,v,n)
 cases.append((label,m,tuple(rng.getrandbits(32) for _ in range(8))))
r=0x1c11f4;cases=[('natural-noinput-frame1343-prefull1f914',original,regs)]
for d in (-41,-40,-39,-32,-31,-1,0,1,31,32,39,40,41):
 for timer in (1,2,0x101):
  add(f'derived-midpoint{d}-wordtimer{timer}',[(0x1c05b6,800,2),(0x1c0af2,1000,2),(r+6,900+d,2),(r+0x1c,0x8003,2),(r+0x20,3,2),(r+0x22,timer,2),(r+0x24,0,2),(0x1c169a,0,2)])
for count in (0,16,17,19,20):
 for phase in (0,78,79):
  for outside in (0,1,2,3):
   changes=[(r+0x1c,0x8003,2),(r+0x20,3,2),(r+0x22,1,2),(r+0x24,phase,2),(0x1c169a,count,2),(0x1c05e3,1|(4 if outside&1 else 0),1),(0x1c0b1f,129|(4 if outside&2 else 0),1)]
   add(f'derived-count{count}-phase{phase}-outside{outside}',changes)
if a.quick:cases=cases[:1]
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();extra=['src/wf_r18_referee.c','src/wf_r18_match_result.c','src/wf_r18_video.c','tests/r18/referee_ringside_oracle_bridge.c'];results=[];coverage=set()
with tempfile.TemporaryDirectory() as tmp:
 lib=Path(tmp)/'ref.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*dict.fromkeys(core+extra),'-o',str(lib)],cwd=root,check=True);native=ctypes.CDLL(str(lib));native.referee_ringside_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 for label,m,regs in cases:
  nb=ctypes.create_string_buffer(bytes(m));nr=(ctypes.c_uint32*8)(*regs);status=native.referee_ringside_oracle(nb,nr)
  machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,bytes(m));cpu.w_sr(0x2700)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1f914)
  for steps in range(100000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1f934:break
   cpu.execute(1)
  assert cpu.r_pc()==0x1f934,(label,'ROM runaway',hex(cpu.r_pc()))
  diffs=[]
  for at,n in [(0x1c0000,0x2000),(0xc0000,0x4000)]:
   diffs.extend({'address':hex(at+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(nb.raw[at:at+n],mem.r_block(at,n))) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(cpu.r_reg(i))} for i in range(8) if nr[i]!=cpu.r_reg(i)]
  results.append({'label':label,'status':status,'memory_differences':diffs,'register_differences':regdiff});machine.cleanup()
failed=[r for r in results if r['status'] or r['memory_differences'] or r['register_differences']];report={'rom_sha1':hashlib.sha1(rom).hexdigest(),'fixture_sha256':hashlib.sha256(snap).hexdigest(),'boundary':'1F934 before render owners','count':len(results),'passed':len(results)-len(failed),'natural_count':1,'rom_pc_coverage':[hex(pc) for pc in sorted(coverage)],'results':results}
if a.report:a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
print('Referee RINGSIDE ROM',report['passed'],'/',report['count'])
for r in failed[:5]:print(r['label'],'status',r['status'],'memory',r['memory_differences'][:8],'regs',r['register_differences'])
raise SystemExit(bool(failed))
