"""Composed10120/export aliases versus complete original fixed callbacks.

The production adapter owns synchronization and rollback. Phase rendering,
queue and live-D equivalence are outside this narrow alias proof.
"""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,subprocess,tempfile
import machine68k as mus
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1];RAM=0x1c0000
import argparse
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
if 'src/wf_r18_runtime_handoff.c'not in core:core+=['src/wf_r18_runtime_handoff.c']
callbacks=(0x10186,0x101b2,0x10228,0x1029e,0x102f6);proved=0;cases=0;guards=0
def put(b,at,v,n=2):b[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def fixture(slot,state,terminal=False,mode=0):
 raw=bytearray(0x4000);raw[0x161]=mode
 for i in range(4):
  o=0x1134+i*0x30;put(raw,o,0x8000);put(raw,o+2,14);put(raw,o+6,0x1c0 if i<2 else 0x330);put(raw,o+10,0x178);put(raw,o+14,0x140);put(raw,o+0x12,1 if i%2==0 else 3);put(raw,o+0x2e,0 if i<2 else 0x8000)
 if terminal:
  o=0x1134+slot*0x30;put(raw,o+0x1c,0x8000|state);put(raw,o+0x22,1);put(raw,o+0x24,(6,6,3,2)[state-1]-1)
 return raw
with tempfile.TemporaryDirectory(prefix='wf-export-alias-review-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 shutil.copy2(HERE/'runtime_export_alias_oracle_bridge.c',frozen/'alias_bridge.c')
 hashes={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in[*core,'alias_bridge.c']}
 lib=frozen/'alias.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'alias_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.runtime_export_alias_oracle.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for slot in range(4):
  for terminal,states in((False,range(5)),(True,range(1,5))):
   for state in states:
    raw=fixture(slot,state,terminal);o=0x1134+slot*0x30;value=(0x8000|state)if terminal else state
    expected=bytearray(0x200000);expected[:len(rom)]=rom;expected[RAM:RAM+len(raw)]=raw;put(expected,RAM+o+0x1c,value)
    mem.w_block(0,bytes(expected));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(callbacks[state]);cpu.w_reg(8,RAM+o)
    for i in range(8):cpu.w_reg(i,0x12345670+i)
    for _ in range(20000):
     if cpu.r_pc()==0x70000:break
     cpu.execute(1)
    assert cpu.r_pc()==0x70000,('original callback runaway',hex(cpu.r_pc()))
    want=bytes(mem.r_block(RAM+o,0x30));required=(int.from_bytes(want[0x1c:0x1e],'big'),int.from_bytes(want[4:6],'big'),int.from_bytes(want[0x22:0x24],'big'),int.from_bytes(want[0x24:0x26],'big'))
    inp=ctypes.create_string_buffer(bytes(raw));out=ctypes.create_string_buffer(0x4000);m=(ctypes.c_uint32*9)();assert native.runtime_export_alias_oracle(inp,out,slot,value,0,m)==0;assert m[0]==0
    got=(m[3],m[4],m[5],m[6]);assert got==required,('fixed alias mismatch',slot,state,terminal,got,required);assert m[1]==m[2]==m[3]
    proved+=1;cases+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 # Skipped mode retains a new action write in both views.
 for slot in range(4):
  raw=fixture(slot,1,mode=2);inp=ctypes.create_string_buffer(bytes(raw));out=ctypes.create_string_buffer(0x4000);m=(ctypes.c_uint32*9)();assert native.runtime_export_alias_oracle(inp,out,slot,1,0,m)==0;assert m[0]==0 and m[1]==m[2]==m[3]==1
 # Failure restores the whole runtime, including the previously stale compact latch.
 for slot in range(4):
  for state,fault,status,dep in((5,0,1,0x10172),(1,1,1,0x2052),(1,2,1,0x2052),(0,3,2,0x10120)):
   raw=fixture(slot,0);inp=ctypes.create_string_buffer(bytes(raw));out=ctypes.create_string_buffer(0x4000);m=(ctypes.c_uint32*9)();assert native.runtime_export_alias_oracle(inp,out,slot,state,fault,m)==0;assert (m[0],m[7],m[8])==(status,dep,1),('rollback',slot,state,fault,list(m));guards+=1
 assert proved==36 and guards==16
 report={'status':'PASS','ROM_callback_cases':cases,'mode_gate_controls':4,'unchanged_runtime_failure_controls':guards,'compiled_sources_sha256':hashes,'rom_sha1':hashlib.sha1(rom).hexdigest(),'scope':['Complete original fixed callback at each state0..4 and terminal exits for all four records','Native composed phase10120 plus runtime_export latch/pose/timer/index compared','Mode gate preserves action-global write; unsupported state/missing audio/audio failure/queue failure restore entire runtime'],'limits':['Not a full phase10120 rendering/queue/live-register ROM oracle','Synthetic coherent imports and explicit action-global writes; no hardware trajectory claim']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k!='compiled_sources_sha256'}))
