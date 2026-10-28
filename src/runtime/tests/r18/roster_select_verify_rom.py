#!/usr/bin/env python3
"""Original roster scroll/reconciliation/navigation versus intact ROM.

External $26A42 is intercepted symmetrically as the explicit scroll transport
boundary. Its call arguments/order and all caller D-registers/RAM are checked.
No 68000 instruction or navigation table is modified.
"""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile,zipfile
from pathlib import Path
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
RAM=0x1c0000
BASE=0x5b0
ACTOR=0x10c
SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
def put(m,a,v,n=2):m[a:a+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def get(m,a,n=2):return int.from_bytes(m[a:a+n],'big')
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--pc',default='all',choices=('all','658c','600e','5b3e'));ap.add_argument('--smoke',action='store_true');ap.add_argument('--report',type=Path);v=ap.parse_args()
    rompath=ROOT/'audit/runtime18-cd/ROM68K.BIN'
    if rompath.exists():rom=rompath.read_bytes()
    else:
        with zipfile.ZipFile(ROOT.parents[2]/'local-rom/wwfwfest.zip') as z:
            e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
        p=bytearray(len(e)+len(o));p[0::2]=e;p[1::2]=o;rom=bytes(p)
    assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x658c600e);coverage=set();counts={};event_count=0
    def seed():
        m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000)
        for i in range(4):
            a=RAM+BASE+i*ACTOR;put(m,a,0);put(m,a+0x33,0,1);put(m,a+0x23,i,1);put(m,a+0x25,i,1)
        for i in range(10):put(m,RAM+0x11f4+i*2,0)
        return m,[rng.getrandbits(32) for _ in range(8)]
    def cases():
        if v.pc in ('all','658c'):
            for direction,start,flags in itertools.product((0,1),(*range(0x140,0x218,8),0xfffc,0,0x7ffc),(0,8,0x81,0xff)):
                m,d=seed();m[RAM+0x15e]=(flags&0xfd)|(direction<<1);put(m,RAM+0x17e6,start)
                yield f'scroll-{direction}-{start:x}-{flags:x}',m,d,0x658c,BASE
                if v.smoke:return
        if v.pc in ('all','600e'):
            for mode,page,active,locked,occupied in itertools.product((0,1),(0,1),(0,1,5,15),(0,1,6),(0,0x3ff,0x155,0x2aa)):
                m,d=seed();m[RAM+0x161]=mode;m[RAM+0x15e]=(rng.randrange(256)&0xfd)|(page<<1)
                for i in range(4):
                    a=RAM+BASE+i*ACTOR;put(m,a,0x8000 if active&(1<<i) else 0);m[a+0x33]=locked&(1<<i) and 1 or 0;m[a+0x23]=(mode+page+i*3)%10;m[a+0x25]=9-i
                for i in range(10):put(m,RAM+0x11f4+i*2,0x8001 if occupied&(1<<i) else 0)
                yield f'reconcile-{mode}-{page}-{active}-{locked}-{occupied:x}',m,d,0x600e,BASE
                if v.smoke:return
        if v.pc in ('all','5b3e'):
            for mode,page,slot,direction,occupied in itertools.product((0,1),(0,1),range(10),range(16),(0,0x3ff,0x155,0x2aa)):
                m,d=seed();a=RAM+BASE+(slot%4)*ACTOR;m[RAM+0x161]=mode;m[RAM+0x15e]=(rng.randrange(256)&0xf4)|(page<<1)
                put(m,a,0x8000);put(m,a+0x22,slot);m[a+0x25]=9-slot;put(m,a+0xa8,direction)
                for i in range(10):put(m,RAM+0x11f4+i*2,1 if occupied&(1<<i) else 0)
                yield f'nav-{mode}-{page}-{slot}-{direction}-{occupied:x}',m,d,0x5b3e,a-RAM
                if v.smoke:return
    with tempfile.TemporaryDirectory(prefix='wf-roster-',ignore_cleanup_errors=os.name=='nt') as tmp:
        lib=Path(tmp)/'roster.so';sources=['tests/r18/roster_select_oracle_bridge.c'];options=[]
        if (ROOT/'src/wf_r18_roster_select.c').exists():
            sources+=['src/wf_r18_roster_select.c','src/wf_r18_frontend_input.c','src/wf_r17_m68k_shadow.c'];options=['-DWF_R18_ROSTER_IMPLEMENTED']
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*options,*sources,'-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.roster_select_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
        for name,m,regs,pc,actor in cases():
            counts[hex(pc)]=counts.get(hex(pc),0)+1
            buf=ctypes.create_string_buffer(bytes(m),len(m));d=(ctypes.c_uint32*8)(*regs);events=(ctypes.c_uint32*321)()
            status=native.roster_select_oracle(buf,d,pc,actor,events)
            assert status==0,(name,'owner missing/blocked',status,hex(events[320]))
            memory.w_block(0,bytes(m))
            for i in range(15):cpu.w_reg(i,regs[i] if i<8 else 0)
            cpu.w_reg(8,RAM+actor);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(pc)
            expected=[];target=0x5cd0 if pc==0x5b3e else 0x70000
            for n in range(10000):
                at=cpu.r_pc()
                if at==target:break
                coverage.add(at)
                if at==0x26a42:
                    expected.extend([at,cpu.r_reg(8),*[cpu.r_reg(i) for i in range(8)]])
                    cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
                cpu.execute(1)
            assert cpu.r_pc()==target,(name,'ROM runaway',hex(cpu.r_pc()))
            assert events[0]*10==len(expected),(name,'effect count',events[0],len(expected))
            assert list(events)[1:1+len(expected)]==expected,(name,'effects')
            event_count+=events[0]
            got=buf.raw[RAM:RAM+0x2000];want=bytes(memory.r_block(RAM,0x2000))
            assert got==want,(name,'RAM',[(hex(RAM+i),x,y) for i,(x,y) in enumerate(zip(got,want)) if x!=y][:12])
            expected_d=[cpu.r_reg(i) for i in range(8)]
            assert list(d)==expected_d,(name,'D0-D7',[hex(x) for x in d],[hex(x) for x in expected_d])
            if pc==0x658c:
                old=get(m,RAM+0x17e6);right=bool(m[RAM+0x15e]&2);target_scroll=0x210 if right else 0x140
                actual_scroll=get(buf.raw,RAM+0x17e6)
                assert actual_scroll==((old+(8 if right else -8))&0xffff)
                if m[RAM+0x15e]&8:
                    assert bool(buf.raw[RAM+0x15e]&8)==(actual_scroll!=target_scroll)
            if name=='nav-0-0-2-1-0':
                assert buf.raw[RAM+actor+0x23]==2 and (buf.raw[RAM+0x15e]&9)==9
                assert get(buf.raw,RAM+0x11f4+2*2)==1 and get(buf.raw,RAM+0x11f4+3*2)==0
        machine.cleanup()
    report={'rom_sha1':SHA1,'passed':counts,'external_effects':event_count,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_roster_select.c').read_bytes()).hexdigest(),'compared':['8192-byte RAM','D0-D7','external owner calls/argument registers'],'covered_rom_pcs':[hex(x) for x in sorted(coverage)],'limitations':['Pure owner calls; $26A42 transport intercepted symmetrically','No full selector scheduler/hardware/rendering claim']}
    if v.report:v.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',counts,'external effects',event_count)
if __name__=='__main__':main()
