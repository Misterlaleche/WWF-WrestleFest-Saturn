"""Replay the natural instrumented Royal frame 1550 pair in ROM and HOST."""
from pathlib import Path
import ctypes as ct,hashlib,json,os,re,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];PROJECT=ROOT.parents[1]
evidence=PROJECT/'validation/local-20261028/collision-24aa8-natural-capture/royal-rush'
log=(evidence/'host.log').read_text();assert 'TRACE_24AA8 frame=1550 source=8 target=0 sid=7 tid=20' in log
actors=(evidence/'natural-actors.bin').read_bytes();globs=(evidence/'natural-globals.bin').read_bytes();regs=struct.unpack('=8I',(evidence/'natural-dregs.bin').read_bytes());assert len(actors)==9*0x10c
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split();RAM=0x1c0000;src=RAM+0x5b0+8*0x10c;dst=RAM+0x5b0
with tempfile.TemporaryDirectory(prefix='wf-natural24aa8-',ignore_cleanup_errors=os.name=='nt') as td:
 lib=Path(td)/'collision.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/collision_249be_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 dll=ct.CDLL(str(lib));project=dll.collision_24984_project;project.argtypes=[ct.c_void_p,ct.c_void_p,ct.c_void_p];assert len(globs)==dll.collision_24984_globals_size()
 fn=dll.collision_24984_oracle;fn.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_uint,ct.c_uint,ct.c_uint,ct.c_uint]
 memory=bytearray(0x200000);memory[:len(rom)]=rom;raw=(ct.c_ubyte*0x2000)();project(raw,ct.create_string_buffer(globs),ct.create_string_buffer(actors));memory[RAM:RAM+0x2000]=bytes(raw)
 m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem;mem.w_block(0,bytes(memory));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x2435a)
 for i,v in enumerate(regs):cpu.w_reg(i,v)
 for i,v in ((8,src),(9,dst),(10,0x24ef6+7*7),(11,0x24ef6+20*7)):cpu.w_reg(i,v)
 pcs=set()
 for _ in range(10000):
  pc=cpu.r_pc()
  if pc==0x70000:break
  pcs.add(pc);cpu.execute(1)
 assert cpu.r_pc()==0x70000 and 0x24aa8 in pcs
 b=ct.create_string_buffer(bytes(memory));d=(ct.c_uint32*8)(*regs);got=fn(b,d,8,0,7,20);expected=0 if cpu.r_sr()&1 else 1
 assert got==expected==1,(got,expected)
 want=bytes(mem.r_block(RAM,0x2000));actual=b.raw[RAM:RAM+0x2000];delta=[(hex(RAM+i),x,y)for i,(x,y) in enumerate(zip(actual,want)) if x!=y];assert not delta,delta[:20]
 assert list(d)==[cpu.r_reg(i)for i in range(8)];m.cleanup()
 if os.name=='nt':ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'status':'PASS','frame':1550,'source_slot':8,'target_slot':0,'source_profile':7,'target_profile':20,'rom_sha1':hashlib.sha1(rom).hexdigest(),'actor_snapshot_sha256':hashlib.sha256(actors).hexdigest(),'globals_snapshot_sha256':hashlib.sha256(globs).hexdigest(),'source_sha256':hashlib.sha256((ROOT/'src/wf_collision_dispatch.c').read_bytes()).hexdigest(),'path':'0x24ab8' if 0x24ab8 in pcs else '0x24b02','limits':['Natural occurrence in instrumented HOST Royal rush; not Saturn emulator or hardware.','ROM compare is isolated pair from a projected 8 KiB RAM snapshot, not full match comparison.']}
(PROJECT/'validation/local-20261028/collision-24aa8-natural-rom.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
