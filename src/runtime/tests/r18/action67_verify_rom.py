#!/usr/bin/env python3
"""Raw $67: unchanged arcade ROM versus the bounded native dispatcher.

Catches missing ownership, targetFF/flag setup, TAG/Royal cleanup effects,
phase2 reaction/position and FE state7/facing release; preserved motion/D1/D2.
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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action67_snapshot_after5d.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x67*4:0x12618+0x67*4]==bytes.fromhex('00019776')
assert rom[0x19776:0x1978a]==bytes.fromhex('0001978a0001000300100010000801d901da01db')
rng=random.Random(0x1978a)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action67_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(4771,1,1,0x1c03e,1328,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x67
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8001);put(mem,at+2,i);put(mem,at+6,0x250);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x2e,0)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9];q=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x67);put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,t+0x26,a,4);put(mem,t+0x86,q,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c17e6,0);put(mem,0x1c17ee,0)
    for k in range(6):put(mem,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
    for i in range(11):mem[0x1c1258+i*0x2a]|=0x80
    return mem,a,t,p,q

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,latched,selector,frame,timer in itertools.product(range(9),(False,True),(0x67,0x4067,0x8067,0xc067),(0,1,2,0xfe,0xff,0x100,0x102,0x1fe),(0,1,0xff00,0xffff)):
        mem,a,t,p,q=seed(slot);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x60,selector)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer)
        yield (f'phase-{slot}-{latched}-{selector:x}-{frame}-{timer}',mem,slot)
    # $10B9A signed facing offset and 16-bit position wrap. D1.W=1/D2.W=0
    # survive; targetY ends equal to ownerY after the following SUBQ.W1.
    for x,y,z,face,self_link in itertools.product((0,0x250,0xffff),(0,0x150,0xffff),(0,0x140,0xffff),(0,0x8000),(False,True)):
        mem,a,t,p,q=seed();put(mem,a+0x24,2);put(mem,a+0x22,0);put(mem,a+6,x);put(mem,a+10,y);put(mem,a+14,z);put(mem,a+0x2e,face)
        if self_link:put(mem,a+0x26,a,4)
        yield (f'position-{x}-{y}-{z}-{face:x}-{self_link}',mem,1)
    # $212D4 on both linked participants; human/CPU partner cleanup and
    # the branch that clears A1 based on the opposing partner.
    for credit,flags,role,pflags,pcpu,qcpu in itertools.product((0,1),(0,1,3),(0,0x80),(0,2),(0,1),(0,1)):
        mem,a,t,p,q=seed();put(mem,a+0x24,2);put(mem,a+0x22,0);put(mem,0x1c007c,credit)
        put(mem,a+0x33,flags,1);put(mem,a+0x56,role,1);put(mem,p+0x33,pflags,1)
        put(mem,p+0x32,pcpu,1);put(mem,q+0x32,qcpu,1)
        put(mem,t+0x33,1,1);put(mem,t+0x56,0x80,1)
        yield (f'tag-{credit}-{flags}-{role}-{pflags}-{pcpu}-{qcpu}',mem,1)
    for credit,link in itertools.product((0,1),(0,actors[0],actors[1],actors[7])):
        mem,a,t,p,q=seed();put(mem,a+0x24,2);put(mem,a+0x22,0);put(mem,0x1c007c,credit);put(mem,0x1c0161,1,1);put(mem,a+0x7e,link,4)
        yield (f'royal-{credit}-{link:x}',mem,1)
    for face in (0,0x100,0x8000,0x8101,0xffff):
        mem,a,t,p,q=seed();put(mem,a+0x24,0xfe);put(mem,a+0x2e,face)
        yield (f'release-facing-{face:x}',mem,1)
    # No linked dereference on latched inactive phases or FE release.
    for frame,timer,link in itertools.product((0,1,2,0xfe),(1,0xff00),(0,0x80000000)):
        mem,a,t,p,q=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,link,4)
        yield (f'released-{frame}-{timer}-{link:x}',mem,1)
    mem,a,t,p,q=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,1)

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
with tempfile.TemporaryDirectory(prefix='wf-action67-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action67.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action67_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action67_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action67_oracle(nb,nr,slot)
        assert status==0,(name,'native action67 blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(160):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x67:break
    else:raise AssertionError('natural raw67 failed to release within160 ticks')
    required=(0x1978a,0x19792,0x19798,0x197a6,0x197b4,0x197e0,0x197e6,0x197ec,0x19804,0x19810,0x10b9a,0x10bbc,0x212a0,0x212de,0x212f2,0x212fa,0x2130a,0x21332,0x21344,0x21358,0x21368,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x1978a,0x19792,0x197b4,0x19804,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $67 only; dereferenced gameplay pointers identify actors','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action67 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
