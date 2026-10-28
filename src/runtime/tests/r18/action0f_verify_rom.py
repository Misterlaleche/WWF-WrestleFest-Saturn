#!/usr/bin/env python3
"""Raw0F versus intact ROM; natural HOST capture matches all emulated actors.

Only the sound-output device is intercepted. No gameplay instructions are patched.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--snapshot-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'action0f_manifest.json').read_text());capture=(HERE/'action0f_snapshot_frame2759.bin').read_bytes();assert hashlib.sha256(capture).hexdigest()==meta['sha256']
assert rom[0x12614+15*4:0x12618+15*4]==bytes.fromhex('00013f78')
rng=random.Random(0x13f90);coverage=set();actors=[0x1c05b0+i*0x10c for i in range(9)]
def put(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(n*8))-1)).to_bytes(n,'big')
def natural():
    frame,slot,kind,pc,sequence,status=struct.unpack_from('=6I',capture)
    assert (frame,slot,kind,pc,sequence,status)==(2759,5,1,0x1c03e,971,1)
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=capture[56:8248]
    return 'natural-frame2759',mem,slot,struct.unpack_from('=8I',capture,24)
def seed(slot=5):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,a in enumerate(actors):
        put(mem,a,0x8001);put(mem,a+2,i);put(mem,a+6,640);put(mem,a+10,320);put(mem,a+14,320)
        put(mem,a+0x26,actors[(i+1)%9],4);put(mem,a+0x86,actors[(i+2)%9],4);put(mem,a+0x7e,0,4)
        for off in (0x32,0x33,0x34,0x35,0x37,0x56,0x74,0xb5,0xb6):put(mem,a+off,0,1)
        put(mem,a+0x2e,0)
    a=actors[slot];t=actors[(slot+1)%9]
    put(mem,a+0x20,0x8005);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x0f);put(mem,a+0x24,2);put(mem,a+0x22,1)
    put(mem,t+0x26,a,4);put(mem,t+0x20,4);put(mem,t+0x64,8)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1)
    return mem,a,t
def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,latched,selector,index,timer,contact,face in itertools.product((0,5,8),(False,True),(0x0f,0x400f,0x800f,0xc00f),(0,1,2,3,0xfe,0x100,0x102),(0,1,0xff00),(0,16),(0,0x8000)):
        mem,a,t=seed(slot);put(mem,a+0x1c,0x8005 if latched else 5);put(mem,a+0x60,selector);put(mem,a+0x24,index);put(mem,a+0x22,timer);put(mem,a+0x37,contact,1);put(mem,a+0x2e,face)
        yield f'phase-{slot}-{latched}-{selector:x}-{index:x}-{timer:x}-{contact}-{face:x}',mem,slot
    for face,dx,dy,kind,reciprocal,mode in itertools.product((0,0x8000),(-105,-104,-8,0,8,104,105),(-73,-72,0,24,25),range(5),(False,True),(0,1)):
        mem,a,t=seed();put(mem,a+0x37,16,1);put(mem,a+6,640+dx);put(mem,a+10,320+dy);put(mem,t+0x2e,face);put(mem,0x1c0161,mode,1)
        put(mem,t+0x20,4 if kind<2 else 5);put(mem,t+0x64,8+kind);put(mem,t+0x60,(0x61,0x54,0x0f)[max(0,kind-2)])
        if not reciprocal:put(mem,t+0x26,actors[1],4)
        yield f'landing-{face:x}-{dx}-{dy}-{kind}-{reciprocal}-{mode}',mem,5
    for credit,af,tf,face,mode in itertools.product((0,1),(0,1,4,5,255),(0,1,255),(0,0x8000),(0,1)):
        mem,a,t=seed();put(mem,a+0x37,16,1);put(mem,a+6,641);put(mem,a+0x33,af,1);put(mem,t+0x33,tf,1);put(mem,a+0x2e,face);put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1)
        yield f'gating-{credit}-{af}-{tf}-{face:x}-{mode}',mem,5
    # Make each $213A6 cleanup observable, including TAG's alternate +7E.
    for mode,flags,tag in itertools.product((0,1),(0,2,255),(0,0x80000000)):
        mem,a,t=seed();put(mem,a+0x37,16,1);put(mem,a+6,641);put(mem,t+0x20,5);put(mem,t+0x60,0x61)
        put(mem,t+0x26,actors[1],4);put(mem,t+0x7e,actors[7]|tag,4);put(mem,0x1c0161,mode,1)
        for actor in actors:
            put(mem,actor+0x33,flags,1)
            for off in (0x34,0x56,0xb5):put(mem,actor+off,255,1)
        yield f'interrupt-cleanup-{mode}-{flags}-{tag:x}',mem,5
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action0f-',ignore_cleanup_errors=os.name=='nt') as temp:
    lib=Path(temp)/'action0f.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action0f_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(lib));native.wf_action0f_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3];regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];count+=1
        mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem));descriptor=0
        for i in range(8,15):cpu.w_reg(i,0)
        for i,v in enumerate(regs):cpu.w_reg(i,v)
        cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
        for _ in range(10000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x1c0c8:descriptor=cpu.r_reg(9)
            if pc==0x70000:break
            if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.wf_action0f_oracle(nb,nr,slot,ctypes.byref(nd))
        assert status==0,(name,'native blocked',hex(-status))
        assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];want=bytes(memory.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
        want=[cpu.r_reg(i) for i in range(8)];assert list(nr)==want,(name,'D0-D7',list(nr),want)
        machine.cleanup()
    required=(0x13fa2,0x13fd8,0x13fec,0x14002,0x14028,0x140a4,0x140e0,0x14100,0x14186,0x141d0,0x141d6)
    if not args.snapshot_only:assert all(pc in coverage for pc in required),[hex(pc) for pc in required if pc not in coverage]
    report={'passed':count,'rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':meta['sha256'],'actions_source_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'compared':['RAM1C0000..1C1FFF','FG0C0000..0C3FFF','D0-D7','RNG','tick descriptor'],'covered_rom_sites':[hex(x) for x in required if x in coverage],'intercepts':['2052 sound output only'],'limitations':['Isolated owner calls and derived boundaries; no complete scheduler, device sound or hardware timing claim']}
    if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {count} raw0F fixtures; RAM/FG/D0-D7/RNG/descriptor')
