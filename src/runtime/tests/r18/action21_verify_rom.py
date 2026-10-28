#!/usr/bin/env python3
"""Raw21 versus unchanged ROM; natural human in-ring action at frame1921.

Catches missing ownership, WORD profile guard mistaken for BYTE, motion
initialization, FE BYTE release, facing WORD copy and original-descriptor
ticks. There are no helper calls, ROM interceptions or patched ROM bytes.
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

ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path);parser.add_argument('--rom-only',action='store_true');parser.add_argument('--snapshot-only',action='store_true')
parser.add_argument('--actions-source',type=Path,help='Separate complete actions source for isolated validation; never edits production')
args=parser.parse_args();manifest=json.loads((HERE/'action21_manifest.json').read_text())
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
assert rom[0x12614+0x21*4:0x12618+0x21*4]==bytes.fromhex('00015b24')
assert rom[0x15b24:0x15b34]==bytes.fromhex('00015b3400010002000e001800b400b5')
data=(HERE/'action21_snapshot_ring_entry.bin').read_bytes();assert hashlib.sha256(data).hexdigest()==manifest['sha256']
assert len(data)==16472 and struct.unpack_from('=6I',data)==tuple(manifest['header'])
actors=[0x1c05b0+0x10c*i for i in range(9)];rng=random.Random(0x15b34);coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    return ('real-frame1921-slot0',mem,0,struct.unpack_from('=8I',data,24))

def seed(slot=0):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    a=actors[slot];put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x21)
    return mem,a

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,latched,frame,timer in itertools.product((0,1,4,5,8),(False,True),(0,1,2,0xfe,0x100,0x101,0x1fe,0xffff),(0,1,24,0xff00,0xffff)):
        mem,a=seed(slot);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x24,frame);put(mem,a+0x22,timer)
        yield (f'phase-{slot}-{latched}-{frame:x}-{timer:x}',mem,slot)
    for facing,link,flags18,health in itertools.product((0,0x8000,0x0080,0x8101,0xffff),(0,0x80000000,actors[4],0x801c09e0,0xdeadbeef),(0,0x80,0xff),(0,1,0xffff)):
        mem,a=seed();put(mem,a+0x24,0x1fe);put(mem,a+0x22,0);put(mem,a+0x2e,facing)
        put(mem,a+0x26,link,4);put(mem,a+0x18,flags18,1);put(mem,a+0x66,health)
        yield (f'release-{facing:x}-{link:x}-{flags18}-{health}',mem,0)
    for high,frame,profile,motion in itertools.product((0,0x4000,0x8000,0xc000),(0,1,0x101),(0,8,0xffff),(0,1,0xff)):
        mem,a=seed();put(mem,a+0x60,high|0x21);put(mem,a+0x24,frame);put(mem,a+0x22,1)
        put(mem,a+0x4c,profile);put(mem,a+1,motion,1)
        yield (f'word-profile-{high:x}-{frame}-{profile}-{motion}',mem,0)
    mem,a=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,0)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
    for _ in range(1000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split();actions=args.actions_source or ROOT/'src/wf_r17_actions_r11.c';actions_hash=hashlib.sha256(actions.read_bytes()).hexdigest()
if args.actions_source:core=[str(args.actions_source.resolve()) if f=='src/wf_r17_actions_r11.c' else f for f in core]
with tempfile.TemporaryDirectory(prefix='wf-action21-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action21.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action21_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action21_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action21_oracle(nb,nr,slot)
        assert status==0,(name,'native action21 blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(100):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x21:break
    required=(0x15b34,0x15b3c,0x15b42,0x15b4a,0x15b50,0x15b58,0x15b5e,0x15b64,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc) for pc in required if pc not in coverage])
    assert 0x21b4 not in coverage and 0x2052 not in coverage
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':manifest['rom_sha1'],'snapshot_sha256':manifest['sha256'],'actions_source_sha256':actions_hash,'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','intercepted_rom_owners':[],'helper_calls':[],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['Selector21 only; natural isolated callback ticks are separate from the complete controlled fight','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action21 fixtures; {continuations} natural ticks; {len(required)} sites; RAM/FG/D0-D7/RNG')
