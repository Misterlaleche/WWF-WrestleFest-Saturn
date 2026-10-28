"""Compare F18A state-2 register and movement paths with the arcade ROM."""
from pathlib import Path
import ctypes as ct
import hashlib
import itertools
import json
import os
import re
import struct
import subprocess
import tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2];PROJECT=ROOT.parents[1]
capture=PROJECT/'validation/local-20261027/pre-f4c2-royal-pressure-2200-after-kind2/pre-f4c2-snapshot.bin'
snap=capture.read_bytes();assert len(snap)==24+32+0x4000+0x4000
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-f18a-run-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'prepass.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                 '-Iinclude',*core,'src/wf_r18_runtime_handoff.c',
                 'tests/r18/f18a_facing_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));fn=dll.f18a_facing_oracle
 fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32)]
 cases=[];rng_cases=0
 for latched,face,held,mode,global_a3,timer in itertools.product((0,1),(0,0x80),range(4),(0,1),range(4),(0,1,2)):
  memory=bytearray(0x200000);memory[:len(rom)]=rom
  memory[0x1c0000:0x1c4000]=snap[56:56+0x4000];memory[0xc0000:0xc4000]=snap[56+0x4000:]
  for slot in range(9):memory[0x1c05b0+slot*0x10c]&=0x7f
  a=0x1c05b0;memory[a]|=0x80;memory[a+0x56]&=0x3f
  memory[a+0x33]&=~2;memory[a+0x34]&=~2
  memory[a+0x20:a+0x22]=(0xc002 if latched else 0x8002).to_bytes(2,'big')
  memory[a+0x1d]=0;memory[a+0x2e]=face
  memory[a+0x44:a+0x46]=timer.to_bytes(2,'big')
  memory[a+0xa8:a+0xaa]=held.to_bytes(2,'big')
  memory[0x1c0161]=mode;memory[0x1c00a3]=global_a3
  regs=struct.unpack_from('=8I',snap,24)
  machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
  mem.w_block(0,bytes(memory));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0xf18a)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  reached_rng=False
  for _ in range(20000):
   if cpu.r_pc()==0x70000:break
   if cpu.r_pc()==0x1129e:reached_rng=True
   cpu.execute(1)
  assert cpu.r_pc()==0x70000
  buf=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs);blocked=ct.c_uint32()
  st=fn(buf,d,ct.byref(blocked));assert st==0,(latched,face,held,mode,global_a3,st,hex(blocked.value))
  expected=bytes(mem.r_block(0x1c0000,0x4000));actual=buf.raw[0x1c0000:0x1c4000]
  delta=[(hex(0x1c0000+i),x,y)for i,(x,y)in enumerate(zip(actual,expected))if x!=y]
  assert not delta,('RAM',latched,face,held,mode,global_a3,delta[:8])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],('D0-D7',latched,face,held,mode,global_a3,list(d),[cpu.r_reg(i)for i in range(8)])
  rng_cases+=int(reached_rng)
  cases.append([latched,face,held,mode,global_a3,timer]);machine.cleanup()
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
assert rng_cases>0, 'Expected to reach original $1129E'
report={'status':'PASS','cases':len(cases),'rom_sha1':hashlib.sha1(rom).hexdigest(),
        'rng_cases':rng_cases,
        'source_sha256':hashlib.sha256((ROOT/'src/wf_actor_prepass.c').read_bytes()).hexdigest(),
        'limits':['Controlled starting states, not full frames.','Physical Saturn and arbitrary RNG states are outside this matrix.']}
(PROJECT/'validation/local-20261028/f18a-run-state-rom.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
