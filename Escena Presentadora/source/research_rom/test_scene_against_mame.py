#!/usr/bin/env python3
"""Compare independent MAME paid TAG RAM capture to the native controller.
Capture record: BE frame4, RAM16384, sprites8192, text8192, bg0/1 4096,
scroll8. Four logical participants resolve through original fallback rules.
Physical player/CPU ownership flags intentionally differ in the independent
four-primary-slot fixture; presentation selectors/XYZ/clocks are compared.
"""
from pathlib import Path
import argparse,ctypes as C,hashlib,json,subprocess
from test_scene_against_rom import Scene,actor,HERE
SIZE=40972
u16=lambda m,a:int.from_bytes(m[a:a+2],'big')
u32=lambda m,a:int.from_bytes(m[a:a+4],'big')
def raw_actor(m,a):
    return dict(active=u16(m,a),type=u16(m,a+2),selector=u16(m,a+4),x=u32(m,a+6),y=u32(m,a+10),z=u32(m,a+14),state=u16(m,a+0x1c),counter=u16(m,a+0x22),phase=u16(m,a+0x24),counter1e=u16(m,a+0x1e),flags33=m[a+0x33])
def main():
    ap=argparse.ArgumentParser();ap.add_argument('capture',type=Path);args=ap.parse_args();b=args.capture.read_bytes();assert len(b)%SIZE==0
    so=HERE/'wp02_scene_test.so';subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(HERE/'wp02_scene.c'),'-o',str(so)],check=True)
    lib=C.CDLL(str(so));lib.wp02_init.argtypes=[C.POINTER(Scene),C.POINTER(C.c_uint8),C.c_uint32];lib.wp02_tick.argtypes=[C.POINTER(Scene)]
    m=b[4:4+16384];ids=[];addrs=[]
    for n in range(4):
        a=0x5b0+n*0x10c
        if not u16(m,a)&0x8000:a=0x9e0+(n&1)*0x10c
        ids.append(u16(m,a+0x56)&255);addrs.append(a)
    seed=u32(m,0x5c);s=Scene();assert lib.wp02_init(C.byref(s),(C.c_uint8*4)(*ids),seed)==1
    states=[];last=None
    for ix in range(len(b)//SIZE):
        r=b[ix*SIZE:(ix+1)*SIZE];m=r[4:4+16384];lib.wp02_tick(C.byref(s))
        assert actor(s.presenter)==raw_actor(m,0x14ce),(ix,'presenter')
        assert actor(s.referee)==raw_actor(m,0x11f4),(ix,'referee')
        assert s.rng==u32(m,0x5c),(ix,'RNG')
        for n,a in enumerate(addrs):
            got=actor(s.wrestlers[n]);want=raw_actor(m,a)
            for key in ['type','selector','x','y','z','state','counter','phase','counter1e']:
                assert got[key]==want[key],(ix,n,key,got[key],want[key])
        text=r[4+16384+8192:4+16384+8192*2]
        if ix < len(b)//SIZE-1:
            assert s.names_visible==int(any(text[0x100:0x1e00])),(ix,'name layer')
        else:
            # Actual common teardown starts gameplay HUD initialization via
            # $7506 before the presenter clear; it is not intro name artwork.
            assert s.names_visible==0
        state=s.presenter.state&255
        if last!=state:states.append({'capture_index':ix,'arcade_frame':u32(r,0),'state':state,'selector':s.presenter.selector});last=state
    assert s.done and s.presenter.active==0 and s.ticks==len(b)//SIZE-1
    result={'status':'PASS','capture_sha256':hashlib.sha256(b).hexdigest(),'records':len(b)//SIZE,'first_arcade_frame':u32(b,0),'last_arcade_frame':u32(b,(len(b)//SIZE-1)*SIZE),'visible_updates':s.ticks,'identities_raw':ids,'physical_actor_records':[hex(0x1c0000+a) for a in addrs],'initial_rng_state':hex(seed),'matched':'Every presenter/referee field exposed by native API; every wrestler selector/XYZ/local-state/phase/counter; shared RNG; names visibility; exact terminal clear','excluded':'Physical active-mode and nonvisual ownership flag differences between user-selected four-primary initialization and MAME 1P primary/fallback topology','state_boundaries':states}
    (HERE/'mame_differential_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='state_boundaries'},indent=2))
if __name__=='__main__':main()
