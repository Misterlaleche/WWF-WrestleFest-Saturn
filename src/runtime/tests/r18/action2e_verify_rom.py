#!/usr/bin/env python3
"""Raw2E: real frame5918 snapshot and derived boundaries versus intact ROM.

Catches missing owner, lost tagged capture pointers, BYTE/WORD phase guards,
wrong linked coordinates/cleanup, damage and partner flags, live landing RNG,
and sentinel contact progression. Only the sound device at $2052 is intercepted.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--rom-only',action='store_true');p.add_argument('--snapshot-only',action='store_true');p.add_argument('--actions-source',type=Path)
args=p.parse_args();manifest=json.loads((HERE/'action2e_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
assert rom[0x12614+0x2e*4:0x12618+0x2e*4]==bytes.fromhex('00016a56')
assert rom[0x16a56:0x16a7a]==bytes.fromhex('00016a7a0001000700200002000800090006ff00001000b600b700b800b900ba00bb00bc')
assert rom[0x25ca+9*6:0x25ca+10*6]==bytes.fromhex('008004000040')
data=(HERE/'action2e_snapshot_main.bin').read_bytes();assert len(data)==16472 and hashlib.sha256(data).hexdigest()==manifest['sha256'];assert struct.unpack_from('=6I',data)==tuple(manifest['header'])
rng=random.Random(0x16a7a);actors=[0x1c05b0+0x10c*i for i in range(9)];coverage=set();rng_samples={}
def put(mem,at,value,width=2):mem[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'big')
def word(mem,at):return int.from_bytes(mem[at:at+2],'big')
def natural():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data[56:8248]
    return ('real-frame5918-slot1',mem,1,struct.unpack_from('=8I',data,24))
def seed(slot=1):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    for i,a in enumerate(actors):
        put(mem,a+2,i%12);put(mem,a+0x20,4);put(mem,a+0x64,0x1f);put(mem,a+0x2e,0)
        put(mem,a+0x26,actors[(i+1)%9],4);put(mem,a+0x86,actors[(i+2)%9],4)
    a=actors[slot];t=actors[(slot+1)%9];partner=actors[(slot+3)%9]
    put(mem,a+0x20,5);put(mem,a+0x1c,0x8005);put(mem,a+0x60,0x802e);put(mem,a+0x24,0);put(mem,a+0x22,1)
    put(mem,a+0x33,0,1);put(mem,a+0x92,0,4);put(mem,a+0x37,0,1)
    put(mem,partner+0x20,1);put(mem,partner+0xae,1)
    put(mem,0x1c007c,1);put(mem,0x1c0161,0,1);put(mem,0x1c005c,rng.getrandbits(32),4)
    return mem,a,t,partner

def fixtures():
    yield natural()
    if args.snapshot_only:return
    for slot,selector,frame,timer in itertools.product((0,1,4,5,8),(0x2e,0x402e,0x802e,0xc02e),(0,1,2,3,4,5,6,0xfe,0x101,0x102,0x105,0x1fe,0xffff),(0,1,2,0xff00,0xffff)):
        mem,a,t,partner=seed(slot);put(mem,a+0x60,selector);put(mem,a+0x24,frame);put(mem,a+0x22,timer)
        yield (f'phase-{slot}-{selector:x}-{frame:x}-{timer:x}',mem,slot)
    for slot,selector,link in itertools.product((0,1,5,8),(0x2e,0x402e,0x802e,0xc02e),(0,0x80000000,0xdeadbeef)):
        mem,a,t,partner=seed(slot);put(mem,a+0x1c,5);put(mem,a+0x60,selector);put(mem,a+0x92,0x801c06bc,4);put(mem,a+0x26,link,4)
        yield (f'init-no-target-{slot}-{selector:x}-{link:x}',mem,slot)
    for slot,state,sub,tag,timer in itertools.product((0,1,5,8),(0,4,0x104,5,0xff),(0x1f,0x101f,0x1e),(0,0x80000000),(0,1,0xff00)):
        mem,a,t,partner=seed(slot);put(mem,a+0x60,0x402e);put(mem,a+0x92,t|tag,4);put(mem,t+0x20,state);put(mem,t+0x64,sub);put(mem,a+0x22,timer)
        yield (f'capture-{slot}-{state:x}-{sub:x}-{tag:x}-{timer:x}',mem,slot)
    for face,frame,timer in itertools.product((0,0x8000,0x0080,0x8101,0xffff),(0,1,0x101,0x1fe),(0,1,0xff00)):
        if frame&255==0xfe and timer:continue # FE dereferences its link; a released null is outside the bounded actor pool.
        mem,a,t,partner=seed();put(mem,a+0x60,0x2e);put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x26,0x80000000,4);put(mem,a+0x2e,face)
        yield (f'abort-no-link-{face:x}-{frame:x}-{timer:x}',mem,1)
    for frame,contact,timer,sound in itertools.product((5,0x105),(0,0x10,0xff),(0,1,0xff00),(0,4)):
        mem,a,t,partner=seed();put(mem,a+0x24,frame);put(mem,a+0x22,timer);put(mem,a+0x37,contact,1);put(mem,a+0x33,sound,1)
        yield (f'contact-{frame:x}-{contact:x}-{timer:x}-{sound}',mem,1)
    for ident,face,credit,flags33,mode,partnerpair in itertools.product(range(12),(0,0x8000),(0,1),(0,2),(0,1),((1,1),(0,1),(1,0),(0x101,0x101))):
        mem,a,t,partner=seed(ident%9);put(mem,a+2,ident);put(mem,a+0x24,0xfe);put(mem,a+0x22,0);put(mem,a+0x2e,face);put(mem,a+0x33,flags33,1)
        put(mem,0x1c007c,credit);put(mem,0x1c0161,mode,1);put(mem,partner+0x20,partnerpair[0]);put(mem,partner+0xae,partnerpair[1])
        yield (f'finish-rng-{ident}-{face:x}-{credit}-{flags33}-{mode}-{partnerpair}',mem,ident%9)
    for slot,frame,tag,face,edge,c7 in itertools.product((0,1,5,8),(0xfe,0x1fe),(0,0x80000000),(0,0x8000),(0,0x7fff,0xffff),(0,252,255)):
        mem,a,t,partner=seed(slot);put(mem,a+0x24,frame);put(mem,a+0x26,t|tag,4);put(mem,a+0x2e,face);put(mem,a+6,edge);put(mem,a+10,edge);put(mem,a+14,edge);put(mem,t+0xc7,c7,1)
        yield (f'finish-wrap-{slot}-{frame:x}-{tag:x}-{face:x}-{edge:x}-{c7}',mem,slot)
    for face,mode,credit in itertools.product((0,0x8000),(0,1),(0,1)):
        mem,a,t,partner=seed();put(mem,a+0x24,0xfe);put(mem,a+0x26,a,4);put(mem,a+0x2e,face);put(mem,0x1c0161,mode,1);put(mem,0x1c007c,credit)
        yield (f'finish-self-alias-{face:x}-{mode}-{credit}',mem,1)
    mem,a,t,partner=seed();put(mem,a+0x20,0xff);yield ('disabled-owner',mem,1)

def oracle(mem,slot,regs,name):
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
    for i in range(8,15):cpu.w_reg(i,0)
    for i,v in enumerate(regs):cpu.w_reg(i,v)
    cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
    for _ in range(10000):
        pc=cpu.r_pc();coverage.add(pc)
        if pc==0x70000:break
        if pc==0x110a6:
            ident=word(mem,actors[slot]+2);rng_samples.setdefault(ident,set()).add(cpu.r_reg(0)&255)
        if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
    result=bytearray(mem)
    for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):result[at:at+n]=bytes(memory.r_block(at,n))
    outregs=[cpu.r_reg(i) for i in range(8)];machine.cleanup();return result,outregs

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split();actions=args.actions_source or ROOT/'src/wf_r17_actions_r11.c'
if args.actions_source:core=[str(args.actions_source.resolve()) if f=='src/wf_r17_actions_r11.c' else f for f in core]
with tempfile.TemporaryDirectory(prefix='wf-action2e-') as temp:
    if not args.rom_only:
        lib=Path(temp)/'action2e.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action2e_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.wf_action2e_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0;natural_ticks=0;derived_ticks=0
    def compare(case):
        global count
        name,mem,slot=case[:3];regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];count+=1
        want,wantregs=oracle(mem,slot,regs,name)
        if args.rom_only:return want,wantregs
        nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.wf_action2e_oracle(nb,nr,slot)
        assert status==0,(name,'native action2e blocked',hex(-status))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];expected=want[at:at+n]
            assert got==expected,(name,'memory',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,expected)) if g!=e][:16])
        assert list(nr)==wantregs,(name,'D0-D7',list(nr),wantregs)
        return want,wantregs
    for case in fixtures():case[1][0xc0000:0xc4000]=rng.randbytes(0x4000);compare(case)
    if not args.snapshot_only:
        name,mem,slot,regs=natural()
        for tick in range(160):
            mem,regs=compare((f'natural-isolated-{tick}',mem,slot,regs));natural_ticks+=1
            if mem[actors[slot]+0x21]!=5:break
        for ident,face,initial_state in itertools.product((0,1,5,8,11),(0,0x8000),(5,0x8005)):
            mem,a,t,partner=seed(ident%9);put(mem,a+2,ident);put(mem,a+0x60,0x402e);put(mem,a+0x22,0);put(mem,a+0x92,t|0x80000000,4);put(mem,a+0x2e,face)
            put(mem,a+0x20,initial_state)
            regs=[rng.getrandbits(32) for _ in range(8)]
            for tick in range(160):
                # Derived contact is an external scheduler condition, not claimed natural physics.
                if word(mem,a+0x24)==5:put(mem,a+0x37,0x10,1)
                mem,regs=compare((f'derived-captured-{ident}-{face:x}-{initial_state:x}-{tick}',mem,ident%9,regs));derived_ticks+=1
                if mem[a+0x21]!=5:break
            assert word(mem,a+0x20)==7 and word(mem,t+0x20)==4
    required=(0x16a82,0x16aaa,0x16ab6,0x16ac2,0x16aca,0x16b00,0x16b06,0x16b42,0x16b5c,0x16b96,0x16bbe,0x16be2,0x16bee,0x16c00,0x16c12,0x16c3e,0x16c56,0x258e,0x25c0,0x10b9a,0x10bd0,0x1108c,0x110a0,0x110bc,0x110ee,0x11102,0x1110e,0x1111e,0x2052,0x21b4,0x1c0dc,0x1c0fa,0x1c10a,0x1c0ba)
    if not args.snapshot_only:
        assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc) for pc in required if pc not in coverage])
        for ident,chance in enumerate((136,128,68,64,120,120,80,128,128,133,128,64)):
            assert min(rng_samples[ident])<chance<=max(rng_samples[ident]),('missing RNG result',ident)
    report={'count':count,'passed':0 if args.rom_only else count,'natural_isolated_ticks':natural_ticks,'derived_captured_sequence_ticks':derived_ticks,'mode':'ROM only' if args.rom_only else 'native versus unchanged ROM','rom_sha1':manifest['rom_sha1'],'snapshot_sha256':manifest['sha256'],'actions_source':str(actions),'actions_source_sha256':hashlib.sha256(actions.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','intercepted_rom_owners':['$2052 sound device only'],'required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'landing_rng_samples_by_character':{str(i):sorted(v) for i,v in sorted(rng_samples.items())},'limitations':['Only first real snapshot is natural; boundary and captured/contact-injected sequences are explicitly derived','Isolated owner ticks do not prove complete scheduler/match or hardware timing','Dereferenced pointers outside the bounded actor pool remain explicit blocks; no silent target substitution']}
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"ROM ONLY" if args.rom_only else "PASS"} {count} action2e fixtures; {natural_ticks} natural isolated ticks; {derived_ticks} derived captured ticks; {len(required)} sites; RAM/FG/D0-D7/RNG')
    if os.name=='nt' and not args.rom_only:
        assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
