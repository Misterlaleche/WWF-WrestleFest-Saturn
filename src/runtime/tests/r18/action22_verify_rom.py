#!/usr/bin/env python3
"""Action $22 through the real dispatcher versus the unmodified arcade ROM.

The missing case, an incorrect acquisition/release, a dropped TAG partner
update, or a descriptor tick/register clobber must fail this comparison.
Only sound output $2052 is intercepted; no owner or branch is patched.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import random
import re
import subprocess
import struct
import tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action22_snapshot_a4be15f.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1269c:0x126a0]==bytes.fromhex('00015b6c')
rng=random.Random(0x15b8c)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def seed(slot):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[actors[0]:actors[0]+9*0x10c]=rng.randbytes(9*0x10c)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i)
        put(mem,at+6,0x250+12*i);put(mem,at+10,0x150+3*i)
        put(mem,at+0x26,actors[(i+1)%9],4)
        put(mem,at+0x86,actors[(i+2)%9],4)
        put(mem,at+0x70,i%3)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x4022)
    put(mem,a+0x24,1);put(mem,a+0x22,3);put(mem,a+0x2e,0x8000)
    put(mem,t+0x20,4);put(mem,t+0x64,8);put(mem,t+0x26,a,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1)
    put(mem,0x1c1682,0x1234);put(mem,0x1c15d2,0x5678)
    return mem,a,t,p

def fixtures():
    # Init and every animation transition, high selector flag and facing variants.
    for slot,sel,face,frame,timer in itertools.product((0,5,8),(0x22,0x4022,0x8022,0xc022),(0,0x8000),(0,1,2,3,4,5,0xfe),(0,1,2,0xff00)):
        mem,a,t,p=seed(slot)
        put(mem,a+0x60,sel);put(mem,a+0x2e,face)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer)
        put(mem,a+0xfe,0)
        if frame==0 and timer==2:put(mem,a+0x1c,5)
        yield (f'tick-{slot}-{sel:x}-{face:x}-{frame}-{timer}',mem,slot)
    # Acquisition must reject each bad state, substate, or reciprocal link.
    for state,sub,reciprocal in itertools.product((0,4,5,0x104),(0,8,9,0x108),(False,True)):
        mem,a,t,p=seed(5);put(mem,a+0x24,0);put(mem,a+0x22,0)
        put(mem,t+0x20,state);put(mem,t+0x64,sub)
        put(mem,t+0x26,a if reciprocal else p,4)
        yield (f'acquire-{state}-{sub}-{reciprocal}',mem,5)
    # First completion sets bit3 of the high selector byte. Include every
    # qualification boundary and partner route of $215B6.
    for credit,mode,flags,health,cpu,partner_flags,tier,locked in itertools.product(
            (0,1,0x8000),(0,1),(0,1,3,5),(0,1),(0,1),(0,2),(0,1,2),(False,True)):
        mem,a,t,p=seed(5)
        put(mem,a+0x24,0xfe);put(mem,a+0x22,0)
        put(mem,a+0x60,0x4022);put(mem,a+0xfe,0x8000 if locked else 0)
        put(mem,a+0x33,flags,1);put(mem,a+0x32,0,1)
        put(mem,t+0x33,1,1);put(mem,t+0x66,health)
        put(mem,p+0x32,cpu,1);put(mem,p+0x33,partner_flags,1);put(mem,p+0x70,tier)
        put(mem,p+0x74,0,1)
        put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        yield (f'complete-{credit}-{mode}-{flags}-{health}-{cpu}-{partner_flags}-{tier}-{locked}',mem,5)
    # TAG guards: a CPU owner skips partner activation; an apron partner
    # cannot receive the human rescue link. Existing selector bit7 skips
    # the complete partner helper while keeping the same animation tick.
    for owner_cpu,apron,selector in itertools.product((0,1),(0,0x80),(0x4022,0xc022)):
        mem,a,t,p=seed(5)
        put(mem,a+0x24,0xfe);put(mem,a+0x22,0);put(mem,a+0x60,selector)
        put(mem,a+0xfe,0);put(mem,a+0x33,1,1);put(mem,a+0x32,owner_cpu,1)
        put(mem,t+0x66,1);put(mem,p+0x32,0,1);put(mem,p+0x33,0,1)
        put(mem,p+0x74,apron,1)
        yield (f'partner-guard-{owner_cpu}-{apron}-{selector}',mem,5)
    # Released links can be cleared between render passes. Neither an
    # ordinary animation tick nor the pre-dispatch FF guard reads A2.
    for frame,timer,state,link in itertools.product((1,3),(1,0xff00),(5,0xff),(0,0x80000000)):
        mem,a,t,p=seed(5)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x20,state)
        put(mem,a+0x26,link,4)
        yield (f'released-link-{frame}-{timer}-{state}-{link}',mem,5)
    # Repeat completion with bit3 already set must enter $1120A, without
    # patching either the selector or the helper in the ROM oracle.
    for mode,config,role,rescue,selector in itertools.product((0,1),(0,0x2000,0x4000,0x6000),(0,0x8000),(False,True),(0x0822,0xc822)):
        mem,a,t,p=seed(5)
        put(mem,a+0x24,0xfe);put(mem,a+0x22,0);put(mem,a+0x60,selector)
        put(mem,a+0x33,1,1);put(mem,t+0x33,1,1);put(mem,t+0x66,0)
        put(mem,a+0xfe,0);put(mem,a+0x56,role)
        put(mem,a+0x7e,p if rescue else 0,4)
        put(mem,0x1c0161,mode,1);put(mem,0x1c0066,config)
        yield (f'result-{mode}-{config}-{role}-{rescue}-{selector}',mem,5)
    if args.snapshot:
        data=args.snapshot.read_bytes()
        if args.snapshot.name=='action22_snapshot_a4be15f.bin':
            manifest=json.loads(args.snapshot.with_name('action22_manifest.json').read_text())
            assert hashlib.sha256(data).hexdigest()==manifest['sha256']
        frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
        assert len(data)==16472 and kind and pc==0x1c03e
        mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
        a=actors[slot];assert mem[a+0x61]==0x22
        yield (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))
    # The disabled-current-actor path is handled before the action callback.
    mem,a,t,p=seed(5);put(mem,a+0x20,0xff);yield ('state-ff',mem,5)

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-action22-') as temp:
    library=Path(temp)/'action22.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action22_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(library))
    native.wf_action22_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3]
        count+=1
        regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
        status=native.wf_action22_oracle(nb,nr,slot)
        assert status==0,(name,'native action22 blocked',hex(-status))
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
        memory.w_block(0,bytes(mem))
        for i in range(8,15):cpu.w_reg(i,0)
        for i,value in enumerate(regs):cpu.w_reg(i,value)
        cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
        for _ in range(10000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x70000:break
            if pc==0x2052:
                cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            want=bytes(memory.r_block(at,n));got=nb.raw[at:at+n]
            assert got==want,(name,'memory',hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
        want_regs=[cpu.r_reg(i) for i in range(8)]
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        machine.cleanup()
    required=(0x1120a,0x11214,0x11256,0x11278,0x11284,0x21358,0x21368,0x215d0,0x21630,0x15b94,0x15bd2,0x15c04,0x15c2c,0x15c44,0x15c8e,0x15cba,0x15cc2,0x11200,0x215e2,0x21612,0x21638,0x21674,0x216a4,0x216b8,0x216cc,0x216e8)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    assert 0x1120a in coverage,'Repeated-completion continuation untested'
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        report={'count':count,'passed':count,'rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Dereferenced pointers must identify actors','CPU partner tier0..2','This implements selector$22 only']}
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {count} action22 unmodified-ROM fixtures: init/acquire/release/TAG, all actors, globals and D0-D7; {len(required)} branch sites')
