#!/usr/bin/env python3
"""Differentially replay observed ally owner calls against the untouched ROM.

This catches a wrong CPU branch, target mutation, action transition, actor or
global side effect on real no-input gameplay snapshots. No actor is forced.
"""
import argparse
import collections
import hashlib
import json
import struct
from pathlib import Path
import machine68k as mus

parser=argparse.ArgumentParser()
parser.add_argument('snapshots',type=Path)
parser.add_argument('--limit',type=int)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
ranges=[(0x5b0,9*0x10c),(0x5c,4),(0x15e4,8),(0x1670,4),(0x167a,1),
        (0x1682,2),(0x1684,8),(0x168c,4),(0x1692,2),(0x1697,1),
        (0x1800,4),(0x15d2,2),(0x1150,2),(0x1180,2),(0x11b0,2),(0x11e0,2)]
results=[]
for path in sorted(args.snapshots.glob('snapshot-*.bin'))[:args.limit]:
 data=path.read_bytes()
 frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
 assert status==0,(path,status)
 regs=struct.unpack_from('=8I',data,24)
 before=data[56:56+8192]
 afterregs=struct.unpack_from('=8I',data,56+8192)
 after=data[88+8192:88+16384]
 assert len(after)==8192
 machine=mus.Machine(mus.CPUType.M68000,2048)
 u=machine.cpu;m=machine.mem
 m.w_block(0,rom);m.w_block(0x1c0000,before)
 # Preserve the original ROM even for data accesses through tagged/null links.
 for page in range(0,0x80000,0x10000):
  for width in range(3):
   m.set_special_range_write_func(page,width,lambda address,value:None)
   m.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
 for i in range(8,15):u.w_reg(i,0)
 for i,value in enumerate(regs):u.w_reg(i,value)
 u.w_reg(8,0x1c05b0+slot*0x10c)
 # $1C1DC..$1C1E6 calls through A1; some handlers preserve and later read it.
 if not kind:u.w_reg(9,pc)
 u.w_sp(0x1d0000)
 m.w32(0x1d0000,0x70000);u.w_pc(pc)
 visited=set();sound=0
 for steps in range(100000):
  at=u.r_pc();visited.add(at)
  if at==0x70000:break
  # Explicit silent sound-device contract already used by the runtime.
  if at==0x2052:
   u.w_pc(m.r32(u.r_sp()));u.w_sp(u.r_sp()+4);sound+=1;continue
  u.execute(1)
 differences=[]
 if u.r_pc()!=0x70000:differences.append({'runaway':hex(u.r_pc())})
 else:
  for at,n in ranges:
   want=bytes(m.r_block(0x1c0000+at,n));got=after[at:at+n]
   differences.extend({'address':hex(0x1c0000+at+i),'native':g,'rom':e}
                      for i,(g,e) in enumerate(zip(got,want)) if g!=e)
 regdiff=[{'register':'D'+str(i),'native':hex(g),'rom':hex(u.r_reg(i))}
          for i,g in enumerate(afterregs) if g!=u.r_reg(i)]
 a=0x5b0+slot*0x10c
 read=lambda offset,n=2:int.from_bytes(before[a+offset:a+offset+n],'big')
 result={'snapshot':path.name,'frame':frame,'slot':slot,'kind':'dispatch' if kind else 'cpu',
         'pc':hex(pc),'state':hex(read(0x20)),'ae':hex(read(0xae)),
         'selector':hex(read(0x60)),'target':hex(read(0x7a,4)),
         'steps':steps,'sound_calls':sound,'memory_differences':differences,
         'register_differences':regdiff,'visited':[hex(v) for v in sorted(visited)]}
 results.append(result)
 machine.cleanup()
report={'rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_count':len(results),
        'memory_pass':sum(not x['memory_differences'] for x in results),
        'register_pass':sum(not x['register_differences'] for x in results),
        'owners':dict(collections.Counter(x['pc'] for x in results)),'results':results}
(args.snapshots/'rom-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='results'},indent=2))
for result in results:
 if result['memory_differences'] or result['register_differences']:
  print(result['snapshot'],result['frame'],result['slot'],result['kind'],result['state'],result['ae'],result['selector'],
        'memory',result['memory_differences'][:8],'registers',result['register_differences'][:3])
raise SystemExit(0 if report['memory_pass']==len(results) and report['register_pass']==len(results) else 1)
