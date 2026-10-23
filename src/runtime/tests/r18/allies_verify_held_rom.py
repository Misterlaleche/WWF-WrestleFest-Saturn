#!/usr/bin/env python3
"""Actual held-ally snapshots: $1C07A bypasses the $1C082 FF-state guard.

Catches skipping the held callback/animation, and losing the facing XOR when
the main11 callback selects that same $125C0 descriptor.
"""
import argparse,ctypes,hashlib,json,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('snapshots',type=Path);p.add_argument('--report',type=Path)
args=p.parse_args();root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();results=[]
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'held.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/allies_held_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.allies_held_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for path in sorted(args.snapshots.glob('snapshot-*.bin')):
  data=path.read_bytes();frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
  if not kind:continue
  before=data[56:8248];a=0x5b0+slot*0x10c
  domain=before[a+0x1d]
  if not(domain==255 or domain==11 and before[a+0x44]&128):continue
  regs=struct.unpack_from('=8I',data,24)
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=before
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs)
  status=native.allies_held_oracle(nb,nr,slot)
  assert status==0,('native blocked',path.name,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for i in range(8,15):u.w_reg(i,0)
  for i,value in enumerate(regs):u.w_reg(i,value)
  u.w_reg(8,0x1c0000+a);u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(pc)
  visited=[]
  for steps in range(100000):
   at=u.r_pc();visited.append(hex(at))
   if at==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',path.name,hex(u.r_pc()))
  want=bytes(m.r_block(0x1c05b0,9*0x10c));got=nb.raw[0x1c05b0:0x1c05b0+9*0x10c]
  differences=[{'address':hex(0x1c05b0+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(got,want)) if g!=e]
  registers=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  results.append({'snapshot':path.name,'frame':frame,'slot':slot,'domain':domain,'memory_differences':differences,'register_differences':registers,'visited':visited})
  if domain==255:assert '0x1c07a' in visited and '0x1c082' not in visited and '0x125cc' in visited
  machine.cleanup()
report={'count':len(results),'passed':sum(not r['memory_differences'] and not r['register_differences'] for r in results),'results':results}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('Held-ally real-snapshot ROM checks:',report['passed'],'/',report['count'])
for result in results:
 if result['memory_differences'] or result['register_differences']:print(result['snapshot'],result['frame'],'memory',result['memory_differences'][:6],'registers',result['register_differences'])
assert report['count']>0
raise SystemExit(0 if report['passed']==report['count'] else 1)
