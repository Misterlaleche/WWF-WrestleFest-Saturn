"""Compare the Royal result participant scan and ranking handoff to ROM."""
from pathlib import Path
import ctypes as ct
import hashlib
import json
import os
import random
import subprocess
import tempfile

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
rom = (ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'

class Shadow(ct.Structure):
    _fields_ = [('d', ct.c_uint32*8), ('valid', ct.c_uint8)]
class Machine(ct.Structure):
    _fields_ = [('ram',ct.POINTER(ct.c_uint8)),('regs',ct.POINTER(Shadow)),
                ('a',ct.c_uint32*7),('effect',ct.c_void_p),('user',ct.c_void_p),('dependency',ct.c_uint32)]
class Context(ct.Structure):
    _fields_ = [('machine',ct.POINTER(Machine)),('read_rom',ct.c_void_p),('user',ct.c_void_p)]
class Request(ct.Structure):
    _fields_ = [(x,ct.c_uint32) for x in ('kind','owner','call_pc','return_pc','address','value')]
class State(ct.Structure):
    _fields_ = [(x,ct.c_uint32) for x in ('pc','entry_pc','saved_d0','sub_return','blocked_dependency')] + [
        ('saved_d',ct.c_uint32*8),('saved_a',ct.c_uint32*4),('request',Request),
        ('pending',ct.c_uint8),('fatal',ct.c_uint8),('carry',ct.c_uint8),('callback_a0',ct.c_uint32)]
assert ct.sizeof(State) == 100
rng = random.Random(0x1b1e)
cases = 0
with tempfile.TemporaryDirectory(prefix='wf-royal-collect-',ignore_cleanup_errors=os.name=='nt') as td:
    lib = Path(td)/'post.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
        '-Iinclude','src/wf_r18_post_tag.c','src/wf_r17_m68k_shadow.c',
        'src/wf_r17_rng1129e.c','-o',str(lib)],cwd=ROOT,check=True)
    dll = ct.CDLL(str(lib))
    dll.wf_r18_post_tag_init.argtypes = [ct.POINTER(State),ct.c_uint32]
    dll.wf_r18_post_tag_step.argtypes = [ct.POINTER(State),ct.POINTER(Context)]
    dll.wf_r18_post_tag_complete.argtypes = [ct.POINTER(State),ct.POINTER(Context),ct.c_int,ct.c_uint16]
    for mask in range(16):
        for variant in range(8):
            raw = bytearray(rng.randbytes(0x4000))
            for slot in range(4):
                offset = 0x5b0+slot*0x10c
                raw[offset] = (raw[offset]&0x7f) | (0x80 if mask & (1<<slot) else 0)
            regs = [rng.getrandbits(32) for _ in range(15)]
            oracle = mus.Machine(mus.CPUType.M68000,2048)
            oracle.mem.w_block(0,rom)
            oracle.mem.w_block(0x1c0000,bytes(raw))
            for i,value in enumerate(regs): oracle.cpu.w_reg(i,value)
            oracle.cpu.w_sp(0x1d0000)
            oracle.cpu.w_pc(0x1b1e)
            for _ in range(100):
                if oracle.cpu.r_pc()==0x1b42: break
                oracle.cpu.execute(1)
            assert oracle.cpu.r_pc()==0x1b42
            data = (ct.c_uint8*0x4000).from_buffer_copy(raw)
            shadow = Shadow((ct.c_uint32*8)(*regs[:8]),1)
            native = Machine(data,ct.pointer(shadow),(ct.c_uint32*7)(*regs[8:]),None,None,0)
            context = Context(ct.pointer(native),None,None)
            state = State()
            dll.wf_r18_post_tag_init(ct.byref(state),0x1b1e)
            for _ in range(100):
                outcome = dll.wf_r18_post_tag_step(ct.byref(state),ct.byref(context))
                if outcome != 0: break
            assert outcome==1, ('expected ranking request',mask,variant,outcome,hex(state.blocked_dependency))
            assert (state.pc,state.request.kind,state.request.owner,state.request.return_pc)==(0x1b42,0,0x65c0,0x1b48)
            assert bytes(data)==bytes(oracle.mem.r_block(0x1c0000,0x4000)), ('RAM',mask,variant)
            assert list(shadow.d)==[oracle.cpu.r_reg(i) for i in range(8)], ('D',mask,variant)
            assert list(native.a)==[oracle.cpu.r_reg(i) for i in range(8,15)], ('A',mask,variant)
            assert dll.wf_r18_post_tag_complete(ct.byref(state),ct.byref(context),0,0)==0
            assert dll.wf_r18_post_tag_step(ct.byref(state),ct.byref(context))==0
            assert dll.wf_r18_post_tag_step(ct.byref(state),ct.byref(context))==4 and state.pc==0x6fc
            oracle.cleanup()
            cases+=1
    if os.name=='nt': ct.windll.kernel32.FreeLibrary(ct.c_void_p(dll._handle))
report={'status':'PASS','cases':cases,'participant_masks':16,'rom_sha1':hashlib.sha1(rom).hexdigest(),
        'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'post_source_sha256':hashlib.sha256((ROOT/'src/wf_r18_post_tag.c').read_bytes()).hexdigest(),
        'compared':['RAM16KiB','D0-D7','A0-A6','ranking request','boot boundary'],
        'limits':['Ranking call is a handoff, not executed here.','No physical Saturn or full Royal match claim.']}
out=ROOT.parents[1]/'validation/local-20261028/royal-gameover-collect-rom.json'
out.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
