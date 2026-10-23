#!/usr/bin/env python3
"""CPU state1 target-ringside choice vs untouched ROM.

Only wf_cpu_runtime.c is taken from the working tree. Every other native
source is extracted from immutable f88acad. Natural frame1328 slot5 is
preserved verbatim; derived boundaries are labeled. Checks include all nine
actors, globals, RNG, D0-D7 and direction scratch. The $1C8F0 {30,70} choice
must either set B6.bit4/state0 or B7.bit6/state5/selector3D, preserving the
other flags and the ROM's exact RNG register inputs. All four FLAGS33.bit0
combinations are covered, including partner retargets, pose offsets and the
B6.bit5/global1697 effects before the existing $1CA82 continuation.
"""
import argparse,collections,ctypes,hashlib,io,json,random,re,struct,subprocess,tarfile,tempfile
from pathlib import Path
import machine68k as mus

p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--cpu-source',type=Path);p.add_argument('--source-root',type=Path);p.add_argument('--natural-only',action='store_true')
args=p.parse_args();here=Path(__file__).resolve().parent;root=here.parents[1];repo=root.parents[1]
manifest=json.loads((here/'cpu_target_ringside_manifest.json').read_text());base=manifest['base_commit']
snapshot=(here/'cpu_target_ringside_snapshot_f88acad.bin').read_bytes()
assert hashlib.sha256(snapshot).hexdigest()==manifest['snapshot_sha256']
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
frame,slot,kind,pc,_,_=struct.unpack_from('=6I',snapshot)
assert (frame,slot,kind,pc)==(1328,5,0,0x1c8f4)
input_regs=struct.unpack_from('=8I',snapshot,24);original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snapshot[56:8248]
a=0x1c05b0+slot*0x10c;t=int.from_bytes(original[a+0x7a:a+0x7e],'big');partner=int.from_bytes(original[a+0x86:a+0x8a],'big')
rng=random.Random(0x1c9b2);cases=[]
def w(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def add(label,changes=(),natural=False):
 mem=bytearray(original)
 if not natural:w(mem,0x1c005c,rng.getrandbits(32),4)
 for at,v,n in changes:w(mem,at,v,n)
 regs=input_regs if natural else tuple(rng.getrandbits(32) for _ in range(8))
 cases.append((label,mem,regs,natural))
add('actual-noinput-frame1328-slot5',natural=True)
for b6 in (0,1,2,16,32,64,128,255):
 for b7 in (0,1,2,4,8,16,64,128,255):
  for repeat in range(8):add(f'weighted-preserve-{b6}-{b7}-{repeat}',[(a+0xb6,b6,1),(a+0xb7,b7,1)])
for own_flags in (1,3,9,17,33,65,129,251):
 for target_flags in (5,7,13,21,37,69,133,255):
  for b5 in (0,8,64,128,255):add(f'flags-{own_flags}-{target_flags}-{b5}',[(a+0x33,own_flags,1),(t+0x33,target_flags,1),(a+0xb5,b5,1)])

retarget=int.from_bytes(original[t+0x86:t+0x8a],'big')
for own_inside,target_inside in ((0,0),(0,1),(1,0)):
 for own_flags in (0,2,8,16,32,64,128):
  for pose,sub,sel in ((1,0,0),(4,8,0),(4,9,0),(5,0,0x22),(5,0,0x52)):
   for new_flags in (0,64,128,192):
    ax=0x200;ay=0x200
    changes=[(a+0x33,own_flags|own_inside,1),(t+0x33,4|target_inside,1),(a+6,ax,2),(a+10,ay,2),
             (a+0xb4,2,1),(a+0xb5,0,1),(a+0xb6,32,1),(a+0xb7,128,1),(a+0xb0,256,2),(a+0xb2,32,2),
             (retarget+6,ax+256,2),(retarget+10,ay+32,2),(retarget+0x20,pose,2),(retarget+0x64,sub,2),
             (retarget+0x60,sel,2),(retarget+0x32,0,1),(retarget+0x33,new_flags,1),(retarget+0x34,0,1)]
    add(f'retarget-{own_inside}-{target_inside}-{own_flags}-{pose}-{sub}-{sel}-{new_flags}',changes)

if args.natural_only:cases=cases[:1]
source_root=args.source_root or root
cpu_source=(args.cpu_source or source_root/'src/wf_cpu_runtime.c').read_bytes()
support_sources={}
results=[];coverage=collections.Counter();continuations=collections.Counter()
with tempfile.TemporaryDirectory() as temp:
 frozen=Path(temp);archive=subprocess.run(['git','archive',base,'src/runtime'],cwd=repo,stdout=subprocess.PIPE,check=True).stdout
 with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(frozen,filter='data')
 fr=frozen/'src/runtime';(fr/'src/wf_cpu_runtime.c').write_bytes(cpu_source)
 for name,content in support_sources.items():(fr/name).write_bytes(content)
 (fr/'tests/r18/cpu_target_ringside_oracle_bridge.c').write_bytes((here/'cpu_target_ringside_oracle_bridge.c').read_bytes())
 core=re.search(r'^CORE=(.*)$',(fr/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_cpu_runtime.c')
 lib=frozen/'cpu-target-ringside.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_target_ringside_oracle_bridge.c','-o',str(lib)],cwd=fr,check=True)
 unit=frozen/'test-state5'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/test_state5.c',str(lib),'-o',str(unit)],cwd=fr,check=True)
 unit_run=subprocess.run([str(unit)],cwd=fr,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 assert unit_run.returncode==0,unit_run.stdout
 native=ctypes.CDLL(str(lib));native.cpu_target_ringside_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for index,(label,mem,regs,natural) in enumerate(cases):
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.cpu_target_ringside_oracle(nb,nr,slot)
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for page in range(0,0x80000,0x10000):
   for width in range(3):
    m.set_special_range_write_func(page,width,lambda address,value:None)
    m.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for reg in range(8,15):u.w_reg(reg,0)
  for reg,value in enumerate(regs):u.w_reg(reg,value)
  entry=0x1c8f4
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
   if at in (0x1c9b2,0x1c9d6,0x1ca6e,0x24cc,0x1c9ec,0x1ca14,0x1ca08):continuations[hex(at)]+=1
  differences=[]
  for address,n in [(0x1c05b0,9*0x10c),(0x1c005c,4),(0x1c15e4,8),(0x1c1670,4),(0x1c167a,1),(0x1c1682,14),(0x1c1692,2),(0x1c1697,1),(0x1c1800,4)]:
   got=nb.raw[address:address+n];want=bytes(m.r_block(address,n))
   differences.extend({'address':hex(address+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(got,want)) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  results.append({'index':index,'label':label,'natural':natural,'entry':hex(entry),'native_status':status,'memory_differences':differences,'register_differences':regdiff})
  machine.cleanup()
required=[0x1c9b2,0x1c9d6,0x24cc]+([] if args.natural_only else [0x1ca6e,0x24e4,0x1c9ec,0x1ca14,0x1ca08])
report={'base_commit':base,'snapshot_sha256':manifest['snapshot_sha256'],'cpu_source_sha256':hashlib.sha256(cpu_source).hexdigest(),
        'support_source_sha256':{name:hashlib.sha256(content).hexdigest() for name,content in support_sources.items()},
        'state5_unit_output':unit_run.stdout.strip(),
        'rom_sha1':manifest['rom_sha1'],'count':len(results),'passed':sum(not r['native_status'] and not r['memory_differences'] and not r['register_differences'] for r in results),
        'natural_count':sum(r['natural'] for r in results),'continuations':dict(continuations),'missing_required_rom_pcs':[hex(at) for at in required if not coverage[at]],
        'rom_pc_coverage':{hex(at):count for at,count in sorted(coverage.items())},'results':results}
if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print('CPU target ringside ROM:',report['passed'],'/',report['count'],'natural:',report['natural_count'],'continuations:',dict(continuations),'missing ROM PCs:',report['missing_required_rom_pcs'])
failures=[r for r in results if r['native_status'] or r['memory_differences'] or r['register_differences']]
for r in failures[:12]:print(r['label'],'blocked',hex(-r['native_status']) if r['native_status'] else None,'memory',r['memory_differences'][:5],'registers',r['register_differences'][:3])
raise SystemExit(0 if not failures and not report['missing_required_rom_pcs'] else 1)
