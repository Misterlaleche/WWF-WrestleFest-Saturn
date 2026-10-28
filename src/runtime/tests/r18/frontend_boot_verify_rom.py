#!/usr/bin/env python3
"""BOOT controller and native credit/start/clear owners against intact ROM."""
import argparse
import ctypes as ct
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import machine68k as mus
from title_verify_rom import Registers,Event,Effect,Context

ROOT=Path(__file__).resolve().parents[2];RAM=0x1c0000;SIZE=0x4000
SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
class State(ct.Structure):_fields_=[('pc',ct.c_uint32),('blocked',ct.c_uint32),('hold_return',ct.c_uint32)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');args=p.parse_args()
    rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x6fc);counts={'clear':0,'credit':0,'start':0,'boot_boundaries':0,'wait_polls':0,'missing_dependency':0};dep_counts={};hold_counts=[]
    with tempfile.TemporaryDirectory(prefix='wf-frontend-boot-',ignore_cleanup_errors=os.name=='nt') as temp:
        lib=Path(temp)/'boot.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
            'src/wf_r18_frontend_boot.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ct.CDLL(str(lib));native.wf_r18_frontend_boot_step.argtypes=[ct.POINTER(State),ct.POINTER(Context)]
        native.wf_r18_frontend_boot_init.argtypes=[ct.POINTER(State)]
        for name in ('clear_1f38','clear_1f52','credit_055a'):getattr(native,'wf_r18_frontend_'+name).argtypes=[ct.POINTER(Context)]
        native.wf_r18_frontend_start_0978.argtypes=[ct.POINTER(Context),ct.POINTER(ct.c_uint16),ct.POINTER(ct.c_uint)]
        @Effect
        def unexpected(*unused):return -1
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
        memory.w_block(0,rom)
        def standalone(name,pc,ram,d=None,ports=(0xffff,)*4):
            regs=Registers((ct.c_uint32*8)(*(d if d is not None else [rng.getrandbits(32) for _ in range(8)])),1)
            a=(ct.c_uint32*7)(*[rng.getrandbits(32) for _ in range(7)])
            buf=ct.create_string_buffer(bytes(ram),SIZE);ctx=Context(ct.addressof(buf),SIZE,ct.pointer(regs),a,unexpected,None)
            memory.w_block(RAM,bytes(ram))
            for i in range(8):cpu.w_reg(i,regs.d[i])
            for i in range(7):cpu.w_reg(8+i,a[i])
            cpu.w_sr(0x2710);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(pc)
            for i,v in enumerate(ports):memory.w16(0x140020+i*2,v)
            for instruction in range(30000):
                if cpu.r_pc()==0x70000:break
                cpu.execute(1)
            assert cpu.r_pc()==0x70000,('ROM runaway',name,hex(cpu.r_pc()))
            if name=='start':
                mask=ct.c_uint(0);result=native.wf_r18_frontend_start_0978(ct.byref(ctx),(ct.c_uint16*4)(*ports),ct.byref(mask))
            else:result=getattr(native,'wf_r18_frontend_'+name)(ct.byref(ctx))
            assert buf.raw==bytes(memory.r_block(RAM,SIZE)),(name,'RAM mismatch',pc)
            assert list(regs.d)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7')
            assert list(a)==[cpu.r_reg(8+i) for i in range(7)],(name,'A0-A6')
            if name in ('credit_055a','start'):assert result==(cpu.r_sr()&1),(name,'carry',result,cpu.r_sr())
            else:assert result==0
            return buf.raw,result,mask.value if name=='start' else 0

        for address,name in ((0x1f38,'clear_1f38'),(0x1f52,'clear_1f52')):
            standalone(name,address,rng.randbytes(SIZE));counts['clear']+=1
        if not args.smoke:
            for credits,selector in itertools.product(range(256),(0,4,0x104)):
                ram=bytearray(rng.randbytes(SIZE));ram[0x4f]=credits
                d=[rng.getrandbits(32) for _ in range(8)];d[0]=(d[0]&0xffff0000)|selector
                standalone('credit_055a',0x55a,ram,d);counts['credit']+=1
            for config,credits,pressed in itertools.product((0,0x800,0xf7ff,0xffff),(0,1,2,3,255),range(16)):
                ram=bytearray(rng.randbytes(SIZE));ram[0x66:0x68]=config.to_bytes(2,'big');ram[0x4f]=credits
                ports=tuple(0xff7f if pressed&(1<<i) else 0xffff for i in range(4))
                actual,result,mask=standalone('start',0x978,ram,ports=ports);counts['start']+=1
                assert actual[0x160:0x162]==ram[0x160:0x162],('mode predecessor clobbered',config,pressed)
                if result:
                    if config&0x800:
                        want=1 if pressed&1 else 3
                    else:want=pressed&-pressed
                    assert mask==want,('selected port mask',mask,want)
            call_map={
                0x72e:(0x1fc0,0x732),0x732:(0x1f6c,0x736),0x736:(0x1fde,0x73a),0x73a:(0x1f9e,0x73e),0x73e:(0x2988,0x742),
                0x760:(0x2a06,0x764),0x764:(0x26772,0x76a),0x77a:(0x26e66,0x780),0x780:(0x26844,0x786),0x78a:(0x264e2,0x790),
                0x790:(0x1fc0,0x794),0x794:(0x1f6c,0x798),0x798:(0x1fde,0x79c),0x79c:(0x1f9e,0x7a0),0x7a0:(0x2a06,0x7a6),
                0x7a6:(0x26772,0x7ac),0x7b6:(0x26844,0x7bc),0x7c0:(0x264e2,0x7c6),0x7c6:(0x1fc0,0x7ca),0x7ca:(0x1f6c,0x7ce),
                0x7ce:(0x1fde,0x7d2),0x7d2:(0x1f9e,0x7d6),0x7f6:(0x2a06,0x7fc),0x7fc:(0x26772,0x802),0x818:(0x26e66,0x81e),
                0x81e:(0x26844,0x824),0x82a:(0x264e2,0x830),0x93a:(0x1fde,0x93e),0x93e:(0x1f6c,0x942),0x95a:(0x2a06,0x95e)}
            targets={key:value[1] for key,value in call_map.items()}
            targets.update({0x6fc:0x714,0x714:0x724,0x742:0x760,0x76a:0x77a,0x7ac:0x7b6,
                0x7d6:0x7f6,0x802:0x808,0x808:0x818,0x830:0x91c,0x91c:0x924,0x924:0x936,
                0x936:0x93a,0x942:0x95a,0x95e:0x81ba,0x786:0x1e6e,0x7bc:0x1e6e,0x824:0x1e6e,0x1e6e:0x1e7c})
            dependencies={value[0] for value in call_map.values()}|{0x2503c,0x9aca,0x2052}
            for skip_fbi in (False,True):
                ram=bytearray(rng.randbytes(SIZE));ram[0x67]=0x80 if skip_fbi else 0
                ram[0x4e:0x50]=b'\0\3';ram[0x4b]=1
                ram[0x5c:0x60]=b'\x12\x34\x56\x78'
                before_persistent={offset:bytes(ram[offset:offset+length]) for offset,length in ((0x4b,5),(0x5c,4),(0x66,2),(0xc2,132))}
                regs=Registers((ct.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]),1)
                a=(ct.c_uint32*7)(*[rng.getrandbits(32) for _ in range(7)])
                buf=ct.create_string_buffer(bytes(ram),SIZE);state=State();native.wf_r18_frontend_boot_init(ct.byref(state))
                memory.w_block(RAM,bytes(ram))
                for i in range(8):cpu.w_reg(i,regs.d[i])
                for i in range(7):cpu.w_reg(8+i,a[i])
                cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x6fc)
                pending=[];errors=[];syncs=0;trace=[]
                def snapshot():return bytes(memory.r_block(RAM,SIZE)),[cpu.r_reg(i) for i in range(8)],[cpu.r_reg(8+i) for i in range(7)]
                @Effect
                def effect(user,event,at,registers,addresses):
                    try:
                        want,before,after=pending.pop(0)
                        got=(event.contents.kind,event.contents.owner_rom,event.contents.address,event.contents.value)
                        assert got==want,('effect',got,want)
                        assert ct.string_at(at,SIZE)==before[0],('effect RAM',want)
                        assert list(registers.contents.d)==before[1],('effect D',want)
                        assert list(addresses[:7])==before[2],('effect A',want)
                        ct.memmove(at,after[0],SIZE)
                        for i,v in enumerate(after[1]):registers.contents.d[i]=v
                        for i,v in enumerate(after[2]):addresses[i]=v
                        return 0
                    except Exception as error:errors.append(repr(error));return -1
                ctx=Context(ct.addressof(buf),SIZE,ct.pointer(regs),a,effect,None)
                # Independent RED for the missing BOOT transition.
                if skip_fbi is False:
                    assert native.wf_r18_frontend_boot_step(ct.byref(state),ct.byref(ctx))==0
                    assert state.pc==0x714,('BOOT entry did not reach714',hex(state.pc))
                    ct.memmove(ct.addressof(buf),bytes(ram),SIZE);native.wf_r18_frontend_boot_init(ct.byref(state))
                while state.pc!=0x81ba:
                    start=state.pc;pending.clear()
                    allowed={targets[start]} if start in targets else ({0x72e,0x790} if start==0x724 else {0x1e6e,state.hold_return})
                    steps=0
                    while True:
                        pc=cpu.r_pc()
                        if steps and pc in allowed:break
                        if pc in dependencies:
                            before=snapshot();ret=memory.r32(cpu.r_sp());trace.append(pc)
                            for helper_steps in range(2000000):
                                if cpu.r_pc()==ret:break
                                memory.w16(0x140026,0x0400 if helper_steps%128<64 else 0)
                                cpu.execute(1)
                            assert cpu.r_pc()==ret,('helper runaway',hex(pc),hex(cpu.r_pc()))
                            pending.append(((0,pc,0,0),before,snapshot()));dep_counts[hex(pc)]=dep_counts.get(hex(pc),0)+1
                            steps+=1;continue
                        if pc==0x71c:
                            before=snapshot();cpu.execute(1);pending.append(((1,0x71c,0x14000c,0x3100),before,snapshot()));steps+=1;continue
                        if pc==0x1e7c and memory.r8(RAM+0x6f)&2==0:
                            before=snapshot();memory.w8(RAM+0x6f,memory.r8(RAM+0x6f)|2)
                            pending.append(((2,0x1e7c,RAM+0x6f,2),before,snapshot()));syncs+=1
                        cpu.execute(1);steps+=1
                        assert steps<100000,('BOOT runaway',hex(start),hex(cpu.r_pc()))
                    result=native.wf_r18_frontend_boot_step(ct.byref(state),ct.byref(ctx));counts['boot_boundaries']+=1
                    assert not errors,(hex(start),errors)
                    assert not pending,(hex(start),'missing effects',len(pending))
                    assert result>=0,(hex(start),'blocked',hex(state.blocked))
                    assert state.pc==cpu.r_pc(),('BOOT PC',hex(start),hex(state.pc),hex(cpu.r_pc()))
                    actual=snapshot()
                    assert buf.raw==actual[0],('BOOT RAM',hex(start))
                    assert list(regs.d)==actual[1],('BOOT D',hex(start))
                    assert list(a)==actual[2],('BOOT A',hex(start))
                assert result==2
                hold_counts.append(syncs);assert syncs==(256 if skip_fbi else 384)
                assert trace.count(0x26844)==(2 if skip_fbi else 3)
                assert trace.count(0x264e2)==(2 if skip_fbi else 3)
                for offset,data in before_persistent.items():assert buf.raw[offset:offset+len(data)]==data,('persistent state',hex(offset))
                assert int.from_bytes(buf.raw[0x80:0x84],'big')==0
                assert bytes(memory.r_block(0,len(rom)))==rom
            @Effect
            def no_ack(user,event,at,registers,addresses):return 0 if event.contents.kind==2 else -1
            ctx.effect=no_ack;state.pc=0x1e7c;state.hold_return=0x78a
            buf[0x6f]=0
            before=(buf.raw,list(regs.d),list(a))
            for _ in range(3):
                assert native.wf_r18_frontend_boot_step(ct.byref(state),ct.byref(ctx))==1
                assert state.pc==0x1e7c and (buf.raw,list(regs.d),list(a))==before
                counts['wait_polls']+=1
            state.pc=0x760
            assert native.wf_r18_frontend_boot_step(ct.byref(state),ct.byref(ctx))==-1
            assert state.pc==0x760 and state.blocked==0x2a06
            assert (buf.raw,list(regs.d),list(a))==before
            counts['missing_dependency']+=1
        machine.cleanup()
        report={'cases':counts,'rom_sha1':SHA1,'dependency_calls':dep_counts,'hold_counts':hold_counts,
            'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_boot.c').read_bytes()).hexdigest(),
            'limitations':'Native clears/credit0+4/START/fullBOOT control+hold128; graphics/fades helpers use original-ROM pre/post contracts, not native effect validation. Reset654 and IRQ caller8AE..8F2 outside; CCR only returned credit carry.',
            'rom_modifications':[],'helper_intercepts':[]}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} frontend boot native/ROM cases')

if __name__=='__main__':main()
