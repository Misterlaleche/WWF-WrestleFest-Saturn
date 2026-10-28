#!/usr/bin/env python3
"""State4 $1BE7C: unchanged arcade ROM versus the real bounded C dispatcher.

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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('state4_1be7c_snapshot_after34.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1afd4+0x23*4:0x1afd8+0x23*4]==bytes.fromhex('0001be6c')
assert rom[0x1be6c:0x1be7c]==bytes.fromhex('0001be7c00010002000cff0000b200b3')
assert rom[0x25ca+0x1a*6:0x25ca+0x1b*6]==bytes.fromhex('ff0001000058')
rng=random.Random(0x1be7c)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def seed(slot):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    a=actors[slot]
    put(mem,a+0x20,4);put(mem,a+0x1c,0x8004);put(mem,a+0x64,0x2023)
    put(mem,a+0x24,0);put(mem,a+0x22,0xff00);put(mem,a+0x36,0)
    put(mem,0x1c0161,0,1)
    return mem,a

def fixtures():
    for slot,latched,face,high,land,health,timer,index in itertools.product((0,4,8),(False,True),(0,0x8000,0x7f24,0xff3a),(0,0x20,0x80,0xff),(0,16),(0,1,42),(0,1,0xff00),(0,1,0x100,0x101,0xfe)):
        mem,a=seed(slot);put(mem,a+0x1c,0x8004 if latched else 4);put(mem,a+0x2e,face)
        put(mem,a+0x64,(high<<8)|0x23);put(mem,a+0x37,land,1);put(mem,a+0x66,health)
        put(mem,a+0x22,timer);put(mem,a+0x24,index)
        yield (f'tick-{slot}-{latched}-{face:x}-{high}-{land}-{health}-{timer}-{index}',mem,slot)
    for mode,contact,bits,cache in itertools.product((0,1,2,3),(0,1,3,5),(0,1,2,3,16,17,18,19),(0,1,0x100)):
        mem,a=seed(1);put(mem,0x1c0161,mode,1);put(mem,a+0x36,(contact<<8)|bits)
        put(mem,0x1c1150,cache);put(mem,0x1c11b0,cache)
        yield (f'edge-{mode}-{contact}-{bits}-{cache}',mem,1)
    for x,y,z,face in itertools.product((0,1,0x7fff,0xffff),(0,0xffff),(0,0xffff),(0,0x8000)):
        mem,a=seed(1);put(mem,a+6,x);put(mem,a+10,y);put(mem,a+14,z);put(mem,a+0x2e,face);put(mem,a+0x37,16,1)
        yield (f'position-{x}-{y}-{z}-{face:x}',mem,1)
    mem,a=seed(1);put(mem,a+0x20,0xff)
    yield ('state-ff',mem,1)
    if args.snapshot:
        data=args.snapshot.read_bytes()
        manifest=json.loads(args.snapshot.with_name('state4_1be7c_manifest.json').read_text())
        assert hashlib.sha256(data).hexdigest()==manifest['sha256']
        frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
        assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(5091,1,1,0x1c03e,1393,1)
        mem=bytearray(0x200000);mem[:len(rom)]=rom
        mem[0x1c0000:0x1c2000]=data[56:8248]
        a=actors[slot];assert mem[a+0x1d]==4 and mem[a+0x65]==0x23
        regs=struct.unpack_from('=8I',data,24)
        yield (f'real-frame{frame}-slot{slot}',bytearray(mem),slot,regs)
        # Isolated callbacks wait for physics contact; retain the untouched
        # natural RAM/register stream for 120 additional owner calls.
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
with tempfile.TemporaryDirectory(prefix='wf-state4_1be7c-') as temp:
    if not args.rom_only:
        library=Path(temp)/'state4_1be7c.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/state4_1be7c_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library))
        native.wf_state4_1be7c_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
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
        status=native.wf_state4_1be7c_oracle(nb,nr,slot)
        assert status==0,(name,'native state4_1be7c blocked',hex(-status))
        for (at,n),expected in zip(((0x1c0000,0x2000),(0xc0000,0x4000)),want):
            got=nb.raw[at:at+n]
            assert got==expected,(name,'memory',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x1be7c,0x1be84,0x1be90,0x1be9e,0x1bea4,0x1bebc,0x1bec4,0x258e,0x25c0,0x10fc6,0x10fd8,0x10ff0,0x10ff8,0x11012,0x1101a,0x1102c,0x10bd8,0x10bda,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x1be7c,0x1be84,0x1be90,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'natural_continuations':120,'passed':0 if args.rom_only else count,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot) if args.snapshot and args.snapshot.exists() else None,'compared_memory':[] if args.rom_only else ['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':None if args.rom_only else 'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['State4subtypeLOW23 only; no gameplay owner intercepted; RNG state RAM compared unchanged']}
    if args.snapshot:report['snapshot_sha256']=hashlib.sha256(args.snapshot.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} state4_1be7c unmodified-ROM fixtures; {len(required)} required branch sites')
