#!/usr/bin/env python3
"""Independent state0 B7.bit0/bit4 entry versus unchanged ROM1C2CC.

The old entry gate only reached this existing prefix for other trigger bits.
Natural A600 frame767 is also reproduced on the delivered emulated Saturn.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');a=p.parse_args()
meta=json.loads((HERE/'cpu_state0_b7_entry_manifest.json').read_text());capture=(HERE/'cpu_state0_b7_entry_snapshot_A600.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['sha256']
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
actors=[0x1c05b0+i*0x10c for i in range(9)];rng=random.Random(0x1c370);coverage=set()
def put(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    own=actors[slot];target=actors[(slot+1)%9]
    put(mem,own+0x20,0x8000);put(mem,own+0x7a,target,4);put(mem,own+0x32,0);put(mem,own+0x34,0,1)
    put(mem,own+0xb5,0,1);put(mem,own+0xb6,1);put(mem,own+0xb9,0,1)
    put(mem,target+0x20,0);put(mem,target+0x64,0);put(mem,target+0x60,0);put(mem,target+0x2e,0)
    put(mem,0x1c0161,0,1);return mem,own,target
def fixtures():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=capture[56:8248]
    yield 'natural-A600-frame767',mem,1,struct.unpack_from('=8I',capture,24)
    if a.natural_only:return
    for slot,b7,mode in itertools.product((0,1,8),(v for v in range(256) if v&0x11),range(4)):
        mem,own,target=seed(slot);put(mem,own+0xb7,b7,1);put(mem,0x1c0161,mode,1)
        yield f'entry-{slot}-{b7:x}-{mode}',mem,slot
    for b7,flags34,b5,b6,mode in itertools.product((1,0x10,0x11,0x91),(0,0x10),(0,8,0x80),(0,0x10,0x40,0x50),range(4)):
        mem,own,target=seed();put(mem,own+0xb7,b7,1);put(mem,own+0x34,flags34,1);put(mem,own+0xb5,b5,1);put(mem,own+0xb6,b6,1);put(mem,0x1c0161,mode,1)
        yield f'priority-{b7:x}-{flags34:x}-{b5:x}-{b6:x}-{mode}',mem,1
    for x,tx,state,face,b7 in itertools.product((0,0x7fff,0xffff),(0,0x8000,0xffff),(0,4,5),(0,0x8000,0x0080,0xffff),(1,0x10)):
        mem,own,target=seed();put(mem,own+6,x);put(mem,target+6,tx);put(mem,target+0x20,state);put(mem,target+0x64,9);put(mem,target+0x60,0x52);put(mem,target+0x2e,face);put(mem,own+0xb7,b7,1)
        yield f'face-{x:x}-{tx:x}-{state}-{face:x}-{b7:x}',mem,1
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-b7-entry-',ignore_cleanup_errors=os.name=='nt') as temp:
    lib=Path(temp)/'entry.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state0_b7_entry_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(lib));native.wf_cpu_state0_b7_entry_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3];regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];count+=1
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000);machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
        for i in range(8,15):cpu.w_reg(i,0)
        for i,v in enumerate(regs):cpu.w_reg(i,v)
        cpu.w_reg(8,actors[slot]);cpu.w_reg(9,0x1c2cc);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c2cc)
        for _ in range(2000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));status=native.wf_cpu_state0_b7_entry_oracle(nb,nr,slot)
        assert status==0,(name,'native blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];want=bytes(memory.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
        assert list(nr)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7')
        assert bytes(memory.r_block(0x1c005c,4))==mem[0x1c005c:0x1c0060],(name,'RNG')
        machine.cleanup()
    if not a.natural_only:
        for b5 in (8,9,0x48,0x4b):
            mem,own,target=seed();put(mem,own+0x34,0x10,1);put(mem,own+0xb5,b5,1);put(mem,own+0xb7,0,1)
            nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)])
            assert native.wf_cpu_state0_b7_entry_oracle(nb,nr,1)==-0x1c2cc,('unrelated guard removed',b5)
        for pc in (0x1c370,0x1c378,0x1c37e,0x1c388):assert pc in coverage,hex(pc)
    report={'passed':count,'rom_sha1':meta['rom_sha1'],'snapshot_sha256':meta['sha256'],'cpu_source_sha256':hashlib.sha256((ROOT/'src/wf_cpu_runtime.c').read_bytes()).hexdigest(),'compared':['RAM1C0000..1C1FFF','FG0C0000..0C3FFF','D0-D7','RNG'],'covered_rom_sites':[hex(pc) for pc in (0x1c2cc,0x1c370,0x1c378,0x1c37e,0x1c388) if pc in coverage],'unrelated_controls':0 if a.natural_only else 4,'intercepts':[],'limitations':'Isolated CPU calls; original video evidence has no physical RAM dump.'}
    if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {count} CPU state0 independent B7 entries; RAM/FG/D0-D7/RNG')
