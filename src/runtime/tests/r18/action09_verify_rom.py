#!/usr/bin/env python3
"""Raw09 submission: unchanged ROM versus native bounded dispatcher.

Catches absent owner, BYTE frame/state/facing handling, reciprocal capture,
C7 BYTE wrap, shared result/partner effects and old-descriptor final ticks.
Only sound output2052 is intercepted. The natural frame727 snapshot is
untouched; other fixtures are explicitly synthesized boundary cases.
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
parser.add_argument('--snapshot',type=Path,default=HERE/'action09_snapshot_pad_variation.bin')
parser.add_argument('--main-snapshot',type=Path,default=HERE/'action09_snapshot_after6d.bin')
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--report',type=Path)
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
parser.add_argument('--actions-source',type=Path,help='Validation copy of the complete actions source; never mutates production')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+9*4:0x12618+9*4]==bytes.fromhex('000135c8')
assert rom[0x135c8:0x135ec]==bytes.fromhex('000135ec0001000700100010000c0016000a00100010018b00d800d900da00db00dc00dd')
rng=random.Random(0x135ec)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def natural(main=False):
    snapshot=args.main_snapshot if main else args.snapshot
    data=snapshot.read_bytes();manifest=json.loads((HERE/('action09_main_manifest.json' if main else 'action09_manifest.json')).read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    frame,sequence=(4519,1285) if main else (727,206)
    assert len(data)==16472 and struct.unpack_from('=6I',data)==(frame,4,1,0x1c03e,sequence,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    assert mem[actors[4]+0x60:actors[4]+0x62]==bytes.fromhex('4009')
    return (f'real-frame{frame}-slot4',mem,4,struct.unpack_from('=8I',data,24))

def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i);put(mem,at+6,0x250+11*i);put(mem,at+10,0x150+3*i)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4);put(mem,at+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x56,0x74,0xb5,0xb6):put(mem,at+off,0,1)
        put(mem,at+0x66,10);put(mem,at+0x70,i%3);put(mem,at+0x72,100);put(mem,at+0xfe,0)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,a+0x1c,0x8005);put(mem,a+0x20,5);put(mem,a+0x60,9)
    put(mem,a+0x24,0);put(mem,a+0x22,0);put(mem,t+0x20,4);put(mem,t+0x64,8);put(mem,t+0x26,a,4)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1)
    return mem,a,t,p

def fixtures():
    yield natural()
    yield natural(main=True)
    if args.snapshot_only:return
    for slot,face,selector in itertools.product((0,1,4,5,8),(0,0x8000,0x0080,0x8101),(9,0x4009,0x8009,0xc009)):
        mem,a,t,p=seed(slot);put(mem,a+0x1c,5);put(mem,t+0x2e,face);put(mem,a+0x60,selector)
        yield (f'init-{slot}-{face:x}-{selector:x}',mem,slot)
    for frame,timer,done,selector in itertools.product((0,1,4,5,6,0xfe,0x100,0x1fe),(0,1,0xff00),(0,0x80),(9,0x8009)):
        mem,a,t,p=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0xfe,done,1);put(mem,a+0x60,selector)
        yield (f'phase-{frame}-{timer}-{done}-{selector:x}',mem,4)
    for state,sub,back,count in itertools.product((0,1,4,0x104,5,0xff),(0,8,0x108,9),(True,False),(0,0xff)):
        mem,a,t,p=seed();put(mem,t+0x20,state);put(mem,t+0x64,sub);put(mem,t+0xc7,count,1)
        if not back:put(mem,t+0x26,p,4)
        yield (f'capture-{state}-{sub}-{back}-{count}',mem,4)
    for credit,mode,flags,tflags,health,prior,role,config in itertools.product((0,1),(0,1),(0,1,3,5),(0,1),(0,1),(0,8),(0,0x80),(0,0x2000,0x4000,0x6000)):
        mem,a,t,p=seed();put(mem,a+0x24,0xfe);put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        put(mem,a+0x33,flags,1);put(mem,t+0x33,tflags,1);put(mem,t+0x66,health)
        put(mem,a+0x60,(prior<<8)|9);put(mem,a+0x56,role,1);put(mem,0x1c0066,config)
        put(mem,a+0x7e,p,4);put(mem,p+0x66,0xfff8);put(mem,p+0x6a,0xffff)
        yield (f'result-{credit}-{mode}-{flags}-{tflags}-{health}-{prior}-{role}-{config}',mem,4)
    for credit,mode,cpu,active,human,pcpu,phuman,apron,tier in itertools.product((0,1),(0,1),(0,1),(0,1),(0,2),(0,1),(0,2),(0,0x80),(0,1,2)):
        mem,a,t,p=seed();put(mem,a+0x24,0xfe);put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        put(mem,a+0x32,cpu,1);put(mem,a+0x33,active|human,1);put(mem,p+0x32,pcpu,1)
        put(mem,p+0x33,phuman,1);put(mem,p+0x74,apron,1);put(mem,p+0x70,tier)
        yield (f'partner-{credit}-{mode}-{cpu}-{active}-{human}-{pcpu}-{phuman}-{apron}-{tier}',mem,4)
    for frame,timer,link in itertools.product((1,4,6),(0,1,0xff00),(0,0x80000000)):
        mem,a,t,p=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,link,4)
        yield (f'released-{frame}-{timer}-{link:x}',mem,4)
    mem,a,t,p=seed();put(mem,a+0x20,0xff);yield ('disabled-state',mem,4)

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
if args.actions_source:core=[str(args.actions_source.resolve()) if f=='src/wf_r17_actions_r11.c' else f for f in core]
with tempfile.TemporaryDirectory(prefix='wf-action09-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action09.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action09_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library));native.wf_action09_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;continuations=0
    def compare(case):
        global count
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)]
        want,want_regs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,want_regs
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_action09_oracle(nb,nr,slot)
        assert status==0,(name,'native action09 blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
        return want,want_regs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    for main in (False,True):
        name,mem,slot,regs=natural(main)
        for tick in range(200):
            mem,regs=compare((f'{name}-continuation-{tick}',mem,slot,regs));continuations+=1
            if mem[actors[slot]+0x21]!=5 or mem[actors[slot]+0x61]!=9:break
    required=(0x135ec,0x135f4,0x1360e,0x1361a,0x1362a,0x13632,0x1366a,0x13686,0x13698,0x136b0,0x136ba,0x136e2,0x1370e,0x13716,0x13722,0x111c8,0x11214,0x11270,0x11284,0x215b6,0x2164e,0x216b8,0x216cc,0x216e8,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x135ec,0x135f4,0x13632,0x13698,0x13716,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branches',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'natural_continuations':continuations,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'main_snapshot_sha256':hashlib.sha256(args.main_snapshot.read_bytes()).hexdigest(),'actions_source_sha256':hashlib.sha256((args.actions_source or ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','intercepted_rom_owners':['$2052 sound output only'],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['Selector09 only; target/partner pointers identify actors; partner tier0..2','Isolated callback continuation; full scheduler progression is a separate validation','No Saturn hardware timing or audible sound validation']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action09 unchanged-ROM fixtures; {continuations} natural ticks; {len(required)} branch sites; RAM/FG/D0-D7/RNG')
