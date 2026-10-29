#!/usr/bin/env python3
"""Catch wrong selectors, off-by-one dwells, callback order, movement,
RNG context, team skipping, and audio by differential execution of ROM.
Expectations come from supplied 68000 bytes, not native helper logic.
"""
from pathlib import Path
import ctypes as C, subprocess, sys, json, itertools, argparse
from rom_oracle import Oracle, load_rom
HERE=Path(__file__).resolve().parent
AFIELDS='active type selector x y z state counter phase counter1e flags33'.split()
class Actor(C.Structure): _fields_=[(s,C.c_uint32) for s in AFIELDS]
class Scene(C.Structure):
    _fields_=[(s,C.c_uint32) for s in 'ticks done terminal_pending error rng rng_global80'.split()]+[('rng_mix',C.c_uint32*7),('group_flag',C.c_uint32),('sound_count',C.c_uint32),('raw_commands',C.c_uint32*8),('effective_commands',C.c_uint32*8),('presenter',Actor),('wrestlers',Actor*9),('referee',Actor),('names_visible',C.c_uint32)]
def actor(a):return {k:getattr(a,k) for k in AFIELDS}
def test_case(lib,rom,ids,seed):
    oracle=Oracle(rom,ids,seed);frames=oracle.run()
    native=Scene();ident=(C.c_uint8*4)(*ids)
    assert lib.wp02_init(C.byref(native),ident,seed)==1
    for frame in frames:
        t=frame['tick']
        got=lib.wp02_tick(C.byref(native))
        assert got==1, f'{ids} tick{t}: missing visible display update (got {got})'
        assert native.ticks==t and not native.done,(ids,t,'tick/done')
        assert actor(native.presenter)==frame['presenter'],(ids,t,'presenter',actor(native.presenter),frame['presenter'])
        for n in range(4):assert actor(native.wrestlers[n])==frame['wrestlers'][n],(ids,t,'wrestler',n,actor(native.wrestlers[n]),frame['wrestlers'][n])
        assert actor(native.referee)==frame['referee'],(ids,t,'referee',actor(native.referee),frame['referee'])
        assert native.names_visible==frame['names_visible'],(ids,t,'names visibility')
        assert native.rng==frame['rng'],(ids,t,'RNG',hex(native.rng),hex(frame['rng']))
        assert native.group_flag==frame['group'],(ids,t,'group')
        assert list(native.raw_commands)[:native.sound_count]==frame['raw_sounds'],(ids,t,'raw sound')
        assert list(native.effective_commands)[:native.sound_count]==frame['hardware_sounds'],(ids,t,'hardware sound')
    # Clear is after final state18 display, and gameplay may never run.
    assert native.presenter.state==0x18 and native.presenter.active==0x8001
    assert lib.wp02_tick(C.byref(native))==0
    assert native.done and native.presenter.active==oracle.cleared_presenter==0
    assert native.ticks==len(frames) and native.sound_count==0
    snapshot=bytes(native);assert lib.wp02_tick(C.byref(native))==0
    assert bytes(native)==snapshot,'Done must be stable'
    return {'identities':list(ids),'seed':seed,'ticks':len(frames),'rng_calls':len(oracle.rng_calls),'sound_events':oracle.raw_events}
def main():
    ap=argparse.ArgumentParser();ap.add_argument('romzip');ap.add_argument('--wide',action='store_true');args=ap.parse_args()
    so=HERE/'wp02_scene_test.so'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-O2','-fPIC','-shared',str(HERE/'wp02_scene.c'),'-o',str(so)],check=True)
    lib=C.CDLL(str(so));lib.wp02_init.argtypes=[C.POINTER(Scene),C.POINTER(C.c_uint8),C.c_uint32];lib.wp02_tick.argtypes=[C.POINTER(Scene)]
    rom=load_rom(args.romzip)
    cases=[((0,1,2,3),0x12345678),((4,5,10,11),0x12345678),((8,9,6,7),0x89abcdef),((10,11,0,1),0),((9,8,7,6),0xffffffff),((3,5,8,10),0xace1),((0,6,10,11),0x1a2b3c4d),((10,11,8,9),0xdeadbeef)]
    if args.wide:
        # Distinct Latin rotations put every identity in every logical slot;
        # deterministic selection here is a test matrix, not game random policy.
        cases += [(tuple((a+b)%12 for b in (0,3,6,9)),0x31415926+a) for a in range(12)]
    rows=[]
    for ids,seed in cases:
        r=test_case(lib,rom,ids,seed);rows.append(r);print('PASS',','.join(map(str,ids)),r['ticks'],'updates')
    # Input validation matters because unbounded IDs index actual ROM tables.
    s=Scene();assert lib.wp02_init(C.byref(s),(C.c_uint8*4)(0,0,1,2),0)==0
    assert lib.wp02_init(C.byref(s),(C.c_uint8*4)(0,1,2,12),0)==0
    (HERE/'native_differential_results.json').write_text(json.dumps({'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','cases':rows,'status':'PASS'},indent=2)+'\n')
if __name__=='__main__':main()
