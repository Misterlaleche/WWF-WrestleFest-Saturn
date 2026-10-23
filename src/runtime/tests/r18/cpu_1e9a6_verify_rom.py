#!/usr/bin/env python3
"""CPU $1E9A6: unchanged arcade ROM versus the real bounded C dispatcher.

The ROM image is never patched. Only sound output $2052 may be intercepted.
RAM $1C0000..$1C1FFF, foreground RAM and D0-D7 must match byte for byte.
--rom-only records branch evidence before the missing production case exists.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('cpu_1e9a6_snapshot_f88acad.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1e9cc:0x1e9d2]==bytes.fromhex('7409600006b4')
assert rom[0x1e070:0x1e074]==bytes.fromhex('8001ea2e')
rng=random.Random(0x1e9a6)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def seed(slot):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i);put(mem,at+0x57,i,1)
        put(mem,at+6,0x250+12*i);put(mem,at+10,0x150+3*i)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x7a,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4)
        for off in (0x32,0x33,0x34,0x35,0x56,0xb5,0xb6,0xb7,0xb9):put(mem,at+off,0,1)
        put(mem,at+0x70,0);put(mem,at+0x66,100)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,a+0x20,0x8005);put(mem,a+0x60,0x4016);put(mem,a+0x57,2,1)
    put(mem,a+0xbe,int.from_bytes(mem[a+6:a+8],'big'));put(mem,a+0xc0,int.from_bytes(mem[a+10:a+12],'big'))
    put(mem,a+0xbc,1);put(mem,a+0xb0,1);put(mem,a+0x100,0x18)
    put(mem,t+0x20,0x8004);put(mem,t+0x64,8);put(mem,t+0x26,0,4)
    put(mem,a+0x103,0,1);put(mem,a+0x31,4,1)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c0166,0)
    put(mem,0x1c16a0,0x10);put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t,p

def fixtures():
    # Natural failure first: controller16 arrival timer expires at2143.
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('cpu_1e9a6_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and kind==0 and pc==0x1dfe2
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert mem[actors[slot]+0x61]==0x16
    yield (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))
    for ident,face,lane,tier,clock,rep in itertools.product(range(12),(0,0x8000),(0,2,3,8,9,0xff),(0,1,2),(0xf,0x10),(0,1)):
        slot=(ident+rep)%9;mem,a,t,p=seed(slot)
        put(mem,a+0x57,ident,1);put(mem,a+0x2e,face);put(mem,a+0x31,lane,1);put(mem,t+0x70,tier);put(mem,0x1c16a0,clock)
        yield (f'weighted9-{ident}-{face:x}-{lane}-{tier}-{clock}-{rep}',mem,slot)
    for ident,index,credit,mode,weaker,defender in itertools.product((0,1),range(6),(0,1),(0,1),(False,True),(0,0x80)):
        mem,a,t,p=seed(4);put(mem,a+0x57,ident,1);put(mem,a+0x103,index,1)
        put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        put(mem,t+0x66,99 if weaker else 100);put(mem,a+0xb5,defender,1)
        yield (f'arbitrate9-{ident}-{index}-{credit}-{mode}-{weaker}-{defender}',mem,4)
    # Controller16 entry, wait, timer expiry, arrival and tagged-partner guards.
    for b6,selector,arrived,timer,active,partner_cpu in itertools.product((0,8),(0x16,0x4016),(False,True),(0,1,2),(0,1),(0,1)):
        mem,a,t,p=seed(4);put(mem,a+0xb6,b6,1);put(mem,a+0x60,selector);put(mem,a+0xbc,timer)
        put(mem,a+0x33,active,1);put(mem,p+0x32,partner_cpu,1)
        if not arrived:put(mem,a+0xbe,0x100);put(mem,a+0xc0,0x100)
        yield (f'controller16-{b6}-{selector}-{arrived}-{timer}-{active}-{partner_cpu}',mem,4)
    for selector in (0x16,0x4016):
        for x in (0x307,0x308,0x309):
            mem,a,t,p=seed(4);put(mem,a+0x57,7,1);put(mem,0x1c007c,0);put(mem,a+6,x);put(mem,a+0x60,selector)
            yield (f'special7-{selector}-{x}',mem,4)
    for mode,global166 in ((2,0),(0,1)):
        mem,a,t,p=seed(4);put(mem,a+0x60,0x4015);put(mem,0x1c0161,mode,1);put(mem,0x1c0166,global166)
        yield (f'controller15-{mode}-{global166}',mem,4)
    # Regression of the extracted bank1 owner, reached from mainstate12.
    for ident,face,lane,tier in itertools.product(range(12),(0,0x8000),(2,3,9),(0,1,2)):
        mem,a,t,p=seed(1);put(mem,a+0x20,0x800c);put(mem,a+0xbd,0x30,1)
        put(mem,a+0x57,ident,1);put(mem,a+0x2e,face);put(mem,a+0x31,lane,1);put(mem,t+0x70,tier)
        yield (f'bank1-regression-{ident}-{face:x}-{lane}-{tier}',mem,1,[rng.getrandbits(32) for _ in range(8)],0x1f06a)

def oracle(mem,slot,regs,name,entry=0x1dfe2):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
    memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000)
    memory.w32(0x1d0000,0x70000);cpu.w_pc(entry)
    for _ in range(10000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x2052:
            cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    out=[bytes(memory.r_block(at,n)) for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000))]
    out_regs=[cpu.r_reg(i) for i in range(8)]
    machine.cleanup()
    return out,out_regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-cpu_1e9a6-') as temp:
    if not args.rom_only:
        library=Path(temp)/'cpu_1e9a6.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_1e9a6_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library))
        native.wf_cpu_1e9a6_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3]
        if args.snapshot_only and not name.startswith('real-'):continue
        count+=1
        regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        entry=case[4] if len(case)>4 else 0x1dfe2
        want,want_regs=oracle(mem,slot,regs,name,entry)
        if args.rom_only:continue
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
        status=native.wf_cpu_1e9a6_oracle(nb,nr,slot)
        assert status==0,(name,'native cpu_1e9a6 blocked',hex(-status))
        for (at,n),expected in zip(((0x1c0000,0x2000),(0xc0000,0x4000)),want):
            got=nb.raw[at:at+n]
            assert got==expected,(name,'memory',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x1e9a6,0x1e9cc,0x1f084,0x1f7f0,0x1f860,0x1f83c,0x1f12a,0x1f094,0x1f0aa,0x1f0ce,0x1f0e8,0x1f0de,0x1f106,0x1f118,0x24cc,0x21b4,0x1f130,0x1f142,0x1eaae,0x1eb12,0x1eb3c,0x1eb50,0x1eb5e,0x1f06a)
    if args.snapshot_only:required=(0x1e9a6,0x1e9cc,0x1f084,0x1f118,0x1f130)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot) if args.snapshot and args.snapshot.exists() else None,'compared_memory':[] if args.rom_only else ['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':None if args.rom_only else 'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Controller15/16 paths reaching1E9A6 and extracted bank1 regression','Weighted target tier0..2','Arbitration bank0 covered for internalIDs0/1; unchanged arbitration owner has its existing bounded data contract','No gameplay owners intercepted']}
    if args.snapshot:report['snapshot_sha256']=hashlib.sha256(args.snapshot.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} cpu_1e9a6 unmodified-ROM fixtures; {len(required)} required branch sites')
