#!/usr/bin/env python3
"""Raw $79: unchanged arcade ROM versus the bounded native dispatcher.

Catches missing ownership, repeat initialization/underflow, character-specific descriptor selection,
BYTE FE versus WORD frame, and the exit retaining the original descriptor.
Only sound device $2052 is intercepted; every gameplay owner executes intact.
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
import sys
import tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action79_snapshot_afterflag34.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x79*4:0x12618+0x79*4]==bytes.fromhex('0001a1be')
assert rom[0x1a1be:0x1a1e6]==bytes.fromhex('0001a28e00010008000600060006000600060006001000300059005a005b0059005a005b005c005d')
assert rom[0x1a33c:0x1a348]==bytes.fromhex('010202010101010101040201')
assert [int.from_bytes(rom[0x1a30c+4*i:0x1a310+4*i],'big') for i in range(12)]==[0x1a1be,0x1a1e6,0x1a1f6,0x1a20e,0x1a20e,0x1a20e,0x1a21e,0x1a22e,0x1a24a,0x1a25e,0x1a26e,0x1a27e]
rng=random.Random(0x1a28e)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action79_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(5677,1,1,0x1c03e,1529,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x79
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8001);put(mem,at+2,i);put(mem,at+6,0x250);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x2e,0)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9];q=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x79);put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,t+0x26,a,4);put(mem,t+0x86,q,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c17e6,0);put(mem,0x1c17ee,0)
    for k in range(6):put(mem,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
    for i in range(11):mem[0x1c1258+i*0x2a]|=0x80
    return mem,a,t,p,q

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for ident,latched,selector,frame,timer,count in itertools.product(range(12),(False,True),(0x79,0x4079,0x8079,0xc079),(0,1,7,0xfe,0x100,0x1fe),(0,1,0xff00),(0,1,2,0xffff)):
        slot=ident%9;mem,a,t,p,q=seed(slot);put(mem,a+2,ident);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x60,selector)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x44,count)
        yield (f'phase-{ident}-{latched}-{selector:x}-{frame}-{timer}-{count}',mem,slot)
    # Load every character descriptor entry and both facing variants; this
    # catches a wrong literal pose/duration in any of the longer animations.
    for ident,frame,face in itertools.product(range(12),range(9),(0,0x8000)):
        mem,a,t,p,q=seed();put(mem,a+2,ident);put(mem,a+0x24,frame);put(mem,a+0x22,0);put(mem,a+0x2e,face)
        yield (f'descriptor-{ident}-{frame}-{face:x}',mem,1)
    for ident,face,flags,timer in itertools.product(range(12),(0,0x8000,0x7f24,0xff3a),(0,0x10,0xff),(0,1,0xff00)):
        mem,a,t,p,q=seed();put(mem,a+2,ident);put(mem,a+0x24,0xfe);put(mem,a+0x44,1);put(mem,a+0x2e,face);put(mem,a+0x34,flags,1);put(mem,a+0x22,timer)
        yield (f'finish-default-{ident}-{face:x}-{flags}-{timer}',mem,1)
    for latched,frame,link in itertools.product((False,True),(0,0xfe,0x1fe),(0,0x80000000)):
        mem,a,t,p,q=seed();put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x24,frame);put(mem,a+0x26,link,4)
        yield (f'released-{latched}-{frame}-{link:x}',mem,1)
    mem,a,t,p,q=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,1)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem));tick_descriptor=0
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
    for _ in range(20000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x1c0c8:tick_descriptor=cpu.r_reg(9)
        if pc==0x70000:break
        if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,regs,tick_descriptor

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-action79-',ignore_cleanup_errors=True) as temp:
    if not args.rom_only:
        library=Path(temp)/'action79.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action79_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action79_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs,want_descriptor=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));descriptor=ctypes.c_uint32();status=native.wf_action79_oracle(nb,nr,slot,ctypes.byref(descriptor))
        assert status==0,(name,'native action79 blocked',hex(-status))
        assert descriptor.value==want_descriptor,(name,'tick descriptor',hex(descriptor.value),hex(want_descriptor))
        if name.startswith('finish-default-'):assert want_descriptor==0x1a1be
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(160):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x79:break
    else:raise AssertionError('natural raw79 failed to release within160 ticks')
    assert mem[actors[slot]+0x21]==0
    required=(0x1a28e,0x1a296,0x1a29e,0x1a2a2,0x1a2ae,0x1a2b2,0x1a2c2,0x1a2c8,0x1a2d0,0x1a2d8,0x1a2de,0x1a2e6,0x1a2f2,0x1a2fa,0x1a306,0x1c0c8,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x1a28e,0x1a29e,0x1a2c2,0x1a2d8,0x1a2de,0x1a2e6,0x1c0c8)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','compared_tick_descriptor':'ROM A1 at $1C0C8','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $79 only; characters0..11; no gameplay link dereferenced','No Saturn hardware timing or audible sound validation']}
    report['action_source_sha256']=hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
    report['verifier_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action79 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
    if not args.rom_only and sys.platform=='win32':
        assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
