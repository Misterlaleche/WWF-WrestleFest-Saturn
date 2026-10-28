#!/usr/bin/env python3
"""Intact8F50/8FD6 raw FG flashes,24ACK each and original21E6 wait."""
import argparse
import ctypes as ct
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import machine68k as mus
from title_verify_rom import ROOT,SHA1,Registers
from match_hud_verify_rom import Machine,Effect

IO=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint,ct.c_uint32,ct.c_uint32,ct.POINTER(ct.c_uint16),ct.POINTER(Machine))
class Context(ct.Structure):
    _fields_=[('machine',ct.POINTER(Machine)),('fg',ct.c_void_p),('size',ct.c_size_t),('io',IO),('user',ct.c_void_p)]
class State(ct.Structure):
    _fields_=[('pc',ct.c_uint32),('blocked',ct.c_uint32),('owner',ct.c_uint32),('saved_d',ct.c_uint32*8),('saved_a',ct.c_uint32*7),('hold_d0',ct.c_uint32)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x8fd6);counts={'sequences':0,'boundaries':0,'ACK':0,'VBlank_periods':0,'wait_polls':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-match-flash-',ignore_cleanup_errors=os.name=='nt') as temp:
        lib=Path(temp)/'flash.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
                        'src/wf_r18_match_flash.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ct.CDLL(str(lib));native.wf_r18_match_flash_init.argtypes=[ct.POINTER(State),ct.c_uint32]
        native.wf_r18_match_flash_step.argtypes=[ct.POINTER(State),ct.POINTER(Context)]
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
        @Effect
        def unexpected(*unused):return 1
        def snap():return bytes(mem.r_block(0x1c0000,0x4000)),bytes(mem.r_block(0xc0000,0x4000)),[cpu.r_reg(i) for i in range(8)],[cpu.r_reg(8+i) for i in range(7)]
        for owner in (0x8f50,0x8fd6):
            for variant in range(4):
                ram=rng.randbytes(0x4000);fg=rng.randbytes(0x4000);regs=Registers((ct.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]),1)
                original_d=list(regs.d);addresses=[rng.getrandbits(32) for _ in range(7)]
                rb=ct.create_string_buffer(ram,0x4000);fb=ct.create_string_buffer(fg,0x4000)
                ctx_machine=Machine(ct.addressof(rb),ct.pointer(regs),(ct.c_uint32*7)(*addresses),unexpected,None,0)
                pending=[];errors=[];ack=0;periods=0;colors=[]
                @IO
                def io(user,kind,pc,address,value,context):
                    try:
                        expected,before,after,word=pending.pop(0)
                        assert (kind,pc,address)==expected
                        assert (rb.raw,fb.raw,list(regs.d),list(ctx_machine.a))==before,('IO prestate',hex(pc))
                        ct.memmove(ct.addressof(rb),after[0],0x4000)
                        for i,v in enumerate(after[2]):regs.d[i]=v
                        for i,v in enumerate(after[3]):ctx_machine.a[i]=v
                        if kind==1:value.contents.value=word
                        return 0
                    except Exception as e:errors.append(repr(e));return 1
                context=Context(ct.pointer(ctx_machine),ct.addressof(fb),0x4000,io,None);state=State()
                native.wf_r18_match_flash_init(ct.byref(state),owner)
                mem.w_block(0x1c0000,ram);mem.w_block(0xc0000,fg)
                for i,v in enumerate(original_d):cpu.w_reg(i,v)
                for i,v in enumerate(addresses):cpu.w_reg(i+8,v)
                cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(owner)
                if owner==0x8f50:
                    targets={0x8f50:{0x8f5a},0x8f5a:{0x8f7e},0x8f7e:{0x8f8c},0x8f8c:{0x8f96},0x8f96:{0x8f7e,0x8fa8},
                             0x8fa8:{0x8f5a,0x8fc0},0x8fc0:{0x21ea},0x21ea:{0x21f4},0x21f4:{0x21ea,0x2202},0x2202:{0x8fca},0x8fca:{0x8fd0},0x8fd0:{0x8fd4}}
                    stop=0x8fd4
                else:
                    targets={0x8fd6:{0x8fe0},0x8fe0:{0x9004},0x9004:{0x9012},0x9012:{0x901c},0x901c:{0x9004,0x902e},
                             0x902e:{0x8fe0,0x9046},0x9046:{0x904a},0x904a:{0x9050}}
                    stop=0x9050
                while state.pc!=stop:
                    start=state.pc;steps=0
                    while True:
                        pc=cpu.r_pc()
                        if steps and pc in targets[start]:break
                        if pc in (0x8f8c,0x9012) and not mem.r8(0x1c006f)&2:
                            before=snap();mem.w8(0x1c006f,mem.r8(0x1c006f)|2)
                            pending.append(((0,pc,0x1c006f),before,snap(),2));ack+=1
                        if pc in (0x21ea,0x21f4):
                            word=0x400 if pc==0x21ea else 0;before=snap();mem.w16(0x140026,word)
                            pending.append(((1,pc,0x140026),before,snap(),word));periods+=pc==0x21f4
                        cpu.execute(1);steps+=1;assert steps<20000
                    result=native.wf_r18_match_flash_step(ct.byref(state),ct.byref(context))
                    assert not errors and not pending,(errors,len(pending))
                    assert result>=0 and state.pc==cpu.r_pc(),('PC',hex(start),hex(state.pc),hex(cpu.r_pc()))
                    assert (rb.raw,fb.raw,list(regs.d),list(ctx_machine.a))==snap(),('state',hex(start))
                    if start in (0x8f5a,0x8fe0):colors.append(int.from_bytes(fb.raw[:4],'big'))
                    counts['boundaries']+=1
                assert result==2 and ack==24 and periods==(33 if owner==0x8f50 else 0)
                assert colors==[(0xffbeff04+(i<<16)) if owner==0x8f50 else (0xffc5ff04-(i<<16)) for i in range(8)]
                assert fb.raw[0x3000:]==fg[0x3000:] and rb.raw[0x80:0x84]==bytes(4)
                assert list(regs.d)==original_d and list(ctx_machine.a)==addresses
                assert bytes(mem.r_block(0,len(rom)))==rom
                counts['sequences']+=1;counts['ACK']+=ack;counts['VBlank_periods']+=periods
        @IO
        def no_progress(user,kind,pc,address,value,context):
            if kind==1:value.contents.value=0 if pc==0x21ea else 0x400
            return 0
        context.io=no_progress;rb[0x6f]=0
        for pc in (0x8f8c,0x9012,0x21ea,0x21f4):
            state.pc=pc;before=(rb.raw,fb.raw,list(regs.d),list(ctx_machine.a))
            for _ in range(3):
                assert native.wf_r18_match_flash_step(ct.byref(state),ct.byref(context))==1
                assert state.pc==pc and (rb.raw,fb.raw,list(regs.d),list(ctx_machine.a))==before;counts['wait_polls']+=1
        context.size=0x3fff
        assert native.wf_r18_match_flash_step(ct.byref(state),ct.byref(context))==-2;counts['guards']+=1
        machine.cleanup()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_match_flash.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'All8F50/8FD6 and21E6 control native. PhysicalIRQ ACK andVBlank high/low are external inputs; no injectedROM helperreturns. FullRAM16KB/FG16KB padding/D/A compared. CCR/temporary stack outside prematchABI; no hardware/timing/audio certification.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} match flash checks')

if __name__=='__main__':main()
