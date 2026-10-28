#!/usr/bin/env python3
"""Original state4 subtype0F $1B684 through actual $1C03E, no intercepted owner."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;RAM=0x1c0000
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--natural-only',action='store_true');p.add_argument('--rom-only',action='store_true');args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
capture=(HERE/'victim1b684_snapshot_00936.bin').read_bytes();meta=json.loads((HERE/'victim1b684_manifest.json').read_text());assert hashlib.sha256(capture).hexdigest()==meta['sha256'];assert struct.unpack_from('=6I',capture)==tuple(meta['header'])
assert int.from_bytes(rom[0x1afd4+15*4:0x1afd8+15*4],'big')==0x1b66c
assert rom[0x1b66c:0x1b684]==bytes.fromhex('0001b68400010004000800080008ff00001b00b100b200b3')
rng=random.Random(0x1b684);coverage=set();count=0;actors=[RAM+0x5b0+i*0x10c for i in range(9)]
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def fixtures():
    m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248]
    a=actors[5];assert m[a+0x1c:a+0x1e]==bytes.fromhex('0004') and m[a+0x64:a+0x66]==bytes.fromhex('000f')
    yield 'natural-frame2935-slot5',m,5,struct.unpack_from('=8I',capture,24)
    if args.natural_only:return
    for slot,high,latch,phase,contact,timer in itertools.product((0,4,8),(0,0x10,0x20,0x80,0xff),(False,True),(0,1,2,3,0x103,0xff03,0xfe),(0,0x110,0x501,0x502,0x103,0x310),(0,1,0xff00)):
        m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000);a=actors[slot]
        put(m,a+0x1c,0x8004 if latch else 4);put(m,a+0x20,0x8004);put(m,a+0x64,(high<<8)|15);put(m,a+0x24,phase);put(m,a+0x22,timer);put(m,a+0x36,contact)
        # Original descriptors accept every WORD index on a held FF00 timer;
        # other timers use valid original animation indices or terminal FE.
        put(m,a+0x2e,0x8000 if (count+slot)&1 else 0);put(m,a+10,rng.choice((0,0xf7,0xf8,0x177,0x178,0xffe0,0xffff)));m[RAM+0x161]=slot%4
        put(m,a+0x18,0x8000 if slot==8 else 0);put(m,a+0x1a,0xabcd)
        yield f'case-{slot}-{high:x}-{latch}-{phase:x}-{contact:x}-{timer:x}',m,slot,[rng.getrandbits(32) for _ in range(8)]
    for mode,flags,bits in itertools.product(range(4),(0,0x80,1,0x81),(0,1,2,3,0x10,0x13)):
        m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=capture[56:8248];a=actors[5]
        put(m,a+0x1c,0x8004);put(m,a+0x24,3);put(m,a+0x22,0);put(m,a+0x36,(1<<8)|bits);put(m,a+0x2e,flags<<8);m[RAM+0x161]=mode
        for at in (0x1150,0x1180,0x11b0,0x11e0):put(m,RAM+at,0xab00)
        yield 'rope-and-landing',m,5,[rng.getrandbits(32) for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-victim1b684-',ignore_cleanup_errors=os.name=='nt') as tmp:
    if not args.rom_only:
        lib=Path(tmp)/'victim.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/victim1b684_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.victim1b684_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    for name,m,slot,regs in fixtures():
        m[0xc0000:0xc4000]=rng.randbytes(0x4000);mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);cpu.w_reg(8,actors[slot])
        for i,v in enumerate(regs):cpu.w_reg(i,v)
        for _ in range(20000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        count+=1
        if args.rom_only:continue
        b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);status=native.victim1b684_oracle(b,d,slot)
        assert status==0,(name,'native blocked',hex(-status))
        for at,n in ((RAM,0x2000),(0xc0000,0x4000)):
            got=b.raw[at:at+n];want=bytes(mem.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),x,y) for i,(x,y) in enumerate(zip(got,want)) if x!=y][:12])
        assert list(d)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7',list(d),[cpu.r_reg(i) for i in range(8)])
        old=m[actors[slot]:actors[slot]+0x10c];got=b.raw[actors[slot]:actors[slot]+0x10c]
        if not old[0x1c]&128:
            assert got[0x64]==old[0x64]&0xef,'init clears subtypehigh bit4 only'
            assert int.from_bytes(got[0x24:0x26],'big')==(1 if old[0x64]&16 else 0),'original init chooses descriptor entry1 after explicit latch, otherwise entry0'
            assert got[0x5c:0x60]==bytes.fromhex('05000038')
        if old[0x1c]&128 and old[0x25]==3 and old[0x37]&16:
            assert got[6:8]==old[6:8],'original landing loads displacement registers but does not move X'
            assert d[1]&0xffff==0 and d[2]&0xffff==0
    guards=0
    if not args.rom_only:
        # Existing bounded dispatcher rejection is retained for out-of-table
        # victim ids. This is a native contract check, not a ROM admission.
        for bad in (43,127,255):
            m=bytearray(0x200000);m[RAM:RAM+0x2000]=capture[56:8248];put(m,actors[5]+0x64,bad)
            b=ctypes.create_string_buffer(bytes(m));regs=[rng.getrandbits(32) for _ in range(8)];d=(ctypes.c_uint32*8)(*regs)
            assert native.victim1b684_oracle(b,d,5)==-0x1afd4
            assert b.raw[RAM:RAM+0x2000]==m[RAM:RAM+0x2000] and list(d)==regs
            guards+=1
    machine.cleanup()
required=(0x1b684,0x1b68c,0x1b6a0,0x1b6a8,0x1b6b8,0x1b6d4,0x1b6f6,0x258e,0x10f9c,0x10fc6,0x1110e,0x1c0c8,0x1c0ba)
if not args.natural_only:assert all(pc in coverage for pc in required),('missing branches',[hex(pc) for pc in required if pc not in coverage])
report={'status':'ROM ONLY' if args.rom_only else 'PASS','cases':count,'retained_guard_checks':guards,'source_sha256':hashlib.sha256((ROOT/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest(),'snapshot_sha256':meta['sha256'],'rom_sha1':hashlib.sha1(rom).hexdigest(),'compared':[] if args.rom_only else ['RAM8KB','FG16KB','D0-D7'],'intercepts':[],'covered_rom_pcs':[hex(x) for x in sorted(coverage)],'required_rom_sites':[hex(x) for x in required],'limits':['NaturalHOST frame2935 slot5 state4 subtype0F; selectorC048 is inherited, not this callback selector','Only callback1B684 admission added; no unrelated guard removed','Audio device writes execute in ROM but physical sound transport is outside native visual contract','Does not establish combat completion or a pin outcome']}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(report['status'],count,'victim1B684 fixtures; intact ROM, no intercepts')
