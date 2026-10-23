#!/usr/bin/env python3
"""Raw $03: unchanged arcade ROM versus the real bounded C dispatcher.

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
parser.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action03_snapshot_f88acad.bin'))
parser.add_argument('--report',type=Path)
parser.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN')
parser.add_argument('--rom-only',action='store_true')
parser.add_argument('--snapshot-only',action='store_true')
args=parser.parse_args()
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12620:0x12624]==bytes.fromhex('00012ae6')
assert rom[0x12ae6:0x12afe]==bytes.fromhex('00012b1600010004000800080008000801aa01ab81000000')
assert rom[0x12afe:0x12b16]==bytes.fromhex('00012b16000100040008000800080008009d009e81000000')
rng=random.Random(0x12b16)
actors=[0x1c05b0+0x10c*i for i in range(9)]
coverage=set()

def put(mem,at,value,width=2):
    mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')

def seed(slot):
    mem=bytearray(0x200000);mem[:len(rom)]=rom
    mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,at in enumerate(actors):
        put(mem,at,0x8000);put(mem,at+2,i)
        put(mem,at+6,0x250+12*i);put(mem,at+10,0x150+3*i)
        put(mem,at+0x26,actors[(i+1)%9],4)
        put(mem,at+0x92,actors[(i+1)%9],4)
    a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x4003)
    put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,t+0x64,0x11);put(mem,t+0x92,a,4)
    return mem,a,t,p

def fixtures():
    for slot,sel,face,latched,frame,timer in itertools.product((0,4,8),(3,0x4003,0x8003,0xc003),(0,0x8000),(False,True),(0,1,2,3,0xfe,0x100,0x101,0x1fe),(0,1,0xff00)):
        mem,a,t,p=seed(slot);put(mem,a+0x60,sel);put(mem,a+0x2e,face)
        put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x24,frame);put(mem,a+0x22,timer)
        yield (f'tick-{slot}-{sel:x}-{face:x}-{latched}-{frame}-{timer}',mem,slot)
    for related,state4,back in itertools.product(('null','tagzero','actor','tagactor'),(0,0x11,0x111,0x12),('null','owner','tagowner','other')):
        mem,a,t,p=seed(4)
        put(mem,a+0x92,0 if related=='null' else 0x80000000 if related=='tagzero' else t|(0x80000000 if related=='tagactor' else 0),4)
        put(mem,t+0x64,state4)
        put(mem,t+0x92,0 if back=='null' else a if back=='owner' else a|0x80000000 if back=='tagowner' else p,4)
        yield (f'acquire-{related}-{state4}-{back}',mem,4)
    # Dispatcher A2 may be its null-link ROM sentinel; $10B9A rereads
    # the literal link. Writes to either ROM range have no compared RAM effect.
    for slot,face,link in itertools.product((0,4,8),(0,0x8000),('null','tagzero','actor','tagactor','self')):
        mem,a,t,p=seed(slot);put(mem,a+0x60,0xc003);put(mem,a+0x2e,face)
        put(mem,a+0x24,1);put(mem,a+0x22,0)
        put(mem,a+0x26,0 if link=='null' else 0x80000000 if link=='tagzero' else a if link=='self' else t|(0x80000000 if link=='tagactor' else 0),4)
        yield (f'release-{slot}-{face:x}-{link}',mem,slot)
    mem,a,t,p=seed(4);put(mem,a+0x20,0xff)
    yield ('state-ff',mem,4)
    if args.snapshot:
        data=args.snapshot.read_bytes()
        manifest=json.loads(args.snapshot.with_name('action03_manifest.json').read_text())
        assert hashlib.sha256(data).hexdigest()==manifest['sha256']
        frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',data)
        assert len(data)==16472 and kind and pc==0x1c03e
        mem=bytearray(0x200000);mem[:len(rom)]=rom
        mem[0x1c0000:0x1c2000]=data[56:8248]
        a=actors[slot];assert mem[a+0x61]==3
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
with tempfile.TemporaryDirectory(prefix='wf-action03-') as temp:
    if not args.rom_only:
        library=Path(temp)/'action03.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action03_oracle_bridge.c','-o',str(library)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(library))
        native.wf_action03_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
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
        status=native.wf_action03_oracle(nb,nr,slot)
        assert status==0,(name,'native action03 blocked',hex(-status))
        for (at,n),expected in zip(((0x1c0000,0x2000),(0xc0000,0x4000)),want):
            got=nb.raw[at:at+n]
            assert got==expected,(name,'memory',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==want_regs,(name,'D0-D7',list(nr),want_regs)
    required=(0x12b16,0x12b1e,0x12b2a,0x12b34,0x12b42,0x12b4e,0x12b5a,0x12b62,0x12b76,0x12b8c,0x12bb0,0x12bb6,0x12bbe,0x12bca,0x12bd2,0x10b9e,0x10bbc,0x1c0dc,0x1c0f4,0x1c10a,0x1c0ba)
    if args.snapshot_only:required=(0x12b16,0x12b1e,0x1c0dc)
    assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
    report={'count':count,'passed':0 if args.rom_only else count,'mode':'ROM execution only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot':str(args.snapshot) if args.snapshot and args.snapshot.exists() else None,'compared_memory':[] if args.rom_only else ['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':None if args.rom_only else 'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':['$2052 sound output only'],'limitations':['Non-ROM related/link pointers identify actors','Selector$03 only; no gameplay owners intercepted; RNG state RAM compared unchanged']}
    if args.snapshot:report['snapshot_sha256']=hashlib.sha256(args.snapshot.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action03 unmodified-ROM fixtures; {len(required)} required branch sites')
