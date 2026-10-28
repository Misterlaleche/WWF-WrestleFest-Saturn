"""Controlled $24A90 relation pointer, full original $2435A vs runtime."""
from pathlib import Path
import ctypes as ct,hashlib,itertools,json,os,re,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];PROJECT=ROOT.parents[1]
evidence=PROJECT/'validation/local-20261028/royal-direction-collision-fixed'
actors=(evidence/'blocked-actors.bin').read_bytes();globs=(evidence/'blocked-globals.bin').read_bytes()
regs=struct.unpack('=8I',(evidence/'blocked-dregs.bin').read_bytes())
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
RAM=0x1c0000;slots=[RAM+0x5b0+i*0x10c for i in range(9)];src=slots[4];dst=slots[6]
with tempfile.TemporaryDirectory(prefix='wf-24a90-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'collision.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/collision_249be_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));project=dll.collision_24984_project;project.argtypes=[ct.c_void_p,ct.c_void_p,ct.c_void_p]
 fn=dll.collision_24984_oracle;fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.c_uint,ct.c_uint,ct.c_uint]
 count=0
 for normal,related,face in itertools.product((0,slots[5],slots[7]),(slots[0],slots[4],slots[5],slots[6],slots[7]),(0,0x80)):
  memory=bytearray(0x200000);memory[:len(rom)]=rom
  raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors));memory[RAM:RAM+0x2000]=bytes(raw)
  memory[src+0x2e]=face;memory[dst+0x2e]=face^0x80;memory[dst+0x4c:dst+0x4e]=(14).to_bytes(2,'big')
  memory[dst+0x26:dst+0x2a]=normal.to_bytes(4,'big');memory[dst+0x76:dst+0x7a]=related.to_bytes(4,'big')
  memory[dst+0x74:dst+0x76]=(0x1234).to_bytes(2,'big');memory[related+0x1c:related+0x1e]=(0x7777).to_bytes(2,'big')
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem
  mem.w_block(0,bytes(memory));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x2435a)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  for i,v in ((8,src),(9,dst),(10,0x24ef6+7*7),(11,0x24ef6+14*7)):cpu.w_reg(i,v)
  pcs=set()
  for _ in range(10000):
   pc=cpu.r_pc()
   if pc==0x70000:break
   pcs.add(pc);cpu.execute(1)
  assert cpu.r_pc()==0x70000 and 0x24a90 in pcs
  expected=0 if cpu.r_sr()&1 else 1
  b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs);got=fn(b,d,4,6,7,14)
  assert got==expected,(hex(normal),hex(related),'status',got,expected)
  want=bytes(mem.r_block(RAM,0x2000));actual=b.raw[RAM:RAM+0x2000]
  delta=[(hex(RAM+i),x,y)for i,(x,y)in enumerate(zip(actual,want)) if x!=y]
  assert not delta,(hex(normal),hex(related),delta[:12])
  assert list(d)==[cpu.r_reg(i)for i in range(8)];count+=1;m.cleanup()
 # Invalid consumed relation pointer must reject atomically even when +$26 is valid.
 memory=bytearray(0x200000);memory[:len(rom)]=rom
 raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors));memory[RAM:RAM+0x2000]=bytes(raw)
 memory[dst+0x4c:dst+0x4e]=(14).to_bytes(2,'big')
 memory[dst+0x26:dst+0x2a]=slots[5].to_bytes(4,'big');memory[dst+0x76:dst+0x7a]=(0xdeadbeef).to_bytes(4,'big')
 b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs)
 assert fn(b,d,4,6,7,14)==2 and b.raw[:len(memory)]==bytes(memory) and list(d)==list(regs)
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'status':'PASS','cases':count,'invalid_relation_guards':1,'rom_sha1':hashlib.sha1(rom).hexdigest(),'source_sha256':hashlib.sha256((ROOT/'src/wf_collision_dispatch.c').read_bytes()).hexdigest(),'limits':['Controlled HOST-snapshot variants, not proof of this Saturn physical incident.']}
(PROJECT/'validation/local-20261028/collision-24a90-rom.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
