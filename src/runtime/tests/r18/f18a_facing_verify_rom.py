"""Diagnostic comparison of full F18A on captured combat states.

Snapshots were captured before F4C2, not F18A; inputs here are controlled
starting states based on real gameplay, not natural pre-phase evidence.
"""
import ctypes as ct
import hashlib
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path
import struct
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT.parents[1]
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
captures=sorted((PROJECT/'validation/local-20261027').glob('pre-f4c2-*/pre-f4c2-snapshot.bin'))
captures+=sorted((PROJECT/'validation/local-20261028').glob('action*-tag-natural-*/pre-f4c2-snapshot.bin'))
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
results=[]
with tempfile.TemporaryDirectory(prefix='wf-f18a-audit-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'prepass.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                 '-Iinclude',*core,'src/wf_r18_runtime_handoff.c',
                 'tests/r18/f18a_facing_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));fn=dll.f18a_facing_oracle
 fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32)]
 for path in captures:
  snapshot=path.read_bytes();assert len(snapshot)==24+32+0x4000+0x4000
  frame,_,_,owner,_,_=struct.unpack_from('=6I',snapshot);assert owner==0xf4c2
  initial=bytearray(0x200000);initial[:len(rom)]=rom
  initial[0x1c0000:0x1c4000]=snapshot[56:56+0x4000]
  initial[0xc0000:0xc4000]=snapshot[56+0x4000:]
  d0=struct.unpack_from('=8I',snapshot,24)
  machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
  mem.w_block(0,bytes(initial));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0xf18a)
  for i,d in enumerate(d0):cpu.w_reg(i,d)
  pcs=set();intercepts=[]
  for step in range(200000):
   pc=cpu.r_pc()
   if pc==0x70000:break
   pcs.add(pc)
   if pc==0x2052:
    intercepts.append(pc);cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
   cpu.execute(1)
  else:raise AssertionError(('ROM runaway',path,hex(cpu.r_pc())))
  buf=ct.create_string_buffer(bytes(initial));d=(ct.c_uint32*8)(*d0);blocked=ct.c_uint32()
  status=fn(buf,d,ct.byref(blocked));expected=bytes(mem.r_block(0x1c0000,0x4000));actual=buf.raw[0x1c0000:0x1c4000]
  differences=[{'address':hex(0x1c0000+i),'native':x,'rom':y} for i,(x,y) in enumerate(zip(actual,expected)) if x!=y]
  entry={'capture':str(path.relative_to(PROJECT)),'sha256':hashlib.sha256(snapshot).hexdigest(),
         'frame':frame,'status':status,'blocked':hex(blocked.value),'rom_instructions':step,
         'ram_differences':differences[:30],'ram_difference_count':len(differences),
         'd_native':list(d),'d_rom':[cpu.r_reg(i) for i in range(8)],
         'entered_facing_10be8':0x10be8 in pcs,'sound_intercepts':len(intercepts)}
  results.append(entry);machine.cleanup()
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'results':results,'rom_sha1':hashlib.sha1(rom).hexdigest(),
 'source_sha256':hashlib.sha256((ROOT/'src/wf_actor_prepass.c').read_bytes()).hexdigest(),
 'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 'limits':['F4C2 snapshots reused as controlled F18A starting points; not captured before F18A.',
           'No comparison of address registers or final video pixels.',
           'The 2052 sound device is intercepted if reached; no audio verification.']}
out=PROJECT/'validation/local-20261028/f18a-facing-audit.json';out.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps([{'frame':r['frame'],'status':r['status'],'blocked':r['blocked'],
                   'ram_differences':r['ram_difference_count'],
                   'd_match':r['d_native']==r['d_rom'],'facing':r['entered_facing_10be8']} for r in results]))
