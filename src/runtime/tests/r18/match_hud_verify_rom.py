#!/usr/bin/env python3
"""Raw7506/782C/C1EA HUD owners versus intact ROM, all padding and live D/A."""
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
from title_verify_rom import ROOT,SHA1,Registers

class Machine(ct.Structure):pass
Effect=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.POINTER(Machine))
Machine._fields_=[('ram',ct.c_void_p),('regs',ct.POINTER(Registers)),('a',ct.c_uint32*7),
                 ('effect',Effect),('user',ct.c_void_p),('dependency',ct.c_uint32)]
Read=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_void_p,ct.c_size_t)
Palette=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_uint16)
class Context(ct.Structure):
    _fields_=[('machine',ct.POINTER(Machine)),('fg',ct.c_void_p),('fg_size',ct.c_size_t),
              ('pal',ct.c_void_p),('pal_size',ct.c_size_t),('read',Read),('user',ct.c_void_p),('write_palette',Palette)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x7506);counts={'health7506':0,'portrait782C':0,'paletteC1EA':0,'palette_callback':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-match-hud-',ignore_cleanup_errors=os.name=='nt') as temp:
        lib=Path(temp)/'hud.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
                        'src/wf_r18_match_hud.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ct.CDLL(str(lib));native.wf_r18_match_hud_call.argtypes=[ct.POINTER(Context),ct.c_uint32]
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
        @Read
        def read(user,address,out,count):
            if address>len(rom) or count>len(rom)-address:return 1
            ct.memmove(out,rom[address:address+count],count);return 0
        @Effect
        def unexpected(*unused):raise AssertionError('HUD must not delegate helper effects')
        def run(owner,mask=1,cpus=0,health=110,identity=0,mode=0,credit=1,slot=0,primary=15,callback=False):
            ram=bytearray(rng.randbytes(0x4000));ram[0x161]=mode;ram[0x7c:0x7e]=credit.to_bytes(2,'big')
            for i in range(4):
                at=0x5b0+i*0x10c;ram[at]=0x80 if mask&(1<<i) else 0
                ram[at+0x56]=(0x80 if cpus&(1<<i) else 0)|(0x40 if i&1 else 0)
                ram[at+0x57]=(identity+i)%12;ram[at+0x66:at+0x68]=health.to_bytes(2,'big')
                ram[at+0x8a:at+0x8e]=(0x140020+i*2).to_bytes(4,'big');ram[at+0x33]=(ram[at+0x33]&0xfe)|bool(primary&(1<<i))
            fg=rng.randbytes(0x4000);pal=rng.randbytes(0x9000)
            rb=ct.create_string_buffer(bytes(ram),0x4000);fb=ct.create_string_buffer(fg,0x4000);pb=ct.create_string_buffer(pal,0x9000)
            regs=Registers((ct.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]),1)
            addresses=[rng.getrandbits(32) for _ in range(7)];addresses[0]=0x1c05b0+slot*0x10c
            context_machine=Machine(ct.addressof(rb),ct.pointer(regs),(ct.c_uint32*7)(*addresses),unexpected,None,0)
            context=Context(ct.pointer(context_machine),ct.addressof(fb),0x4000,ct.addressof(pb),0x9000,read,None)
            writes=[]
            @Palette
            def palette_store(user,address,word):
                if not 0x180000<=address<=0x188ffe:return 1
                writes.append((address,word));ct.memmove(ct.addressof(pb)+address-0x180000,word.to_bytes(2,'big'),2);return 0
            if callback:context.pal=None;context.pal_size=0;context.write_palette=palette_store
            mem.w_block(0x1c0000,bytes(ram));mem.w_block(0xc0000,fg);mem.w_block(0x180000,pal)
            for i in range(8):cpu.w_reg(i,regs.d[i])
            for i in range(7):cpu.w_reg(8+i,addresses[i])
            cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(owner)
            for _ in range(30000):
                if cpu.r_pc()==0x70000:break
                cpu.execute(1)
            assert cpu.r_pc()==0x70000,('ROM runaway',hex(owner),hex(cpu.r_pc()))
            result=native.wf_r18_match_hud_call(ct.byref(context),owner)
            assert result==0,('native blocked',hex(owner),hex(context_machine.dependency))
            for label,buffer,base,size in (('RAM',rb,0x1c0000,0x4000),('FG',fb,0xc0000,0x4000),('pal',pb,0x180000,0x9000)):
                want=bytes(mem.r_block(base,size));got=buffer.raw
                assert got==want,(hex(owner),label,[(hex(i),a,b) for i,(a,b) in enumerate(zip(got,want)) if a!=b][:8])
            assert list(regs.d)==[cpu.r_reg(i) for i in range(8)],('D',hex(owner),list(regs.d),[cpu.r_reg(i) for i in range(8)])
            assert list(context_machine.a)==[cpu.r_reg(i+8) for i in range(7)],('A',hex(owner),list(context_machine.a),[cpu.r_reg(i+8) for i in range(7)])
            assert bytes(mem.r_block(0,len(rom)))==rom
            if callback:assert len(writes)==(16*mask.bit_count() if credit and not mode&1 else 0)
            return context,(rb,fb,pb,regs,context_machine)
        for mask,health,identity in itertools.product(range(16),(0,1,15,16,95,96,97,255,256,500,0xffff),(0,5,11)):
            run(0x7506,mask,mask^15,health,identity,mode=mask,credit=mask&1);counts['health7506']+=1
        for mask,cpus in itertools.product(range(16),range(16)):
            run(0x7506,mask,cpus,110,mask%12);counts['health7506']+=1
        for slot,identity,credit,active,cpu_actor in itertools.product((0,1,2,3,4),range(12),(0,1,0x8000),(False,True),(False,True)):
            run(0x782c,(1<<slot) if active and slot<4 else 0,(1<<slot) if cpu_actor else 0,identity=identity,credit=credit,slot=slot);counts['portrait782C']+=1
        for mask,identity,primary,mode,credit in itertools.product((0,1,5,15),range(12),(0,5,15),(0,1,2,255),(0,1)):
            run(0xc1ea,mask,identity=identity,primary=primary,mode=mode,credit=credit);counts['paletteC1EA']+=1
        for identity,primary,mode in itertools.product(range(12),(0,5,15),(0,1,2)):
            run(0xc1ea,15,identity=identity,primary=primary,mode=mode,callback=True);counts['palette_callback']+=1
        context,keep=run(0x7506);before=tuple(b.raw for b in keep[:3]);context.fg_size=0x3fff
        assert native.wf_r18_match_hud_call(ct.byref(context),0x7506)==2
        assert tuple(b.raw for b in keep[:3])==before;counts['guards']+=1
        context.fg_size=0x4000
        assert native.wf_r18_match_hud_call(ct.byref(context),0xdead)==1
        assert keep[4].dependency==0xdead;counts['guards']+=1
        @Read
        def reject(*unused):return 1
        context.read=reject
        assert native.wf_r18_match_hud_call(ct.byref(context),0xc1ea)==1
        assert keep[4].dependency in (0xc264,0xc294) and tuple(b.raw for b in keep[:3])==before;counts['guards']+=1
        machine.cleanup()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_match_hud.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'Native7506/782C/C1EA and all localHUD helpers; rawROM tables supplied. Full16KB RAM/16KBFG/36KBpalette padding/D/A compare. No tile conversion, native presentation, audio, IRQ cadence or hardware claim; CCR and temporary stack outside prematch ABI.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} match HUD cases')

if __name__=='__main__':main()
