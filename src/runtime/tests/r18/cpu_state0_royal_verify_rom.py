#!/usr/bin/env python3
"""Original Royal state0 prefix/continuations versus intact1C2CC.

Catches the blanket mode guard, incorrect B6.bit6 early return, priority/mask
drift, omitted normal continuation, target flags and changes to remaining guards.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'cpu_state0_royal_snapshot_00012.bin').read_bytes();meta=json.loads((HERE/'cpu_state0_royal_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
rng=random.Random(0x1c300);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    m[a:a+0x10c]=capture[56+0x9e0:56+0xaec]
    put(m,a+0x20,0x8000);put(m,a+0x32,1);put(m,a+0x34,0,1);put(m,a+0x7a,t,4);put(m,a+0x7e,0,4);put(m,a+0x86,p,4)
    for off in (0xb5,0xb6,0xb7,0xb9):put(m,a+off,0,1)
    put(m,t,0x8001);put(m,t+0x20,0x8000);put(m,t+0x32,1);put(m,t+0x34,0,1);put(m,t+0x7e,0,4);put(m,t+0x86,p,4)
    put(m,p,0x8001);put(m,p+0x20,0x8001);put(m,p+0xae,1);put(m,p+0x32,1);put(m,p+0x34,0,1)
    put(m,RAM+0x161,1,1);return m,a,t,p
def fixtures():
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-Royal-frame1-slot4',m,4,struct.unpack_from('=8I',capture,24)
    if args.natural_only:return
    for slot,mode,b6,b5,f34,b7 in itertools.product((0,4,8),(1,3),(0,0x40,0x50,0xff),(0,8,0x80,0x88),(0,2,4,0x10),(0,1,0x10,0x80)):
        m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0xb6,b6,1);put(m,a+0xb5,b5,1);put(m,a+0x34,f34,1);put(m,a+0xb7,b7,1)
        yield f'prefix-{slot}-{mode}-{b6:x}-{b5:x}-{f34:x}-{b7:x}',m,slot,[rng.getrandbits(32)for _ in range(8)]
    for slot,mode,b9,active,flags32,flags33,flags34,state,sub in itertools.product((0,4,8),(1,3),(0,1),(0,0x8001),(0,1,0x10),(0,1,0x40),(0,0x10),(0,4),(1,6,8)):
        m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0xb9,b9,1);put(m,t,active);put(m,t+0x32,(flags32<<8)|flags33);put(m,t+0x34,flags34,1);put(m,t+0x20,state);put(m,t+0x64,sub)
        yield f'target-{slot}-{mode}-{b9}-{active:x}-{flags32:x}-{flags33:x}-{flags34:x}-{state}-{sub}',m,slot,[rng.getrandbits(32)for _ in range(8)]
    for alias,face,state,sub in itertools.product((0,0x40000000,0x80000000,0xff000000),(0,0x8000,0x80,0xffff),(0,4,5),(8,9,0x22,0x52)):
        m,a,t,p=seed(4);put(m,a+0x7a,t|alias,4);put(m,t+0x2e,face);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,t+0x60,sub)
        yield 'target-alias-and-face-offsets',m,4,[rng.getrandbits(32)for _ in range(8)]
    for target in (0,0x123456,0xdeadbeef):
        m,a,t,p=seed(4);put(m,a+0x20,0);put(m,a+0x7a,target,4)
        yield 'inactive-state-does-not-consume-target',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_cpu_runtime.c')
with tempfile.TemporaryDirectory(prefix='wf-royal-state0-',ignore_cleanup_errors=os.name=='nt')as tmp:
    frozen=Path(tmp)
    for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
    for name in ('cpu_state0_royal_oracle_bridge.c','cpu_state0_continue_oracle_bridge.c'):
        p=frozen/'tests/r18'/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(HERE/name,p)
    source_sha=hashlib.sha256((frozen/'src/wf_cpu_runtime.c').read_bytes()).hexdigest()
    lib=frozen/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state0_royal_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
    native=ctypes.CDLL(str(lib));native.cpu_state0_royal_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    for name,m,slot,regs in fixtures():
        m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c2cc);cpu.w_reg(8,actors[slot])
        for i,x in enumerate(regs):cpu.w_reg(i,x)
        for _ in range(20000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);status=native.cpu_state0_royal_oracle(b,d,slot)
        assert status==0,(name,'native blocked',hex(-status),slot)
        for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
            got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
        assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
        count+=1
    guards=0
    if not args.natural_only:
        for mode,expected in ((0,0x1c2cc),(2,0x1c608)):
            m,a,t,p=seed(4);put(m,RAM+0x161,mode,1);put(m,a+0x34,0x10 if mode==0 else 2,1);put(m,a+0xb5,8 if mode==0 else 0,1)
            if mode==2:put(m,a+0x86,0x123456,4)
            b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
            assert native.cpu_state0_royal_oracle(b,d,4)==-expected;guards+=1
        for pointer in (0,0x123456,0xdeadbeef):
            m,a,t,p=seed(4);put(m,a+0x7a,pointer,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
            assert native.cpu_state0_royal_oracle(b,d,4)==-0x10be8;guards+=1
    assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1c300,0x1c30a,0x1c312,0x1c31a,0x1c322,0x1c334,0x1c348,0x1c358,0x1c366,0x1c370,0x1c380,0x1c390,0x1c3d6,0x1c3f2,0x1c404,0x1c410,0x1c418,0x1c420,0x1c428,0x1c5ec,0x1c880)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(pc)for pc in required if pc not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'cpu_source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KiB','FG16KiB','D0-D7','RNG and globals'],'intercepts':[],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST manual Royal frontend/paid flow frame1 actor4','Only Royal state0 entry expanded; state1/state5 guards unchanged','Invalid consumed targets/partner/tables retain explicit dependencies','Typed combat ABI excludes A/CCR; no physical hardware equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'Royal state0 cases,',guards,'guards; intact ROM, no intercepts')
