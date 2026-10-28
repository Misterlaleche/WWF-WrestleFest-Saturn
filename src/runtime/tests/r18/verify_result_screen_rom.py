#!/usr/bin/env python3
"""Compare presentation $11CE..$122B with original instructions and real $2503C.

The oracle runs the original $21E6 including DBRA and register save/restore.
Only its physical input $140026.bit2 is driven at the observed polling PCs.
No text routine, decision, branch, wait or return is intercepted/replaced.
The $11B6 scratch clears and $122C onward scene transition are outside scope.
"""
import argparse
import ctypes
import hashlib
import os
from pathlib import Path
import random
import subprocess
import tempfile
import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--rom', type=Path, default=ROOT / 'audit/runtime18-cd/ROM68K.BIN')
args = parser.parse_args()
rom = args.rom.resolve().read_bytes()
assert hashlib.sha256(rom).hexdigest() == '265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
assert (ROOT / 'include/wf_r18_result_screen.h').is_file(), 'missing in-ring presentation API'
assert (ROOT / 'src/wf_r18_result_screen.c').is_file(), 'missing in-ring presentation implementation'

rng = random.Random(0x11ce122c)
sources = ['src/wf_r18_result_screen.c', 'src/wf_r18_frontend_palette.c', 'src/wf_r18_video.c', 'src/wf_r17_m68k_shadow.c']
flags = ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation', '-O2', '-Iinclude']
with tempfile.TemporaryDirectory(prefix='wf-result-screen-',ignore_cleanup_errors=os.name=='nt') as temporary:
    tmp = Path(temporary)
    host = tmp / 'result-screen-host'
    subprocess.run([*flags, *sources, 'tests/r18/test_result_screen.c', '-o', str(host)], cwd=ROOT, check=True)
    subprocess.run([str(host), str(args.rom.resolve())], check=True)
    library = tmp / 'result-screen.so'
    subprocess.run([*flags, '-shared', '-fPIC', *sources, 'tests/r18/result_screen_oracle_bridge.c', '-o', str(library)], cwd=ROOT, check=True)
    native = ctypes.CDLL(str(library)).wf_result_screen_oracle
    native.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p,
                       ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint, ctypes.c_void_p,
                       ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p,
                       ctypes.POINTER(ctypes.c_uint)]
    native.restype = ctypes.c_int
    rom_buffer = ctypes.create_string_buffer(rom)
    cases = 0
    for winner_mask in range(16):
        for progression in (0, 8, 9, 10, 0x0900, 0xab09):
            actors = bytearray(rng.randbytes(9 * 0x10c))
            for slot in range(4):
                address = slot * 0x10c + 0xa0
                actors[address] = (actors[address] & ~0x40) | (0x40 if winner_mask & (1 << slot) else 0)
            regs = [rng.getrandbits(32) for _ in range(8)]
            initial_fg = rng.randbytes(0x4000)
            na = ctypes.create_string_buffer(bytes(actors))
            nr = (ctypes.c_uint32 * 8)(*regs)
            fg = ctypes.create_string_buffer(initial_fg)
            events = (ctypes.c_uint32 * 50)()
            snapshots = ctypes.create_string_buffer(5 * 0x4000)
            count = ctypes.c_uint()
            assert native(rom_buffer, len(rom), na, nr, progression, fg, events, snapshots, ctypes.byref(count)) == 0

            machine = mus.Machine(mus.CPUType.M68000, 2048)
            cpu, mem = machine.cpu, machine.mem
            mem.w_block(0, rom)
            mem.w_block(0x1c05b0, bytes(actors))
            mem.w_block(0xc0000, initial_fg)
            scratch = rng.randbytes(16)
            mem.w_block(0x1c0092, scratch)
            mem.w16(0x1c0162, progression)
            for index, value in enumerate(regs):
                cpu.w_reg(index, value)
            cpu.w_sp(0x1d0000)
            cpu.w_pc(0x11ce)
            observed, observed_snapshots = [], []
            active_wait = False
            wait_edges = []
            pending_text = None
            for step in range(30000):
                pc = cpu.r_pc()
                if pc == 0x122c:
                    break
                if pc == 0x21e6:
                    active_wait = True
                    wait_edges.append(0)
                    observed.append([0, (cpu.r_reg(0) & 0xffff) + 1,
                                     *[cpu.r_reg(i) for i in range(8)]])
                    observed_snapshots.append(bytes(mem.r_block(0xc0000, 0x4000)))
                if active_wait and pc == 0x21ea:
                    mem.w8(0x140026, 4)
                    wait_edges[-1] += 1
                elif active_wait and pc == 0x21f4:
                    mem.w8(0x140026, 0)
                elif pc == 0x2206:
                    active_wait = False
                if pc == 0x2503c:
                    pending_text = cpu.r_reg(0) & 0xffff
                if pending_text is not None and pc in (0x11e2, 0x1222):
                    observed.append([1, pending_text, *[cpu.r_reg(i) for i in range(8)]])
                    observed_snapshots.append(bytes(mem.r_block(0xc0000, 0x4000)))
                    pending_text = None
                cpu.execute(1)
            assert cpu.r_pc() == 0x122c, ('ROM runaway', cases, hex(cpu.r_pc()))
            expected_count = 4 if winner_mask and (progression & 255) == 9 else 5
            assert count.value == len(observed) == expected_count, ('boundary count', cases)
            assert wait_edges == [33, 33, 65], ('physical DBRA waits', cases, wait_edges)
            assert [observed[i][1] for i in range(len(observed)) if observed[i][0] == 1] == (
                [0x8050] if expected_count == 4 else [0x8050, 0x51 if winner_mask else 0x50]
            ), ('text choice', cases)
            for index, event in enumerate(observed):
                assert list(events[index * 10:(index + 1) * 10]) == event, ('boundary/registers', cases, index)
                assert snapshots.raw[index * 0x4000:(index + 1) * 0x4000] == observed_snapshots[index], ('boundary FG0', cases, index)
            assert na.raw[:len(actors)] == bytes(mem.r_block(0x1c05b0, len(actors))) == bytes(actors), ('actors changed', cases)
            assert list(nr) == [cpu.r_reg(i) for i in range(8)], ('final D0-D7', cases)
            assert fg.raw[:0x4000] == bytes(mem.r_block(0xc0000, 0x4000)), ('final FG0', cases)
            assert mem.r_block(0x1c0092, 16) == scratch, ('scope crossed into $11B6 clears', cases)
            machine.cleanup()
            cases += 1
    print(f'PASS {cases} original-ROM result presentations: all winner masks, byte progression, FG0, D0-D7, texts and 131 physical VBlank edges per case')
