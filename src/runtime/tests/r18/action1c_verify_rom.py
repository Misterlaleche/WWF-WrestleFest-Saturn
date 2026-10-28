#!/usr/bin/env python3
"""Raw $1C: native bounded dispatcher versus the unchanged arcade ROM.

Catches missing callback ownership, wrong contact limits, byte/word timer or
facing handling, lost partner/result side effects and wrong effect row/queue.
Only $2052 sound output is intercepted; no gameplay owner is substituted.
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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action1c_snapshot_805be2a.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x1c*4:0x12618+0x1c*4]==bytes.fromhex('00015668')
assert rom[0x15668:0x1567c]==bytes.fromhex('0001567c00010003000c00160016016101620163')
assert rom[0x10dda:0x10df8]==bytes.fromhex('fffe01640000fffe01650000fffe016a0000fffc01660000fffe01670000')
rng=random.Random(0x1567c)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural():
    data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action1c_manifest.json').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
    assert len(data)==16472 and (frame,slot,kind,pc,sequence,status)==(3373,1,1,0x1c03e,1030,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert int.from_bytes(mem[actors[slot]+0x60:actors[slot]+0x62],'big')==0x401c
    return (f'real-frame{frame}-slot{slot}',mem,slot,struct.unpack_from('=8I',data,24))

def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i)
        put(mem,at+6,0x250);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4)
        put(mem,at+0x7e,0,4);put(mem,at+0x2e,0)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0xfe,0);put(mem,at+0x70,i%3);put(mem,at+0x66,10);put(mem,at+0x72,100)
        put(mem,at+0x20,1);put(mem,at+0x64,1)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,t+0x26,a,4)
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x1c)
    put(mem,a+0x24,0);put(mem,a+0x22,0)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c1697,0,1)
    put(mem,0x1c17e6,0);put(mem,0x1c17ee,0)
    for k in range(6):put(mem,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
    for i in range(11):mem[0x1c1258+i*0x2a]|=0x80
    return mem,a,t,p

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,selector,face in itertools.product(range(9),(0x1c,0x401c,0x801c,0xc01c),(0,0x8000)):
        mem,a,t,p=seed(slot);put(mem,a+0x1c,5);put(mem,a+0x60,selector);put(mem,a+0x2e,face)
        yield (f'init-{slot}-{selector:x}-{face:x}',mem,slot)
    # The callback reads the frame WORD for profile, then its BYTE for phase.
    for frame,timer,cpu,selector in itertools.product((0,1,2,3,0xfe,0x100,0x101,0x1fe),(0,1,0xff00,0xffff),(0,1),(0x1c,0x801c)):
        mem,a,t,p=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x32,cpu,1);put(mem,a+0x60,selector)
        yield (f'phase-{frame}-{timer}-{cpu}-{selector:x}',mem,1)
    # Unsigned subtraction/negation, strict <16 Y and <80 X contact limits.
    for base,dx,dy in itertools.product((0,0x250,0xffff),(-81,-80,-79,0,79,80,81),(-17,-16,-15,0,15,16,17)):
        mem,a,t,p=seed();put(mem,a+6,base);put(mem,a+10,base);put(mem,t+6,base+dx);put(mem,t+10,base+dy)
        yield (f'contact-{base}-{dx}-{dy}',mem,1)
    for state,sub,face,back,cpu in itertools.product((0,1,4,5,0xff),(0,1,0x101),(0,0x8000,0x100,0x8100),(True,False),(0,1)):
        mem,a,t,p=seed();put(mem,t+0x20,state);put(mem,t+0x64,sub);put(mem,t+0x2e,face)
        put(mem,a+0x32,cpu,1)
        if not back:put(mem,t+0x26,p,4)
        yield (f'guard-{state}-{sub}-{face:x}-{back}-{cpu}',mem,1)
    # Capture must preserve high target selector byte, except first-entry
    # $80 override. Exercise every $215B6 TAG branch and its Royal bypass.
    for credit,mode,cpu,active,human,pcpu,phuman,apron,tier in itertools.product((0,1),(0,1),(0,1),(0,1),(0,2),(0,1),(0,2),(0,0x80),(0,1,2)):
        mem,a,t,p=seed();put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        put(mem,a+0x32,cpu,1);put(mem,a+0x33,active|human,1);put(mem,p+0x32,pcpu,1)
        put(mem,p+0x33,phuman,1);put(mem,p+0x74,apron,1);put(mem,p+0x70,tier)
        yield (f'partner-{credit}-{mode}-{cpu}-{active}-{human}-{pcpu}-{phuman}-{apron}-{tier}',mem,1)
    # $111C8 gate, first-call bit3 latch, victory health bonus/wrap/clamp,
    # GIVE UP foreground write, TAG cleanup, and Royal $21358 cleanup.
    for credit,mode,flags,tflags,health,previous,role,config in itertools.product((0,1),(0,1),(0,1,3,5),(0,1),(0,1),(0,8),(0,0x80),(0,0x2000,0x4000,0x6000)):
        mem,a,t,p=seed();put(mem,a+0x24,0xfe);put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        put(mem,a+0x33,flags,1);put(mem,t+0x33,tflags,1);put(mem,t+0x66,health)
        put(mem,a+0x60,(previous<<8)|0x1c);put(mem,a+0x56,role,1);put(mem,0x1c0066,config)
        put(mem,a+0x7e,p,4);put(mem,p+0x66,0xfff8);put(mem,p+0x6a,0xffff)
        yield (f'result-{credit}-{mode}-{flags}-{tflags}-{health}-{previous}-{role}-{config}',mem,1)
    for mode,done,flags,partner_flags,partner_cpu,target_cpu in itertools.product((0,1),(0,0x80),(0,1,3),(0,2),(0,1),(0,1)):
        mem,a,t,p=seed();put(mem,a+0x24,0xfe);put(mem,a+0xfe,done,1)
        put(mem,0x1c0161,mode,1);put(mem,a+0x33,flags,1);put(mem,p+0x33,partner_flags,1)
        put(mem,p+0x32,partner_cpu,1);put(mem,actors[4]+0x32,target_cpu,1)
        yield (f'cleanup-{mode}-{done}-{flags}-{partner_flags}-{partner_cpu}-{target_cpu}',mem,1)
    # Effects0,1+3,4: signed offset and phase-specific Y/Z, pool allocation,
    # saturated two-allocation path, cross-record facing, clip and queue append.
    for phase,timer,free,face,offset,nextface,clip in itertools.product((0,1,2),(0,1),(0,5,10,11),(0,0x8000),(0x80,0,0x7f),(0,0x80),(False,True)):
        mem,a,t,p=seed();put(mem,a+0x24,phase);put(mem,a+0x22,timer);put(mem,a+0x60,0x801c)
        put(mem,a+0x2e,face);put(mem,a+0x19,offset,1)
        if free<11:mem[0x1c1258+free*0x2a]&=0x7f;put(mem,0x1c1258+free*0x2a+0x2e,nextface,1)
        if free<10:mem[0x1c1258+(free+1)*0x2a]&=0x7f
        if clip:put(mem,a+6,0x7fff);put(mem,a+10,0x7fff)
        put(mem,0x1c19b8,2)
        yield (f'effect-{phase}-{timer}-{free}-{face:x}-{offset}-{nextface}-{clip}',mem,1)
    for frame,timer,link in itertools.product((0,1,2),(1,0xff00),(0,0x80000000)):
        mem,a,t,p=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,link,4)
        yield (f'released-{frame}-{timer}-{link:x}',mem,1)
    mem,a,t,p=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,1)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
    memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,value in enumerate(regs):cpu.w_reg(i,value)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
    for _ in range(20000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x2052:
            cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    regs=[cpu.r_reg(i) for i in range(8)];machine.cleanup()
    return result,regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-action1c-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action1c.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action1c_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action1c_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1
        regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem))
        status=native.wf_action1c_oracle(nb,nr,slot)
        assert status==0,(name,'native action1c blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():
        case[1][0xc0000:0xc4000]=rng.randbytes(0x4000)
        compare(case)
    # Untouched natural root continues for 240 isolated callback ticks. Damage
    # consumption belongs to the scheduler, so this callback may keep holding.
    name,mem,slot,regs=natural()
    for tick in range(240):
        mem,regs=compare((f'natural-continuation-{tick}',mem,slot,regs));continuations+=1
        if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=0x1c:break
    required=(0x1567c,0x156aa,0x156be,0x156d0,0x156e4,0x156f8,0x15706,0x15730,0x15750,0x1578c,0x1579a,0x157b0,0x157ba,0x157ca,0x157e0,0x15800,0x1580e,0x15820,0x15834,0x1583e,0x1584c,0x10d5a,0x10d5e,0x10dc6,0x10dcc,0x111c8,0x11200,0x11214,0x11270,0x11284,0x215b6,0x215ee,0x21630,0x2164e,0x216b8,0x216cc,0x216e8,0x212a0,0x21358,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x1567c,0x15684,0x1c0dc,0x1c0fa,0x15730,0x1580e,0x157ca)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selector $1C only; dereferenced gameplay pointers identify actors; partner tier0..2','No Saturn hardware timing or audible sound validation']}
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action1c unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} required branch sites; RAM/FG/D0-D7/RNG')
