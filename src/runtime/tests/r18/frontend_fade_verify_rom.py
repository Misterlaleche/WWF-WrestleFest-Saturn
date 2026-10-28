#!/usr/bin/env python3
"""Pure $26642 transform versus intact ROM, all 16-bit colors and phases.

Catches channel swaps, substituted fade arithmetic, lost ROM-table entries,
unmasked upper nibbles, wrong 256-color extent, and broken in-place operation.
No gameplay or device owner is intercepted or patched.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--report', type=Path)
p.add_argument('--smoke', action='store_true')
args = p.parse_args()
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
rng = random.Random(0x26642)
SOURCE, DEST, RETURN = 0x1c1000, 0x1c1400, 0x70000
WORD = ctypes.c_uint16
PTR = ctypes.POINTER(WORD)
count = colors = 0
coverage = set()


def native_buffers(function, values, phase, expected):
    """Real destination canaries catch writes beyond the fixed palette."""
    guard = [0xdead] * 8
    incoming = (WORD * 272)(*(guard + values + guard))
    outgoing = (WORD * 272)(*([0xbeef] * 272))
    source = ctypes.cast(ctypes.byref(incoming, 16), PTR)
    output = ctypes.cast(ctypes.byref(outgoing, 16), PTR)
    assert function(source, output, phase) == 0, ('native owner unavailable', phase)
    assert list(outgoing)[8:264] == expected, ('out-of-place color', phase, values[0])
    assert list(outgoing)[:8] + list(outgoing)[264:] == [0xbeef] * 16
    assert list(incoming) == guard + values + guard, 'source modified'
    assert function(source, source, phase) == 0, ('in-place status', phase)
    assert list(incoming) == guard + expected + guard, ('in-place color/bounds', phase, values[0])


with tempfile.TemporaryDirectory(prefix='wf-frontend-fade-', ignore_cleanup_errors=os.name == 'nt') as temp:
    library = Path(temp) / 'frontend-fade.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Iinclude', 'src/wf_r18_frontend_fade.c', '-o', str(library)], cwd=ROOT, check=True)
    function = ctypes.CDLL(str(library)).wf_r18_frontend_fade_palette_26642
    function.argtypes = [PTR, PTR, ctypes.c_uint]
    function.restype = ctypes.c_int
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, memory = machine.cpu, machine.mem
    memory.w_block(0, rom)
    try:
        for phase in range(8):
            for base in (range(0, 0x10000, 256) if not args.smoke else (0x100,)):
                values = list(range(base, base + 256))
                ram = bytearray(rng.randbytes(0x2000))
                ram[SOURCE - 0x1c0000:SOURCE - 0x1c0000 + 512] = b''.join(v.to_bytes(2, 'big') for v in values)
                memory.w_block(0x1c0000, bytes(ram))
                for register in range(8):
                    cpu.w_reg(register, rng.getrandbits(32))
                cpu.w_reg(0, phase)
                cpu.w_reg(8, SOURCE)
                cpu.w_reg(9, DEST)
                cpu.w_sp(0x1d0000)
                memory.w32(0x1d0000, RETURN)
                cpu.w_pc(0x26642)
                for _ in range(20000):
                    pc = cpu.r_pc()
                    coverage.add(pc)
                    if pc == RETURN:
                        break
                    cpu.execute(1)
                assert cpu.r_pc() == RETURN, ('ROM runaway', phase, base, hex(cpu.r_pc()))
                actual_ram = bytes(memory.r_block(0x1c0000, 0x2000))
                raw = actual_ram[DEST - 0x1c0000:DEST - 0x1c0000 + 512]
                expected = [int.from_bytes(raw[i:i + 2], 'big') for i in range(0, 512, 2)]
                ram[DEST - 0x1c0000:DEST - 0x1c0000 + 512] = raw
                assert actual_ram == ram, 'unexpected ROM RAM write'
                native_buffers(function, values, phase, expected)
                count += 1
                colors += 256
    finally:
        machine.cleanup()
    sentinel = (WORD * 256)(*([0xaaaa] * 256))
    assert function(sentinel, sentinel, 8) == 2
    assert function(sentinel, sentinel, 0xffffffff) == 2
    assert function(None, sentinel, 0) == 2
    assert function(sentinel, None, 0) == 2
    assert list(sentinel) == [0xaaaa] * 256, 'invalid call changed palette'

report = {
    'passed_palettes': count,
    'compared_color_phase_pairs': colors,
    'native_alias_modes': ['separate buffers', 'exact in-place'],
    'invalid_call_controls': 4,
    'rom_sha1': hashlib.sha1(rom).hexdigest(),
    'table_sha256': hashlib.sha256(rom[0x266b2:0x26732]).hexdigest(),
    'source_sha256': hashlib.sha256((ROOT / 'src/wf_r18_frontend_fade.c').read_bytes()).hexdigest(),
    'covered_owner_sites': [hex(pc) for pc in sorted(coverage) if 0x26642 <= pc <= 0x266b0],
    'intercepts': [],
    'limitations': ['Pure compact 256-word palette transform only; no controller, timing, video upload or hardware claim',
                    'Host-order uint16_t API; caller converts any byte-packed external palette'],
}
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
print(f'PASS {count} ROM palettes, {colors} color/phase pairs, separate and in-place, four invalid controls')
