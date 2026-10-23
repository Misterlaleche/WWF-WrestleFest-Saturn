#!/usr/bin/env python3
"""Fresh native/ROM replay of real ally apron and B7-reset observations.

The test catches omitted live D-register writes in $1D398 and restoration of
the bit cleared by ANDI.W #$F1FD at $1C2F4. Expectations come from ROM code.
"""
import argparse,ctypes,hashlib,json,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus

p=argparse.ArgumentParser();p.add_argument('snapshots',type=Path);p.add_argument('--report',type=Path)
args=p.parse_args();root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_cpu_runtime.c')
results=[]
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'allies.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_decision_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for path in sorted(args.snapshots.glob('snapshot-*.bin')):
  data=path.read_bytes();frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
  if kind:continue
  before=data[56:8248];a=0x5b0+slot*0x10c
  state=int.from_bytes(before[a+0x20:a+0x22],'big')
  if not ((state&127)==1 and before[a+0xaf]==1 or (state&127)==0 and before[a+0xb7]&2):continue
  regs=struct.unpack_from('=8I',data,24)
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=before
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*9)(*regs,0)
  status=native.wf_cpu_oracle(nb,nr,slot)
  assert status==0,('native blocked',path.name,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for page in range(0,0x80000,0x10000):
   for width in range(3):
    m.set_special_range_write_func(page,width,lambda address,value:None)
    m.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for i in range(8,15):u.w_reg(i,0)
  for i,value in enumerate(regs):u.w_reg(i,value)
  u.w_reg(8,0x1c0000+a);u.w_reg(9,pc);u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(pc)
  visited=[]
  for steps in range(100000):
   at=u.r_pc();visited.append(hex(at))
   if at==0x70000:break
   if at==0x2052:u.w_pc(m.r32(u.r_sp()));u.w_sp(u.r_sp()+4);continue
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',path.name,hex(u.r_pc()))
  differences=[]
  # This existing bridge loads actors/RNG/D registers; $1697 input is not
  # represented by it and is covered by the recorded-runtime comparison.
  for at,n in [(0x1c05b0,9*0x10c),(0x1c005c,4)]:
   want=bytes(m.r_block(at,n));got=nb.raw[at:at+n]
   differences.extend({'address':hex(at+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(got,want)) if g!=e)
  registers=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  results.append({'snapshot':path.name,'frame':frame,'slot':slot,'pc':hex(pc),'state':hex(state),'substate':before[a+0xaf],
                  'memory_differences':differences,'register_differences':registers,'visited':visited})
  machine.cleanup()
report={'rom_sha1':hashlib.sha1(rom).hexdigest(),'count':len(results),'passed':sum(not r['memory_differences'] and not r['register_differences'] for r in results),'results':results}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('Ally real-snapshot CPU ROM checks:',report['passed'],'/',report['count'])
for result in results:
 if result['memory_differences'] or result['register_differences']:
  print(result['snapshot'],'frame',result['frame'],'slot',result['slot'],result['pc'],'memory',result['memory_differences'][:3],'registers',result['register_differences'][:3])
assert report['count']>0
raise SystemExit(0 if report['passed']==report['count'] else 1)
