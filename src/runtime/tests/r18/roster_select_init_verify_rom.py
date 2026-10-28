#!/usr/bin/env python3
"""Original $57E0 initialization through $5990, with explicit effect boundaries."""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile,zipfile
from pathlib import Path
import machine68k as mus
from roster_select_verify_rom import ROOT,RAM,BASE,ACTOR,SHA1,put

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--smoke',action='store_true');ap.add_argument('--report',type=Path);v=ap.parse_args()
    p=ROOT/'audit/runtime18-cd/ROM68K.BIN'
    if p.exists():rom=p.read_bytes()
    else:
        with zipfile.ZipFile(ROOT.parents[2]/'local-rom/wwfwfest.zip') as z:
            e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
        r=bytearray(len(e)*2);r[0::2]=e;r[1::2]=o;rom=bytes(r)
    assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x57e0);coverage=set();cases=steps=effects=waits=0
    with tempfile.TemporaryDirectory(prefix='wf-roster-init-',ignore_cleanup_errors=os.name=='nt') as tmp:
        lib=Path(tmp)/'roster.so';sources=['src/wf_r18_roster_select.c','src/wf_r18_frontend_input.c','src/wf_r17_m68k_shadow.c','tests/r18/roster_select_oracle_bridge.c'];flags=['-DWF_R18_ROSTER_IMPLEMENTED']
        if 'wf_r18_roster_select_step(' in (ROOT/'src/wf_r18_roster_select.c').read_text():flags+=['-DWF_R18_ROSTER_INIT_IMPLEMENTED']
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*flags,*sources,'-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.roster_select_init_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(ctypes.c_uint32)]
        for mode,config,flags,active in itertools.product((0,1),(0,0x800),(0,0x40,0x80,0xc0),range(16)):
            cases+=1;m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000)
            m[RAM+0x161]=mode;m[RAM+0x160]=flags;put(m,RAM+0x66,config)
            for i in range(4):put(m,RAM+BASE+i*ACTOR,0x8000 if active&(1<<i) else 0)
            ds=[rng.getrandbits(32) for _ in range(8)];cursor=[0x57e0,*[rng.randrange(0x200000) for _ in range(7)]]
            machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
            for tick in range(20):
                steps+=1;inputs=(0xffff,0xffff,0xffff,0xfbff if tick&1 else 0xffff)
                original_cursor=list(cursor);original_ds=list(ds);before=bytes(m)
                b=ctypes.create_string_buffer(before,len(m));d=(ctypes.c_uint32*8)(*ds);cur=(ctypes.c_uint32*8)(*cursor);ports=(ctypes.c_uint16*4)(*inputs);events=(ctypes.c_uint32*321)()
                st=native.roster_select_init_oracle(b,d,cur,ports,events)
                assert st>=0,('init missing/blocked',mode,config,flags,active,hex(cursor[0]),st,hex(events[320]))
                mem.w_block(0,before)
                for i,x in enumerate(original_ds):cpu.w_reg(i,x)
                for i,x in enumerate(original_cursor[1:]):cpu.w_reg(i+8,x)
                cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(original_cursor[0])
                for i,x in enumerate(inputs):mem.w16(0x140020+i*2,x)
                calls=[];target=(cur[0],cur[1])
                for instruction in range(10000):
                    pc=cpu.r_pc()
                    if (pc,cpu.r_reg(8))==target and (instruction or st==1):break
                    coverage.add(pc)
                    if pc in (0x2aea,0x26e66,0x1f9e,0x2a06,0x21e6):
                        calls.extend([pc,cpu.r_reg(8),*[cpu.r_reg(i) for i in range(8)]])
                        cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
                    cpu.execute(1)
                assert (cpu.r_pc(),cpu.r_reg(8))==target,('runaway',original_cursor,list(cur),hex(cpu.r_pc()))
                assert list(events)[1:1+events[0]*10]==calls,('effects',original_cursor,list(events)[:1+events[0]*10],calls)
                want=bytes(mem.r_block(RAM,0x2000));got=b.raw[RAM:RAM+0x2000]
                assert got==want,('RAM',mode,config,flags,active,original_cursor,[(hex(RAM+i),x,y) for i,(x,y) in enumerate(zip(got,want)) if x!=y][:12])
                assert list(d)==[cpu.r_reg(i) for i in range(8)],('D',original_cursor,[hex(x) for x in d],[hex(cpu.r_reg(i)) for i in range(8)])
                assert list(cur)[1:]==[cpu.r_reg(i) for i in range(8,15)],('A',original_cursor,list(cur)[1:],[cpu.r_reg(i) for i in range(8,15)])
                effects+=events[0];waits+=st==1;m=bytearray(b.raw);ds=list(d);cursor=list(cur)
                if st==2:
                    assert cursor[0]==0x5990;break
            else:raise AssertionError(('init not ready',cursor))
            machine.cleanup()
            if v.smoke:break
    report={'rom_sha1':SHA1,'cases':cases,'steps':steps,'effects':effects,'sync_waits':waits,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_roster_select.c').read_bytes()).hexdigest(),'compared':['RAM8KB','D0-D7','A0-A6','external calls in order'],'covered_rom_pcs':[hex(x) for x in sorted(coverage)],'boundaries':['palette2AEA/world26E66/unmask1F9E/palette2A06/wait21E6 effects intercepted symmetrically','Stops BEFORE live selector5990; no opponent roster generation']}
    if v.report:v.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS init',cases,'cases',steps,'steps',effects,'effects',waits,'sync waits')
if __name__=='__main__':main()
