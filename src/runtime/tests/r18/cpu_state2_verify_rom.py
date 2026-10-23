#!/usr/bin/env python3
"""CPU2/3 vs untouched ROM, with an immutable real a4be15f input fixture.

Only wf_cpu_runtime.c, its public header and wf_state5_exec.c are taken from
the working tree. Other native dependencies are extracted from the manifest's
Git commit, so parallel edits to action owners cannot alter this test. Derived boundary fixtures are not
presented as natural gameplay. Mutations caught: missing state2 dispatch,
wrong threshold/branch, missing BYTE/WORD writes, RNG/table/register drift,
incorrect nine-actor scan, and missing state3 continuation.
"""
import argparse,collections,ctypes,hashlib,io,json,random,re,struct,subprocess,tarfile,tempfile
from pathlib import Path
import machine68k as mus

p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--cpu-source',type=Path);p.add_argument('--source-root',type=Path);p.add_argument('--baseline-dependencies',action='store_true')
args=p.parse_args();here=Path(__file__).resolve().parent;root=here.parents[1];repo=root.parents[1]
manifest=json.loads((here/'cpu_state2_manifest.json').read_text());base=manifest['base_commit']
snapshot=(here/'cpu_state2_snapshot_a4be15f.bin').read_bytes()
assert hashlib.sha256(snapshot).hexdigest()==manifest['snapshot_sha256']
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
frame,slot,kind,pc,_,_=struct.unpack_from('=6I',snapshot)
assert (frame,slot,kind,pc)==(669,1,0,0x1d7b6)
input_regs=struct.unpack_from('=8I',snapshot,24);original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snapshot[56:8248]
a=0x1c05b0+slot*0x10c;t=int.from_bytes(original[a+0x7a:a+0x7e],'big');partner=int.from_bytes(original[a+0x86:a+0x8a],'big')
rng=random.Random(0x1d7b6);cases=[]
def w(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def add(label,changes=(),natural=False):
 mem=bytearray(original)
 if not natural:
  for at,v,n in [(a+0x20,0x8002,2),(a+0x45,1,1),(a+0x32,0,1),(a+0x33,0,1),(a+0x34,0,1),
                 (a+0xb5,0,1),(a+0xb7,0,1),(a+0xb0,100,2),(a+0xb2,0,2),(a+0xbc,80,2),
                 (t+0x20,0x8001,2),(t+0x32,0,1),(t+0x33,0,1),(t+0x34,0,1),(t+0x35,0,1),
                 (t+0x2e,original[a+0x2e]^128,1),(0x1c0161,0,1),(0x1c007c,1,2),(0x1c0162,0,2)]:w(mem,at,v,n)
  w(mem,0x1c005c,rng.getrandbits(32),4)
 for at,v,n in changes:w(mem,at,v,n)
 regs=input_regs if natural else tuple(rng.getrandbits(32) for _ in range(8))
 cases.append((label,mem,regs,natural))
add('actual-noinput-frame669-slot1',natural=True)
for state in (2,3,0x8003):
 for b7 in (0,1,3,255):
  for magnitude in (0,1,2,3,0xffff):add(f'entry-{state:x}-{b7}-{magnitude}',[(a+0x20,state,2),(a+0xb7,b7,1),(a+0x2a,magnitude,2)])
for wait in (0,1,2,3,255):
 for mode in (0,1,2,3):
  for route in (0,1):
   for distance in (0,71,72,79,80,159,160,191,192,0xffff):
    add(f'wait-mode-route-distance-{wait}-{mode}-{route}-{distance}',[(a+0x45,wait,1),(0x1c0161,mode,1),(0x1c007c,route,2),(a+0xb0,distance,2),(a+0xb7,3,1)])
for dy in (7,8,9,10,0xffff):
 for distance in (71,72,79,80,159,160):
  for equal in (False,True):
   for latch in (0,2):add(f'facing-distance-latch-{dy}-{distance}-{equal}-{latch}',[(a+0xb2,dy,2),(a+0xb0,distance,2),(t+0x2e,original[a+0x2e] if equal else original[a+0x2e]^128,1),(a+0xb7,latch,1)])
for table in ('normal','guard-flag','guard-selector','royal'):
 for ident in range(12):
  for stage in (0,1,5,6,9):
   for tier in range(3):
    for repetition in range(2):
     changes=[(a+0x57,ident,1),(0x1c0162,stage,2),(a+0x70,tier,2),(a+0xb0,72 if repetition else 79,2)]
     if table=='guard-flag':changes.append((t+0x33,16,1))
     if table=='guard-selector':changes.extend([(t+0x20,0x8005,2),(t+0x60,3,2)])
     if table=='royal':changes.append((0x1c0161,1,1))
     add(f'weights-{table}-{ident}-{stage}-{tier}-{repetition}',changes)
for tf32 in (0,1,2):
 for tf33 in (0,64):
  for ts in (0,1,4,5):
   for selector in (0,0x48,0x1a,0x23,0x61):
    add(f'zero-wait-target-{tf32}-{tf33}-{ts}-{selector}',[(a+0x45,0,1),(a+0xb7,2,1),(a+0xb5,8,1),(a+0xbc,0,2),(t+0x32,tf32,1),(t+0x33,tf33,1),(t+0x20,ts,2),(t+0x60,selector,2),(t+0x64,8,2)])
for subtype in (0,1,5,8,9,10):
 for b5 in (0,8):
  for b7 in (0,2):
   for defender in (0,128):
    add(f'zero-wait-decision-{subtype}-{b5}-{b7}-{defender}',[(a+0x45,0,1),(a+0xb5,b5,1),(a+0xb7,b7,1),(a+0xb0,80,2),(a+0xbc,90,2),(t+0x20,4,2),(t+0x64,subtype,2),(t+0xb5,defender,1)])
for candidate in (-1,0,1,4,5,8):
 for distance in (0,127,128,0xffff):
  for own_partner_flags in ((0,0),(2,0),(2,2)):
   changes=[(a+0x45,0,1),(a+0x34,4,1),(a+0x33,own_partner_flags[0],1),(partner+0x33,own_partner_flags[1],1)]
   changes.extend((0x1c05b0+i*0x10c+0x35,0,1) for i in range(9))
   if candidate>=0:
    c=0x1c05b0+candidate*0x10c;changes.extend([(c,0x80,1),(c+0x35,1 if candidate%2 else 2,1),(c+6,(int.from_bytes(original[a+6:a+8],'big')+distance)&65535,2)])
   add(f'pin-scan-{candidate}-{distance}-{own_partner_flags}',changes)
for flags in (4,0x84):add(f'zero-wait-own-flag32-{flags}',[(a+0x45,0,1),(a+0x32,flags,1)])

source_root=args.source_root or root
cpu_source=(args.cpu_source or source_root/'src/wf_cpu_runtime.c').read_bytes()
support_sources={} if args.baseline_dependencies else {name:(source_root/name).read_bytes() for name in ('include/wf_cpu_runtime.h','src/wf_state5_exec.c')}
results=[];coverage=collections.Counter();continuations=collections.Counter()
with tempfile.TemporaryDirectory() as temp:
 frozen=Path(temp);archive=subprocess.run(['git','archive',base,'src/runtime'],cwd=repo,stdout=subprocess.PIPE,check=True).stdout
 with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(frozen,filter='data')
 fr=frozen/'src/runtime';(fr/'src/wf_cpu_runtime.c').write_bytes(cpu_source)
 for name,content in support_sources.items():(fr/name).write_bytes(content)
 (fr/'tests/r18/cpu_state2_oracle_bridge.c').write_bytes((here/'cpu_state2_oracle_bridge.c').read_bytes())
 core=re.search(r'^CORE=(.*)$',(fr/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_cpu_runtime.c')
 lib=frozen/'cpu-state2.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state2_oracle_bridge.c','-o',str(lib)],cwd=fr,check=True)
 unit=frozen/'test-state5'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/test_state5.c',str(lib),'-o',str(unit)],cwd=fr,check=True)
 unit_run=subprocess.run([str(unit)],cwd=fr,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 assert unit_run.returncode==0,unit_run.stdout
 native=ctypes.CDLL(str(lib));native.cpu_state2_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for index,(label,mem,regs,natural) in enumerate(cases):
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.cpu_state2_oracle(nb,nr,slot)
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for page in range(0,0x80000,0x10000):
   for width in range(3):
    m.set_special_range_write_func(page,width,lambda address,value:None)
    m.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for reg in range(8,15):u.w_reg(reg,0)
  for reg,value in enumerate(regs):u.w_reg(reg,value)
  entry=0x1da66 if mem[a+0x21]==3 else 0x1d7b6
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
   if at in (0x1de66,0x1dfe2,0x24cc,0x1da66):continuations[hex(at)]+=1
  differences=[]
  for address,n in [(0x1c05b0,9*0x10c),(0x1c005c,4),(0x1c15e4,8),(0x1c1670,4),(0x1c167a,1),(0x1c1682,14),(0x1c1692,2),(0x1c1697,1),(0x1c1800,4)]:
   got=nb.raw[address:address+n];want=bytes(m.r_block(address,n))
   differences.extend({'address':hex(address+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(got,want)) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  results.append({'index':index,'label':label,'natural':natural,'entry':hex(entry),'native_status':status,'memory_differences':differences,'register_differences':regdiff})
  machine.cleanup()
required=[0x1d7be,0x1d7e4,0x1d802,0x1d84e,0x1d85a,0x1d87a,0x1d8a2,0x1d8ae,0x1d914,0x1d91c,0x1d966,0x1d9a8,0x1d9f0,0x1da00,0x1da2c,0x1da3e,0x1da4c,0x1da5e,0x1da78]
report={'base_commit':base,'snapshot_sha256':manifest['snapshot_sha256'],'cpu_source_sha256':hashlib.sha256(cpu_source).hexdigest(),
        'support_source_sha256':{name:hashlib.sha256(content).hexdigest() for name,content in support_sources.items()},
        'state5_unit_output':unit_run.stdout.strip(),
        'rom_sha1':manifest['rom_sha1'],'count':len(results),'passed':sum(not r['native_status'] and not r['memory_differences'] and not r['register_differences'] for r in results),
        'natural_count':sum(r['natural'] for r in results),'continuations':dict(continuations),'missing_required_rom_pcs':[hex(at) for at in required if not coverage[at]],
        'rom_pc_coverage':{hex(at):count for at,count in sorted(coverage.items())},'results':results}
if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print('CPU state2/3 ROM:',report['passed'],'/',report['count'],'natural:',report['natural_count'],'continuations:',dict(continuations),'missing ROM PCs:',report['missing_required_rom_pcs'])
failures=[r for r in results if r['native_status'] or r['memory_differences'] or r['register_differences']]
for r in failures[:12]:print(r['label'],'blocked',hex(-r['native_status']) if r['native_status'] else None,'memory',r['memory_differences'][:5],'registers',r['register_differences'][:3])
raise SystemExit(0 if not failures and not report['missing_required_rom_pcs'] else 1)
