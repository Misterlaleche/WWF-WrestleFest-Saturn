#!/usr/bin/env python3
"""Raw $5D: unchanged arcade ROM versus the real bounded C dispatcher.

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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action5d_snapshot_after09.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12788:0x1278c]==bytes.fromhex('00019530')
assert rom[0x19530:0x19536]==bytes.fromhex('000195360000')
# Both selectors use the same callback and the same recovery argument4.
assert rom[0x12790:0x12794]==rom[0x12788:0x1278c]
assert rom[0x19566:0x1956a]==rom[0x19596:0x1959a]==bytes.fromhex('303c0004')
rng=random.Random(0x19536)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def seed(slot):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i)
        put(mem,at+6,0x250+12*i);put(mem,at+10,0x150+3*i)
        put(mem,at+0x26,actors[(i+1)%9],4)
        put(mem,at+0x7a,actors[(i+3)%9],4)
        put(mem,at+0x86,actors[(i+2)%9],4)
        for off in (0x32,0x33,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x70,i%3)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,5);put(mem,a+0x60,0x005d)
    put(mem,a+0x24,0x1234);put(mem,a+0x22,0x5678)
    put(mem,a+0x33,1,1);put(mem,a+0xa3,1,1)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1)
    return mem,a,t,p

def fixtures():
    # Every physical actor slot, selector high flags and recovery table tier.
    for slot,sel,tier in itertools.product(range(9),(0x5d,0x405d,0x805d,0xc05d),(0,1,2)):
        mem,a,t,p=seed(slot);put(mem,a+0x60,sel);put(mem,a+0x70,tier)
        yield (f'init-{slot}-{sel:x}-{tier}',mem,slot)
    # $21732 TAG routes: no credit, inactive owner, human/CPU partner,
    # human apron guard, CPU/human identity and all CPU timer table entries.
    for credit,active,cpu,human,apron,tier in itertools.product((0,1,0x8000),(0,1),(0,1),(0,2),(0,0x80),(0,1,2)):
        mem,a,t,p=seed(0)
        put(mem,0x1c007c,credit);put(mem,a+0x33,active,1)
        put(mem,p+0x32,cpu,1);put(mem,p+0x33,human,1)
        put(mem,p+0x74,apron,1);put(mem,p+0x70,tier)
        yield (f'tag-{credit}-{active}-{cpu}-{human}-{apron}-{tier}',mem,0)
    # Royal candidate search: each rejection, success, strict distance/tie,
    # target exclusion and same-team selection; every scanned slot is real.
    for rejected in ('none','disabled','b5bit3','b5bit7','flags32bit4','other-team','far','target','self','tie'):
        mem,a,t,p=seed(5);put(mem,0x1c0161,1,1)
        for c in actors[4:]:put(mem,c,0)
        put(mem,a,0x8000);put(mem,actors[4],0x8000)
        put(mem,actors[4]+6,0x260);put(mem,actors[4]+10,0x150)
        if rejected=='disabled':put(mem,actors[4],0)
        if rejected=='b5bit3':put(mem,actors[4]+0xb5,8,1)
        if rejected=='b5bit7':put(mem,actors[4]+0xb5,0x80,1)
        if rejected=='flags32bit4':put(mem,actors[4]+0x32,0x10,1)
        if rejected=='other-team':put(mem,actors[4]+0x56,0x20,1)
        if rejected=='far':put(mem,actors[4]+6,(int.from_bytes(mem[a+6:a+8],'big')+0x400)&0xffff);put(mem,actors[4]+10,int.from_bytes(mem[a+10:a+12],'big'))
        if rejected=='target':put(mem,a+0x26,actors[4],4)
        if rejected=='self':put(mem,actors[4],0)
        if rejected=='tie':
            put(mem,actors[7],0x8000)
            mem[actors[7]+6:actors[7]+12]=mem[actors[4]+6:actors[4]+12]
        yield (f'royal-{rejected}',mem,5)
    # $11432 restores partner release flags, with mode/role bypasses.
    # $10D04 mashing boundaries and terminal sentinel; kind0 never ticks.
    for mode,role,partner_flags,recovery,mash in itertools.product((0,1),(0,0x40,0x80,0xc0),(0,0x40,0x80,0xc0),(0,1,2,0x4000,0xffff),(0,1)):
        mem,a,t,p=seed(8);put(mem,a+0x1c,0x8005)
        put(mem,0x1c0161,mode,1);put(mem,a+0x56,role,1)
        put(mem,p+0x34,partner_flags,1);put(mem,a+0xaa,recovery)
        put(mem,a+0xa3,mash,1)
        yield (f'tick-{mode}-{role}-{partner_flags}-{recovery}-{mash}',mem,8)
    # Disabled current state exits before callback. Null/released gameplay
    # link is never dereferenced by $5D; partner pointers remain valid.
    for latched,state,link in itertools.product((False,True),(5,0xff),(0,0x80000000)):
        mem,a,t,p=seed(0);put(mem,a+0x1c,0x8005 if latched else 5)
        put(mem,a+0x20,state);put(mem,a+0x26,link,4)
        yield (f'link-{latched}-{state}-{link}',mem,0)
    if args.snapshot:
        data=args.snapshot.read_bytes()
        manifest=json.loads(args.snapshot.with_name('action5d_manifest.json').read_text())
        assert hashlib.sha256(data).hexdigest()==manifest['sha256']
        frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
        assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(4636,1,1,0x1c03e,1309,1)
        mem=bytearray(0x200000);mem[:len(rom)]=rom
        mem[0x1c0000:0x1c2000]=data[56:8248]
        a=actors[slot];assert mem[a+0x61]==0x5d
        regs=struct.unpack_from('=8I',data,24)
        yield (f'real-frame{frame}-slot{slot}',bytearray(mem),slot,regs)
        # Kind-zero raw5D remains held until another scheduler owner changes
        # it. Continue the original RAM/register stream through its live ticks.
        for tick in range(120):
            out,regs=oracle(mem,slot,regs,f'natural-predecessor-{tick}')
            mem[0x1c0000:0x1c2000]=out[0];mem[0xc0000:0xc4000]=out[1]
            yield (f'real-continuation-{tick}',bytearray(mem),slot,regs)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
    memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000)
    memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
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
with tempfile.TemporaryDirectory(prefix='wf-action5d-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action5d.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action5d_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library))
        native.wf_action5d_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3]
        if args.snapshot_only and not name.startswith('real-'):continue
        count+=1
        regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:continue
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
        status=native.wf_action5d_oracle(nb,nr,slot)
        assert status==0,(name,'native action5d blocked',hex(-status))
        for (at,n),expected in zip(((0x1c0000,0x2000),(0xc0000,0x4000)),want):
            got=nb.raw[at:at+n]
            assert got==expected,(name,'memory',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x19536,0x19540,0x19562,0x195ec,0x19600,0x2173a,0x21756,0x2176e,0x21786,0x2179c,0x217c8,0x217d6,0x217e4,0x217fe,0x21828,0x21936,0x2195c,0x21970,0x11448,0x11456,0x11466,0x10d12,0x10d28,0x10d2e,0x10ca8,0x1c0cc,0x1c0ba)
    if args.snapshot_only:required=(0x19536,0x19540,0x19562,0x10ca8)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':120,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot) if args.snapshot and args.snapshot.exists() else None,'compared_memory':[] if args.rom_only else ['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':None if args.rom_only else 'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Dereferenced partner pointers identify actors','Recovery and CPU partner tier0..2','Selector$5D only; no gameplay owners intercepted','No Saturn hardware timing or audible sound validation']}
    if args.snapshot:report['snapshot_sha256']=hashlib.sha256(args.snapshot.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action5d unmodified-ROM fixtures; {len(required)} required branch sites')
