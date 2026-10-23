#!/usr/bin/env python3
"""Raw $2F and its raw81 reversal: unchanged arcade ROM versus the real bounded C dispatcher.

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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action2f_snapshot_f88acad.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614+0x2f*4:0x12618+0x2f*4]==bytes.fromhex('00016c5e')
assert rom[0x16c5e:0x16c7e]==bytes.fromhex('00016c9e000100060006000a0008001800100038010701080110020e020f0000')
assert rom[0x16c7e:0x16c9e]==bytes.fromhex('00016c9e00010006000600050018001000060012010701080110020e010a010b')
assert rom[0x1a7a4:0x1a7b4]==bytes.fromhex('0001a7b400020002001c000c01070108')
rng=random.Random(0x16c9e)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def seed(slot):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i);put(mem,at+6,0x250+12*i);put(mem,at+10,0x150);put(mem,at+14,0x140)
        put(mem,at+0x26,actors[(i+1)%9],4);put(mem,at+0x7a,actors[(i+1)%9],4);put(mem,at+0x86,actors[(i+2)%9],4)
        for off in (0x32,0x33,0x34,0x35,0x56):put(mem,at+off,0,1)
        put(mem,at+0x40,0);put(mem,at+0x42,0);put(mem,at+0x70,0)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x002f)
    put(mem,a+0x24,0);put(mem,a+0x22,1);put(mem,a+0x44,2);put(mem,a+0x36,0)
    put(mem,t+0x20,4);put(mem,p+0x20,1);put(mem,p+0xae,1)
    put(mem,0x1c007c,1);put(mem,0x1c007e,0);put(mem,0x1c0161,0,1);put(mem,0x1c0162,0)
    put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t,p

def fixtures():
    # First-entry RNG1129E, both results, all character variants and clamped
    # repetition/progression boundaries. Resolver calls remain unmodified.
    for ident,face,repetition,mode,role,target_role,progression in itertools.product(range(12),(0,0x8000),(0,3,4,0xff,0xffff),(0,1),(0,0x80),(0,0x80),(0,9)):
        slot=ident%9;mem,a,t,p=seed(slot);put(mem,a+2,ident);put(mem,a+0x2e,face);put(mem,a+0x1c,5)
        put(mem,a+0xf0,repetition);put(mem,a+0x56,role,1);put(mem,t+0x56,target_role,1)
        put(mem,0x1c0161,mode,1);put(mem,0x1c0162,progression)
        yield (f'init-{ident}-{face:x}-{repetition}-{mode}-{role}-{target_role}-{progression}',mem,slot)
    for ident,face,selector,frame,timer,count in itertools.product((1,5,9),(0,0x8000),(0x2f,0x402f,0x802f,0xc02f),(0,1,2,3,4,5,0xfe,0x103,0x104),(0,1,0xff00),(0,1,2)):
        mem,a,t,p=seed(1);put(mem,a+2,ident);put(mem,a+0x2e,face);put(mem,a+0x60,selector)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x44,count)
        yield (f'tick-{ident}-{face:x}-{selector:x}-{frame}-{timer}-{count}',mem,1)
    for ident,mode,credit,owner_flags,partner_state,partner_sub in itertools.product((1,5),(0,1),(0,1),(0,2),(0,1),(0,1)):
        mem,a,t,p=seed(1);put(mem,a+2,ident);put(mem,a+0x24,3);put(mem,a+0x22,0);put(mem,a+0x44,1)
        put(mem,0x1c0161,mode,1);put(mem,0x1c007c,credit);put(mem,a+0x33,owner_flags,1)
        put(mem,p+0x20,partner_state);put(mem,p+0xae,partner_sub)
        yield (f'landing-{ident}-{mode}-{credit}-{owner_flags}-{partner_state}-{partner_sub}',mem,1)
    for ident,face in itertools.product((1,5,9),(0,0x8000)):
        mem,a,t,p=seed(1);put(mem,a+2,ident);put(mem,a+0x2e,face);put(mem,a+0x26,a,4)
        put(mem,a+0x24,3);put(mem,a+0x22,0);put(mem,a+0x44,1);put(mem,0x1c0161,1,1)
        yield (f'self-release-{ident}-{face:x}',mem,1)
    for frame,timer in itertools.product((1,4,0xfe),(1,0xff00)):
        mem,a,t,p=seed(1);put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,0x80000000,4)
        yield (f'released-link-{frame}-{timer}',mem,1)
    for scene,side,face,x,y,contact in itertools.product((0,1,2,3,5,6),(0,4),(0,0x8000),(0x180,0x250,0x380),(0x110,0x150,0x1a0),(0,3)):
        mem,a,t,p=seed(1);put(mem,a+0x1c,5);put(mem,a+0x56,0x80,1);put(mem,0x1c007e,scene)
        put(mem,a+0x32,0,1);put(mem,a+0x33,side,1);put(mem,a+0x2e,face);put(mem,a+6,x);put(mem,a+10,y);put(mem,a+0x36,contact)
        yield (f'resolver-{scene}-{side}-{face:x}-{x}-{y}-{contact}',mem,1)
    for slot,latched,frame,timer,count in itertools.product((0,4,8),(False,True),(0,1,0xfe),(0,1,0xff00),(0,1,2,80,0xffff)):
        mem,a,t,p=seed(slot);put(mem,a+0x60,0x81);put(mem,a+0x1c,0x8005 if latched else 5)
        put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x44,count)
        yield (f'reversal81-{slot}-{latched}-{frame}-{timer}-{count}',mem,slot)
    # Full successful throw, including its repeated frame1..3 phase, for all
    # descriptor/damage variants. Owner role7 makes ROM1129E return carry.
    for ident,face in itertools.product((1,5,9),(0,0x8000)):
        mem,a,t,p=seed(1);put(mem,a+0x1c,5);put(mem,a+2,ident);put(mem,a+0x56,0x80,1);put(mem,a+0x2e,face)
        regs=[rng.getrandbits(32) for _ in range(8)]
        for step in range(240):
            name=f'sequence2f-{ident}-{face}-{step}';yield (name,bytearray(mem),1,regs)
            out,regs=oracle(mem,1,regs,name);mem[0x1c0000:0x1c2000]=out[0]
            if int.from_bytes(mem[a+0x20:a+0x22],'big')==0:break
        assert int.from_bytes(mem[a+0x20:a+0x22],'big')==0
        assert int.from_bytes(mem[t+0x20:t+0x22],'big')==4
    mem,a,t,p=seed(1);put(mem,a+0x60,0x81);put(mem,a+0x1c,5)
    regs=[rng.getrandbits(32) for _ in range(8)]
    for step in range(80):
        name=f'sequence81-{step}';yield (name,bytearray(mem),1,regs)
        out,regs=oracle(mem,1,regs,name);mem[0x1c0000:0x1c2000]=out[0]
    assert int.from_bytes(mem[a+0x60:a+0x62],'big')==0xc07b
    assert int.from_bytes(mem[t+0x60:t+0x62],'big')==0x15
    mem,a,t,p=seed(1);put(mem,a+0x20,0xff)
    yield ('state-ff',mem,1)
    if args.snapshot:
        data=args.snapshot.read_bytes();manifest=json.loads(args.snapshot.with_name('action2f_manifest.json').read_text())
        assert hashlib.sha256(data).hexdigest()==manifest['sha256']
        frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
        assert len(data)==16472 and kind and pc==0x1c03e
        mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
        assert mem[actors[slot]+0x61]==0x2f
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
        if pc==0x2052:
            cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    out=[bytes(memory.r_block(at,n)) for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000))]
    out_regs=[cpu.r_reg(i) for i in range(8)]
    machine.cleanup()
    return out,out_regs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-action2f-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action2f.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action2f_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library))
        native.wf_action2f_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
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
        status=native.wf_action2f_oracle(nb,nr,slot)
        assert status==0,(name,'native action2f blocked',hex(-status))
        for (at,n),expected in zip(((0x1c0000,0x2000),(0xc0000,0x4000)),want):
            got=nb.raw[at:at+n]
            assert got==expected,(name,'memory',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x16c9e,0x16cb4,0x16cc6,0x1129e,0x1130c,0x1132a,0x11340,0x16cce,0x16cde,0x16cf8,0x16d10,0x16d28,0x16d2e,0x16d3c,0x16d50,0x16d54,0x16d90,0x16dc8,0x1108c,0x16dde,0x16df0,0x16e0a,0x16e18,0x16e20,0x1a7bc,0x1a7c2,0x1a7c8,0x1a7dc,0x1a7ee,0x1a7fe,0x1a810,0x10b62,0x280dc,0x10b9a,0x10bbc,0x21b4,0x1c0dc,0x1c10a,0x1c11e,0x1c0ba)
    if args.snapshot_only:required=(0x16c9e,0x16cb4,0x16cc6,0x1129e,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot) if args.snapshot and args.snapshot.exists() else None,'compared_memory':[] if args.rom_only else ['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':None if args.rom_only else 'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Selectors$2F/81 only; no gameplay owner intercepted','Dereferenced links identify actors; init RNG1129E uses canonical actor pointers','Resolver scenes0,1,2,3,5,6 (scene4 not tested); progression0..9']}
    if args.snapshot:report['snapshot_sha256']=hashlib.sha256(args.snapshot.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action2f unmodified-ROM fixtures; {len(required)} required branch sites')
