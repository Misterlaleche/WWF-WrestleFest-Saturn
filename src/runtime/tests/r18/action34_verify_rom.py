#!/usr/bin/env python3
"""Raw $34: unchanged arcade ROM versus the bounded native dispatcher.

Catches missing ownership, RNG/repetition outcome, nine-frame descriptor, velocity25,
landing/partner recovery, target damage/position and FE release.
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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action34_snapshot_after67.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x34*4:0x12618+0x34*4]==bytes.fromhex('0001747e')
assert rom[0x1747e:0x174aa]==bytes.fromhex('000174aa0001000900080009000a00080008ff000010001000280090009101380139013a013b013c013d0068')
assert rom[0x25ca+25*6:0x25ca+26*6]==bytes.fromhex('010003000058')
rng=random.Random(0x174aa)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action34_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(5015,4,1,0x1c03e,1383,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x34
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8001);put(mem,at+2,i);put(mem,at+6,0x250);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x2e,0);put(mem,at+0x20,1);put(mem,at+0xae,1);put(mem,at+0x37,0,1);put(mem,at+0x7a,actors[(i+1)%9],4)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9];q=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x34);put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,t+0x26,a,4);put(mem,t+0x86,q,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c0162,0);put(mem,0x1c005c,rng.getrandbits(32),4);put(mem,0x1c17e6,0);put(mem,0x1c17ee,0)
    for k in range(6):put(mem,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
    for i in range(11):mem[0x1c1258+i*0x2a]|=0x80
    return mem,a,t,p,q

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for ident,face,repetition,mode,role,target_role,progression in itertools.product(range(12),(0,0x8000),(0,3,4,0xff,0xffff),(0,1),(0,0x80),(0,0x80),(0,9)):
        slot=ident%9;mem,a,t,p,q=seed(slot);put(mem,a+2,ident);put(mem,a+0x2e,face);put(mem,a+0x1c,5)
        put(mem,a+0xfa,repetition);put(mem,a+0x56,role,1);put(mem,t+0x56,target_role,1)
        put(mem,0x1c0161,mode,1);put(mem,0x1c0162,progression)
        yield (f'init-{ident}-{face:x}-{repetition}-{mode}-{role}-{target_role}-{progression}',mem,slot)
    for selector,frame,timer,land in itertools.product((0x34,0x4034,0x8034,0xc034),(0,1,2,3,4,5,6,7,8,0xfe,0x103,0x106,0x1fe),(0,1,0xff00,0xffff),(0,0x10)):
        mem,a,t,p,q=seed();put(mem,a+0x60,selector);put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x37,land,1)
        yield (f'phase-{selector:x}-{frame}-{timer}-{land}',mem,4)
    # $110E0 only marks a TAG partner in state1/AE1. Exact BYTE reads
    # matter for nonzero high state/substate bytes and role-independent mode.
    for mode,state,sub,bonus in itertools.product((0,1),(0,1,0x101),(0,1,0x101),(0,0xfc)):
        mem,a,t,p,q=seed();put(mem,a+0x24,6);put(mem,a+0x22,0);put(mem,0x1c0161,mode,1)
        put(mem,q+0x20,state);put(mem,q+0xae,sub);put(mem,t+0xc7,bonus,1)
        yield (f'partner-{mode}-{state}-{sub}-{bonus}',mem,4)
    for x,y,z,face,self_link in itertools.product((0,0x250,0xffff),(0,0x150,0xffff),(0,0x140,0xffff),(0,0x8000),(False,True)):
        mem,a,t,p,q=seed();put(mem,a+0x24,6);put(mem,a+0x22,0);put(mem,a+6,x);put(mem,a+10,y);put(mem,a+14,z);put(mem,a+0x2e,face)
        if self_link:put(mem,a+0x26,a,4)
        yield (f'position-{x}-{y}-{z}-{face:x}-{self_link}',mem,4)
    for face in (0,0x100,0x8000,0x8101,0xffff):
        mem,a,t,p,q=seed();put(mem,a+0x24,0xfe);put(mem,a+0x2e,face)
        yield (f'release-facing-{face:x}',mem,4)
    for frame,timer,link in itertools.product((0,1,3,5,6,0xfe),(1,0xff00),(0,0x80000000)):
        mem,a,t,p,q=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,link,4)
        yield (f'released-{frame}-{timer}-{link:x}',mem,4)
    mem,a,t,p,q=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,4)

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
with tempfile.TemporaryDirectory(prefix='wf-action34-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action34.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action34_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action34_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action34_oracle(nb,nr,slot)
        assert status==0,(name,'native action34 blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(160):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x34:break
    # Frame5 waits for contact from scheduler physics; isolated ROM/native
    # callbacks may remain there, so 160 ticks are a bounded comparison.
    required=(0x174aa,0x174b2,0x174bc,0x174d0,0x174e0,0x174f4,0x17502,0x17510,0x1751e,0x1753c,0x17564,0x17568,0x1757a,0x17594,0x258e,0x25c0,0x110ee,0x11102,0x1129e,0x1130c,0x1132a,0x11340,0x10b9a,0x10bbc,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x174aa,0x174b2,0x174f4,0x17502,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $34 only; actor links/characters0..11/progression0..9','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action34 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
