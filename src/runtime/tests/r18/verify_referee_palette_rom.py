#!/usr/bin/env python3
"""ROM-derived regression for the $10718 palette allocation side effects.
Run with machine68k installed: python3 tests/r18/verify_referee_palette_rom.py
Optional --rom PATH overrides audit/runtime18-cd/ROM68K.BIN.
Builds the native integration probe in a temporary directory only.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile
import machine68k as mus

root = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--rom', type=Path, default=root / 'audit/runtime18-cd/ROM68K.BIN')
a = p.parse_args()
rom = a.rom.resolve().read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
# Whole native core, with the existing test double only at the Yaul hardware API.
core = next(line[5:].split() for line in (root / 'Makefile').read_text().splitlines() if line.startswith('CORE='))
extra = ['src/wf_r18_bootstrap.c', 'src/wf_r18_video.c', 'src/wf_r18_metasprite.c', 'src/wf_r18_present.c', 'tests/r18/test_referee_palette.c']
with tempfile.TemporaryDirectory(prefix='wf-referee-palette-') as tmp:
    executable = Path(tmp) / 'referee-palette'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-DWF_PLATFORM_SATURN', '-Iinclude', '-Itests/r18/mock-yaul', *core, *extra, '-o', str(executable)], cwd=root, check=True)
    for scenario, roster in enumerate(((0, 1, 6, 7), (3, 2, 10, 11))):
        machine = mus.Machine(mus.CPUType.M68000, 2048)
        cpu, mem = machine.cpu, machine.mem
        mem.w_block(0, rom)
        def call(pc):
            cpu.w_sp(0x1d0000)
            mem.w32(0x1d0000, 0x70000)
            cpu.w_pc(pc)
            for _ in range(10000):
                if cpu.r_pc() == 0x70000:
                    return
                cpu.execute(1)
            raise AssertionError(('ROM did not return', hex(cpu.r_pc())))
        for resource in roster:
            cpu.w_reg(0, resource)
            call(0x2aea)
        # No interception: the original initializer calls $2AEA itself.
        call(0x10718)
        expected = bytes(mem.r_block(0x1c1610, 256)) + bytes(mem.r_block(0x1c1600, 16)) + bytes(mem.r_block(0x182000, 0x800))
        fixture = Path(tmp) / f'expected-{scenario}.bin'
        fixture.write_bytes(expected)
        machine.cleanup()
        subprocess.run([str(executable), str(a.rom.resolve()), str(fixture), str(scenario)], check=True)
