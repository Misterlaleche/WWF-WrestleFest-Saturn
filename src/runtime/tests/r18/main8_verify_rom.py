#!/usr/bin/env python3
"""Main8 routes0..9, including the natural emulated TAG-entry guard."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
meta=json.loads((HERE/'main8_manifest.json').read_text());data=(HERE/'main8_snapshot_emulated.bin').read_bytes();assert hashlib.sha256(data).hexdigest()==meta['sha256']
rng=random.Random(0x11f3a);coverage=set();actors=[0x1c05b0+i*0x10c for i in range(9)]
def put(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(n*8))-1)).to_bytes(n,'big')
def fixtures():
    mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=data
    yield 'emulated-frame2659-main8',mem,meta['slot'],meta['regs']
    for route,high,mode,index,timer in itertools.product(range(10),(0,0x80,0xff),range(4),(0,1,2,3,4,5,6,0xfe,0x101,0x1fe),(0,1,0xff00,0xffff)):
        mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=rng.randbytes(0x2000);slot=(route+index)%9;a=actors[slot]
        put(mem,a+0x1c,(high<<8)|8);put(mem,a+0x20,0x8008);put(mem,a+0x44,route);put(mem,a+0x24,index);put(mem,a+0x22,timer)
        put(mem,0x1c0161,mode,1);put(mem,a+0x26,0,4);put(mem,a+3,route%12,1)
        yield f'route-{route}-{high:x}-{mode}-{index:x}-{timer:x}',mem,slot,[rng.getrandbits(32) for _ in range(8)]
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-main8-',ignore_cleanup_errors=os.name=='nt') as temp:
    lib=Path(temp)/'main8.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/main8_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ctypes.CDLL(str(lib));native.wf_main8_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    count=0
    for name,mem,slot,regs in fixtures():
        count+=1;mem[0xc0000:0xc4000]=rng.randbytes(0x4000)
        nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.wf_main8_oracle(nb,nr,slot);assert status==0,(name,'native blocked',hex(-status))
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,bytes(mem))
        for i in range(8,15):cpu.w_reg(i,0)
        for i,v in enumerate(regs):cpu.w_reg(i,v)
        cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
        for _ in range(3000):
            pc=cpu.r_pc();coverage.add(pc)
            if pc==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
        for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
            got=nb.raw[at:at+n];want=bytes(memory.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
        assert list(nr)==[cpu.r_reg(i) for i in range(8)],(name,'D0-D7')
        assert nb.raw[0x1c005c:0x1c0060]==mem[0x1c005c:0x1c0060],(name,'RNG')
        machine.cleanup()
    required=(0x11f3a,0x11fae,0x11fb6,0x12018,0x12020,0x1203c,0x1205a,0x1206a,0x12078,0x12112,0x12122,0x1212a,0x12192,0x121be)
    assert all(pc in coverage for pc in required),[hex(pc) for pc in required if pc not in coverage]
    report={'passed':count,'rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':meta['sha256'],'dispatcher_source_sha256':hashlib.sha256((ROOT/'src/wf_r17_dispatch_1c03e.c').read_bytes()).hexdigest(),'compared':['RAM1C0000..1C1FFF','FG0C0000..0C3FFF','D0-D7','RNG'],'covered_rom_sites':[hex(x) for x in required],'limitations':[meta['limitations'],'Isolated main8 dispatcher calls, not full emulated frames; device audio output is not compared']}
    if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {count} main8 fixtures, routes0..9; RAM/FG/D0-D7/RNG')
