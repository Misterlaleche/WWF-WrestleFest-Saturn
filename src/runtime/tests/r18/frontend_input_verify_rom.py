#!/usr/bin/env python3
"""$64E0 human and automatic input versus the unmodified 68000 ROM.

Breaks caught: lost +$91 direction edge history, held actions reconfirming,
START becoming confirm, gameplay $514E substituted for frontend input,
wrong automatic-counter boundaries, missing live RNG register context,
and writes wider than the original byte/word instructions.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'
RAM_BASE = 0x1c0000
ACTOR_BASE = 0x5b0
ACTOR_BYTES = 0x10c


def put(mem, offset, value, width=2):
    mem[offset:offset + width] = (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'big')


def word(mem, offset):
    return int.from_bytes(mem[offset:offset + 2], 'big')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true', help='Only literal human sequence and its ROM comparison')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest() == ROM_SHA1
    rng = random.Random(0x64e0)
    coverage = set()
    counts = {'literal_sequence': 0, 'human_exhaustive': 0,
              'automatic_boundaries': 0, 'mixed_sequences': 0}
    automatic_directions = set()
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, memory = machine.cpu, machine.mem
    memory.w_block(0, rom)

    with tempfile.TemporaryDirectory(prefix='wf-frontend-input-', ignore_cleanup_errors=os.name == 'nt') as tmp:
        lib_path = Path(tmp) / 'frontend_input.so'
        subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall',
                        '-Wextra', '-Werror', '-Iinclude',
                        'src/wf_r18_frontend_input.c', 'src/wf_r17_m68k_shadow.c',
                        'tests/r18/frontend_input_oracle_bridge.c', '-o', str(lib_path)],
                       cwd=ROOT, check=True)
        native = ctypes.CDLL(str(lib_path))
        native.frontend_input_oracle.argtypes = [ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint, ctypes.c_uint16]
        native.frontend_input_oracle.restype = ctypes.c_int
        native.frontend_input_invalid_contexts.restype = ctypes.c_int
        assert native.frontend_input_invalid_contexts() == 0, 'Missing exact input context was accepted or mutated state'

        def compare(name, mem, regs, slot, active_low, category):
            counts[category] += 1
            actor = RAM_BASE + ACTOR_BASE + ACTOR_BYTES * slot
            memory.w_block(RAM_BASE, bytes(mem))
            port = 0x140020 + slot * 2
            memory.w16(port, active_low)
            for reg in range(15):
                cpu.w_reg(reg, regs[reg] if reg < 8 else 0)
            cpu.w_reg(8, actor)
            cpu.w_sp(0x1d0000)
            memory.w32(0x1d0000, 0x70000)
            cpu.w_pc(0x64e0)
            sites = set()
            for _ in range(200):
                pc = cpu.r_pc()
                if pc == 0x70000:
                    break
                sites.add(pc)
                cpu.execute(1)
            assert cpu.r_pc() == 0x70000, (name, 'ROM runaway', hex(cpu.r_pc()))
            coverage.update(sites)
            buffer = ctypes.create_string_buffer(bytes(mem), len(mem))
            d = (ctypes.c_uint32 * 8)(*regs)
            assert native.frontend_input_oracle(buffer, d, slot, active_low) == 0, (name, 'native rejected')
            actual = buffer.raw
            expected = bytes(memory.r_block(RAM_BASE, len(mem)))
            assert actual == expected, (name, 'RAM mismatch',
                [(hex(RAM_BASE + i), g, e) for i, (g, e) in enumerate(zip(actual, expected)) if g != e][:12])
            assert list(d) == [cpu.r_reg(i) for i in range(8)], (name, 'D0-D7', list(d), [cpu.r_reg(i) for i in range(8)])
            assert memory.r16(port) == active_low, (name, 'physical input modified')
            if 0x6570 in sites:
                automatic_directions.add(actual[ACTOR_BASE + ACTOR_BYTES * slot + 0xa9])
            return bytearray(actual), list(d)

        def initial(slot=0):
            mem = bytearray(rng.randbytes(0x2000))
            at = ACTOR_BASE + ACTOR_BYTES * slot
            put(mem, at + 0x8a, 0x140020 + slot * 2, 4)
            return mem, [rng.getrandbits(32) for _ in range(8)]

        # Literal expectations are hand-derived independently of the native port.
        # Replacing the frontend direction-edge rule with gameplay $514E breaks
        # the second step; making START an action breaks the last two steps.
        mem, regs = initial()
        mem[0x15e] = 0
        mem[ACTOR_BASE + 0x91] = 0
        put(mem, ACTOR_BASE + 0xa6, 0x8000)
        sequence = [
            (0xffee, 1, 1, 1, 1),  # RIGHT + action1, fresh
            (0xffee, 0, 1, 0, 1),  # held direction and action
            (0xffdd, 2, 2, 2, 2),  # LEFT + action2, new edges
            (0xffff, 0, 0, 0, 0),  # release clears direction history
            (0xfffe, 1, 1, 0, 0),  # RIGHT again
            (0xff7e, 0, 1, 0, 0),  # START held with RIGHT is no confirm
            (0xffbf, 0, 0, 0, 0),  # action3 is no confirm either
        ]
        for n, (raw, direction, previous, fresh, held) in enumerate(sequence):
            mem, regs = compare(f'literal-{n}', mem, regs, 0, raw, 'literal_sequence')
            actual = (word(mem, ACTOR_BASE + 0xa8), mem[ACTOR_BASE + 0x91],
                      word(mem, ACTOR_BASE + 0xa2), word(mem, ACTOR_BASE + 0xa6))
            assert actual == (direction, previous, fresh, held), ('literal', n, actual)

        if not args.smoke:
            # All 256 active low bytes, every legal previous direction plus
            # high/invalid previous bytes, and every prior action pair.
            for active, previous, held in itertools.product(range(256),
                    (*range(16), 0x10, 0x80, 0xff), range(4)):
                slot = (active + previous + held) % 4
                mem, regs = initial(slot)
                at = ACTOR_BASE + slot * ACTOR_BYTES
                mem[0x15e] &= 0xef
                mem[at + 0x91] = previous
                put(mem, at + 0xa6, held | 0xa500)
                raw = (rng.randrange(256) << 8) | (active ^ 0xff)
                compare(f'human-{active:x}-{previous:x}-{held}', mem, regs, slot, raw, 'human_exhaustive')

            for terminal, timer, seed in itertools.product(
                    (0, 1, 0x3e, 0x3f, 0x40, 0x7fff, 0xfffe, 0xffff),
                    (0, 3, 4, 5, 7, 15, 16, 17, 0xffff),
                    (0, 1, 0x80000000, 0xffffffff, 0x12345678, 0xdeadbeef)):
                slot = timer % 4
                mem, regs = initial(slot)
                at = ACTOR_BASE + slot * ACTOR_BYTES
                mem[0x15e] |= 0x10
                put(mem, at + 0x5e, terminal)
                put(mem, at + 0x56, timer)
                put(mem, 0x5c, seed, 4)
                compare(f'auto-{terminal:x}-{timer:x}-{seed:x}', mem, regs, slot,
                        rng.getrandbits(16), 'automatic_boundaries')

            # Keep records, RNG and inherited register high words alive across
            # alternating holds, release, simultaneous directions and timeout.
            for number in range(16):
                slot = number % 4
                mem, regs = initial(slot)
                mem[0x15e] &= 0xef
                put(mem, ACTOR_BASE + slot * ACTOR_BYTES + 0x5e, 0)
                put(mem, ACTOR_BASE + slot * ACTOR_BYTES + 0x56, 0)
                for tick in range(192):
                    if tick == 64:
                        mem[0x15e] |= 0x10
                    if tick == 160:
                        mem[0x15e] &= 0xef
                    put(mem, 0x80, 0xfffffff0 + tick, 4)
                    raw = (0xffff, 0xffee, 0xffdd, 0xfff0, 0xffcf,
                           0xff7f, rng.getrandbits(16))[tick % 7]
                    mem, regs = compare(f'mixed-{number}-{tick}', mem, regs, slot,
                                        raw, 'mixed_sequences')
            required = (0x64e0, 0x650c, 0x6514, 0x6534, 0x6540,
                        0x6544, 0x6558, 0x655c, 0x6576, 0x21b4)
            assert all(pc in coverage for pc in required), ('uncovered ROM branches', [hex(pc) for pc in required if pc not in coverage])
            assert automatic_directions == {1, 2, 4, 8}, automatic_directions
        assert bytes(memory.r_block(0, len(rom))) == rom, 'ROM changed during execution'
        machine.cleanup()
        report = {
            'passed': sum(counts.values()), 'cases': counts, 'rom_sha1': ROM_SHA1,
            'source_sha256': hashlib.sha256((ROOT / 'src/wf_r18_frontend_input.c').read_bytes()).hexdigest(),
            'compared': ['all RAM1C0000..1C1FFF', 'D0-D7', 'physical input preserved'],
            'automatic_directions': sorted(automatic_directions),
            'missing_context_rejections': 5,
            'covered_rom_pcs': [hex(pc) for pc in sorted(coverage)],
            'rom_modifications': [], 'intercepts': [],
            'limitations': 'Isolated $64E0 calls; no frontend loop, mode/roster consumer, SMPC mapping, video, SH2 or physical Saturn qualification. A3/CCR/stack are not represented by the native API.'
        }
        if args.report:
            args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(f"PASS {report['passed']} frontend $64E0 comparisons; RAM/D0-D7/RNG; ROM intact")


if __name__ == '__main__':
    main()
