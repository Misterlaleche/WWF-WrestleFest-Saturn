#!/usr/bin/env python3
"""CPU1D6E4 credited-contact approach versus unchanged ROM.

Exercises the real CPU1C8F4 dispatcher, 1D5A2 pursuit and full 1F45A tail.
Catches a missing owner, partial success, wrong target/pin/facing offsets,
BYTE contact clear, WORD selector assignment, arrival links, direction
scratch, register widths and RNG corruption. Only sound2052 may be ignored.
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
parser.add_argument('--snapshot',type=Path,default=HERE/'cpu_1d6e4_snapshot_after75.bin')
parser.add_argument('--report',type=Path)
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--natural-only',action='store_true')
args=parser.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1d6e4:0x1d6ee]==bytes.fromhex('317c000a006060001d6e')
manifest=json.loads((HERE/'cpu_1d6e4_manifest.json').read_text());data=args.snapshot.read_bytes()
assert hashlib.sha256(data).hexdigest()==manifest['sha256']
assert len(data)==16472 and struct.unpack_from('=6I',data)==(3459,4,0,0x1c8f4,1068,1)
actors=[0x1c05b0+0x10c*i for i in range(9)]
rng=random.Random(0x1d6e4);coverage=set();records=[]

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert mem[actors[4]+0xae:actors[4]+0xb0]==bytes.fromhex('000a')
    return ('real-frame3459-slot4',mem,4,struct.unpack_from('=8I',data,24))

def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i);put(mem,at+6,0x250);put(mem,at+10,0x140)
        put(mem,at+0x26,0,4);put(mem,at+0x7a,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4)
        for off in (0x32,0x33,0x34,0x35,0x56,0xb4,0xb5,0xb6,0xb7,0xb9):put(mem,at+off,0,1)
        put(mem,at+0x70,i%3)
    a=actors[slot];t=actors[(slot+1)%9]
    put(mem,a+0x20,0x8001);put(mem,a+0xae,10);put(mem,a+0x60,0x10)
    put(mem,a+0x36,0x0101);put(mem,a+0xbe,0x500);put(mem,a+0xc0,0x300)
    put(mem,t+0x20,4);put(mem,t+0x64,8);put(mem,t+0x60,0);put(mem,t+0x2e,0)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t

def fixtures():
    yield natural()
    if args.natural_only:return
    # Royal defender response to an opponent's selector $4A. Original ROM
    # $1F5DA assigns WORD selector $49 and enters the full $1F45A approach.
    mem,a,t=seed();put(mem,0x1c0161,1,1);put(mem,a+0xae,0)
    put(mem,a+0x74,0,1);put(mem,a+0xb0,0);put(mem,a+0xb2,0)
    put(mem,a+0xb5,0x80,1)
    put(mem,t+0x20,0x8005);put(mem,t+0x60,0x004a)
    put(mem,a+6,0x200);put(mem,a+10,0x150)
    put(mem,t+6,0x280);put(mem,t+10,0x150)
    yield ('royal-assist-target-4a',mem,4)
    # Every full 1F45A coordinate branch reached through credited contact.
    for state,sub,selector,pin,facing,y,cpu in itertools.product((4,5),(0,1,8,9),(9,0x1a,0x4a,0x61),(0,1,2,3),(0,0x8000),(0x15f,0x160),(0,1)):
        mem,a,t=seed();put(mem,t+0x20,state);put(mem,t+0x64,sub);put(mem,t+0x60,selector)
        put(mem,t+0x35,pin,1);put(mem,t+0x2e,facing);put(mem,t+10,y);put(mem,t+0x32,cpu,1)
        yield (f'offset-{state}-{sub}-{selector:x}-{pin}-{facing:x}-{y}-{cpu}',mem,4)
    # Selector is rewritten as a WORD; incoming high selector bits cannot
    # leak into 000A. Contact HIGH BYTE survives the low-byte clear.
    for slot,selector,contact,mode,credit in itertools.product((0,1,4,5,8),(0x10,0x4010,0x8010,0xc010),(0,1,0x0102,0xff80),(0,1,2,3),(0,1,0x8000)):
        mem,a,t=seed(slot);put(mem,a+0x60,selector);put(mem,a+0x36,contact)
        put(mem,0x1c0161,mode,1);put(mem,0x1c007c,credit)
        yield (f'gate-{slot}-{selector:x}-{contact:x}-{mode}-{credit:x}',mem,slot)
    for targetx,targety,facing,ownx,owny in itertools.product((0,1,0xfffe,0xffff),(0,0xffff),(0,0x8000),(0,0x250,0xffff),(0,0x140,0xffff)):
        mem,a,t=seed();put(mem,t+6,targetx);put(mem,t+10,targety);put(mem,t+0x2e,facing)
        put(mem,a+6,ownx);put(mem,a+10,owny)
        yield (f'wrap-direction-{targetx}-{targety}-{facing:x}-{ownx}-{owny}',mem,4)
    # Exact arrival <8 independently reaches the original state5 selector0A
    # callback and its reciprocal link owner; compare every mutated actor.
    for dx,dy,back,tagged in itertools.product((-8,-7,0,7,8),(-8,-7,0,7,8),(False,True),(False,True)):
        mem,a,t=seed();put(mem,a+6,0x250+72+dx);put(mem,a+10,0x140+dy)
        if back:put(mem,t+0x26,a,4)
        if tagged:put(mem,a+0x7a,t|0x80000000,4)
        yield (f'arrival-{dx}-{dy}-{back}-{tagged}',mem,4)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_reg(9,0x1c8f4);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c8f4)
    visited=set()
    for _ in range(20000):
        pc=cpu.r_pc();coverage.add(pc);visited.add(pc)
        if pc==0x70000:break
        if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,regs,visited

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
source_hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('src/wf_cpu_runtime.c','src/wf_state5_exec.c')}
with tempfile.TemporaryDirectory(prefix='wf-cpu1d6e4-') as temp:
    if not args.rom_only:
        library=Path(temp)/'cpu1d6e4.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_1d6e4_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_cpu_1d6e4_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;tail=0;arrivals=0;rng_changes=0
    for case in fixtures():
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        want,want_regs,visited=oracle(mem,slot,regs,name)
        tail+=0x1d6e4 in visited;arrivals+=0x1d728 in visited;rng_changes+=want[0x1c005c:0x1c0060]!=mem[0x1c005c:0x1c0060]
        if args.rom_only:continue
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_cpu_1d6e4_oracle(nb,nr,slot)
        assert status==0,(name,'native CPU blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x1c8f4,0x1d5a2,0x1d6d0,0x1d6e4,0x1f5da,0x1f45a,0x1f47e,0x1f492,0x1f4a0,0x1f4a6,0x1f4b4,0x1f4c2,0x1f4c8,0x1f4d6,0x1f4fc,0x1f50e,0x1f512,0x1f544,0x1f15c,0x1f180,0x1f184,0x1d728,0x1dfe2,0x1e664,0x1e6e2)
    if args.natural_only:required=(0x1c8f4,0x1d6e4,0x1f45a,0x1f544,0x1f15c)
    assert all(pc in coverage for pc in required),('missing ROM branch sites',[hex(pc) for pc in required if pc not in coverage])
    assert 0x21b4 not in coverage and rng_changes==0,('unexpected RNG path',rng_changes)
    report={'count':count,'passed':0 if args.rom_only else count,'natural_count':1,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(data).hexdigest(),'source_sha256':source_hashes,'reached_1d6e4_cases':tail,'arrival_cases':arrivals,'rng_calls':0,'rng_changed_cases':rng_changes,'rng_evidence':'No ROM21B4 reached on this owner/tail; input/output RNG compared unchanged for every seed','compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','intercepted_rom_owners':['$2052 sound output only'],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['CPUstate1/AE000A credited-contact1D6E4 tail; unrelated explicit pursuit dependencies remain outside these fixtures','No full-match, Saturn timing or audible sound claim']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} CPU1D6E4 fixtures; {tail} credited-contact tails; {arrivals} arrivals; {len(required)} sites; RAM/FG/D0-D7/RNG')
