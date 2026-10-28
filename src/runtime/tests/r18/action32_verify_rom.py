#!/usr/bin/env python3
"""Original raw32 reachable17194 branches, full1C03E ROM comparison, no intercepts."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action32_snapshot_01654.bin').read_bytes();meta=json.loads((HERE/'action32_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+50*4:0x12618+50*4]==bytes.fromhex('0001716c')
assert rom[0x1716c:0x17180]==bytes.fromhex('00017194000100030018000800140159015a004d')
rng=random.Random(0x17194);coverage=set();count=0;actors=[RAM+0x5b0+i*0x10c for i in range(9)]
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x32);put(m,a+0x26,t,4);put(m,t+0x86,p,4);put(m,p+0x20,1);put(m,p+0xae,1);m[RAM+0x161]=0
    return m,a,t,p
def fixtures():
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-frame4865-slot5',m,5,struct.unpack_from('=8I',capture,24)
    if args.natural_only:return
    for slot,sel,latch,phase,timer,hits,face in itertools.product((0,4,8),(0x32,0x4032,0x8032,0xff32),(False,True),(0,1,2,0xfe,0x100,0x101,0x1fe,0xffff),(0,1,0xff00,0xffff),(0,1,2,3,0xffff),(0,0x8000)):
        m,a,t,p=seed(slot);put(m,a+0x1c,0x8005 if latch else 5);put(m,a+0x60,sel);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x44,hits);put(m,a+0x2e,face)
        yield 'latch-phase-hits',m,slot,[rng.getrandbits(32) for _ in range(8)]
    for mode,partner,tag,face in itertools.product(range(4),(0,1,0x101,0x8001),(0,0x80000000,0xff000000),(0,0x80,0x8000,0xffff)):
        m,a,t,p=seed(4);put(m,a+0x24,1);put(m,a+0x22,0);put(m,a+0x44,1);put(m,a+0x2e,face);put(m,a+0x26,t|tag,4);put(m,p+0xae,partner);m[RAM+0x161]=mode
        yield 'release-partner-tags',m,4,[rng.getrandbits(32) for _ in range(8)]
    for link in (0,0x80000000,0x123456):
        m,a,t,p=seed(4);put(m,a+0x24,0xfe);put(m,a+0x22,0);put(m,a+0x26,link,4)
        yield 'terminal-no-repeat-release',m,4,[rng.getrandbits(32) for _ in range(8)]
    for face in (0,0x8000):
        m,a,t,p=seed(4);put(m,a+0x1c,5);put(m,a+0x2e,face);regs=[rng.getrandbits(32) for _ in range(8)]
        for tick in range(200):
            yield 'sequence-three-hits',m,4,regs
            if int.from_bytes(m[a+0x20:a+0x22],'big')==0:break
        else:raise AssertionError('raw32 sequence did not terminate')
        assert int.from_bytes(m[a+0x44:a+0x46],'big')==0
        assert m[t+0x20:t+0x22]==bytes.fromhex('0004') and m[t+0x64:t+0x66]==bytes.fromhex('0002')
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action32-',ignore_cleanup_errors=os.name=='nt') as tmp:
    lib=Path(tmp)/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action32_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(lib));native.action32_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
    for name,m,slot,regs in fixtures():
        m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(RAM,bytes(m[RAM:RAM+0x2000]));mem.w_block(0xc0000,bytes(m[0xc0000:0xc4000]));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
        for i,x in enumerate(regs):cpu.w_reg(i,x)
        descriptor=0
        for _ in range(20000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x1c0c8:descriptor=cpu.r_reg(9)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action32_oracle(b,d,slot,ctypes.byref(nd))
        assert status==0,(name,'native blocked',hex(-status));assert nd.value==descriptor,(name,'descriptor')
        for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
            got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w) for i,(g,w) in enumerate(zip(got,want)) if g!=w][:12])
        assert list(d)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i) for i in range(8)])
        assert b.raw[RAM+0x5c:RAM+0x60]==m[RAM+0x5c:RAM+0x60],'raw32 adds no RNG call'
        if name=='sequence-three-hits':
            old=int.from_bytes(m[actors[slot]+0x44:actors[slot]+0x46],'big')
            m[RAM:RAM+0x2000]=b.raw[RAM:RAM+0x2000];regs[:]=list(d)
            now=int.from_bytes(m[actors[slot]+0x44:actors[slot]+0x46],'big')
            if old in (2,3) and now==old-1:
                assert m[actors[(slot+1)%9]+0x20:actors[(slot+1)%9]+0x22]==bytes.fromhex('00ff'),'only third hit releases target'
        count+=1
    guards=0
    if not args.natural_only:
        for selector in (0x35,0x4035,0xff35):
            m,a,t,p=seed(4);put(m,a+0x60,selector);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32) for _ in range(8)];d=(ctypes.c_uint32*8)(*values);nd=ctypes.c_uint32()
            assert native.action32_oracle(b,d,4,ctypes.byref(nd))==-0x12614
            assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values;guards+=1
        for route,expected in (('init',0x171b0),('hit',0x171de),('partner',0x110ee)):
            m,a,t,p=seed(4);put(m,a+0x1c,5 if route=='init' else 0x8005);put(m,a+0x24,1);put(m,a+0x22,0);put(m,a+0x44,1)
            if route=='partner':put(m,t+0x86,0x123456,4)
            else:put(m,a+0x26,0x123456,4)
            b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]);nd=ctypes.c_uint32()
            assert native.action32_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,'native dependency');guards+=1
    machine.cleanup()
required=(0x17194,0x1719c,0x171a6,0x171b0,0x171c6,0x171de,0x171ec,0x171f8,0x17208,0x17218,0x17238,0x17244,0x17252,0x110e0,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing ROM sites',[hex(pc) for pc in required if pc not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG unchanged','tick descriptor'],'intercepts':[],'covered_rom_pcs':[hex(x) for x in sorted(coverage)],'required_rom_sites':[hex(x) for x in required],'limits':['Only raw32 reachable subset of shared17194 admitted; no sibling selectors','NaturalHOST4865 capture; no full combat or physical audio qualification']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw32 fixtures; intact ROM, no intercepts')
