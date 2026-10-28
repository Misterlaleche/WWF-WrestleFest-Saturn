#!/usr/bin/env python3
"""Compare selector $00 entry and its arena resolver against original ROM.

Inputs are controlled variants of a real pre-$F4C2 HOST snapshot. No ROM
calls are intercepted. This samples boundary geometry; it is not a proof of
every coordinate, physical Saturn timing, or full arcade frame scheduling.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
import os
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus
from capstone import Cs, CS_ARCH_M68K, CS_MODE_BIG_ENDIAN, CS_MODE_M68K_000

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
WORKSPACE = ROOT.parents[1]
RAM = 0x1c0000
ACTOR = RAM + 0x5b0 + 4 * 0x10c
ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--capture', type=Path, default=WORKSPACE / 'validation/local-20261028/action00-tag-natural-1261/pre-f4c2-snapshot.bin')
parser.add_argument('--report', type=Path, required=True)
parser.add_argument('--limit', type=int, help='stop early for verifier development')
parser.add_argument('--negative-control', action='store_true',
                    help='mutate one frozen source line to prove a ROM mismatch is detected')
args = parser.parse_args()
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == ROM_SHA1
capture = args.capture.read_bytes()
assert len(capture) == 24 + 32 + 0x4000 + 0x4000
assert struct.unpack_from('=6I', capture) == (1261, 0, 0, 0xf4c2, 0, 1)
initial_ram = capture[56:56+0x4000]
initial_fg = capture[56+0x4000:]
core = re.search(r'^CORE=(.*)$', (ROOT/'Makefile').read_text(), re.M)[1].split()
bridge = 'tests/r18/action_reversal_family_oracle_bridge.c'

def put(mem, absolute, value, width=2):
    offset = absolute - RAM
    mem[offset:offset+width] = (value & ((1 << (width*8))-1)).to_bytes(width, 'big')

def fixtures():
    # Coordinates span the ring, apron and exterior, including both height
    # regimes. Mode 7 is not a legal arena dispatch in the native contract.
    for character, mode, side, facing, x, y, z in itertools.product(
            range(12), range(7), (0, 1), (0, 0x8000),
            (0x140, 0x1c8, 0x220, 0x280, 0x320, 0x390, 0x3a0, 0x400, 0x480),
            (0xff, 0x110, 0x160, 0x1a0), (0xff, 0x100, 0x140, 0x17f, 0x180, 0x190)):
        yield character, mode, side, facing, x, y, z, False, False, False, False, 0, False, 0, 0, True, False
    # The flags that skip the resolver and select the alternate descriptor
    # are independent of location. Exercise each arena, side and direction.
    for character, mode, side, facing, bypass, skip, role, flag, contact in itertools.product(
            range(12), range(7), (0, 1), (0, 0x8000), (False, True),
            (False, True), (False, True), (False, True), (0, 1, 2, 3)):
        yield character, mode, side, facing, 0x180, 0x160, 0x140, bypass, skip, role, flag, contact, False, 0, 0, True, False
    # Exercise the latched animation/profile and terminal branches explicitly.
    for character, role, flag, frame, timer, has_link, clearflag in itertools.product(
            range(12), (False, True), (False, True), (0, 3, 4, 0xfe),
            (0, 1, 0xff00), (False, True), (False, True)):
        yield character, 0, 0, 0, 0x280, 0x160, 0x140, False, False, role, flag, 0, True, frame, timer, has_link, clearflag

with tempfile.TemporaryDirectory(prefix='wf-action00-resolver-', ignore_cleanup_errors=os.name == 'nt') as temp:
    frozen = Path(temp)
    for folder in ('src', 'include', 'generated'):
        shutil.copytree(ROOT/folder, frozen/folder)
    (frozen/'tests/r18').mkdir(parents=True)
    shutil.copy2(ROOT/bridge, frozen/bridge)
    if args.negative_control:
        target = frozen/'src/wf_r17_actions_r11.c'
        source = target.read_text()
        correct = 'if(wf_u8(a,WF_A_CONTACT+1u)&3u)wf_set_be16(a,WF_A_VX,0u);'
        wrong = 'if(wf_u8(a,WF_A_CONTACT+1u)&3u)wf_set_be16(a,WF_A_VX,1u);'
        assert source.count(correct) == 1
        target.write_text(source.replace(correct, wrong))
    source_hashes = {str(path.relative_to(frozen)).replace('\\', '/'):
                     hashlib.sha256(path.read_bytes()).hexdigest()
                     for path in sorted(frozen.rglob('*')) if path.is_file()}
    lib = frozen/'action00.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-UNDEBUG',
                    '-Wall', '-Wextra', '-Werror', '-Iinclude', *core,
                    'src/wf_r18_runtime_handoff.c', bridge, '-o', str(lib)],
                   cwd=frozen, check=True)
    native = ctypes.CDLL(str(lib))
    native.action_reversal_family_oracle.argtypes = [ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32)]
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, memory = machine.cpu, machine.mem
    memory.w_block(0, rom)
    for page in range(0, len(rom), 0x10000):
        for width in range(3):
            memory.set_special_range_write_func(page, width, lambda address, value: None)
            memory.set_special_range_read_func(page, width,
                lambda address, n=1 << width: int.from_bytes(rom[address:address+n], 'big'))
    native_mem = ctypes.create_string_buffer(0x200000)
    ptr = ctypes.addressof(native_mem)
    coverage = set()
    edges = set()
    counts = {'vx_stopped': 0, 'vx_retained': 0, 'x_corrected': 0,
              'alternate_descriptor': 0, 'bypass': 0, 'skip_flag': 0, 'latched': 0}
    per_mode = {str(mode): 0 for mode in range(7)}
    cases = 0
    for character, mode, side, facing, x, y, z, bypass, skip, role, flag, contact, latched, frame, timer, has_link, clearflag in fixtures():
        data = bytearray(initial_ram)
        for address, value, width in (
            (RAM+0x7e, mode, 2), (ACTOR+2, character, 2),
            (ACTOR+6, x, 2), (ACTOR+10, y, 2), (ACTOR+14, z, 2),
            (ACTOR+0x18, 0x8001 if clearflag else 1, 2), (ACTOR+0x1a, 0x1234, 2),
            (ACTOR+0x1c, 0x8005 if latched else 5, 2), (ACTOR+0x20, 0x8005, 2),
            (ACTOR+0x22, timer, 2), (ACTOR+0x24, frame, 2),
            (ACTOR+0x26, RAM+0x5b0 if has_link else 0, 4),
            (ACTOR+0x2e, facing, 2),
            (ACTOR+0x32, (0x20 if bypass else 0) | (1 if skip else 0), 1),
            (ACTOR+0x33, (4 if side else 0) | (0x20 if flag else 0), 1),
            (ACTOR+0x36, 0x5a00 | contact, 2), (ACTOR+0x3e, 0, 2),
            (ACTOR+0x40, 0, 2), (ACTOR+0x42, 0, 2),
            (ACTOR+0x56, 0x80 if role else 0, 1),
            (ACTOR+0x58, 0x1234, 2), (ACTOR+0x60, 0, 2)):
            put(data, address, value, width)
        memory.w_block(RAM, bytes(data))
        memory.w_block(0xc0000, initial_fg)
        ctypes.memmove(ptr+RAM, bytes(data), len(data))
        ctypes.memmove(ptr+0xc0000, initial_fg, len(initial_fg))
        regs = [(0x91827364 + i*0x11111111 + cases*17) & 0xffffffff for i in range(8)]
        for i, value in enumerate(regs):
            cpu.w_reg(i, value)
        cpu.w_reg(8, ACTOR)
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1c03e)
        descriptor = None
        for _ in range(20000):
            pc = cpu.r_pc()
            if pc == 0x70000:
                break
            traced = (0x10b62 <= pc < 0x10b9a or 0x12888 <= pc < 0x128de or
                      0x1c03e <= pc < 0x1c150 or 0x280dc <= pc < 0x28600)
            if traced:
                coverage.add(pc)
            if pc == 0x1c0c8:
                descriptor = cpu.r_reg(9)
            cpu.execute(1)
            if traced:
                edges.add((pc, cpu.r_pc()))
        assert cpu.r_pc() == 0x70000, ('ROM runaway', cases, hex(cpu.r_pc()))
        d = (ctypes.c_uint32*8)(*regs)
        selected = ctypes.c_uint32()
        status = native.action_reversal_family_oracle(native_mem, d, 4, ctypes.byref(selected))
        tag = (character, mode, side, facing, x, y, z, bypass, skip, role, flag, contact, latched, frame, timer, has_link, clearflag)
        assert status == 0, ('native blocked', tag, hex(-status))
        assert selected.value == descriptor == (0x1286c if role and flag else 0x12850), (
            'descriptor', tag, hex(selected.value), hex(descriptor or 0))
        for start, size, name in ((RAM, 0x4000, 'RAM'), (0xc0000, 0x4000, 'FG')):
            got = ctypes.string_at(ptr+start, size)
            expected = bytes(memory.r_block(start, size))
            assert got == expected, (name, tag, [(hex(start+i), a, b)
                for i, (a, b) in enumerate(zip(got, expected)) if a != b][:12])
        assert list(d) == [cpu.r_reg(i) for i in range(8)], ('D0-D7', tag)
        result_x = memory.r16(ACTOR+6)
        result_vx = memory.r16(ACTOR+0x58)
        assert memory.r16(ACTOR+0x36) == (0x5a00 | contact), ('contact restoration', tag)
        assert (memory.r16(ACTOR+0x18), memory.r16(ACTOR+0x1a)) == (
            (0, 0) if clearflag else (1, 0x1234)), ('dispatcher clear flags', tag)
        assert result_vx in (0, 0x1234), ('unexpected horizontal velocity', tag, result_vx)
        if bypass:
            assert result_vx == (0 if contact else 0x1234) and result_x == x, (
                'resolver bypass', tag, result_x, result_vx)
        if latched:
            assert result_vx == 0x1234 and result_x == x
            assert memory.r16(ACTOR+0x4c) == (7 if frame == 3 else 0x21), ('profile', tag)
            assert memory.r16(ACTOR+0x20) == (0 if frame == 0xfe else 0x8005), ('terminal', tag)
            if frame == 0xfe:
                assert memory.r8(ACTOR+0x26) & 0x80, ('terminal link flag', tag)
        counts['vx_stopped' if result_vx == 0 else 'vx_retained'] += 1
        counts['x_corrected'] += result_x != x
        counts['alternate_descriptor'] += role and flag
        counts['bypass'] += bypass
        counts['skip_flag'] += skip
        counts['latched'] += latched
        per_mode[str(mode)] += 1
        cases += 1
        if args.limit and cases >= args.limit:
            break
    machine.cleanup()
    if os.name == 'nt':
        assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))

assert cases and counts['vx_stopped'] and counts['vx_retained'] and counts['x_corrected']
assert not args.negative_control, 'negative control unexpectedly escaped the comparator'
branch_pairs = []
if args.limit is None:
    required_edges = {(0x10b7e, 0x10b80), (0x10b7e, 0x10b84),
        (0x128cc, 0x128ce), (0x128cc, 0x128dc),
        (0x128d4, 0x128d6), (0x128d4, 0x128dc),
        (0x1288e, 0x12890), (0x1288e, 0x1289e),
        (0x128aa, 0x128ac), (0x128aa, 0x128b2),
        (0x128b8, 0x128ba), (0x128b8, 0x128c6),
        (0x1c09e, 0x1c0a0), (0x1c09e, 0x1c0a6),
        (0x1c0b8, 0x1c0ba), (0x1c0b8, 0x1c0c2),
        (0x1c0da, 0x1c0dc), (0x1c0da, 0x1c0ec),
        (0x1c0f2, 0x1c0f4), (0x1c0f2, 0x1c126),
        (0x1c0f8, 0x1c0fa), (0x1c0f8, 0x1c126),
        (0x1c106, 0x1c10a), (0x1c106, 0x1c122),
        (0x280f2, 0x280f6), (0x280f2, 0x2811e),
        (0x28110, 0x28112), (0x28110, 0x28116),
        (0x28198, 0x2819c), (0x28198, 0x28282),
        (0x28326, 0x2832a), (0x28326, 0x2847a)}
    assert required_edges <= edges, ('missing required branch outcomes',
                                     sorted(required_edges - edges))
    disassembler = Cs(CS_ARCH_M68K, CS_MODE_BIG_ENDIAN | CS_MODE_M68K_000)
    conditionals = {'beq', 'bne', 'bcc', 'bcs', 'bhi', 'bls', 'bge', 'blt',
                    'bgt', 'ble', 'bmi', 'bpl', 'bvc', 'bvs'}
    for start, end in ((0x10b62, 0x10b9a), (0x12888, 0x128de),
                       (0x280dc, 0x28154), (0x2818e, 0x285da)):
        for instruction in disassembler.disasm(rom[start:end], start):
            if instruction.mnemonic.split('.')[0] not in conditionals:
                continue
            target = int(instruction.op_str.strip().replace('$', ''), 16)
            fallthrough = instruction.address + instruction.size
            assert (instruction.address, target) in edges and (instruction.address, fallthrough) in edges, (
                'uncovered ROM conditional', hex(instruction.address), hex(target), hex(fallthrough))
            branch_pairs.append((instruction.address, target, fallthrough))
    assert len(branch_pairs) == 51, ('ROM conditional count changed', len(branch_pairs))
report = {'status': 'PASS', 'cases': cases, 'limited': args.limit is not None,
          'rom_sha1': ROM_SHA1, 'capture_sha256': hashlib.sha256(capture).hexdigest(),
          'verifier_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          'sources_sha256': source_hashes, 'per_mode': per_mode, 'outcomes': counts,
          'visited_callback_and_resolver_pcs': [hex(pc) for pc in sorted(coverage)],
          'observed_branch_edges': [[hex(src), hex(dst)] for src, dst in sorted(edges)],
          'callback_helper_resolver_conditionals_with_both_outcomes': len(branch_pairs),
          'dispatcher_animation_conditionals_with_both_outcomes': 6 if args.limit is None else None,
          'compared': ['RAM16KB', 'FG16KB', 'D0-D7', 'selected animation descriptor'],
          'limits': ['Controlled variants of one HOST pre-phase capture; not all coordinates or all game states',
                     'Each case compares one $00 entry, not a whole combat frame',
                     'Mode 7 is rejected by the native contract and excluded',
                     'No Saturn timing or VDP1 pixels measured']}
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('status', 'cases', 'limited', 'per_mode', 'outcomes')}))
