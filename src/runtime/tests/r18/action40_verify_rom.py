#!/usr/bin/env python3
"""Raw40 natural A600 frame1850 versus intact ROM, including rope helper.

Catches missing callback, initial $258E row11/sign, WORD zero versus BYTE
frame2/FE gates, rope-mode/contact priority, LOW BYTE rope-global guards,
landing timer release, descriptor identity/tick, and register/RNG corruption.
Only sound-output $2052 is intercepted; gameplay ROM is unchanged.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--snapshot-only',action='store_true');args=p.parse_args()
meta=json.loads((HERE/'action40_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12614+0x40*4:0x12618+0x40*4]==bytes.fromhex('000183ec')
assert rom[0x183ec:0x18404]==bytes.fromhex('000184040001000400040004ff0000080084008501af0084')
capture=(HERE/'action40_snapshot_frame1850.bin').read_bytes();assert len(capture)==16472 and hashlib.sha256(capture).hexdigest()==meta['sha256']
assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[0x1c05b0+0x10c*i for i in range(9)];rng=random.Random(0x18404);coverage=set()

def put(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(n*8))-1)).to_bytes(n,'big')
def seed(slot=4):
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
    actor=actors[slot];put(mem,actor+0x20,0x8005);put(mem,actor+0x1c,0x8005);put(mem,actor+0x60,0x40)
    put(mem,actor+0x24,2);put(mem,actor+0x22,1);put(mem,actor+0x36,0);put(mem,actor+0x26,0,4)
    put(mem,0x1c0161,0,1);return mem,actor
def fixtures():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=capture[56:8248]
    yield 'natural-A600-frame1850',mem,4,struct.unpack_from('=8I',capture,24)
    if args.snapshot_only:return
    for slot,latched,selector,index,timer,face in itertools.product((0,4,8),(False,True),(0x40,0x4040,0x8040,0xff40),(0,1,2,3,0xfe,0x100,0x102,0x1fe,0xffff),(0,1,4,8,0xff00,0xffff),(0,0x8000,0x0080,0xffff)):
        mem,actor=seed(slot);put(mem,actor+0x1c,0x8005 if latched else 5);put(mem,actor+0x60,selector);put(mem,actor+0x24,index);put(mem,actor+0x22,timer);put(mem,actor+0x2e,face)
        yield f'phase-{slot}-{latched}-{selector:x}-{index:x}-{timer:x}-{face:x}',mem,slot
    for mode,kind,bits in itertools.product(range(4),(0,1,2,3,5,0x81,0xff),range(256)):
        mem,actor=seed();put(mem,0x1c0161,mode,1);put(mem,actor+0x36,kind,1);put(mem,actor+0x37,bits,1)
        yield f'contact-{mode}-{kind:x}-{bits:x}',mem,4
    for mode,kind,bits,left,right in itertools.product(range(4),(1,3,5),(0,1,2,3,0x10,0x11,0x12,0x13,0xff),(0,0x100,1,0x101,0xff00,0xffff),(0,0x100,1,0x101,0xff00,0xffff)):
        mem,actor=seed();put(mem,0x1c0161,mode,1);put(mem,actor+0x36,kind,1);put(mem,actor+0x37,bits,1);put(mem,0x1c1150,left);put(mem,0x1c11b0,right)
        yield f'rope-{mode}-{kind}-{bits:x}-{left:x}-{right:x}',mem,4

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action40-',ignore_cleanup_errors=os.name=='nt') as temp:
    lib=Path(temp)/'action40.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action40_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(lib));native.wf_action40_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,rom)
    count=0
    try:
        for case in fixtures():
            name,mem,slot=case[:3];regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];count+=1
            mem[0xc0000:0xc4000]=rng.randbytes(0x4000);memory.w_block(0x1c0000,bytes(mem[0x1c0000:0x1c2000]));memory.w_block(0xc0000,bytes(mem[0xc0000:0xc4000]))
            for i in range(15):cpu.w_reg(i,regs[i] if i<8 else 0)
            cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);descriptor=0
            for _ in range(2000):
                pc=cpu.r_pc();coverage.add(pc)
                if pc==0x1c0c8:descriptor=cpu.r_reg(9)
                if pc==0x70000:break
                if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
                cpu.execute(1)
            assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
            nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.wf_action40_oracle(nb,nr,slot,ctypes.byref(nd))
            assert status==0,(name,'native blocked',hex(-status))
            assert nd.value==descriptor,(name,'tick descriptor',hex(nd.value),hex(descriptor))
            for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
                got=nb.raw[at:at+n];want=bytes(memory.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
            assert list(nr)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7',list(nr),[cpu.r_reg(i) for i in range(8)])
            assert nb.raw[0x1c005c:0x1c0060]==mem[0x1c005c:0x1c0060],(name,'RNG changed')
        assert bytes(memory.r_block(0,len(rom)))==rom,'ROM was modified'
    finally:machine.cleanup()
    required=(0x18404,0x1840c,0x258e,0x25c0,0x1842e,0x18444,0x1844c,0x1845a,0x1846e,0x18484,0x10ff0,0x11012,0x11024,0x1102c)
    if not args.snapshot_only:assert all(pc in coverage for pc in required),('missing ROM branch sites',[hex(pc) for pc in required if pc not in coverage])
    report={'passed':count,'rom_sha1':meta['rom_sha1'],'snapshot_sha256':meta['sha256'],'actions_source_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'compared':['all RAM1C0000..1C1FFF','FG0C0000..0C3FFF','D0-D7','RNG','tick descriptor'],'covered_rom_sites':[hex(pc) for pc in required if pc in coverage],'intercepts':['2052 sound output only'],'limitations':['Natural A600 snapshot plus derived isolated owner boundaries, not free combat','No full scheduler, device audio output or physical Saturn timing equivalence claim']}
    if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {count} raw40 fixtures; RAM/FG/D0-D7/RNG/descriptor; intact ROM')
