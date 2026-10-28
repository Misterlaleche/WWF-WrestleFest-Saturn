#!/usr/bin/env python3
"""Raw27 capture/abandon/animation versus full intact1C03E, no intercepts.

Breaks caught: omitting raw27; wrong BYTE/WORD capture or phase comparisons;
clearing frame high byte; wrong target guard; skipping original descriptor tick.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--natural-only',action='store_true');ap.add_argument('--report',type=Path);args=ap.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'action27_snapshot_00106.bin').read_bytes();meta=json.loads((HERE/'action27_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert rom[0x12614+0x27*4:0x12618+0x27*4]==bytes.fromhex('00016388')
assert rom[0x16388:0x1639c]==bytes.fromhex('0001639c00010003000c000c002000bd00be0000')
rng=random.Random(0x1639c);coverage=set();count=0;actors=[RAM+0x5b0+i*0x10c for i in range(9)]
def put(m,at,x,n=2):m[at:at+n]=(x&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot):
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);a=actors[slot];t=actors[(slot+1)%9]
    put(m,a+0x1c,0x8005);put(m,a+0x20,0x8005);put(m,a+0x60,0x27);put(m,a+0x26,0xdeadbeef,4);put(m,a+0x92,t,4);put(m,t+0x20,4);put(m,t+0x64,0x14)
    return m,a,t
def fixtures():
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];yield 'natural-full-flow-frame410-slot4',m,4,struct.unpack_from('=8I',capture,24)
    if args.natural_only:return
    for slot,sel,latch,phase,timer,relation in itertools.product((0,4,8),(0x27,0x4027,0x8027,0xff27),(False,True),(0,1,2,0xfe,0x101,0x1fe,0xffff),(0,1,0xff00,0xffff),range(4)):
        m,a,t=seed(slot);put(m,a+0x1c,0x8005 if latch else 5);put(m,a+0x60,sel);put(m,a+0x24,phase);put(m,a+0x22,timer)
        if relation==0:put(m,a+0x92,0,4)
        if relation==2:put(m,t+0x20,0x105)
        if relation==3:put(m,t+0x64,0x115)
        yield 'phase-latch-capture',m,slot,[rng.getrandbits(32)for _ in range(8)]
    for state,sub,tag,phase in itertools.product((4,0x104,0x8004,5),(0x14,0x114,0x8014,0x15),(0,0x40000000,0x80000000,0xff000000),(0,0x101)):
        m,a,t=seed(4);put(m,t+0x20,state);put(m,t+0x64,sub);put(m,a+0x92,t|tag,4);put(m,a+0x24,phase);put(m,a+0x22,0)
        yield 'byte-comparisons-pointer-alias',m,4,[rng.getrandbits(32)for _ in range(8)]
    for latch,sel,link in itertools.product((False,True),(0x27,0x8027),(0,0x80000000,0x123456)):
        if latch and sel==0x27 and link:continue
        m,a,t=seed(4);put(m,a+0x1c,0x8005 if latch else 5);put(m,a+0x60,sel);put(m,a+0x92,link,4);put(m,a+0x24,0xfe);put(m,a+0x22,0)
        yield 'unconsumed-invalid-relation',m,4,[rng.getrandbits(32)for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action27-',ignore_cleanup_errors=os.name=='nt')as tmp:
    frozen=Path(tmp)
    for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
    for name in ('action27_oracle_bridge.c','action31_oracle_bridge.c'):
        target=frozen/'tests/r18'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(HERE/name,target)
    source_sha=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
    lib=frozen/'action.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action27_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
    native=ctypes.CDLL(str(lib));native.action27_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
    for name,m,slot,regs in fixtures():
        m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(RAM,bytes(m[RAM:RAM+0x2000]));mem.w_block(0xc0000,bytes(m[0xc0000:0xc4000]));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
        for i,x in enumerate(regs):cpu.w_reg(i,x)
        descriptor=0
        for _ in range(10000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x1c0c8:descriptor=cpu.r_reg(9)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.action27_oracle(b,d,slot,ctypes.byref(nd))
        assert status==0,(name,'native blocked',hex(-status));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
        for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
            got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,w)for i,(g,w)in enumerate(zip(got,want))if g!=w][:12])
        assert list(d)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i)for i in range(8)])
        count+=1
    guards=0
    if not args.natural_only:
        for selector in (0x35,0x4035,0xff35):
            m,a,t=seed(4);put(m,a+0x60,selector);b=ctypes.create_string_buffer(bytes(m));values=[rng.getrandbits(32)for _ in range(8)];d=(ctypes.c_uint32*8)(*values);nd=ctypes.c_uint32()
            assert native.action27_oracle(b,d,4,ctypes.byref(nd))==-0x12614
            assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000]and list(d)==values;guards+=1
        for pointer in (0x123456,0x80000000,0xdeadbeef):
            m,a,t=seed(4);put(m,a+0x92,pointer,4);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*[rng.getrandbits(32)for _ in range(8)]);nd=ctypes.c_uint32()
            assert native.action27_oracle(b,d,4,ctypes.byref(nd))==-0x163c8;guards+=1
    assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
required=(0x1639c,0x163a4,0x163b0,0x163b8,0x163c8,0x163d0,0x163d8,0x163f2,0x16400,0x16408,0x16410,0x1c0c8)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing sites',[hex(pc)for pc in required if pc not in coverage])
report={'status':'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':source_sha,'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':['RAM8KiB','FG16KiB','D0-D7','RNG unchanged','tick descriptor'],'intercepts':[],'required_rom_sites':[hex(x)for x in required],'covered_rom_pcs':[hex(x)for x in sorted(coverage)],'limits':['NaturalHOST full-flow frame410 actor4/id6, not physical Saturn RAM','Only raw27 admitted; unrelated guards remain','Invalid consumed capture pointer fails closed at163C8; unused links do not block','No full match or hardware equivalence claim']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw27 cases,',guards,'guards; intact ROM, no intercepts')
