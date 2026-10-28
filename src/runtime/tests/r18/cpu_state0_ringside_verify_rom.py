#!/usr/bin/env python3
"""CPU0 RINGSIDE $1C6B6 vs untouched ROM and natural frame1380.

Only wf_cpu_runtime.c comes from the working tree; support is frozen at
f88acad. Derived fixtures cover both fixed objects, unavailable objects with
live caller registers, strict nearest/distance choices, all12 character
weight tables, all10 progressions, count10 climb, RNG and D0-D7.
"""
import argparse,collections,ctypes,hashlib,io,json,random,re,struct,subprocess,tarfile,tempfile
from pathlib import Path
import machine68k as mus

p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--cpu-source',type=Path);p.add_argument('--source-root',type=Path);p.add_argument('--natural-only',action='store_true')
args=p.parse_args();here=Path(__file__).resolve().parent;root=here.parents[1];repo=root.parents[1]
manifest=json.loads((here/'cpu_state0_ringside_manifest.json').read_text());base=manifest['base_commit']
snapshot=(here/'cpu_state0_ringside_snapshot_f88acad.bin').read_bytes()
assert hashlib.sha256(snapshot).hexdigest()==manifest['snapshot_sha256']
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
frame,slot,kind,pc,_,_=struct.unpack_from('=6I',snapshot)
assert (frame,slot,kind,pc)==(1380,1,0,0x1c2cc)
input_regs=struct.unpack_from('=8I',snapshot,24);original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snapshot[56:8248]
a=0x1c05b0+slot*0x10c;t=int.from_bytes(original[a+0x7a:a+0x7e],'big');partner=int.from_bytes(original[a+0x86:a+0x8a],'big')
rng=random.Random(0x1c6b6);cases=[]
def w(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def add(label,changes=(),natural=False):
 mem=bytearray(original)
 if not natural:
  for at,v,n in [(a+0x20,0x8000,2),(a+0x33,0,1),(a+0x34,0,1),(a+0xb5,8,1),(a+0xb6,1,1),(a+0xb7,0,1),(a+0xb9,0,1),
                 (t+0x20,0x8001,2),(t+0x32,0,1),(t+0x33,0,1),(t+0x34,0,1),(0x1c0161,0,1)]:w(mem,at,v,n)
  w(mem,0x1c005c,rng.getrandbits(32),4)
 for at,v,n in changes:w(mem,at,v,n)
 regs=input_regs if natural else tuple(rng.getrandbits(32) for _ in range(8))
 cases.append((label,mem,regs,natural))
add('actual-noinput-frame1380-slot1',natural=True)
# Keep the exact new mode2 prefix; derived cases change only labeled fields.
def derived(label,changes):
 mem=bytearray(original)
 for at,v,n in changes:w(mem,at,v,n)
 w(mem,0x1c005c,rng.getrandbits(32),4)
 cases.append((label,mem,tuple(rng.getrandbits(32) for _ in range(8)),False))
for char in range(12):
 for stage in range(10):
  derived(f'weighted-character-{char}-stage-{stage}',[(a+0x57,char,1),(0x1c0162,stage,2)])
for active0 in (0,128):
 for latch0 in (2,3):
  for busy0 in (0,1):
   for active1 in (0,128):
    for latch1 in (2,3):
     for busy1 in (0,1):
      derived(f'objects-{active0}-{latch0}-{busy0}-{active1}-{latch1}-{busy1}',[(0x1c0f1c,active0,1),(0x1c0f39,latch0,1),(0x1c0f92,busy0,2),(0x1c1028,active1,1),(0x1c1045,latch1,1),(0x1c109e,busy1,2)])
for dx0,dx1 in ((0,0),(10,10),(10,9),(9,10),(0xffff,0),(0,0xffff),(0x8000,0x7fff)):
 for threshold in (0,1,9,10,11,0xffff):
  derived(f'distance-{dx0}-{dx1}-threshold-{threshold}',[(a+6,0,2),(0x1c0f22,dx0,2),(0x1c102e,dx1,2),(a+0xb0,threshold,2)])
for count in (0,9,10,19,20,255):
 for role in (0,32,64,96,128,160):
  for apron in (0,128):
   derived(f'climb-count-{count}-role-{role}-apron-{apron}',[(0x1c169a,count,2),(a+0x33,5,1),(a+0x56,role,1),(a+0x74,apron,1)])

if args.natural_only:cases=cases[:1]
source_root=args.source_root or root
cpu_source=(args.cpu_source or source_root/'src/wf_cpu_runtime.c').read_bytes()
# The current CPU owner calls its production state-5 approach wrapper.
# Freeze that exact dependency and its public contract, not an obsolete base.
support_sources={name:(source_root/name).read_bytes() for name in ('include/wf_cpu_runtime.h','src/wf_state5_exec.c')}
results=[];coverage=collections.Counter();continuations=collections.Counter()
with tempfile.TemporaryDirectory() as temp:
 frozen=Path(temp);archive=subprocess.run(['git','archive',base,'src/runtime'],cwd=repo,stdout=subprocess.PIPE,check=True).stdout
 with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(frozen,filter='data')
 fr=frozen/'src/runtime';(fr/'src/wf_cpu_runtime.c').write_bytes(cpu_source)
 for name,content in support_sources.items():(fr/name).write_bytes(content)
 (fr/'tests/r18/cpu_state0_ringside_oracle_bridge.c').write_bytes((here/'cpu_state0_ringside_oracle_bridge.c').read_bytes())
 core=re.search(r'^CORE=(.*)$',(fr/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_cpu_runtime.c')
 lib=frozen/'cpu-state0-continue.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state0_ringside_oracle_bridge.c','-o',str(lib)],cwd=fr,check=True)
 unit=frozen/'test-state5'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/test_state5.c',str(lib),'-o',str(unit)],cwd=fr,check=True)
 unit_run=subprocess.run([str(unit)],cwd=fr,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 assert unit_run.returncode==0,unit_run.stdout
 native=ctypes.CDLL(str(lib));native.cpu_state0_ringside_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for index,(label,mem,regs,natural) in enumerate(cases):
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.cpu_state0_ringside_oracle(nb,nr,slot)
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for page in range(0,0x80000,0x10000):
   for width in range(3):
    m.set_special_range_write_func(page,width,lambda address,value:None)
    m.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for reg in range(8,15):u.w_reg(reg,0)
  for reg,value in enumerate(regs):u.w_reg(reg,value)
  entry=0x1c2cc
  u.w_reg(8,a);u.w_reg(9,entry);u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(entry)
  visited=set()
  for steps in range(100000):
   at=u.r_pc();visited.add(at)
   if at==0x70000:break
   if at==0x2052:u.w_pc(m.r32(u.r_sp()));u.w_sp(u.r_sp()+4);continue
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',label,hex(u.r_pc()))
  coverage.update(visited)
  for at in visited:
   if at in (0x1c6e6,0x1c7c6,0x1c880,0x1c7ba):continuations[hex(at)]+=1
  differences=[]
  for address,n in [(0x1c05b0,9*0x10c),(0x1c005c,4),(0x1c15e4,8),(0x1c1670,4),(0x1c167a,1),(0x1c1682,14),(0x1c1692,2),(0x1c1697,1),(0x1c1800,4)]:
   got=nb.raw[address:address+n];want=bytes(m.r_block(address,n))
   differences.extend({'address':hex(address+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(got,want)) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  results.append({'index':index,'label':label,'natural':natural,'entry':hex(entry),'native_status':status,'memory_differences':differences,'register_differences':regdiff})
  machine.cleanup()
required=[0x1c6b6,0x1c70c,0x1c7ba,0x1c880]+([] if args.natural_only else [0x1c6e6,0x1c7c6,0x1c762,0x1c79a])
report={'base_commit':base,'snapshot_sha256':manifest['snapshot_sha256'],'cpu_source_sha256':hashlib.sha256(cpu_source).hexdigest(),
        'support_source_sha256':{name:hashlib.sha256(content).hexdigest() for name,content in support_sources.items()},
        'state5_unit_output':unit_run.stdout.strip(),
        'rom_sha1':manifest['rom_sha1'],'count':len(results),'passed':sum(not r['native_status'] and not r['memory_differences'] and not r['register_differences'] for r in results),
        'natural_count':sum(r['natural'] for r in results),'continuations':dict(continuations),'missing_required_rom_pcs':[hex(at) for at in required if not coverage[at]],
        'rom_pc_coverage':{hex(at):count for at,count in sorted(coverage.items())},'results':results}
if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print('CPU state0 RINGSIDE ROM:',report['passed'],'/',report['count'],'natural:',report['natural_count'],'continuations:',dict(continuations),'missing ROM PCs:',report['missing_required_rom_pcs'])
failures=[r for r in results if r['native_status'] or r['memory_differences'] or r['register_differences']]
for r in failures[:12]:print(r['label'],'blocked',hex(-r['native_status']) if r['native_status'] else None,'memory',r['memory_differences'][:5],'registers',r['register_differences'][:3])
raise SystemExit(0 if not failures and not report['missing_required_rom_pcs'] else 1)
