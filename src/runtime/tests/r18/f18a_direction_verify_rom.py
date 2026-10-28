"""Compare all 16 walking-direction inputs through the actual F18A phase."""
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
capture=PROJECT/'validation/local-20261027/pre-f4c2-tag-pressure-1500/pre-f4c2-snapshot.bin'
snap=capture.read_bytes();assert len(snap)==24+32+0x4000+0x4000
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
out=PROJECT/'validation/local-20261028/f18a-direction-rom.json'
with tempfile.TemporaryDirectory(prefix='wf-f18a-directions-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'prepass.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                 '-Iinclude',*core,'src/wf_r18_runtime_handoff.c',
                 'tests/r18/f18a_facing_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));fn=dll.f18a_facing_oracle
 fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32)]
 cases=[]
 for state,direction in itertools.product((1,5),range(16)):
  memory=bytearray(0x200000);memory[:len(rom)]=rom
  memory[0x1c0000:0x1c4000]=snap[56:56+0x4000]
  memory[0xc0000:0xc4000]=snap[56+0x4000:]
  for slot in range(9):memory[0x1c05b0+slot*0x10c]&=0x7f
  actor=0x1c05b0;memory[actor]|=0x80;memory[actor+0x56]&=0x3f
  memory[actor+0x33]&=~2;memory[actor+0x34]&=~2
  memory[actor+0x20:actor+0x22]=(0x8000|state).to_bytes(2,'big')
  memory[actor+0xae:actor+0xb0]=b'\0\0'
  memory[actor+0x60:actor+0x62]=(0x16 if state==5 else 0).to_bytes(2,'big')
  memory[actor+0xa8:actor+0xaa]=direction.to_bytes(2,'big')
  regs=struct.unpack_from('=8I',snap,24)
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem
  mem.w_block(0,bytes(memory));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0xf18a)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for _ in range(20000):
   if cpu.r_pc()==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(direction,hex(cpu.r_pc()))
  b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs);blocked=ct.c_uint32()
  status=fn(b,d,ct.byref(blocked));assert status==0,(direction,status,hex(blocked.value))
  expected=bytes(mem.r_block(0x1c0000,0x4000));actual=b.raw[0x1c0000:0x1c4000]
  differences=[(hex(0x1c0000+i),x,y)for i,(x,y)in enumerate(zip(actual,expected))if x!=y]
  assert not differences,('RAM',direction,differences[:10])
  assert list(d)==[cpu.r_reg(i)for i in range(8)],('D0-D7',direction,list(d),[cpu.r_reg(i)for i in range(8)])
  angle=mem.r16(actor+0x2c);cases.append({'state':state,'input':direction,'angle':angle});m.cleanup()
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'status':'PASS','cases':cases,'rom_sha1':hashlib.sha1(rom).hexdigest(),
 'capture_sha256':hashlib.sha256(snap).hexdigest(),
 'source_sha256':hashlib.sha256((ROOT/'src/wf_actor_prepass.c').read_bytes()).hexdigest(),
 'limits':['Controlled input/active states derived from HOST capture; not a full frame or Saturn hardware test.']}
out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
