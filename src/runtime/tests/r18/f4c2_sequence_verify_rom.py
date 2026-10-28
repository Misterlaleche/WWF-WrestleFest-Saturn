"""Repeat whole $F4C2 phases from a HOST capture with the render lists reset each tick.

This is a controlled phase continuation, not a complete arcade frame: input,
AI, collision and referee phases do not run between the compared calls.
"""
import argparse
import ctypes
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('capture', type=Path)
parser.add_argument('--ticks', type=int, default=128)
parser.add_argument('--report', type=Path)
args = parser.parse_args()
assert 1 <= args.ticks <= 1024
capture = args.capture.read_bytes()
assert len(capture) == 24 + 32 + 0x4000 + 0x4000
frame, slot, kind, pc, sequence, status = struct.unpack_from('=6I', capture)
assert (slot, kind, pc, sequence, status) == (0, 0, 0xf4c2, 0, 1)
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
initial = bytearray(0x200000)
initial[:len(rom)] = rom
initial[0x1c0000:0x1c4000] = capture[56:56 + 0x4000]
initial[0xc0000:0xc4000] = capture[56 + 0x4000:]
machine = mus.Machine(mus.CPUType.M68000, 2048)
cpu, mem = machine.cpu, machine.mem
mem.w_block(0, bytes(initial))
for i, value in enumerate(struct.unpack_from('=8I', capture, 24)):
    cpu.w_reg(i, value)
core = re.search(r'^CORE=(.*)$', (ROOT / 'Makefile').read_text(), re.M).group(1).split()
report = {
    'status': 'PENDING', 'capture_sha256': hashlib.sha256(capture).hexdigest(),
    'rom_sha1': hashlib.sha1(rom).hexdigest(), 'capture_frame': frame,
    'ticks_requested': args.ticks, 'ticks_compared': 0, 'rom_instructions': 0,
    'actor_paths': {str(i): [] for i in range(9)},
    'limits': ['Controlled repeated $F4C2 calls with render-list heads reset',
               'No input, AI, collision, referee or complete frame between calls',
               'No physical Saturn timing, audio output or full-match equivalence claim'],
}
with tempfile.TemporaryDirectory(prefix='wf-f4c2-sequence-', ignore_cleanup_errors=os.name == 'nt') as temp:
    frozen = Path(temp)
    for folder in ('src', 'include', 'generated'):
        shutil.copytree(ROOT / folder, frozen / folder)
    bridge = 'tests/r18/f4c2_natural_oracle_bridge.c'
    (frozen / 'tests/r18').mkdir(parents=True)
    shutil.copy2(ROOT / bridge, frozen / bridge)
    report['sources_sha256'] = {
        str(path.relative_to(frozen)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(frozen.rglob('*')) if path.is_file()
    }
    report['verifier_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    lib = frozen / 'phase.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
                    '-Iinclude', *core, 'src/wf_r18_runtime_handoff.c', bridge, '-o', str(lib)],
                   cwd=frozen, check=True)
    native = ctypes.CDLL(str(lib))
    native.f4c2_natural_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32),
                                          ctypes.POINTER(ctypes.c_uint32)]
    buffer = ctypes.create_string_buffer(bytes(initial))
    d = (ctypes.c_uint32 * 8)(*struct.unpack_from('=8I', capture, 24))
    for tick in range(args.ticks):
        if tick:
            for queue in range(6):
                addr = 0x1c19b8 + 4 * queue
                mem.w16(addr, 0)
                ctypes.memset(ctypes.addressof(buffer) + addr, 0, 2)
        cpu.w_sp(0x1d0000)
        mem.w32(0x1d0000, 0x70000)
        cpu.w_pc(0xf4c2)
        for i in range(8, 15):
            cpu.w_reg(i, 0)
        for steps in range(100000):
            at = cpu.r_pc()
            if at == 0x70000:
                break
            if at == 0x2052:
                cpu.w_pc(mem.r32(cpu.r_sp()))
                cpu.w_sp(cpu.r_sp() + 4)
                continue
            cpu.execute(1)
        else:
            report.update(status='ROM_RUNAWAY', first_bad_tick=tick, rom_pc=hex(cpu.r_pc()))
            break
        report['rom_instructions'] += steps
        blocked = ctypes.c_uint32()
        native_status = native.f4c2_natural_oracle(buffer, d, ctypes.byref(blocked))
        differences = []
        for addr, count, label in ((0x1c0000, 0x4000, 'RAM'), (0xc0000, 0x4000, 'FG')):
            expected = bytes(mem.r_block(addr, count))
            got = buffer.raw[addr:addr + count]
            differences += [[label, hex(i), a, b] for i, (a, b) in enumerate(zip(got, expected)) if a != b]
        rom_d = [cpu.r_reg(i) for i in range(8)]
        if native_status or differences or list(d) != rom_d:
            report.update(status='MISMATCH', first_bad_tick=tick,
                          native_status=native_status, blocked_dependency=hex(blocked.value),
                          difference_count=len(differences), first_differences=differences[:32],
                          native_d=list(d), rom_d=rom_d)
            break
        report['ticks_compared'] += 1
        for actor in range(9):
            base = 0x1c05b0 + actor * 0x10c
            if not mem.r8(base) & 0x80:
                continue
            state = mem.r16(base + 0x20)
            selector = mem.r16(base + 0x60)
            pose = mem.r16(base + 4)
            path = report['actor_paths'][str(actor)]
            item = [state, selector, pose]
            if not path or path[-1][1] != item:
                path.append([tick, item])
    else:
        report['status'] = 'PASS'
print(json.dumps(report, indent=2))
if args.report:
    args.report.write_text(json.dumps(report, indent=2) + '\n')
assert report['status'] == 'PASS', report['status']
