#!/usr/bin/env python3
"""CPU state0 B7.bit7: real frame3049 and full $1C390 versus intact ROM.

Catches lost prefix priority/masks, guessed 50/50 choices, skipped target-X
boundaries, omitted resolver contacts or corrupted D-register/RNG context.
No ROM instructions or owners are patched/intercepted.
"""
import argparse,ctypes,hashlib,itertools,json,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--rom-only',action='store_true');p.add_argument('--natural-only',action='store_true');args=p.parse_args()
manifest=json.loads((HERE/'cpu_state0_b7_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1'];assert rom[0x1c8f2:0x1c8f4]==bytes.fromhex('3232')
data=(HERE/'cpu_state0_b7_snapshot_active.bin').read_bytes();assert len(data)==16472 and hashlib.sha256(data).hexdigest()==manifest['sha256'];assert struct.unpack_from('=6I',data)==tuple(manifest['header'])
rng=random.Random(0x1c390);actors=[0x1c05b0+0x10c*i for i in range(9)];coverage=set();choices={0:0,1:0};result_states={}
def put(mem,at,v,width=2):mem[at:at+width]=(v&((1<<(width*8))-1)).to_bytes(width,'big')
def word(mem,at):return int.from_bytes(mem[at:at+2],'big')
def natural():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248];return ('real-frame3049-slot4',mem,4,struct.unpack_from('=8I',data,24))
def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    a=actors[slot];t=actors[(slot+1)%9];put(mem,a+0x20,0x8000);put(mem,a+0x7a,t,4)
    put(mem,a+0x32,0);put(mem,a+0x34,1,1);put(mem,a+0xb5,0,1);put(mem,a+0xb6,0x80);put(mem,a+0xb0,0x42)
    put(mem,a+6,0x290);put(mem,a+10,0x150);put(mem,a+14,0x140);put(mem,a+0x2e,0)
    for off in (0x36,0x3e,0x40,0x42):put(mem,a+off,0)
    put(mem,t+0x20,0);put(mem,t+0x64,0);put(mem,t+0x60,0);put(mem,t+6,0x280);put(mem,t+0x2e,0)
    put(mem,0x1c0161,0,1);put(mem,0x1c007e,0);put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t

def fixtures():
    yield natural()
    if args.natural_only:return
    for b7,b5,mode in itertools.product(range(128,256),(0,0x80),(0,1,2,3)):
        mem,a,t=seed();put(mem,a+0xb7,b7,1);put(mem,a+0xb5,b5,1);put(mem,0x1c0161,mode,1)
        yield (f'b7-byte-{b7:x}-{b5:x}-{mode}',mem,4)
    for b5,b6,flags34,mode in itertools.product(range(256),(0,0x10,0x40,0x50),(1,0x11),(0,3)):
        mem,a,t=seed();put(mem,a+0xb5,b5,1);put(mem,a+0xb6,b6,1);put(mem,a+0x34,flags34,1);put(mem,0x1c0161,mode,1)
        yield (f'prefix-priority-{b5:x}-{b6:x}-{flags34:x}-{mode}',mem,4)
    # Independent target-face/X and relative-distance probe boundaries.
    for slot,ownx,targetx,face,distance in itertools.product((0,1,4,5,8),(0,0x26f,0x270,0x29f,0x2a0,0xffff),(0x26f,0x270,0x29f,0x2a0),(0,0x8000),(0,0xaf,0xb0,0xc0,0xc1,0xffff)):
        mem,a,t=seed(slot);put(mem,a+6,ownx);put(mem,t+6,targetx);put(mem,t+0x2e,face);put(mem,a+0xb0,distance)
        yield (f'probe-{slot}-{ownx:x}-{targetx:x}-{face:x}-{distance:x}',mem,slot)
    for scene,side,skip,x,y,z,contact in itertools.product(range(7),(0,4),(0,0x20),(0x180,0x250,0x380),(0x110,0x150,0x1a0),(0x100,0x140,0x180),(0,1,0x100,0x80)):
        mem,a,t=seed();put(mem,0x1c007e,scene);put(mem,a+0x32,(skip<<8)|side);put(mem,a+6,x);put(mem,a+10,y);put(mem,a+14,z);put(mem,a+0x36,contact)
        yield (f'resolver-{scene}-{side}-{skip}-{x:x}-{y:x}-{z:x}-{contact:x}',mem,4)
    for state,sub,sel,face in itertools.product((0,4,5),(0,8,9),(0,0x22,0x52),(0,0x8000,0x0080,0xffff)):
        mem,a,t=seed();put(mem,t+0x20,state);put(mem,t+0x64,sub);put(mem,t+0x60,sel);put(mem,t+0x2e,face)
        yield (f'face-offset-{state}-{sub}-{sel:x}-{face:x}',mem,4)
    for state in (0,0x8000):
        mem,a,t=seed();put(mem,a+0x20,state);yield (f'state-high-{state:x}',mem,4)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,v in enumerate(regs):cpu.w_reg(i,v)
    cpu.w_reg(8,actors[slot]);cpu.w_reg(9,0x1c2cc);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c2cc)
    for _ in range(5000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x1c3a4:choices[cpu.r_reg(0)&255]+=1
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    outregs=[cpu.r_reg(i) for i in range(8)];key=(memory.r16(actors[slot]+0x20),memory.r16(actors[slot]+0xae));result_states[str(key)]=result_states.get(str(key),0)+1;machine.cleanup();return result,outregs
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-state0-b7-') as temp:
    if not args.rom_only:
        lib=Path(temp)/'state0b7.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state0_b7_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.wf_cpu_state0_b7_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        want,wantregs=oracle(mem,slot,regs,name)
        if args.rom_only:continue
        nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.wf_cpu_state0_b7_oracle(nb,nr,slot)
        assert status==0,(name,'native CPU blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n];assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==wantregs,(name,'D0-D7',list(nr),wantregs)
    required=(0x1c2cc,0x10be8,0x1c2e8,0x1c31a,0x1c334,0x1c358,0x1c378,0x1c388,0x1c398,0x24cc,0x21b4,0x24e4,0x1c3ac,0x1c3b2,0x1c3b8,0x1c3c6,0x1df1e,0x1df2a,0x1df34,0x1df3c,0x1df52,0x1df7e,0x1df98,0x1df9e,0x280dc,0x1dfb2,0x1dfc8)
    if not args.natural_only:assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc) for pc in required if pc not in coverage])
    assert 0x2052 not in coverage
    report={'count':count,'passed':0 if args.rom_only else count,'natural_count':1,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':manifest['rom_sha1'],'snapshot_sha256':manifest['sha256'],'cpu_source_sha256':hashlib.sha256((ROOT/'src/wf_cpu_runtime.c').read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','weighted_choices':choices,'state_ae_results':result_states,'intercepted_rom_owners':[],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['Reached B7.bit7 prefix only; unrelated aggregate guards remain explicit dependencies','Derived boundary cases are not claimed as natural whole-match coverage']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} state0 B7 fixtures; 1 natural root; {len(required)} sites; RAM/FG/D0-D7/RNG')
