#!/usr/bin/env python3
"""CPUstate0 FLAGS34.bit4 prefix versus intact ROM, two natural routes.

Rejects absent/partial prefix, wrong health qualification, reordered mode/B6
priority, skipped face/WORD masks or register/RNG corruption. Explicitly
unsupported aggregate-guard branches remain blocked in separate controls.
Only the sound2052 device may be intercepted; ROM bytes are never patched.
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
parser.add_argument('--report',type=Path);parser.add_argument('--rom-only',action='store_true');parser.add_argument('--natural-only',action='store_true')
args=parser.parse_args();manifest=json.loads((HERE/'cpu_state0_flag34_manifest.json').read_text())
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
assert rom[0x1c348:0x1c366]==bytes.fromhex('08280004003467160828000300b5660e317c00050020317c007900604e75')
actors=[0x1c05b0+0x10c*i for i in range(9)];rng=random.Random(0x1c348);coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def naturals():
    for item in manifest['natural_snapshots']:
        data=(HERE/item['file']).read_bytes();assert hashlib.sha256(data).hexdigest()==item['sha256']
        assert len(data)==16472 and struct.unpack_from('=6I',data)==tuple(item['header'])
        mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
        yield (f"real-frame{item['header'][0]}-slot1",mem,1,struct.unpack_from('=8I',data,24))

def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    a=actors[slot];t=actors[(slot+1)%9]
    put(mem,a+0x20,0x8000);put(mem,a+0x7a,t,4);put(mem,a+0x34,0x11,1)
    put(mem,a+0xb5,0,1);put(mem,a+0xb6,0);put(mem,a+0xb9,0,1)
    put(mem,t+0x20,0);put(mem,t+0x64,0);put(mem,t+0x60,0);put(mem,t+0x2e,0)
    put(mem,0x1c0161,0,1);put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t

def fixtures():
    yield from naturals()
    if args.natural_only:return
    # All FLAGS34 BYTE values with bit4; bit2 may coexist but comes later.
    for flags34,b5,mode in itertools.product((v for v in range(256) if v&16),(0,1,0x80,0xc3),(0,1,2,3)):
        mem,a,t=seed();put(mem,a+0x34,flags34,1);put(mem,a+0xb5,b5,1);put(mem,0x1c0161,mode,1)
        yield (f'flags34-{flags34}-{b5}-{mode}',mem,1)
    # Every B5 BYTE that satisfies the reached bit3-clear gate; mode0/3 and
    # B6 flags prove Royalbit6 and ringsidebit4 precede the raw79 branch.
    for b5,mode,b6 in itertools.product((v for v in range(256) if not v&8),(0,3),(0,0x10,0x40,0x50)):
        mem,a,t=seed();put(mem,a+0xb5,b5,1);put(mem,a+0xb6,b6,1);put(mem,0x1c0161,mode,1)
        yield (f'precedence-{b5}-{mode}-{b6}',mem,1)
    for b7,b6,targetflags in itertools.product(range(256),(0,0x40),(0,0x10)):
        mem,a,t=seed();put(mem,a+0xb6,(b6<<8)|b7);put(mem,t+0x34,targetflags,1)
        yield (f'later-guards-{b7}-{b6}-{targetflags}',mem,1)
    for slot,health,state in itertools.product((0,1,4,5,8),(0,1,0x7fff,0x8000,0xffff),(0,0x8000)):
        mem,a,t=seed(slot);put(mem,a+0x66,health);put(mem,a+0x20,state)
        yield (f'health-state-{slot}-{health}-{state:x}',mem,slot)
    for ownx,targetx,state,sub,selector,facing in itertools.product((0,1,0x7fff,0xffff),(0,1,0x8000,0xffff),(4,5),(8,9),(0x22,0x52),(0,0x80)):
        mem,a,t=seed();put(mem,a+6,ownx);put(mem,t+6,targetx);put(mem,t+0x20,state)
        put(mem,t+0x64,sub);put(mem,t+0x60,selector);put(mem,t+0x2e,facing,1)
        yield (f'face-{ownx}-{targetx}-{state}-{sub}-{selector}-{facing}',mem,1)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_reg(9,0x1c2cc);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c2cc)
    for _ in range(1000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split();source_hash=hashlib.sha256((ROOT/'src/wf_cpu_runtime.c').read_bytes()).hexdigest()
with tempfile.TemporaryDirectory(prefix='wf-state0-flag34-') as temp:
    if not args.rom_only:
        library=Path(temp)/'state0flag34.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state0_flag34_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_cpu_state0_flag34_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000);want,want_regs=oracle(mem,slot,regs,name)
        assert want[0x1c005c:0x1c0060]==mem[0x1c005c:0x1c0060],(name,'unexpected RNG owner')
        if args.rom_only:continue
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_cpu_state0_flag34_oracle(nb,nr,slot)
        assert status==0,(name,'native CPU blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    unsupported=0
    if not args.rom_only and not args.natural_only:
        for b5 in (8,9,0x48,0x4b):
            mem,a,t=seed();put(mem,a+0xb5,b5,1);put(mem,a+0xb7,0,1)
            nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)])
            status=native.wf_cpu_state0_flag34_oracle(nb,nr,1)
            assert status==-0x1c2cc,('unproven B5.bit3 route broadened',b5,status)
            unsupported+=1
    required=(0x1c2cc,0x1c2d6,0x10be8,0x1c2dc,0x1c2e8,0x1c300,0x1c30a,0x1c31a,0x1c322,0x1c32a,0x1c334,0x1c348,0x1c350,0x1c358,0x1c35e)
    if args.natural_only:required=(0x1c2cc,0x10be8,0x1c348,0x1c358,0x1c35e)
    assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc) for pc in required if pc not in coverage])
    assert 0x21b4 not in coverage
    report={'count':count,'passed':0 if args.rom_only else count,'natural_count':2,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':manifest['rom_sha1'],'snapshots':manifest['natural_snapshots'],'cpu_source_sha256':source_hash,'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','rng_calls':0,'unsupported_bit3_routes_still_blocked':unsupported,'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['Reached FLAGS34.bit4/B5.bit3-clear prefix only; unrelated aggregate guards remain explicit dependencies','No full-match or Saturn hardware validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} state0 flag34 fixtures; 2 natural roots; {len(required)} sites; {unsupported} unsupported controls; RAM/FG/D0-D7/RNG')
