#!/usr/bin/env python3
"""State4 $1B288: unchanged arcade ROM versus the real bounded C dispatcher.

The ROM image is never patched. No game or sound routines are intercepted; sound-device stores are outside comparison RAM.
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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('state4_1b288_snapshot_f88acad.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1b268:0x1b26c]==bytes.fromhex('0001b288')
assert rom[0x1b278:0x1b27c]==bytes.fromhex('0001b288')
assert [int.from_bytes(rom[0x1afd4+i*4:0x1afd8+i*4],'big') for i in (6,7)]==[0x1b268,0x1b278]
rng=random.Random(0x1b288)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

SNAPSHOT=args.snapshot.read_bytes()
assert hashlib.sha256(SNAPSHOT).hexdigest()==json.loads(args.snapshot.with_name('state4_1b288_manifest.json').read_text())['sha256']
def seed(slot,sub=6):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=SNAPSHOT[56:8248]
    a=actors[slot];put(mem,a+0x20,4);put(mem,a+0x1c,0x8004);put(mem,a+0x64,sub)
    return mem,a

def fixtures():
    for slot,sub,facing,scene in itertools.product(range(9),(6,7),(0,0x8000),(0,2)):
        mem,a=seed(slot,sub);put(mem,a+0x1c,4);put(mem,a+0x2e,facing);put(mem,0x1c007e,scene)
        yield (f'init-{slot}-{sub}-{facing}-{scene}',mem,slot)
    for sub,frame,timer,contact,role,mash,recovery in itertools.product((6,7),(0,1,0xfe,0x100),(0,1,0xff00),(0,0x10),(0,0x40,0x80,0xc0),(0,1),(0,1,0x4000)):
        mem,a=seed(0,sub);put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x37,contact,1);put(mem,a+0x56,role,1);put(mem,a+0xa3,mash,1);put(mem,a+0xaa,recovery)
        yield (f'tick-{sub}-{frame}-{timer}-{contact}-{role}-{mash}-{recovery}',mem,0)
    for sub,health,maximum,recovery in itertools.product((6,7),(0,1,27,28,54,55,109,110,111,0xffff),(0,1,110,0xffff),(0,1,2,0x4000,0x8000,0xffff)):
        mem,a=seed(0,sub);put(mem,a+0x1c,4);put(mem,a+0x66,health);put(mem,a+0x72,maximum);put(mem,a+0xaa,recovery)
        yield (f'recovery-seed-{sub}-{health}-{maximum}-{recovery}',mem,0)
    for sub,reciprocal,result in itertools.product((6,7),(False,True),(0,1,0x8000)):
        mem,a=seed(0,sub);put(mem,a+0x24,0xfe);put(mem,a+0x22,0xff00);put(mem,a+0xfe,result);put(mem,a+0x26,actors[5],4);put(mem,actors[5]+0x26,a if reciprocal else 0,4)
        yield (f'unlink-{sub}-{reciprocal}-{result}',mem,0)
    for sub,flags in itertools.product((6,7,0xab06,0xff07),(0x8000,0xffff)):
        mem,a=seed(0,sub);put(mem,a+0x24,0xfe);put(mem,a+0x22,0xff00);put(mem,a+0x18,flags);put(mem,a+0x1a,0x1234)
        yield (f'subtype-and-tail-width-{sub:x}-{flags:x}',mem,0)
    if args.snapshot:
        data=args.snapshot.read_bytes()
        manifest=json.loads(args.snapshot.with_name('state4_1b288_manifest.json').read_text())
        assert hashlib.sha256(data).hexdigest()==manifest['sha256']
        frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
        assert len(data)==16472 and kind and pc==0x1c03e
        mem=bytearray(0x200000);mem[:len(rom)]=rom
        mem[0x1c0000:0x1c2000]=data[56:8248]
        a=actors[slot];assert mem[a+0x1d]==4 and mem[a+0x65]==6
        yield (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

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
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    out=[bytes(memory.r_block(at,n)) for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000))]
    out_regs=[cpu.r_reg(i) for i in range(8)]
    machine.cleanup()
    return out,out_regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-state4_1b288-') as temp:
    if not args.rom_only:
        library=Path(temp)/'state4_1b288.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/state4_1b288_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library))
        native.wf_state4_1b288_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
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
        status=native.wf_state4_1b288_oracle(nb,nr,slot)
        assert status==0,(name,'native state4_1b288 blocked',hex(-status))
        for (at,n),expected in zip(((0x1c0000,0x2000),(0xc0000,0x4000)),want):
            got=nb.raw[at:at+n]
            assert got==expected,(name,'memory',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x1b288,0x1b290,0x1b29e,0x1b2a8,0x1b2b0,0x1b2c4,0x1b2d4,0x1b2e2,0x1b2f0,0x1b2f8,0x10cba,0x10d28,0x115d2,0x1c0cc,0x1c0ba)
    if args.snapshot_only:required=(0x1b288,0x1b290,0x1b29e,0x1b2a8,0x1c0cc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot) if args.snapshot and args.snapshot.exists() else None,'compared_memory':[] if args.rom_only else ['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':None if args.rom_only else 'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':[],'limitations':['State4subtypes06/07 only; no gameplay owner intercepted; RNG state RAM compared unchanged']}
    if args.snapshot:report['snapshot_sha256']=hashlib.sha256(args.snapshot.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} state4_1b288 unmodified-ROM fixtures; {len(required)} required branch sites')
