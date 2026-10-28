"""Controlled linked high-timer collision versus intact arcade $2435A."""
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
evidence=PROJECT/'validation/local-20261028/royal-direction-collision-fixed'
log=(evidence/'host.log').read_text()
assert 'BLOCK frame=3834 runtime_frame=3834' in log
assert 'COLLISION PROBE 00000004 00000006 00000007 0000000c 00000001 00000003' in log
actors=(evidence/'blocked-actors.bin').read_bytes();globs=(evidence/'blocked-globals.bin').read_bytes()
regs=struct.unpack('=8I',(evidence/'blocked-dregs.bin').read_bytes())
assert len(actors)==9*0x10c
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
RAM=0x1c0000;src=RAM+0x5b0+4*0x10c;dst=RAM+0x5b0+6*0x10c
with tempfile.TemporaryDirectory(prefix='wf-24a88-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'collision.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                 '-Iinclude',*core,'tests/r18/collision_249be_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));project=dll.collision_24984_project
 project.argtypes=[ct.c_void_p,ct.c_void_p,ct.c_void_p]
 assert len(globs)==dll.collision_24984_globals_size()
 fn=dll.collision_24984_oracle
 fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.c_uint,ct.c_uint,ct.c_uint]
 cases=[]
 cases += [('variant',face,face^0x80,link,mode,timer,high,team,profile,subtype,credit,reserve,selector)
           for face,link,mode,timer,high,team in itertools.product(
               (0,0x80),(0x1c09e0,0x1c0aec,0x1c0bf8,0x1c0d04),(0,1),(3,4),(0,0x80),(0x1c09e0,0x1c0aec))
           for profile,subtype in ((7,0),(0,0),(0,10))
           for credit,reserve,selector in itertools.product((0,0x8000),(0,0x1c05b0),(0,0x53))]
 for name,sface,tface,link,mode,timer,high,team,profile,subtype,credit,reserve,selector in cases:
  memory=bytearray(0x200000);memory[:len(rom)]=rom
  raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors))
  memory[RAM:RAM+0x2000]=bytes(raw)
  if name=='variant':
   memory[src+0x2e]=sface;memory[dst+0x2e]=tface
   memory[dst+0x26:dst+0x2a]=link.to_bytes(4,'big')
   memory[RAM+0x161]=mode;memory[dst+0x4c]=high
   memory[dst+0x52:dst+0x54]=timer.to_bytes(2,'big')
   memory[dst+0x86:dst+0x8a]=team.to_bytes(4,'big');memory[dst+0x33]&=~0x10
   memory[dst+0x64:dst+0x66]=subtype.to_bytes(2,'big')
   memory[RAM+0x7c:RAM+0x7e]=credit.to_bytes(2,'big')
   memory[dst+0x33]|=1;memory[link+0x33]|=1
   memory[link+0x86:link+0x8a]=(0x1c05b0).to_bytes(4,'big')
   memory[link+0x26:link+0x2a]=dst.to_bytes(4,'big')
   memory[dst+0x7e:dst+0x82]=reserve.to_bytes(4,'big')
   memory[dst+0x60:dst+0x62]=selector.to_bytes(2,'big')
   memory[link+0x7e:link+0x82]=(0x1c05b0).to_bytes(4,'big')
   memory[RAM+0x5b0+0x32]|=1
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem
  mem.w_block(0,bytes(memory));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x2435a)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for i,v in ((8,src),(9,dst),(10,0x24ef6+profile*7),(11,0x24ef6+12*7)):cpu.w_reg(i,v)
  pcs=set()
  for _ in range(10000):
   pc=cpu.r_pc()
   if pc==0x70000:break
   pcs.add(pc);cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,hex(cpu.r_pc()))
  expected=0 if cpu.r_sr()&1 else 1
  assert expected==1 and 0x24a06 in pcs,(name,expected,profile,subtype)
  b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs)
  got=fn(b,d,4,6,profile,12)
  assert got==expected,(name,sface,tface,link,mode,timer,high,team,'native/rom',got,expected)
  want=bytes(mem.r_block(RAM,0x2000));actual=b.raw[RAM:RAM+0x2000]
  delta=[(hex(RAM+i),x,y)for i,(x,y)in enumerate(zip(actual,want)) if x!=y]
  assert not delta,(name,sface,tface,link,mode,timer,high,team,delta[:10])
  assert list(d)==[cpu.r_reg(i)for i in range(8)]
  m.cleanup()
 # An invalid team pointer required by credited TAG cleanup must stop before
 # either fighter or the globals are partially modified.
 memory=bytearray(0x200000);memory[:len(rom)]=rom
 raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors))
 memory[RAM:RAM+0x2000]=bytes(raw)
 memory[src+0x2e]=0;memory[dst+0x2e]=0x80
 memory[dst+0x26:dst+0x2a]=(0x1c0d04).to_bytes(4,'big')
 memory[dst+0x52:dst+0x54]=(3).to_bytes(2,'big')
 memory[dst+0x33]|=1
 memory[dst+0x86:dst+0x8a]=(0xdeadbeef).to_bytes(4,'big')
 memory[RAM+0x161]=0;memory[RAM+0x7c:RAM+0x7e]=(0x8000).to_bytes(2,'big')
 b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs)
 assert fn(b,d,4,6,7,12)==2
 assert b.raw[:len(memory)]==bytes(memory) and list(d)==list(regs)
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'status':'PASS','cases':len(cases),'rom_sha1':hashlib.sha1(rom).hexdigest(),
        'actor_snapshot_sha256':hashlib.sha256(actors).hexdigest(),
        'globals_snapshot_sha256':hashlib.sha256(globs).hexdigest(),
        'source_sha256':hashlib.sha256((ROOT/'src/wf_collision_dispatch.c').read_bytes()).hexdigest(),
        'retained_guards':1,
        'limits':['Controlled variations derived from a HOST snapshot; no natural occurrence is claimed.',
                  'Controlled cases exercise the high-timer linked interruption.',
                  'No natural match or physical Saturn claim.']}
(PROJECT/'validation/local-20261028/collision-24a06-rom.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
