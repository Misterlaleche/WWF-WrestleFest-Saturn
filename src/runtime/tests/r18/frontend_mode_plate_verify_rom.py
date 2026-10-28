#!/usr/bin/env python3
"""Original52BE..5339 GAME SELECT plate, retaining predecessor state."""
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
from title_verify_rom import ROOT,RAM,SIZE,SHA1,Registers,Event,Effect,Context,State

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x52be);counts={'sequences':0,'boundaries':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-mode-plate-',ignore_cleanup_errors=os.name=='nt') as temp:
        lib=Path(temp)/'plate.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
                        'src/wf_r18_frontend_mode_plate.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ct.CDLL(str(lib));native.wf_r18_frontend_mode_plate_init.argtypes=[ct.POINTER(State)]
        native.wf_r18_frontend_mode_plate_step.argtypes=[ct.POINTER(State),ct.POINTER(Context)]
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
        calls={0x52c6:(0x2a06,0x52cc),0x52cc:(0x2052,0x52d6),0x52d6:(0x2052,0x52e0),
               0x52e0:(0x2052,0x52ea),0x5318:(0x26e66,0x531e),0x531e:(0x1f9e,0x5324),0x5334:(0x2a06,0x533a)}
        targets={pc:item[1] for pc,item in calls.items()};targets.update({0x52be:0x52c6,0x52ea:0x5318,0x5324:0x5334})
        def snap():return bytes(mem.r_block(RAM,SIZE)),[cpu.r_reg(i) for i in range(8)],[cpu.r_reg(8+i) for i in range(7)]
        for mode in range(256):
            ram=bytearray(rng.randbytes(SIZE));ram[0x161]=mode
            # Natural accepted-START predecessor: IRQ$8AE has executed
            # $1F6C/$1FDE. Random pending map queues are invalid producer state.
            ram[0x16d2:0x19ae]=bytes(0x19ae-0x16d2)
            ram[0x19b2:0x1cd2]=bytes(0x1cd2-0x19b2)
            for offset in (0x15f4,0x15f8,0x15fc):ram[offset:offset+2]=(mode&3).to_bytes(2,'big')
            persistent=[(off,bytes(ram[off:off+length])) for off,length in ((0x4b,5),(0x5c,4),(0x66,2),(0x160,2))]
            buf=ct.create_string_buffer(bytes(ram),SIZE);regs=Registers((ct.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]),1)
            a=(ct.c_uint32*7)(*[rng.getrandbits(32) for _ in range(7)]);pending=[];errors=[];trace=[]
            @Effect
            def effect(user,event,at,registers,addresses):
                try:
                    owner,before,after=pending.pop(0);e=event.contents
                    assert (e.kind,e.owner_rom,e.address,e.value)==(0,owner,0,0)
                    assert (ct.string_at(at,SIZE),list(registers.contents.d),list(addresses[:7]))==before,('callback pre-state',hex(owner))
                    ct.memmove(at,after[0],SIZE)
                    for i,v in enumerate(after[1]):registers.contents.d[i]=v
                    for i,v in enumerate(after[2]):addresses[i]=v
                    return 0
                except Exception as error:errors.append(repr(error));return -1
            ctx=Context(ct.addressof(buf),SIZE,ct.pointer(regs),a,effect,None);state=State()
            native.wf_r18_frontend_mode_plate_init(ct.byref(state));mem.w_block(RAM,bytes(ram))
            for i in range(8):cpu.w_reg(i,regs.d[i])
            for i in range(7):cpu.w_reg(8+i,a[i])
            cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);cpu.w_pc(0x52be)
            while state.pc!=0x533a:
                target=targets[state.pc];steps=0
                while True:
                    pc=cpu.r_pc()
                    if steps and pc==target:break
                    if pc in (0x2a06,0x2052,0x26e66,0x1f9e):
                        before=snap();ret=mem.r32(cpu.r_sp());trace.append(pc)
                        for ticks in range(200000):
                            if cpu.r_pc()==ret:break
                            mem.w16(0x140026,0x400 if ticks%128<64 else 0);cpu.execute(1)
                        assert cpu.r_pc()==ret,('helper runaway',hex(pc),hex(cpu.r_pc()))
                        pending.append((pc,before,snap()));steps+=1;continue
                    cpu.execute(1);steps+=1;assert steps<100000
                result=native.wf_r18_frontend_mode_plate_step(ct.byref(state),ct.byref(ctx))
                assert not errors and not pending,(errors,len(pending))
                assert result>=0 and state.pc==cpu.r_pc(),('PC',hex(state.pc),hex(cpu.r_pc()))
                assert (buf.raw,list(regs.d),list(a))==snap(),('state mismatch',mode,hex(target));counts['boundaries']+=1
            assert result==2 and trace==[0x2a06,0x2052,0x2052,0x2052,0x26e66,0x1f9e,0x2a06]
            for offset,data in persistent:assert buf.raw[offset:offset+len(data)]==data,('persistent',hex(offset))
            assert buf.raw[0x80:0x84]==bytes(4) and bytes(mem.r_block(0,len(rom)))==rom
            counts['sequences']+=1
        @Effect
        def reject(*unused):return -1
        ctx.effect=reject;state.pc=0x52c6;before=(buf.raw,list(regs.d),list(a))
        assert native.wf_r18_frontend_mode_plate_step(ct.byref(state),ct.byref(ctx))==-1
        assert state.pc==0x52c6 and state.blocked==0x2a06 and (buf.raw,list(regs.d),list(a))==before;counts['guards']+=1
        ctx.size=SIZE-1
        assert native.wf_r18_frontend_mode_plate_step(ct.byref(state),ct.byref(ctx))==-2;counts['guards']+=1
        machine.cleanup()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_mode_plate.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'Native52BE..5339 controller; external helper functions run full ROM pre/post contracts. No native graphics/palette/audio certification; CCR/temporary stack outside title ABI. All256 predecessor mode bytes preserved withcredits/config/RNG.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} frontend mode plate checks')

if __name__=='__main__':main()
