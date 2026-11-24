#!/usr/bin/env python3
"""Regression for TAG side ropes retaining their exterior horizontal pose.

Unmodified original 68000 $1004A/$10120/$F8D8/$F8E4 create the fixture and
expected return. The native test runs the real initializer followed by the
real gameplay phase, including its named/compact alias bridge. No renderer,
gameplay callback or original instruction is replaced. machine68k is only a
host test dependency. Source roots allow an explicit frozen-B11 red run.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OBJECT_BASE = 0x1C1134
OBJECT_BYTES = 4 * 0x30
RETURN_PC = 0x70000


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class RopeOracle:
    def __init__(self, rom):
        import machine68k as mus
        self.mus = mus
        self.machine = mus.Machine(mus.CPUType.M68000, 2048)
        self.mem, self.cpu = self.machine.mem, self.machine.cpu
        self.rom = bytes(rom)
        assert len(self.rom) == 0x80000

    def reset(self):
        self.mem.w_block(0, self.rom)
        self.mem.w_block(0x1C0000, bytes(0x4000))
        self.mem.w_block(0x140000, bytes(0x40))
        self.cpu.w_sr(0x2700)
        for i in range(15):
            self.cpu.w_reg(i, 0)
        self.mem.w16(0x1C17E6, 0x1E0)
        self.mem.w16(0x1C17EE, 0x230)

    def call(self, pc, actor=None):
        self.cpu.w_sp(0x1C3FF0)
        self.mem.w32(0x1C3FF0, RETURN_PC)
        if actor is not None:
            self.cpu.w_reg(self.mus.Register.A0, actor)
        self.cpu.w_pc(pc)
        for _ in range(20000):
            if self.cpu.r_pc() == RETURN_PC:
                return
            self.cpu.execute(1)
        raise AssertionError(f"Unmodified ROM ${pc:05X} did not return: ${self.cpu.r_pc():05X}")

    def objects(self):
        return bytes(self.mem.r_block(OBJECT_BASE, OBJECT_BYTES))

    def fixture(self, wrestler_x, referee_x, prior_state):
        self.reset()
        self.call(0x1004A)
        self.call(0x10120)
        if prior_state:
            # Original rope-contact state at the alias addresses. $10120
            # executes the intact original sound and animation callbacks.
            for i in range(4):
                self.mem.w16(OBJECT_BASE + i * 0x30 + 0x1C, prior_state)
            self.call(0x10120)
        interior = self.objects()
        self.mem.w8(0x1C0161, 2)  # original outside-mode flag
        self.mem.w16(0x1C05B0 + 6, wrestler_x)
        self.call(0xF8D8, 0x1C05B0)
        self.mem.w16(0x1C11F4 + 6, referee_x)
        self.call(0xF8E4, 0x1C11F4)
        exterior = self.objects()
        self.mem.w8(0x1C0161, 0)
        # $FCC0 in the real return sequence calls the same $1004A routine.
        self.call(0x1004A)
        aliases = b"".join(self.objects()[i * 0x30 + 0x1C:i * 0x30 + 0x1E]
                            for i in range(4))
        self.call(0x10120)
        return interior, exterior, aliases + self.objects()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'evidence/boot12/ropes/rom')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    source, out = args.source_root.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    binary = out / 'native-return'
    sources = [source / 'core/src' / name for name in (
        'wf_r17_fixed_four_10120.c', 'wf_gameplay_bindings.c',
        'wf_r17_render_contract.c', 'wf_r17_m68k_shadow.c')]
    flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
             '-ffunction-sections', '-fdata-sections']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    subprocess.run([os.environ.get('CC', 'cc'), *flags,
                    '-I' + str(source / 'core/include'),
                    str(ROOT / 'tests/test_tag_ropes.c'), *map(str, sources),
                    '-Wl,--gc-sections', '-o', str(binary)], check=True)
    rom_path = ROOT / 'source_assets/ROM68K.BIN'
    oracle = RopeOracle(rom_path.read_bytes())
    cases = []
    failures = 0
    for x, ref_x in [(0x280, 0x280), (0x2A0, 0x2A0), (0x3B0, 0x3B0)]:
        for state in range(5):
            name = f'x{x:03x}-state{state}'
            interior, exterior, expected = oracle.fixture(x, ref_x, state)
            (out / f'{name}-interior.bin').write_bytes(interior)
            fixture = out / f'{name}-exterior.bin'
            fixture.write_bytes(exterior)
            (out / f'{name}-rom-return.bin').write_bytes(expected)
            actual_path = out / f'{name}-native-return.bin'
            subprocess.run([str(binary), str(fixture), str(actual_path)], check=True,
                           env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
            actual = actual_path.read_bytes()
            equal = actual == expected
            failures += not equal
            mismatch = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
            native_poses = [int.from_bytes(actual[8+i*0x30+4:8+i*0x30+6], 'big') for i in range(4)]
            rom_poses = [int.from_bytes(expected[8+i*0x30+4:8+i*0x30+6], 'big') for i in range(4)]
            cases.append(dict(case=name, equal=equal, mismatch_offsets=mismatch,
                              native_aliases=actual[:8].hex(), rom_aliases=expected[:8].hex(),
                              native_poses=native_poses, rom_poses=rom_poses))
            print(f'{"PASS" if equal else "FAIL"} {name}: aliases={actual[:8].hex()} '
                  f'poses={[hex(p) for p in native_poses]} ROM={[hex(p) for p in rom_poses]}', flush=True)
    report = dict(scope='Exact unmodified 68000 compact-object return vs native producer and first consumer; '
                        '15 fixtures covering left/middle/right exterior pieces and five prior rope states. '
                        'No physical Saturn timing or full interactive replay is claimed.',
                  rom_sha256=sha(rom_path), sources={str(p): sha(p) for p in sources},
                  cases=cases, failures=failures)
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{"FAIL" if failures else "PASS"}: {len(cases)-failures}/{len(cases)} original-ROM rope returns match')
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
