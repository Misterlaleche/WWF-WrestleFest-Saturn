"""First whole wrestler-phase check from an unmodified HOST combat capture."""
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
parser.add_argument('--report', type=Path)
parser.add_argument('--actor4-x', type=lambda value: int(value, 0),
                    help='Controlled pre-phase X variant for ring marker branch boundaries')
parser.add_argument('--actor4-state', type=lambda value: int(value, 0),
                    help='Controlled main-state variant for shared linked-hold animation')
args = parser.parse_args()

raw = args.capture.read_bytes()
assert len(raw) == 24 + 32 + 0x4000 + 0x4000
frame, slot, kind, pc, sequence, status = struct.unpack_from('=6I', raw)
assert (slot, kind, pc, sequence, status) == (0, 0, 0xf4c2, 0, 1)
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
initial = bytearray(0x200000)
initial[:len(rom)] = rom
initial[0x1c0000:0x1c4000] = raw[56:56 + 0x4000]
initial[0xc0000:0xc4000] = raw[56 + 0x4000:]
if args.actor4_x is not None:
    assert 0 <= args.actor4_x <= 0xffff
    actor4_x = 0x1c05b0 + 4 * 0x10c + 6
    initial[actor4_x:actor4_x + 2] = args.actor4_x.to_bytes(2, 'big')
if args.actor4_state is not None:
    assert 0 <= args.actor4_state <= 0xffff
    actor4_state = 0x1c05b0 + 4 * 0x10c + 0x20
    initial[actor4_state:actor4_state + 2] = args.actor4_state.to_bytes(2, 'big')
machine = mus.Machine(mus.CPUType.M68000, 2048)
cpu, memory = machine.cpu, machine.mem
memory.w_block(0, bytes(initial))
for i, value in enumerate(struct.unpack_from('=8I', raw, 24)):
    cpu.w_reg(i, value)
cpu.w_sp(0x1d0000)
memory.w32(0x1d0000, 0x70000)
cpu.w_pc(0xf4c2)
visited = set()
watch_offsets = (0x9e5, 0x9f3, 0xa04, 0xa05, 0xa71,
                 0x1139, 0x113a, 0x113b, 0x113f, 0x1143, 0x1147, 0x1148, 0x1149, 0x114b)
watch_writes = []
for steps in range(1000000):
    at = cpu.r_pc()
    if at == 0x70000:
        break
    visited.add(at)
    if at == 0x2052:  # Native sound output is not part of the gameplay comparison.
        cpu.w_pc(memory.r32(cpu.r_sp()))
        cpu.w_sp(cpu.r_sp() + 4)
        continue
    old = [memory.r8(0x1c0000 + off) for off in watch_offsets]
    cpu.execute(1)
    for off, before in zip(watch_offsets, old):
        after = memory.r8(0x1c0000 + off)
        if before != after:
            watch_writes.append([hex(at), hex(off), before, after])
else:
    raise AssertionError(f'ROM wrestler phase did not return: PC={cpu.r_pc():06x}')
report = {
    'status': 'ROM_RETURNED',
    'frame': frame,
    'actor4_x_variant': args.actor4_x,
    'actor4_state_variant': args.actor4_state,
    'steps': steps,
    'distinct_pcs': len(visited),
    'entered_fixed_four_10120': 0x10120 in visited,
    'visited_101xx': [hex(pc) for pc in sorted(visited) if 0x10100 <= pc < 0x10200],
    'marker_paths': [hex(pc) for pc in (0xf908, 0xf92c, 0xf952) if pc in visited],
    'watched_rom_writes': watch_writes,
    'capture_sha256': hashlib.sha256(raw).hexdigest(),
    'rom_sha1': hashlib.sha1(rom).hexdigest(),
}
core = re.search(r'^CORE=(.*)$', (ROOT / 'Makefile').read_text(), re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-f4c2-natural-', ignore_cleanup_errors=os.name == 'nt') as temp:
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
    d = (ctypes.c_uint32 * 8)(*struct.unpack_from('=8I', raw, 24))
    blocked = ctypes.c_uint32()
    native_status = native.f4c2_natural_oracle(buffer, d, ctypes.byref(blocked))
    differences = []
    for start, count, label in ((0x1c0000, 0x4000, 'RAM'), (0xc0000, 0x4000, 'FG')):
        expected = bytes(memory.r_block(start, count))
        got = buffer.raw[start:start + count]
        differences += [[label, hex(i), a, b] for i, (a, b) in enumerate(zip(got, expected)) if a != b]
    rom_d = [cpu.r_reg(i) for i in range(8)]
    report.update(native_status=native_status, blocked_dependency=hex(blocked.value),
                  difference_count=len(differences), first_differences=differences[:40],
                  native_d=list(d), rom_d=rom_d)
    report['status'] = 'PASS' if native_status == 0 and not differences and list(d) == rom_d else 'MISMATCH'
print(json.dumps(report, indent=2))
if args.report:
    args.report.write_text(json.dumps(report, indent=2) + '\n')
assert report['status'] == 'PASS', report['status']
