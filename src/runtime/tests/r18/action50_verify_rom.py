#!/usr/bin/env python3
"""Raw $50: unchanged arcade ROM versus the bounded native dispatcher.

Catches missing ownership, kind-zero immediate pair release, facing-dependent X offsets,
full target subtype WORD, facing BYTE, link release bits and preserved animation.
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
import tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action50_snapshot_after05_active.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x50*4:0x12618+0x50*4]==bytes.fromhex('00018b8e')
assert rom[0x18b8e:0x18b94]==bytes.fromhex('00018b940000')
rng=random.Random(0x18b94)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action50_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(3047,4,1,0x1c03e,1052,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x50
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8001);put(mem,at+2,i);put(mem,at+6,0x250);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x2e,0)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9];q=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x50);put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,t+0x26,a,4);put(mem,t+0x86,q,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c17e6,0);put(mem,0x1c17ee,0)
    for k in range(6):put(mem,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
    for i in range(11):mem[0x1c1258+i*0x2a]|=0x80
    return mem,a,t,p,q

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,latched,selector,frame,timer,face,tface in itertools.product(range(9),(False,True),(0x50,0x4050,0x8050,0xc050),(0,3,0xfe),(0,1,0xff00),(0,0x8000),(0,0x8000)):
        mem,a,t,p,q=seed(slot);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x60,selector)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x2e,face);put(mem,t+0x2e,tface)
        yield (f'phase-{slot}-{latched}-{selector:x}-{frame}-{timer}-{face:x}-{tface:x}',mem,slot)
    # Whole WORD64/visual writes, signed X wrap, byte mask preservation,
    # and sequential owner/target writes when the same actor aliases A2.
    for x,tx,face,flags,self_link in itertools.product((0,31,32,0xffff),(0,31,32,0xffff),(0,0x8000,0x7f24,0xff3a),(0,0x40,0xff),(False,True)):
        mem,a,t,p,q=seed();put(mem,a+6,x);put(mem,t+6,tx);put(mem,a+0x2e,face)
        put(mem,t+0x2e,face^0x8000);put(mem,a+0x33,flags,1);put(mem,t+0x33,flags,1);put(mem,t+0x64,0xfff0)
        if self_link:put(mem,a+0x26,a,4)
        yield (f'position-{x}-{tx}-{face:x}-{flags}-{self_link}',mem,4)
    # Null dispatcher links map to read-only ROM; no RAM target receives
    # their writes. Tagged physical zero follows the same 24-bit bus rule.
    for latched,link,face in itertools.product((False,True),(0,0x80000000,0xff000000),(0,0x8000)):
        mem,a,t,p,q=seed();put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x26,link,4);put(mem,a+0x2e,face)
        yield (f'null-target-{latched}-{link:x}-{face:x}',mem,4)
    mem,a,t,p,q=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,4)

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
with tempfile.TemporaryDirectory(prefix='wf-action50-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action50.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action50_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action50_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs,want_descriptor=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));descriptor=ctypes.c_uint32();status=native.wf_action50_oracle(nb,nr,slot,ctypes.byref(descriptor))
        assert status==0,(name,'native action50 blocked',hex(-status))
        assert descriptor.value==want_descriptor,(name,'tick descriptor',hex(descriptor.value),hex(want_descriptor))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(160):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x50:break
    else:raise AssertionError('natural raw50 failed to release within160 ticks')
    assert mem[actors[slot]+0x21]==0
    required=(0x18b94,0x18b9a,0x18ba0,0x18ba6,0x18bb2,0x18bc0,0x18bc6,0x18bce,0x18bd4,0x18bda,0x18be0,0x18be6,0x18bec,0x1c0cc,0x1c0ba)
    if args.snapshot_only:required=(0x18b94,0x18ba0,0x18bce,0x18bda,0x1c0cc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','compared_tick_descriptor':'ROM A1 at $1C0C8','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $50 only; dereferenced targets identify actors or known read-only null bus aliases','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action50 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
