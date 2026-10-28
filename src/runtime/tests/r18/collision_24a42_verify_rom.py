"""Controlled equal-facing linked collision versus intact arcade $2435A."""
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
evidence=PROJECT/'validation/local-20261028/royal-direction-fix-collision'
actors=(evidence/'blocked-actors.bin').read_bytes();globs=(evidence/'blocked-globals.bin').read_bytes()
regs=struct.unpack('=8I',(evidence/'blocked-dregs.bin').read_bytes());assert len(actors)==9*0x10c
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
RAM=0x1c0000;src=RAM+0x5b0+4*0x10c;dst=RAM+0x5b0+5*0x10c
with tempfile.TemporaryDirectory(prefix='wf-24a42-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'collision.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                 '-Iinclude',*core,'tests/r18/collision_249be_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));project=dll.collision_24984_project
 project.argtypes=[ct.c_void_p,ct.c_void_p,ct.c_void_p]
 assert len(globs)==dll.collision_24984_globals_size()
 fn=dll.collision_24984_oracle
 fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.c_uint,ct.c_uint,ct.c_uint]
 cases=[]
 for face,link,mode,credit,profile,team,partner_active,reserve,selector in itertools.product(
     (0,0x80),(0x1c09e0,0x1c0aec,0x1c0bf8,0x1c0d04),(0,1),(0,0x8000),(0,7),(0x1c09e0,0x1c0aec),(0,1),(0,0x1c05b0),(0,0x53)):
  memory=bytearray(0x200000);memory[:len(rom)]=rom
  raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors))
  memory[RAM:RAM+0x2000]=bytes(raw)
  memory[src+0x2e]=face;memory[dst+0x2e]=face
  memory[dst+0x26:dst+0x2a]=link.to_bytes(4,'big')
  memory[link+0x7e:link+0x82]=reserve.to_bytes(4,'big')
  memory[link+0x60:link+0x62]=selector.to_bytes(2,'big')
  memory[dst+0x7e:dst+0x82]=(0x1c05b0).to_bytes(4,'big')
  memory[link+0x33]|=1;memory[dst+0x33]|=1
  memory[link+0x86:link+0x8a]=(0x1c05b0).to_bytes(4,'big')
  memory[link+0x26:link+0x2a]=dst.to_bytes(4,'big')
  memory[0x1c05b0+0x32]=(memory[0x1c05b0+0x32]&~1)|partner_active
  memory[RAM+0x161]=mode;memory[RAM+0x7c:RAM+0x7e]=credit.to_bytes(2,'big')
  memory[dst+0x86:dst+0x8a]=team.to_bytes(4,'big')
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem
  mem.w_block(0,bytes(memory));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x2435a)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for i,v in ((8,src),(9,dst),(10,0x24ef6+profile*7),(11,0x24ef6+13*7)):cpu.w_reg(i,v)
  pcs=set()
  for _ in range(10000):
   pc=cpu.r_pc()
   if pc==0x70000:break
   pcs.add(pc);cpu.execute(1)
  assert cpu.r_pc()==0x70000,(hex(cpu.r_pc()),mode,credit)
  expected=0 if cpu.r_sr()&1 else 1
  assert expected==1 and 0x24a42 in pcs,(expected,mode,credit)
  b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs)
  got=fn(b,d,4,5,profile,13)
  assert got==expected,(face,link,mode,credit,profile,team,'native/rom',got,expected)
  want=bytes(mem.r_block(RAM,0x2000));actual=b.raw[RAM:RAM+0x2000]
  delta=[(hex(RAM+i),x,y)for i,(x,y)in enumerate(zip(actual,want)) if x!=y]
  assert not delta,(face,link,mode,credit,profile,team,delta[:15])
  assert list(d)==[cpu.r_reg(i)for i in range(8)]
  cases.append([face,link,mode,credit,profile,team,partner_active,reserve,selector]);m.cleanup()
 guards=0
 for mode,credit,reserve in ((1,0x8000,0xdeadbeef),):
  memory=bytearray(0x200000);memory[:len(rom)]=rom
  raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors))
  memory[RAM:RAM+0x2000]=bytes(raw)
  memory[src+0x2e]=memory[dst+0x2e]=0
  memory[dst+0x26:dst+0x2a]=(0x1c0bf8).to_bytes(4,'big')
  memory[0x1c0bf8+0x7e:0x1c0bf8+0x82]=reserve.to_bytes(4,'big')
  memory[RAM+0x161]=mode;memory[RAM+0x7c:RAM+0x7e]=credit.to_bytes(2,'big')
  b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs)
  assert fn(b,d,4,5,7,13)==2
  assert b.raw[:len(memory)]==bytes(memory) and list(d)==list(regs)
  guards+=1
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'status':'PASS','cases':len(cases),'rom_sha1':hashlib.sha1(rom).hexdigest(),
        'retained_guards':guards,
        'source_sha256':hashlib.sha256((ROOT/'src/wf_collision_dispatch.c').read_bytes()).hexdigest(),
        'limits':['Controlled equal-facing states derived from HOST; no full-match or Saturn hardware claim.',
                  'Only valid controlled participant links were used; arbitrary malformed graphs remain guarded.']}
(PROJECT/'validation/local-20261028/collision-24a42-rom.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
