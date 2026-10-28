#!/usr/bin/env python3
"""Raw75: intact arcade ROM versus bounded native dispatcher.

Catches missing owner, motion-dependent descriptors, first-entry44 clear,
rope helper mode/byte guards, priority of left/right contact, FE return
states, original-descriptor terminal ticks and register/RAM corruption.
Sound2052 is the only intercepted ROM owner; no ROM bytes are changed.
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
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--snapshot',type=Path,default=HERE/'action75_snapshot_late_pad.bin')
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--report',type=Path)
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
parser.add_argument('--actions-source',type=Path,help='Validate a separate complete actions C source without production edits')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x75*4:0x12618+0x75*4]==bytes.fromhex('0001a08c')
assert rom[0x1a08c:0x1a0ac]==bytes.fromhex('0001a0ac00010001002800680001a0ac0001000300060006000601880189018a')
rng=random.Random(0x1a0ac)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads((HERE/'action75_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    assert len(data)==16472 and struct.unpack_from('=6I',data)==(2028,4,1,0x1c03e,756,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert mem[actors[4]+0x60:actors[4]+0x62]==bytes.fromhex('0075')
    return ('real-frame2028-slot4',mem,4,struct.unpack_from('=8I',data,24))

def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    a=actors[slot];put(mem,a+0x1c,0x8005);put(mem,a+0x20,5);put(mem,a+0x60,0x75)
    put(mem,a+1,0,1);put(mem,a+0x24,0);put(mem,a+0x22,0);put(mem,a+0x36,0)
    put(mem,0x1c0161,0,1);put(mem,a+0x26,0,4)
    return mem,a

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,motion,selector,facing in itertools.product((0,1,4,5,8),(0,1,2,0xff),(0x75,0x4075,0x8075,0xc075),(0,0x8000,0x8101)):
        mem,a=seed(slot);put(mem,a+0x1c,5);put(mem,a+1,motion,1);put(mem,a+0x60,selector);put(mem,a+0x2e,facing)
        put(mem,a+0x44,0xffff)
        yield (f'init-{slot}-{motion}-{selector:x}-{facing:x}',mem,slot)
    # Contact high BYTE controls the category, contact low BYTE bits select
    # ropes; bit1 wins when both bits are set. Referee guards read low BYTE.
    for mode,contact,bits,left,right,motion in itertools.product((0,1,2,3),(0,1,3,5,0xff),(0,1,2,3,0x83),(0,1,0xff00),(0,1,0xff00),(0,1)):
        mem,a=seed();put(mem,0x1c0161,mode,1);put(mem,a+0x36,(contact<<8)|bits)
        put(mem,0x1c1150,left);put(mem,0x1c11b0,right);put(mem,a+1,motion,1)
        yield (f'edge-{mode}-{contact}-{bits}-{left}-{right}-{motion}',mem,4)
    for frame,timer,motion,contact in itertools.product((0,1,2,4,0xfe,0x100,0x101,0x104,0x1fe),(0,1,0xff00,0xffff),(0,1,0xff),(0,0x0300,0x0102)):
        mem,a=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+1,motion,1);put(mem,a+0x36,contact)
        yield (f'phase-{frame}-{timer}-{motion}-{contact:x}',mem,4)
    # Neither callback nor the sound helper dereferences dispatcher A2.
    for latched,link,flags18 in itertools.product((False,True),(0,0x80000000,0xdeadbeef),(0,0x80,0xff)):
        mem,a=seed();put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x26,link,4);put(mem,a+0x18,flags18,1)
        yield (f'no-target-{latched}-{link:x}-{flags18}',mem,4)
    mem,a=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,4)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
    for _ in range(1000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
actions_path=args.actions_source or ROOT/'src/wf_r17_actions_r11.c'
actions_hash=hashlib.sha256(actions_path.read_bytes()).hexdigest()
if args.actions_source:core=[str(args.actions_source.resolve()) if f=='src/wf_r17_actions_r11.c' else f for f in core]
with tempfile.TemporaryDirectory(prefix='wf-action75-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action75.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action75_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action75_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action75_oracle(nb,nr,slot)
        assert status==0,(name,'native action75 blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(100):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x75:break
    required=(0x1a0ac,0x1a0b4,0x1a0ba,0x1a0c2,0x1a0ca,0x1a0d8,0x1a0e0,0x1a0e8,0x1a0ee,0x1a0f6,0x10fc6,0x10fe8,0x10ff0,0x11002,0x1100a,0x11012,0x11024,0x1102c,0x11052,0x1112e,0x11146,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x1a0ac,0x1a0b4,0x1a0ba,0x1a0d8,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branches',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'actions_source_sha256':actions_hash,'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','intercepted_rom_owners':['$2052 sound output only'],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['Selector75 only; isolated callback continuation does not establish full-match outcome','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action75 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} branch sites; RAM/FG/D0-D7/RNG')
