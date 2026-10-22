#!/usr/bin/env python3
"""Differential referee geometry/count-display owners against unmodified ROM.
Needs machine68k. Builds only in a temporary directory; no Saturn build changes.
"""
import argparse
import ctypes
import hashlib
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import machine68k as mus

root = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--rom', type=Path, default=root / 'audit/runtime18-cd/ROM68K.BIN')
a = p.parse_args()
rom = a.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
rng = random.Random(0x2067c)
rom_c = (ctypes.c_uint8 * len(rom)).from_buffer_copy(rom)

def word(data, offset, value):
    data[offset:offset+2] = (value & 65535).to_bytes(2, 'big')

with tempfile.TemporaryDirectory(prefix='wf-ref-deps-') as tmp:
    library = Path(tmp) / 'referee.so'
    source = root / 'src/wf_r18_referee.c'
    sources = [str(source)] if source.exists() else []
    linker = ['-Wl,-undefined,dynamic_lookup'] if sys.platform == 'darwin' else []
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Iinclude', 'src/wf_r18_video.c', 'tests/r18/referee_dependencies_probe.c', *sources, *linker, '-o', str(library)], cwd=root, check=True)
    native = ctypes.CDLL(str(library))
    native.referee_probe.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_void_p]
    cases = []
    for dx, dy in ((0,0),(1,0),(0,1),(-1,0),(0,-1),(255,256),(256,255),(-32768,0),(32767,-32768),(-1,-1)):
        cases.append((0, 0, (0x280,0x180,(0x280+dx)&65535,(0x180+dy)&65535)))
    for _ in range(128):
        cases.append((0, 0, tuple(rng.randrange(65536) for _ in range(4))))
    for number in range(21):
        for flags in (0,0x8000):
            cases.append((1, number|flags, None))
    cases += [(2,0,None)]
    cases += [(3,phase,None) for phase in (2,4,6)]
    for index, (op, number, coords) in enumerate(cases):
        actor = bytearray(rng.randbytes(0x10c))
        fg = rng.randbytes(0x4000)
        scratch = rng.randbytes(8)
        regs = [rng.getrandbits(32) for _ in range(8)]
        if coords:
            # Referee dispatch sets D0 with MOVE.L before this call; upper word is zero.
            regs[0] &= 65535
            for offset,value in zip((6,10,0x60,0x62),coords):
                word(actor,offset,value)
        if op == 3:
            word(actor,0x24,number)
        m = mus.Machine(mus.CPUType.M68000,2048)
        cpu,mem = m.cpu,m.mem
        mem.w_block(0,rom);mem.w_block(0x1c11f4,bytes(actor));mem.w_block(0xc0000,fg);mem.w_block(0x1c15e4,scratch)
        for i,v in enumerate(regs): cpu.w_reg(i,v)
        cpu.w_reg(8,0x1c11f4)
        if op == 0:
            mem.w32(0x1c15e4,0x1c11f4);mem.w16(0x1c15e8,coords[2]);mem.w16(0x1c15ea,coords[3]);pc=0x20c8
        else:
            pc=0x206fe if op==2 else 0x2067c
            if op!=2:
                regs[0]=(regs[0]&0xffff0000)|(number//2 if op==3 else number)
                cpu.w_reg(0,regs[0])
        cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(pc)
        for _ in range(10000):
            if cpu.r_pc()==0x70000:break
            cpu.execute(1)
        assert cpu.r_pc()==0x70000
        if op==0:
            # Execute the original caller's MOVE.B scratch,$2D(A0).
            cpu.w_pc(0x1fce0);cpu.execute(1)
            assert cpu.r_pc()==0x1fce8
        want_actor=bytes(mem.r_block(0x1c11f4,0x10c));want_fg=bytes(mem.r_block(0xc0000,0x4000));want_regs=[cpu.r_reg(i) for i in range(8)]
        want_scratch=bytes(mem.r_block(0x1c15e4,8))
        m.cleanup()
        actor_c=(ctypes.c_uint8*len(actor)).from_buffer_copy(actor)
        fg_c=(ctypes.c_uint8*len(fg)).from_buffer_copy(fg)
        regs_c=(ctypes.c_uint32*8)(*regs)
        scratch_c=(ctypes.c_uint8*8).from_buffer_copy(scratch)
        status=native.referee_probe(op,rom_c,actor_c,regs_c,fg_c,number,scratch_c)
        assert status==0, (index,op,'missing or blocked referee owner',status)
        assert bytes(actor_c)==want_actor,(index,op,'actor bytes')
        assert bytes(fg_c)==want_fg,(index,op,number,'FG RAM')
        assert list(regs_c)==want_regs,(index,op,'D0-D7')
        assert bytes(scratch_c)==want_scratch,(index,op,'$1C15E4 scratch',bytes(scratch_c).hex(),want_scratch.hex())
    print(f'PASS {len(cases)} unmodified-ROM cases: direction, all 21 counts in both windows, clear, pin-count adapter; actor/FG/scratch RAM and D0-D7')
