#!/usr/bin/env python3
"""Raw31/17000 original capture and release versus intact1C03E; no intercepted owners."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path);parser.add_argument('--natural-only',action='store_true');v=parser.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action31_snapshot_01371.bin').read_bytes();meta=json.loads((HERE/'action31_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+49*4:0x12618+49*4]==bytes.fromhex('00016fd8')
assert rom[0x16fd8:0x17000]==bytes.fromhex('000170000001000800200004000400040004ff000008001c00c200c300c400c500c600c700c800c9')
rng=random.Random(0x17000);coverage=set();count=0;actors=[RAM+0x5b0+i*0x10c for i in range(9)]
def put(m,at,value,n=2):m[at:at+n]=(value&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);a=actors[slot];t=actors[(slot+1)%9];p=actors[(slot+2)%9]
    put(m,a+0x20,0x8005);put(m,a+0x1c,0x8005);put(m,a+0x60,0x31);put(m,a+2,slot%12);put(m,a+0x26,t,4);put(m,t+0x86,p,4);put(m,a+0x92,0,4)
    put(m,t+0x20,4);put(m,t+0x64,0x28);put(m,p+0x20,1);put(m,p+0xae,1);put(m,RAM+0x7c,0);put(m,a+0x2e,0x8000 if slot else 0);m[RAM+0x161]=slot%2
    return m,a,t,p
def fixtures():
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-frame4172-slot5',m,5,struct.unpack_from('=8I',capture,24)
    if v.natural_only:return
    for slot,selector,latch,phase,timer,contact,relation in itertools.product((0,4,8),(0x31,0x4031,0x8031,0xff31),(False,True),(0,1,4,6,0xfe,0x100,0x101,0x104,0x106,0x1fe),(0,1,0xff00,0xffff),(0,0x10),range(3)):
        m,a,t,p=seed(slot);put(m,a+0x1c,0x8005 if latch else 5);put(m,a+0x60,selector);put(m,a+0x24,phase);put(m,a+0x22,timer);m[a+0x37]=contact
        if relation:put(m,a+0x92,t,4)
        if relation==2:put(m,t+0x64,0x27)
        yield 'phase-and-relation',m,slot,[rng.getrandbits(32) for _ in range(8)]
    for slot,mode,credited,char,inside,partner in itertools.product((0,4,8),range(4),(0,1,0x8000),(0,5,11),(False,True),(False,True)):
        m,a,t,p=seed(slot);put(m,a+0x24,6);put(m,a+0x22,0);put(m,a+0x60,0x8031);put(m,a+0x2e,0x8000 if inside else 0);put(m,a+2,char);put(m,RAM+0x7c,credited);m[a+0x33]=0 if inside else 2;m[RAM+0x161]=mode
        put(m,p+0xae,1 if partner else 0);yield 'release6-rng-partner',m,slot,[rng.getrandbits(32) for _ in range(8)]
    for tag,phase in itertools.product((0,0x40000000,0x80000000,0xff000000),(0,6)):
        m,a,t,p=seed(4);put(m,a+0x24,phase);put(m,a+0x22,0);put(m,a+0x26,t|tag,4)
        if phase==0:put(m,a+0x92,t|tag,4)
        yield 'tagged-actor-pointer',m,4,[rng.getrandbits(32) for _ in range(8)]
    for null,phase in itertools.product((0,0x80000000,0xdeadbeef),(4,0xfe)):
        m,a,t,p=seed(4);put(m,a+0x24,phase);put(m,a+0x22,0);put(m,a+0x26,null,4)
        yield 'unconsumed-invalid-link',m,4,[rng.getrandbits(32) for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action31-',ignore_cleanup_errors=os.name=='nt') as tmp:
    lib=Path(tmp)/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action31_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(lib));native.action31_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
        b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action31_oracle(b,d,slot,ctypes.byref(nd))
        assert status==0,(name,'native blocked',hex(-status));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
        for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
            got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w) for i,(g,w) in enumerate(zip(got,want)) if g!=w][:12])
        assert list(d)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7',[hex(x) for x in d],[hex(cpu.r_reg(i)) for i in range(8)])
        count+=1
    guards=0
    if not v.natural_only:
        # Raw32 is admitted by the subsequent independently qualified owner;
        # raw35 remains an unrelated unported action guard.
        for selector in (0x35,0x4035,0xff35):
            m,a,t,p=seed(4);put(m,a+0x60,selector);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32) for _ in range(8)];d=(ctypes.c_uint32*8)(*values);nd=ctypes.c_uint32()
            assert native.action31_oracle(b,d,4,ctypes.byref(nd))==-0x12614
            assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==values
            guards+=1
        for route,expected in (('capture',0x17038),('release',0x170fa),('partner',0x110ee)):
            m,a,t,p=seed(4);put(m,a+0x24,0 if route=='capture' else 6);put(m,a+0x22,0)
            if route=='capture':put(m,a+0x92,0x123456,4)
            if route=='release':put(m,a+0x26,0x123456,4)
            if route=='partner':m[RAM+0x161]=0;put(m,t+0x86,0x123456,4)
            b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]);nd=ctypes.c_uint32()
            assert native.action31_oracle(b,d,4,ctypes.byref(nd))==-expected,(route,'native dependency gate')
            guards+=1
    machine.cleanup()
required=(0x17000,0x17008,0x17028,0x17048,0x17086,0x170b6,0x170c8,0x170e6,0x170fa,0x1711e,0x17124,0x17132,0x1714c,0x1108c,0x110e0,0x1110e,0x1c0c8)
if not v.natural_only:assert all(pc in coverage for pc in required),('missing branch sites',[hex(pc) for pc in required if pc not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KB','FG16KB','D0-D7','RNG','tick descriptor'],'intercepts':[],'covered_rom_pcs':[hex(x) for x in sorted(coverage)],'required_rom_sites':[hex(x) for x in required],'limits':['NaturalHOST frame4172 slot5 raw4031','Only raw31 admitted; no other action guard removed','Invalid capture/release/partner pointers fail closed at their exact dereference dependency; unused null/invalid links do not block','Does not establish whole combat/match completion or physical audio transport']}
if v.report:v.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw31 fixtures; intact ROM, no intercepts')
