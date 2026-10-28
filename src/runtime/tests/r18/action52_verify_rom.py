#!/usr/bin/env python3
"""Raw $52 victim: unchanged ROM versus the real bounded native dispatcher.

Catches wrong substate parity/descriptor, recovery and character offset,
partner guards, mash sentinel, frame-byte FE restart, and lost D1 upper bits.
Only the sound device $2052 may be intercepted; gameplay executes intact.
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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action52_snapshot_after1c.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x52*4:0x12618+0x52*4]==bytes.fromhex('00018d12')
assert rom[0x18d12:0x18d2e]==bytes.fromhex('00018d2e00010002001600160168016900018d2e000100010008021b')
assert rom[0x18d44:0x18d4c]==bytes.fromhex('00018d4c00018e0c')
assert rom[0x18dfa:0x18e0c]==bytes.fromhex('000001030000030204010101d8d8e0d4d8ff')
rng=random.Random(0x18d2e)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action52_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(3386,4,1,0x1c03e,1032,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x52
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i);put(mem,at+6,0x250+12*i);put(mem,at+10,0x150+3*i)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x70,i%3)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x52)
    put(mem,a+0x24,0);put(mem,a+0x22,0);put(mem,a+0x44,0);put(mem,a+0x2e,0)
    put(mem,a+0xaa,0);put(mem,a+0xa3,0,1)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1)
    return mem,a,t,p

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for ident,face,selector,tier,recovery in itertools.product(range(12),(0,0x8000),(0x52,0x4052,0x8052,0xc052),(0,1,2),(0,1,0x4000,0x8000,0xffff)):
        slot=ident%9;mem,a,t,p=seed(slot);put(mem,a+0x1c,5);put(mem,a+2,ident);put(mem,a+0x2e,face)
        put(mem,a+0x60,selector);put(mem,a+0x70,tier);put(mem,a+0xaa,recovery)
        yield (f'init-{ident}-{face:x}-{selector:x}-{tier}-{recovery}',mem,slot)
    for sub,latched,frame,timer,count,role,recovery,mash in itertools.product((0,1,2,3,0xffff),(False,True),(0,1,0xfe,0x100,0x1fe),(0,1,0xff00),(0,0xffff),(0,0x40,0x80,0xc0),(0,1,0x4000,0xffff),(0,1)):
        mem,a,t,p=seed();put(mem,a+0x44,sub);put(mem,a+0x1c,0x8005 if latched else 5)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x52,count)
        put(mem,a+0x56,role,1);put(mem,a+0xaa,recovery);put(mem,a+0xa3,mash,1)
        yield (f'phase-{sub}-{latched}-{frame}-{timer}-{count}-{role}-{recovery}-{mash}',mem,4)
    for credit,mode,active,targetcpu,pcpu,phuman,apron,tier in itertools.product((0,1),(0,1),(0,1),(0,1),(0,1),(0,2),(0,0x80),(0,1,2)):
        mem,a,t,p=seed();put(mem,a+0x1c,5);put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        put(mem,a+0x33,active,1);put(mem,t+0x32,targetcpu,1);put(mem,p+0x32,pcpu,1)
        put(mem,p+0x33,phuman,1);put(mem,p+0x74,apron,1);put(mem,p+0x70,tier)
        yield (f'partner-{credit}-{mode}-{active}-{targetcpu}-{pcpu}-{phuman}-{apron}-{tier}',mem,4)
    for rejected in ('none','disabled','b5bit3','b5bit7','flags32bit4','other-team','far','target','self','tie'):
        mem,a,t,p=seed(5);put(mem,a+0x1c,5);put(mem,0x1c0161,1,1)
        for c in actors[4:]:put(mem,c,0)
        put(mem,a,0x8000);put(mem,actors[4],0x8000)
        put(mem,actors[4]+6,0x260);put(mem,actors[4]+10,0x150)
        if rejected=='disabled':put(mem,actors[4],0)
        if rejected=='b5bit3':put(mem,actors[4]+0xb5,8,1)
        if rejected=='b5bit7':put(mem,actors[4]+0xb5,0x80,1)
        if rejected=='flags32bit4':put(mem,actors[4]+0x32,0x10,1)
        if rejected=='other-team':put(mem,actors[4]+0x56,0x20,1)
        if rejected=='far':put(mem,actors[4]+6,0x7000)
        if rejected=='target':put(mem,a+0x26,actors[4],4)
        if rejected=='self':put(mem,actors[4],0)
        if rejected=='tie':put(mem,actors[7],0x8000);mem[actors[7]+6:actors[7]+12]=mem[actors[4]+6:actors[4]+12]
        yield (f'royal-{rejected}',mem,5)
    # Only init of the even route dereferences dispatcher A2.
    for sub,latched,link in itertools.product((0,1,0xffff),(False,True),(0,0x80000000)):
        if sub==0 and not latched:continue
        mem,a,t,p=seed();put(mem,a+0x44,sub);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x26,link,4)
        yield (f'released-{sub}-{latched}-{link:x}',mem,4)
    mem,a,t,p=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,4)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
    for _ in range(20000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-action52-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action52.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action52_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action52_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action52_oracle(nb,nr,slot)
        assert status==0,(name,'native action52 blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(160):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x52:break
    required=(0x18d2e,0x18d4c,0x18d54,0x18d5c,0x18d96,0x18da4,0x18db6,0x18dca,0x18dda,0x18dec,0x18df2,0x18e0c,0x18e14,0x18e2a,0x18e34,0x10c6e,0x10ca8,0x10d12,0x10d28,0x10d2e,0x2103e,0x2106e,0x21086,0x2109e,0x210b4,0x210e0,0x210ee,0x210fe,0x21936,0x2195c,0x21970,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x18d2e,0x18d54,0x18db6,0x18dca,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $52 only; dereferenced gameplay pointers identify actors; character0..11 and partner tier0..2','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action52 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
