"""Raw65C0 versus intactROM, explicit dependency and hardware events."""
from pathlib import Path
import argparse,ctypes,hashlib,json,os,random,shutil,struct,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'ranking_capture_manifest.json').read_text());capture=(HERE/'ranking_snapshot_stage9b_guard.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
class Shadow(ctypes.Structure):_fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Machine(ctypes.Structure):_fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('regs',ctypes.POINTER(Shadow)),('a',ctypes.c_uint32*7),('effect',ctypes.c_void_p),('user',ctypes.c_void_p),('dependency',ctypes.c_uint32)]
ReadROM=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t)
class Context(ctypes.Structure):_fields_=[('machine',ctypes.POINTER(Machine)),('read_rom',ReadROM),('user',ctypes.c_void_p)]
class Request(ctypes.Structure):_fields_=[(x,ctypes.c_uint32)for x in('kind','owner','call_pc','return_pc','address','value','width')]
class State(ctypes.Structure):_fields_=[('pc',ctypes.c_uint32),('entry_pc',ctypes.c_uint32),('blocked_dependency',ctypes.c_uint32),('return_stack',ctypes.c_uint32*8),('saved_d',ctypes.c_uint32*5),('saved_a2',ctypes.c_uint32),('saved_d0',ctypes.c_uint32),('io_value',ctypes.c_uint32),('request',Request),('depth',ctypes.c_uint8),('pending',ctypes.c_uint8),('fatal',ctypes.c_uint8),('carry',ctypes.c_uint8)]
@ReadROM
def read_rom(_,at,p,n):
 if at+n>len(rom):return 1
 ctypes.memmove(p,rom[at:at+n],n);return 0
with tempfile.TemporaryDirectory(prefix='wf-ranking-',ignore_cleanup_errors=os.name=='nt')as td:
 f=Path(td)
 for name in('src','include'):shutil.copytree(ROOT/name,f/name)
 sha=hashlib.sha256((f/'src/wf_r18_ranking.c').read_bytes()).hexdigest();lib=f/'ranking.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_ranking.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=f,check=True)
 n=ctypes.CDLL(str(lib));n.wf_r18_ranking_init.argtypes=[ctypes.POINTER(State),ctypes.c_uint32];n.wf_r18_ranking_step.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context)];n.wf_r18_ranking_complete.argtypes=[ctypes.POINTER(State),ctypes.POINTER(Context),ctypes.c_int,ctypes.c_uint32]
 regs=list(struct.unpack_from('=15I',capture));ram=ctypes.create_string_buffer(capture[60:60+0x4000]);dr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);m=Machine(ctypes.cast(ram,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(dr),(ctypes.c_uint32*7)(*regs[8:]),None,None,0);ctx=Context(ctypes.pointer(m),read_rom,None);s=State();n.wf_r18_ranking_init(ctypes.byref(s),0x65c0)
 cpu=mus.Machine(mus.CPUType.M68000,2048);cpu.mem.w_block(0,rom);cpu.mem.w_block(0x1c0000,ram.raw[:0x4000]);cpu.mem.w_block(0xc0000,capture[60+0x4000:]);cpu.cpu.w_pc(0x65c0);cpu.cpu.w_sp(0x1d0000);cpu.mem.w32(0x1d0000,0x70000)
 for i,v in enumerate(regs):cpu.cpu.w_reg(i,v)
 for _ in range(100000):
  if cpu.cpu.r_pc()==0x70000:break
  cpu.cpu.execute(1)
 assert cpu.cpu.r_pc()==0x70000,'natural ROM runaway'
 for steps in range(100000):
  status=n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))
  assert status>=0,('native blocked',hex(s.pc),hex(s.blocked_dependency))
  if status==2:break
  assert status==0,('unexpected natural qualification effect',status,hex(s.pc))
 else:raise AssertionError('native runaway')
 assert ram.raw[:0x4000]==bytes(cpu.mem.r_block(0x1c0000,0x4000)),'RAM mismatch'
 assert list(dr.d)+list(m.a)==[cpu.cpu.r_reg(i)for i in range(15)],('D/A mismatch',list(dr.d)+list(m.a),[cpu.cpu.r_reg(i)for i in range(15)])
 assert bytes(cpu.mem.r_block(0,len(rom)))==rom;cpu.cleanup()
 report={'status':'PASS','natural_cases':1,'source_sha256':sha,'snapshot_sha256':meta['snapshot_sha256'],'compared':['actual raw16KB','D0-D7/A0-A6'],'intercepts':[]}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
