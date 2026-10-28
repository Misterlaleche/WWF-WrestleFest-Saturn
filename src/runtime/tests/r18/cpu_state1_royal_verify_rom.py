#!/usr/bin/env python3
"""Royal state1/sub0 branch1C958, whole intact1C8F4 and retained guards.

Catches the blanket Royal guard; lost old-B6.bit6 semantics; failure to reject
inactive/outside/dead targets; accidentally executing TAG outside-transfer code.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'cpu_state1_royal_snapshot_00023.bin').read_bytes();meta=json.loads((HERE/'cpu_state1_royal_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
rng=random.Random(0x1c958);actors=[RAM+0x5b0+i*0x10c for i in range(9)];coverage=set();count=0
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    m[a:a+0x10c]=capture[56+0x9e0:56+0xaec];put(m,a+0x20,0x8001);put(m,a+0xae,0);put(m,a+0x7a,t,4);put(m,a+0x86,p,4)
    put(m,a+0x32,1);put(m,a+0x34,0,1);put(m,a+0x74,0,1)
    for off in (0xb5,0xb6,0xb7,0xb9):put(m,a+off,0,1)
    put(m,a+0xb0,0x90);put(m,a+0xb2,16);put(m,a+0xb4,1,1)
    put(m,t,0x8001);put(m,t+0x20,0);put(m,t+0x32,1);put(m,t+0x34,0,1);put(m,t+0x86,p,4)
    put(m,p,0x8001);put(m,p+0x20,1);put(m,p+0xae,1);put(m,p+0x32,1);put(m,p+0x34,0,1)
    put(m,RAM+0x161,1,1);return m,a,t,p
def fixtures():
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-Royal-frame2-slot4',m,4,struct.unpack_from('=8I',capture,24)
    if args.natural_only:return
    for slot,mode,b6,b5,active,f32,f33 in itertools.product((0,4,8),(1,3),(0,0x20,0x40,0xff),(0,0x80),(0,0x8001),(0,0x10),(0,1,4,0x40)):
        m,a,t,p=seed(slot);put(m,RAM+0x161,mode,1);put(m,a+0xb6,b6,1);put(m,a+0xb5,b5,1);put(m,t,active);put(m,t+0x32,(f32<<8)|f33)
        yield f'prefix-{slot}-{mode}-{b6:x}-{b5:x}-{active:x}-{f32:x}-{f33:x}',m,slot,[rng.getrandbits(32)for _ in range(8)]
    for mode,ownflags,dy,dx,position,b5,tf33,alias in itertools.product((1,3),(0,4),(12,16,0xffff),(0,11,12,0x90),range(3),range(4),(0,1,0x10,0x40),(0,0x80000000)):
        m,a,t,p=seed(4);put(m,RAM+0x161,mode,1);put(m,a+0x33,ownflags,1);put(m,a+0xb0,dx);put(m,a+0xb2,dy);put(m,a+0xb4,position,1);put(m,a+0xb5,b5,1);put(m,t+0x33,tf33,1);put(m,a+0x7a,t|alias,4)
        yield 'shared-lane-continuation',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_cpu_runtime.c')
with tempfile.TemporaryDirectory(prefix='wf-royal-state1-',ignore_cleanup_errors=os.name=='nt')as tmp:
    frozen=Path(tmp)
    for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
    for name in ('cpu_state1_royal_oracle_bridge.c','cpu_state0_continue_oracle_bridge.c'):
        p=frozen/'tests/r18'/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(HERE/name,p)
    source_sha=hashlib.sha256((frozen/'src/wf_cpu_runtime.c').read_bytes()).hexdigest()
    lib=frozen/'cpu.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_state1_royal_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
    native=ctypes.CDLL(str(lib));native.cpu_state1_royal_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    for name,m,slot,regs in fixtures():
        m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c8f4);cpu.w_reg(8,actors[slot])
        for i,x in enumerate(regs):cpu.w_reg(i,x)
        for _ in range(20000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);status=native.cpu_state1_royal_oracle(b,d,slot)
        assert status==0,(name,'native blocked',hex(-status),slot)
        for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
            got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',slot,[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
        assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
        count+=1
    guards=0
    if not args.natural_only:
        # Royal controller15/1E96E was independently qualified afterwards;
        # retain a still-unported relation branch instead of freezing its guard.
        for route,expected in (('target-pointer',0x10be8),('relation-flag',0x1cd2c)):
            m,a,t,p=seed(4)
            if route=='target-pointer':put(m,a+0x7a,0x123456,4)
            if route=='relation-flag':put(m,a+0xb4,0,1);put(m,a+0xb2,0);put(m,a+0x74,0x80,1)
            b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)])
            assert native.cpu_state1_royal_oracle(b,d,4)==-expected,(route,expected);guards+=1
    assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1c958,0x1c960,0x1c96a,0x1c976,0x1c97e,0x1c986,0x1c98e,0x1c992,0x1ca82,0x1cd5c,0x1cdd0)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(pc)for pc in required if pc not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'cpu_source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KiB','FG16KiB','D0-D7','RNG and globals'],'intercepts':[],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST Royal full-flow frame2 actor4','Only Royal state1/sub0 branch1C958 expanded; later guards remain','Typed combat ABI excludes A/CCR; no physical hardware or whole match equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'Royal state1 cases,',guards,'guards; intact ROM, no intercepts')
