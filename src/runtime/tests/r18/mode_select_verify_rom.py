#!/usr/bin/env python3
"""Original GAME SELECT quanta, sync waits, palettes and 2P assignment."""
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
RAM = 0x1c0000
BASE = 0x5b0


def put(mem, at, value, width=2):
    mem[at:at + width] = (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'big')


def get(mem, at, width=2):
    return int.from_bytes(mem[at:at + width], 'big')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest() == ROM_SHA1
    rng = random.Random(0x52be)
    coverage = set()
    total = 0
    events_seen = set()
    with tempfile.TemporaryDirectory(prefix='wf-mode-select-', ignore_cleanup_errors=os.name == 'nt') as tmp:
        lib = Path(tmp) / 'mode_select.so'
        subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra',
                        '-Werror', '-Iinclude', 'src/wf_r18_mode_select.c',
                        'src/wf_r18_frontend_input.c', 'src/wf_r17_m68k_shadow.c',
                        'tests/r18/mode_select_oracle_bridge.c', '-o', str(lib)], cwd=ROOT, check=True)
        native = ctypes.CDLL(str(lib))
        native.mode_select_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32),
            ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint16), ctypes.POINTER(ctypes.c_uint32)]

        def fixture(pc=0x533a, mode=0, active=1):
            mem = bytearray(0x200000)
            mem[:len(rom)] = rom
            mem[RAM:RAM + 0x2000] = rng.randbytes(0x2000)
            mem[0xc0000:0xc4000] = rng.randbytes(0x4000)
            mem[0x188000:0x189000] = rng.randbytes(0x1000)
            put(mem, RAM + 0x4e, 0)
            put(mem, RAM + 0x72, 0)
            put(mem, RAM + 0x15e, 0)
            mem[RAM + 0x161] = mode
            put(mem, RAM + 0x15c, 0)
            for slot in range(25):
                at = RAM + BASE + slot * 0x10c
                put(mem, at, 0x8000 if active & (1 << slot) else 0)
                if slot < 4:
                    put(mem, at + 0x8a, 0x140020 + slot * 2, 4)
                    put(mem, at + 6, 0)
                    put(mem, at + 0xa, 0)
                    put(mem, at + 0xa6, 0)
                    mem[at + 0x91] = 0
            return mem, [rng.getrandbits(32) for _ in range(8)], [pc, BASE, 0, 0]

        def step(name, mem, regs, state, ports=(0xffff,) * 4):
            nonlocal total
            total += 1
            buf = ctypes.create_string_buffer(bytes(mem), len(mem))
            d = (ctypes.c_uint32 * 8)(*regs)
            cursor = (ctypes.c_uint32 * 4)(*state)
            inputs = (ctypes.c_uint16 * 4)(*ports)
            calls = (ctypes.c_uint32 * 161)()
            status = native.mode_select_oracle(buf, d, cursor, inputs, calls)
            assert status >= 0, (name, 'native blocked', status, list(cursor))
            # First RED proves a missing state transition, independently of ROM.
            if state[0] == 0x533a:
                assert cursor[0] == 0x534a, (name, 'begin did not enter original actor loop', hex(cursor[0]))
            machine = mus.Machine(mus.CPUType.M68000, 2048)
            cpu, memory = machine.cpu, machine.mem
            memory.w_block(0, bytes(mem))
            for i, v in enumerate(regs): cpu.w_reg(i, v)
            for i in range(8, 15): cpu.w_reg(i, 0)
            cpu.w_reg(8, RAM + state[1])
            cpu.w_sp(0x1d0000)
            if state[0] == 0x1e92:
                memory.w32(0x1d0000, 0x5792 if state[2] else 0x5470)
            else:
                memory.w32(0x1d0000, 0x70000)
            cpu.w_pc(state[0])
            for i, v in enumerate(ports): memory.w16(0x140020 + i * 2, v)
            expected_calls = []
            target = (cursor[0], RAM + cursor[1])
            for instruction in range(20000):
                pc = cpu.r_pc()
                if (pc, cpu.r_reg(8)) == target and (instruction or state[0] in (0x539a, 0x53ca, 0x5406, 0x5436, 0x53b4, 0x53ec, 0x5420, 0x5456, 0x5778, 0x5782)):
                    break
                coverage.add(pc)
                if pc in (0x51d2, 0x527e):
                    index = cpu.r_reg(0) & 0xffff
                    source = int.from_bytes(rom[0x5224 + index * 4:0x5228 + index * 4], 'big')
                    expected_calls.append([pc, source, 0xc1770, 0, index])
                if pc == 0x1e92:
                    expected_calls.append([pc, 0, 0, 0, 0])
                if pc in (0x54ba, 0x54d0) and cpu.r_reg(4) & 0xffff == 0:
                    source = cpu.r_reg(10) + (cpu.r_reg(1) & 0xffff)
                    expected_calls.append([0x5484 if pc == 0x54ba else 0x54ca, source, cpu.r_reg(11), 15, 0])
                cpu.execute(1)
            assert (cpu.r_pc(), cpu.r_reg(8)) == target, (name, 'ROM did not reach boundary', hex(cpu.r_pc()), tuple(hex(v) for v in target))
            actual = buf.raw
            for at, size in ((RAM, 0x2000), (0xc0000, 0x4000), (0x188000, 0x1000)):
                expected = bytes(memory.r_block(at, size))
                got = actual[at:at + size]
                assert got == expected, (name, 'RAM/FG/palette', [(hex(at + i), g, e) for i, (g, e) in enumerate(zip(got, expected)) if g != e][:12])
            assert list(d) == [cpu.r_reg(i) for i in range(8)], (name, 'D0-D7', list(d), [cpu.r_reg(i) for i in range(8)])
            emitted = [list(calls[1 + i * 5:6 + i * 5]) for i in range(calls[0])]
            assert emitted == expected_calls, (name, 'effects', emitted, expected_calls)
            assert actual[:len(rom)] == rom, (name, 'native changed ROM')
            assert bytes(memory.r_block(0, len(rom))) == rom, (name, '68000 changed ROM')
            events_seen.update(row[0] for row in emitted)
            machine.cleanup()
            return bytearray(actual), list(d), list(cursor), status

        # First call preserves predecessor mode, clears only the original
        # counter and enters the actor loop. No synthetic TAG reset.
        mem, regs, state = fixture(mode=0xa4)
        mem, regs, state, status = step('begin', mem, regs, state)
        assert mem[RAM + 0x161] == 0xa4 and get(mem, RAM + 0x15c) == 0
        # Fresh action beats same-update left direction and does not flip mode.
        mem, regs, state, status = step('confirm-before-left', mem, regs, state, (0xffdd, 0xffff, 0xffff, 0xffff))
        assert state[0] == 0x5698 and mem[RAM + 0x161] == 0xa4
        if not args.smoke:
            sync_low = (0xffff, 0xffff, 0xffff, 0xfbff)
            sync_high = (0xffff,) * 4
            # The original TST.B lookup consumes every nibble emitted by64E0,
            # including contradictory directions and the adjacent ROM bytes.
            for mode, direction, previous, action in itertools.product(
                    (0, 1, 0xfe, 0xff), range(16), (0, 1), range(4)):
                mem, regs, state = fixture(pc=0x534a, mode=mode)
                mem[RAM + BASE + 0x91] = direction if previous else 0
                put(mem, RAM + BASE + 0xa6, action)
                raw = 0xff00 | (0xff ^ (direction | (action << 4)))
                mem, regs, state, status = step(f'mode-{mode}-{direction}-{previous}-{action}', mem, regs, state, (raw, *sync_high[1:]))
                edge = 0 if previous else direction
                table = 0x54e0 if mode & 1 else 0x54ec
                want = mode ^ (1 if rom[table + edge] else 0)
                assert mem[RAM + 0x161] == want
                for sync in (sync_low, sync_high, sync_high, sync_low):
                    mem, regs, state, status = step('mode-sync', mem, regs, state, sync)

            # Every pulse frame/quantum and predecessor mode: fifteen words
            # are written at5 quanta, frame index9 wraps to0.
            for mode, frame, timer in itertools.product((0, 1), range(10), range(5)):
                mem, regs, state = fixture(pc=0x534a, mode=mode)
                put(mem, RAM + BASE + 6, frame)
                put(mem, RAM + BASE + 0xa, timer)
                mem, regs, state, status = step('pulse-input', mem, regs, state)
                mem, regs, state, status = step(f'pulse-{mode}-{frame}-{timer}', mem, regs, state)
                assert get(mem, RAM + BASE + 0xa) == (timer + 1) % 5
                assert get(mem, RAM + BASE + 6) == ((frame + 1) % 10 if timer == 4 else frame)

            for counter in (0, 0x1fe, 0x1ff, 0x200, 0xffff):
                mem, regs, state = fixture(pc=0x534a)
                put(mem, RAM + 0x15c, counter)
                put(mem, RAM + 0x80, 0xffffffff, 4)
                mem, regs, state, status = step(f'timeout-{counter}', mem, regs, state)
                assert get(mem, RAM + 0x80, 4) == 0
                assert (state[0] == 0x5698) == (((counter + 1) & 0xffff) >= 512)

            mem, regs, state = fixture(pc=0x534a, active=3)
            put(mem, RAM + 0x15c, 510)
            regs[0] = 3
            for ports in ((0xfffd, *sync_high[1:]), sync_high, sync_low, sync_high):
                mem, regs, state, status = step('two-actor-timeout', mem, regs, state, ports)
            assert state[0] == 0x5698 and get(mem, RAM + 0x15c) == 0

            mem, regs, state = fixture(pc=0x534a, active=0)
            regs[0] = 3
            for _ in range(5):
                mem, regs, state, status = step('inactive-scan-credit', mem, regs, state)
            assert state[:2] == [0x534a, BASE]

            # Gate truth table includes predecessor high bits and Royal.
            for config, active, mode, predecessor in itertools.product(
                    (0, 0x800, 0xffff), (1, 3), (0, 1), (0, 0x80, 0xff)):
                mem, regs, state = fixture(pc=0x5698, mode=mode, active=active)
                put(mem, RAM + 0x66, config)
                mem[RAM + 0x160] = predecessor
                mem, regs, state, status = step('assignment-gate', mem, regs, state)
                runs = bool(config & 0x800 and active & 2 and not mode & 1)
                assert (state[0] == 0x56e2) == runs
                assert status == (0 if runs else 2)

            for selection, direction, action in itertools.product((0x40, 0x80), range(16), range(4)):
                mem, regs, state = fixture(pc=0x56e2, active=3)
                mem[RAM + 0x160] = selection | 0x15
                regs[3] = 1
                raw = 0xff00 | (0xff ^ (direction | (action << 4)))
                mem, regs, state, status = step(f'assignment-edge-{selection:x}-{direction:x}-{action}', mem, regs, state, (raw, *sync_high[1:]))
                if action:
                    assert state[0] == 0x57e0
                else:
                    table = 0x54f8 if selection & 0x40 else 0x5503
                    row = (0x80 if rom[table + direction] else 0x40) if selection & 0x40 else (0x40 if rom[table + direction] else 0x80)
                    assert mem[RAM + 0x160] & 0xc0 == row

            for selection, counter in itertools.product((0x40, 0x80), (0x1fe, 0x1ff, 0xffff)):
                mem, regs, state = fixture(pc=0x56e2, active=3)
                mem[RAM + 0x160] = selection
                put(mem, RAM + 0x15c, counter)
                regs[3] = 1
                mem, regs, state, status = step('assignment-timeout', mem, regs, state)
                if counter == 0x1ff:
                    assert state[0] == 0x57e0
                    if selection == 0x40:
                        assert get(mem, RAM + BASE + 0x10c) == 0
                        assert get(mem, RAM + BASE + 2 * 0x10c) == 0x8000
                        assert get(mem, RAM + BASE + 2 * 0x10c + 0x8a, 4) == 0x140022

            mem, regs, state = fixture(pc=0x5698, active=3)
            put(mem, RAM + 0x66, 0x800)
            for ports in (sync_high, sync_high, sync_high, sync_low, sync_high, sync_high, sync_low, sync_high):
                mem, regs, state, status = step('assignment-two-actor-sync', mem, regs, state, ports)
            # A held action cannot reconfirm:512 active-actor quanta elapse,
            # retaining pulse counters and register state across every wait.
            mem, regs, state = fixture(pc=0x534a)
            regs[0] = 3
            put(mem, RAM + BASE + 0xa6, 1)
            frame_before = get(mem, RAM + 0x80, 4)
            held_action = (0xffef, 0xffff, 0xffff, 0xffff)
            for quantum in range(1, 513):
                mem, regs, state, status = step(f'held-action-{quantum}', mem, regs, state, held_action)
                if quantum == 512:
                    assert state[0] == 0x5698 and get(mem, RAM + 0x15c) == 0
                    break
                assert state[0] == 0x539a and get(mem, RAM + 0x15c) == quantum
                # Pulse loads D0 with its frame index before the original
                # DBRA, so inactive records can intervene before the next
                # actor0 quantum. Drive those actual boundaries as well.
                for _ in range(32):
                    ports = sync_low if state[0] in (0x53b4, 0x53ec, 0x5420, 0x5456) else held_action
                    mem, regs, state, status = step('held-action-sync-credit', mem, regs, state, ports)
                    if state[:2] == [0x534a, BASE]:
                        break
                assert state[:2] == [0x534a, BASE]
            assert get(mem, RAM + 0x80, 4) == (frame_before + 512) & 0xffffffff
            required = (0x53c2, 0x542e, 0x5484, 0x54ca, 0x54ba, 0x54d0,
                        0x56aa, 0x56b6, 0x56c2, 0x573e, 0x571c, 0x5796,
                        0x57b8, 0x57d8, 0x5778, 0x5782, 0x1e92)
            assert all(pc in coverage for pc in required), ('uncovered owner branches', [hex(pc) for pc in required if pc not in coverage])
            assert events_seen == {0x5484, 0x54ca, 0x51d2, 0x527e, 0x1e92}
        report = {'passed': total, 'rom_sha1': ROM_SHA1,
            'covered_rom_pcs': [hex(v) for v in sorted(coverage)],
            'effect_owners': [hex(v) for v in sorted(events_seen)],
            'intercepts': [], 'rom_modifications': [],
            'source_sha256': hashlib.sha256((ROOT / 'src/wf_r18_mode_select.c').read_bytes()).hexdigest(),
            'limitations': 'Original logical quanta from533A; compositor52BE..5339 and external1E92 credit cache dirty path are dependencies; A3/A4/CCR/stack excluded.'}
        if args.report: args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(f'PASS {total} GAME SELECT ROM boundaries; RAM/FG/palette/D0-D7/effects')


if __name__ == '__main__':
    main()
