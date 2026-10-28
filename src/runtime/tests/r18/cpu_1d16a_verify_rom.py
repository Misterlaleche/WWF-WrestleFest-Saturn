#!/usr/bin/env python3
"""CPU1D16A: natural state1/AE7 versus intact ROM, including full TAG request.

Catches missing substate, BYTE/WORD latch errors, WORD angle negation,
unsigned Y distance equality, skipped facing, lost tagged partner pointers/global
writes and accidental register/RNG changes. No ROM owner is intercepted.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--rom-only',action='store_true');p.add_argument('--natural-only',action='store_true');args=p.parse_args()
manifest=json.loads((HERE/'cpu_1d16a_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1'];assert rom[0x1c908+7*4:0x1c90c+7*4]==bytes.fromhex('0001d16a')
data=(HERE/'cpu_1d16a_snapshot_active.bin').read_bytes();assert len(data)==16472 and hashlib.sha256(data).hexdigest()==manifest['sha256'];assert struct.unpack_from('=6I',data)==tuple(manifest['header'])
rng=random.Random(0x1d16a);actors=[0x1c05b0+0x10c*i for i in range(9)];coverage=set();transitions=0;requests=0

def put(mem,at,v,width=2):mem[at:at+width]=(v&((1<<(width*8))-1)).to_bytes(width,'big')
def natural():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248];return ('real-frame3098-slot4',mem,4,struct.unpack_from('=8I',data,24))
def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+2)%9]
    put(mem,a+0x20,0x8001);put(mem,a+0xae,7);put(mem,a+0x60,0x0050);put(mem,a+0x7a,t,4);put(mem,a+0x86,partner,4)
    put(mem,a+0x34,1,1);put(mem,a+0xb5,0,1);put(mem,a+0xb2,0);put(mem,a+6,0x280);put(mem,a+0x2e,0)
    put(mem,t+0x20,0);put(mem,t+0x64,0);put(mem,t+0x60,0);put(mem,t+6,0x290);put(mem,t+10,0x160);put(mem,t+0x2e,0)
    put(mem,0x1c0161,0,1);put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t,partner

def fixtures():
    yield natural()
    if args.natural_only:return
    for b5,high,distance in itertools.product(range(256),(0,0x20,0xff),(0,4,5,6,0x7fff,0xffff)):
        mem,a,t,partner=seed();put(mem,a+0xb5,b5,1);put(mem,a+0xae,(high<<8)|7);put(mem,a+0xb2,distance)
        yield (f'init-b5-{b5:x}-{high:x}-{distance:x}',mem,4)
    for flags33,flags34,tag in itertools.product(range(256),(2,0x82),(0,0x80000000)):
        mem,a,t,partner=seed(flags33%9);put(mem,a+0x33,flags33,1);put(mem,a+0x34,flags34,1);put(mem,a+0x86,partner|tag,4)
        yield (f'tag-request-{flags33:x}-{flags34:x}-{tag:x}',mem,flags33%9)
    for slot,tag,state,sub,selector,face in itertools.product((0,1,4,5,8),(0,0x80000000),(0,4,5),(8,9),(0x22,0x52),(0,0x8000)):
        mem,a,t,partner=seed(slot);put(mem,a+0x7a,t|tag,4);put(mem,t+0x20,state);put(mem,t+0x64,sub);put(mem,t+0x60,selector);put(mem,t+0x2e,face);put(mem,t+6,0x270)
        yield (f'face-{slot}-{tag:x}-{state}-{sub}-{selector:x}-{face:x}',mem,slot)
    for targetalias,partneralias,face,latched in itertools.product((False,True),(False,True),(0,0x8000),(False,True)):
        mem,a,t,partner=seed();put(mem,a+0x34,2,1);put(mem,a+0x33,0x81,1);put(mem,a+0x2e,face);put(mem,a+0xae,0x2007 if latched else 7)
        if targetalias:put(mem,a+0x7a,a|0x80000000,4)
        if partneralias:put(mem,a+0x86,a|0x80000000,4)
        yield (f'alias-{targetalias}-{partneralias}-{face:x}-{latched}',mem,4)
    for high in range(256):
        mem,a,t,partner=seed();put(mem,a+0xae,(high<<8)|7);yield (f'ae-high-byte-{high:x}',mem,4)

def oracle(mem,slot,regs,name):
    global transitions,requests
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,v in enumerate(regs):cpu.w_reg(i,v)
    cpu.w_reg(8,actors[slot]);cpu.w_reg(9,0x1c8f4);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c8f4);visited=set()
    for _ in range(1000):
        pc=cpu.r_pc();coverage.add(pc);visited.add(pc)
        if pc==0x70000:break
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    transitions+=0x1d1de in visited;requests+=0x1c5fa in visited
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    outregs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,outregs
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-cpu1d078-',ignore_cleanup_errors=os.name=='nt') as temp:
    if not args.rom_only:
        lib=Path(temp)/'cpu1d078.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_1d078_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.wf_cpu_1d078_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for case in fixtures():
        name,mem,slot=case[:3];count+=1;regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        want,wantregs=oracle(mem,slot,regs,name);assert want[0x1c005c:0x1c0060]==mem[0x1c005c:0x1c0060],(name,'unexpected RNG write')
        if args.rom_only:continue
        nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.wf_cpu_1d078_oracle(nb,nr,slot)
        assert status==0,(name,'native CPU blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n];assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==wantregs,(name,'D0-D7',list(nr),wantregs)
    required=(116980, 119146, 119154, 119160, 116218, 119168, 68584, 119182, 119196, 119210, 119214, 119222, 119230, 119242, 119258, 122850, 119262, 119278)
    if not args.natural_only:assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc) for pc in required if pc not in coverage])
    assert 0x21b4 not in coverage and 0x2052 not in coverage
    report={'count':count,'passed':0 if args.rom_only else count,'natural_count':1,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':manifest['rom_sha1'],'snapshot_sha256':manifest['sha256'],'cpu_source_sha256':hashlib.sha256((ROOT/'src/wf_cpu_runtime.c').read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','sub0_transition_cases':transitions,'tag_request_cases':requests,'rng_calls':0,'intercepted_rom_owners':[],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'limitations':['CPU AE7, includes selector50 tail into state5; other selectors are outside the derived fixtures','Only the first snapshot is naturally reached; all boundary cases are explicitly derived']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} CPU1D16A fixtures; 1 natural root; {len(required)} sites; RAM/FG/D0-D7/RNG')
