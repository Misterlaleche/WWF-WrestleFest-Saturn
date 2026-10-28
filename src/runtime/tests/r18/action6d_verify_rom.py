#!/usr/bin/env python3
"""Raw $6D: unchanged arcade ROM versus the bounded native dispatcher.

Catches missing ownership, motion preservation, FE ground route, byte wrapping and
wrong row4 sprite/queue output.
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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action6d_snapshot_after58.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x6d*4:0x12618+0x6d*4]==bytes.fromhex('00019cf2')
assert rom[0x19cf2:0x19d06]==bytes.fromhex('00019d0600010003001000100010016301630010')
assert rom[0x10dda+4*6:0x10dda+5*6]==bytes.fromhex('fffe01670000')
rng=random.Random(0x19d06)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action6d_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(3464,1,1,0x1c03e,1049,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x6d
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8001);put(mem,at+2,i);put(mem,at+6,0x250);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x2e,0)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9];q=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x6d);put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,t+0x26,a,4);put(mem,t+0x86,q,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c17e6,0);put(mem,0x1c17ee,0)
    for k in range(6):put(mem,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
    for i in range(11):mem[0x1c1258+i*0x2a]|=0x80
    return mem,a,t,p,q

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,latched,selector,frame,timer,cpu in itertools.product(range(9),(False,True),(0x6d,0x406d,0x806d,0xc06d),(0,1,2,0xfe,0xff,0x100,0x101,0x1fe),(0,1,0xff00,0xffff),(0,1)):
        mem,a,t,p,q=seed(slot);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x60,selector)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x32,cpu,1)
        yield (f'phase-{slot}-{latched}-{selector:x}-{frame}-{timer}-{cpu}',mem,slot)
    # Both effective phases emit row4 (never row5). Test the first free
    # record/full guard, signed X, cross-record facing, clip and append.
    for phase,free,face,offset,nextface,clip,scroll in itertools.product((0,1),(0,5,10,11),(0,0x8000),(0x80,0xff,0,0x7f),(0,0x80),(False,True),(0,0x100)):
        mem,a,t,p,q=seed();put(mem,a+0x24,phase);put(mem,a+0x2e,face);put(mem,a+0x19,offset,1)
        if free<11:mem[0x1c1258+free*0x2a]&=0x7f;put(mem,0x1c1258+free*0x2a+0x2e,nextface,1)
        if clip:put(mem,a+6,0x7fff);put(mem,a+10,0x7fff)
        put(mem,0x1c17e6,scroll);put(mem,0x1c17ee,scroll);put(mem,0x1c19b8,2)
        yield (f'effect-{phase}-{free}-{face:x}-{offset}-{nextface}-{clip}-{scroll}',mem,1)
    # No callback branch dereferences A2, including init and FE release.
    for latched,frame,timer,link in itertools.product((False,True),(0,1,2,0xfe),(0,1,0xff00),(0,0x80000000)):
        mem,a,t,p,q=seed();put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,link,4)
        yield (f'released-{latched}-{frame}-{timer}-{link:x}',mem,1)
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
with tempfile.TemporaryDirectory(prefix='wf-action6d-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action6d.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action6d_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action6d_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action6d_oracle(nb,nr,slot)
        assert status==0,(name,'native action6d blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    name,mem,slot,regs=natural()
    for tick in range(160):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x6d:break
    else:raise AssertionError('natural raw6d failed to release within160 ticks')
    required=(0x19d06,0x19d0e,0x19d16,0x19d26,0x19d36,0x19d46,0x19d58,0x19d6c,0x10d5a,0x10d5e,0x10dc6,0x10dcc,0x27da,0x27e0,0x27fc,0x2800,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x19d06,0x19d0e,0x19d16,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $6D only; no gameplay link is dereferenced','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action6d unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
