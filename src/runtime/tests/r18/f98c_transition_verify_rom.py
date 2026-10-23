#!/usr/bin/env python3
"""Untouched-ROM evidence for F98C core through the $26E66 boundary and RAM reset.
Hardware map/palette work remains an explicit blocked dependency in this test.
Natural input is preserved verbatim; all boundary fixtures are labeled derived.
"""
import argparse,ctypes,hashlib,io,json,random,re,struct,subprocess,tarfile,tempfile
from pathlib import Path
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--deps-source',type=Path);p.add_argument('--quick',action='store_true');args=p.parse_args()
here=Path(__file__).resolve().parent;root=here.parents[1];repo=root.parents[1];manifest=json.loads((here/'f98c_transition_manifest.json').read_text())
snapshot=(here/'f98c_transition_reference_f88acad.bin').read_bytes();assert hashlib.sha256(snapshot).hexdigest()==manifest['reference_sha256']
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snapshot[56:8248]
rng=random.Random(0xf98c);cases=[('actual-noinput-frame1343-f98c',0,bytearray(original),struct.unpack_from('=8I',snapshot,24))]
def w(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(n*8))-1)).to_bytes(n,'big')
def add(label,op,changes):
 mem=bytearray(original);mem[0x1c16d2:0x1c19ae]=rng.randbytes(0x2dc);mem[0x1c19b2:0x1c1cd2]=rng.randbytes(0x320)
 for at,v,n in changes:w(mem,at,v,n)
 regs=tuple(rng.getrandbits(32) for _ in range(8));cases.append((label,op,mem,regs))
for i in range(16):add('derived-reset-'+str(i),1,[])
for stage in range(10):
 for royal in (0,1):
  for facing in (0,128):
   for actor_facing in (0,128):
    for ident in (0,4,8):
     a=0x1c05b0+ident*0x10c
     changes=[(0x1c0162,stage,2),(0x1c007e,0,2),(0x1c1678,128|(64 if royal else 0),1),(0x1c1679,facing,1),
              (a,128,1),(a+0x33,5,1),(a+0x60,0x6a,2),(a+0x2e,actor_facing,1)]
     add(f'derived-stage-body-facing-{stage}-{royal}-{facing}-{actor_facing}-{ident}',0,changes)
for hi in (0,1,0x7f,0xff):
 for body in (0,64):add(f'derived-scene-high-byte-{hi}-{body}',0,[(0x1c007e,hi<<8,2),(0x1c0162,1,2),(0x1c1678,128|body,1),(0x1c1679,255,1)])
if args.quick:cases=cases[:1]+cases[16:20]
source=(args.deps_source or root/'src/wf_r17_dependencies.c').read_bytes();results=[];coverage=set()
with tempfile.TemporaryDirectory() as temp:
 frozen=Path(temp);archive=subprocess.run(['git','archive',manifest['base_commit'],'src/runtime'],cwd=repo,stdout=subprocess.PIPE,check=True).stdout
 with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(frozen,filter='data')
 fr=frozen/'src/runtime';(fr/'src/wf_r17_dependencies.c').write_bytes(source);(fr/'include/wf_r17_dependencies.h').write_bytes((root/'include/wf_r17_dependencies.h').read_bytes());(fr/'tests/r18/f98c_transition_oracle_bridge.c').write_bytes((here/'f98c_transition_oracle_bridge.c').read_bytes())
 core=re.search(r'^CORE=(.*)$',(fr/'Makefile').read_text(),re.M)[1].split();lib=frozen/'f98c.so';flags=['-DHAVE_TRANSITION_RESET_RAM'] if b'wf_r17_transition_reset_ram(' in source else []
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*flags,*core,'tests/r18/f98c_transition_oracle_bridge.c','-o',str(lib)],cwd=fr,check=True)
 native=ctypes.CDLL(str(lib));native.f98c_transition_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for label,op,mem,regs in cases:
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.f98c_transition_oracle(nb,nr,op)
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for i,value in enumerate(regs):u.w_reg(i,value)
  u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(0x1f6c if op else 0xf98c);reset_phase=op
  for steps in range(100000):
   at=u.r_pc();coverage.add(at)
   if not op and at==0x26e66:break
   if at==0x70000:
    if reset_phase==1:reset_phase=2;u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(0x1fde);continue
    break
   if at in (0xfb5a,0xfc80):u.w_pc(0xfb64 if at==0xfb5a else 0xfc8a);continue
   u.execute(1)
  expected_pc=0x70000 if op else 0x26e66;assert u.r_pc()==expected_pc,(label,hex(u.r_pc()))
  differences=[]
  for address,n in [(1835134, 2), (1835135, 1), (1835126, 1), (1835132, 2), (1835361, 1), (1835362, 2), (1840752, 4), (1840756, 4), (1840760, 1), (1840761, 1), (1840628, 2), (1840632, 2), (1840850, 2), (1840852, 2), (1840870, 1), (1841110, 2), (1841112, 2), (1841114, 2), (1841116, 2), (1841118, 2), (1841120, 2), (1841122, 2), (1841124, 2), (1841126, 2), (1841130, 2), (1841134, 2), (1841138, 2), (1841142, 2), (1841146, 2), (1841150, 2), (1841152, 2), (1841154, 2), (1841156, 2), (1841158, 2), (1841192, 2), (1836464, 2412), (1838876, 536), (1841160, 16), (1841194, 48), (1841242, 48), (1841290, 192)]:
   differences.extend({'address':hex(address+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(nb.raw[address:address+n],m.r_block(address,n))) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  results.append({'label':label,'native_status':status,'expected_native_status':0 if op else -0x26e66,'memory_differences':differences,'register_differences':regdiff});machine.cleanup()
failures=[r for r in results if r['native_status']!=r['expected_native_status'] or r['memory_differences'] or r['register_differences']]
report={'source_sha256':hashlib.sha256(source).hexdigest(),'rom_sha1':manifest['rom_sha1'],'count':len(results),'passed':len(results)-len(failures),'natural_count':1,'boundary':'26E66 (hardware work remains blocked)','rom_pc_coverage':[hex(at) for at in sorted(coverage)],'results':results}
if args.report:args.report.parent.mkdir(exist_ok=True,parents=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print('F98C transition core ROM:',report['passed'],'/',report['count'])
for r in failures[:6]:print(r['label'],'status',r['native_status'],'memory',r['memory_differences'][:5],'regs',r['register_differences'][:3])
raise SystemExit(1 if failures else 0)
