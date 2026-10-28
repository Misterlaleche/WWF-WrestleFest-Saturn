#!/usr/bin/env python3
"""Intact98BA TAG join and8EB6 command selector with exact D/A and raw stores."""
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
from match_hud_verify_rom import Machine,Effect,Read,Palette,Context

REGIONS=((0x1c0000,0x4000),(0xc0000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x98ba);counts={'TAG_join':0,'Royal_gate':0,'sound_selector':0,'palette_callback':0,'guards':0};coverage=set()
    with tempfile.TemporaryDirectory(prefix='wf-match-support-',ignore_cleanup_errors=os.name=='nt') as temp:
        lib=Path(temp)/'support.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
                        'src/wf_r18_match_support.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ct.CDLL(str(lib));native.wf_r18_match_support_call.argtypes=[ct.POINTER(Context),ct.c_uint32]
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
        @Read
        def read(user,address,out,count):
            if address>len(rom) or count>len(rom)-address:return 1
            ct.memmove(out,rom[address:address+count],count);return 0
        def digest(memory):return hashlib.sha256(b''.join(memory[at:at+n] for at,n in REGIONS)).hexdigest()
        def execute(memory,registers,owner):
            for at,n in REGIONS:mem.w_block(at,bytes(memory[at:at+n]))
            for i,v in enumerate(registers):cpu.w_reg(i,v)
            cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(owner);events=[]
            for _ in range(200000):
                pc=cpu.r_pc()
                if pc==0x70000:break
                coverage.add(pc)
                if pc in (0x9aca,0x2052):
                    before=bytearray(memory)
                    for at,n in REGIONS:before[at:at+n]=mem.r_block(at,n)
                    events.append((pc,[cpu.r_reg(i) for i in range(15)],digest(before)))
                cpu.execute(1)
            assert cpu.r_pc()==0x70000,('ROM runaway',hex(owner),hex(cpu.r_pc()))
            for at,n in REGIONS:memory[at:at+n]=mem.r_block(at,n)
            return [cpu.r_reg(i) for i in range(15)],events
        def run(owner,mode=0,identity=0,mask=1,side=0,callback=False):
            memory=bytearray(0x200000);memory[:len(rom)]=rom
            for at,n in REGIONS:memory[at:at+n]=rng.randbytes(n)
            memory[0x1c0161]=mode
            for i in range(9):
                at=0x1c05b0+i*0x10c;memory[at]=0x80 if mask&(1<<i) else 0
                memory[at+0x56]=rng.randrange(256);memory[at+0x57]=(identity+i)%12;memory[at+0x33]=side
            registers=[rng.getrandbits(32) for _ in range(15)];registers[11]=0xc0e40
            expected=bytearray(memory);want,want_events=execute(expected,registers,owner)
            buffer=ct.create_string_buffer(bytes(memory));regs=Registers((ct.c_uint32*8)(*registers[:8]),1)
            errors=[];events=[]
            @Effect
            def effect(user,address,context):
                try:
                    state=context.contents;current=list(state.regs.contents.d)+list(state.a)
                    events.append((address,current,digest(buffer.raw)))
                    assert address in (0x9aca,0x2052)
                    helper_memory=bytearray(buffer.raw);after,_=execute(helper_memory,current,address)
                    for at,n in REGIONS:ct.memmove(ct.addressof(buffer)+at,bytes(helper_memory[at:at+n]),n)
                    for i,v in enumerate(after[:8]):state.regs.contents.d[i]=v
                    for i,v in enumerate(after[8:]):state.a[i]=v
                    return 0
                except Exception as error:errors.append(repr(error));return 1
            state=Machine(ct.addressof(buffer)+0x1c0000,ct.pointer(regs),(ct.c_uint32*7)(*registers[8:]),effect,None,0)
            context=Context(ct.pointer(state),ct.addressof(buffer)+0xc0000,0x4000,ct.addressof(buffer)+0x180000,0x9000,read,None)
            writes=[]
            @Palette
            def palette_store(user,address,word):
                if not 0x180000<=address<=0x188ffe:return 1
                writes.append((address,word));ct.memmove(ct.addressof(buffer)+address,word.to_bytes(2,'big'),2);return 0
            if callback:context.pal=None;context.pal_size=0;context.write_palette=palette_store
            result=native.wf_r18_match_support_call(ct.byref(context),owner)
            assert result==0,('blocked',hex(owner),hex(state.dependency),errors)
            assert not errors and events==want_events,('effect contracts',errors,events,want_events)
            for at,n in REGIONS:
                actual=buffer.raw[at:at+n];desired=expected[at:at+n]
                assert actual==desired,('memory',hex(owner),[(hex(at+i),a,b) for i,(a,b) in enumerate(zip(actual,desired)) if a!=b][:8])
            assert list(regs.d)+list(state.a)==want,('D/A',hex(owner),list(regs.d)+list(state.a),want)
            if callback:assert len(writes)==16*mask.bit_count()
            assert bytes(mem.r_block(0,len(rom)))==rom
            return context,(buffer,regs,state,effect)
        for identity in range(12):
            for mask in (0,1,2,4,8,16,32,64,128,256,511):
                for side in (0,0x80):run(0x98ba,identity*2,identity,mask,side);counts['TAG_join']+=1
        for mode in range(1,256,2):run(0x98ba,mode,mode%12,511,0x80);counts['Royal_gate']+=1
        for mode in range(256):run(0x8eb6,mode,mask=511);counts['sound_selector']+=1
        for identity in range(12):
            for side in (0,0x80):run(0x98ba,identity=identity,mask=511,side=side,callback=True);counts['palette_callback']+=1
        context,keep=run(0x8eb6)
        @Effect
        def reject(*unused):return 1
        keep[2].effect=reject;high=keep[1].d[0]&0xffff0000
        assert native.wf_r18_match_support_call(ct.byref(context),0x8eb6)==1
        assert keep[2].dependency==0x2052 and keep[1].d[0]==high|0x3100;counts['guards']+=1
        context.fg_size=0x3fff
        assert native.wf_r18_match_support_call(ct.byref(context),0x98ba)==2;counts['guards']+=1
        machine.cleanup()
        required=(0x993a,0x98fc,0x99d0,0x9a78,0x992a,0x8ec0)
        assert all(pc in coverage for pc in required),('coverage',[hex(pc) for pc in required if pc not in coverage])
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_match_support.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'Native98BA/993A/99D0/9A78/8EB6;9ACA strips and2052 commands execute full originalROM via mandatory callback. FullRAM/FG/palette padding/IO/D/A and effect prestate hashes compare. CCR/temporary stack outside prematch ABI. ZeroactorTAG requires valid inheritedA3, supplied explicitly in fixtures.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} match support cases')

if __name__=='__main__':main()
