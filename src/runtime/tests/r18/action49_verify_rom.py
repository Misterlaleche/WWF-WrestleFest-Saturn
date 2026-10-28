"""Compare Royal pin intervention $12614[$49] with the intact 68000 ROM."""
import ctypes
import hashlib
import itertools
import json
import random
import re
import struct
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

root = Path(__file__).resolve().parents[2]
workspace = root.parents[1]
rom = (root / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12614 + 0x49 * 4:0x12618 + 0x49 * 4] == bytes.fromhex('00013434')
assert rom[0x13434:0x13444] == bytes.fromhex('00013444000100020010ff0001e901ea')
actor = [0x1c05b0 + i * 0x10c for i in range(9)]
rng = random.Random(0x13444)


def put(data, address, value, width=2):
    data[address:address + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'big')


def seed(slot=4, held_slot=0, attacker_slot=7):
    data = bytearray(0x200000)
    data[:len(rom)] = rom
    for index, address in enumerate(actor):
        put(data, address, 0x8000)
        put(data, address + 2, index % 12)
        put(data, address + 6, 0x250)
        put(data, address + 10, 0x150)
        put(data, address + 14, 0x140)
        put(data, address + 0x20, 0x8005)
        put(data, address + 0x26, actor[(index + 1) % 9], 4)
        put(data, address + 0x2e, 0)
    a, t, p = (actor[index] for index in (slot, held_slot, attacker_slot))
    put(data, a + 0x1c, 5)
    put(data, a + 0x20, 0x8005)
    put(data, a + 0x26, t, 4)
    put(data, a + 0x60, 0x4049)
    put(data, a + 6, 0x260)
    put(data, t + 0x20, 0x8005)
    put(data, t + 0x26, p, 4)
    put(data, t + 0x60, 0x804a)
    put(data, t + 0x9c, a, 4)
    put(data, t + 10, 0x160)
    put(data, t + 0x64, 8)
    put(data, p + 0x20, 0x8005)
    put(data, p + 0x60, 0xc048)
    put(data, p + 6, 0x280)
    put(data, p + 10, 0x160)
    return data, a, t, p


def fixtures():
    for slot, held, attacker, face in ((4, 0, 7, 0), (0, 4, 7, 0x80), (8, 2, 6, 0)):
        data, a, t, p = seed(slot, held, attacker)
        put(data, a + 0x58, face, 1)
        yield f'initial-{slot}-{face}', data, slot

        linked = bytearray(data)
        put(linked, a + 0x60, 0xc049)
        yield f'completed-with-link-{slot}', linked, slot
        for frame, edge in ((0, 0), (1, 0), (1, 0x10)):
            case = bytearray(data)
            put(case, a + 0x1c, 0x8005)
            put(case, a + 0x24, frame)
            put(case, a + 0x37, edge, 1)
            yield f'latched-{slot}-{frame}-{edge}', case, slot
        for reason in ('wrong-reciprocal', 'wrong-held-state', 'wrong-held-selector',
                       'held-selector-bit5', 'wrong-attacker-state', 'wrong-attacker-selector'):
            case = bytearray(data)
            put(case, a + 0x1c, 0x8005)
            put(case, a + 0x24, 1)
            put(case, a + 0x37, 0x10, 1)
            if reason == 'wrong-reciprocal':put(case, t + 0x9c, p, 4)
            elif reason == 'wrong-held-state':put(case, t + 0x20, 0x8004)
            elif reason == 'wrong-held-selector':put(case, t + 0x60, 0x804b)
            elif reason == 'held-selector-bit5':put(case, t + 0x60, 0xa04a)
            elif reason == 'wrong-attacker-state':put(case, p + 0x20, 0x8004)
            else:put(case, p + 0x60, 0xc047)
            yield f'release-{slot}-{reason}', case, slot
    for substate, face_a, face_t in itertools.product((8, 9), (0, 0x80), (0, 0x80)):
        data, a, t, p = seed()
        put(data, a + 0x1c, 0x8005)
        put(data, a + 0x24, 1)
        put(data, a + 0x37, 0x10, 1)
        put(data, t + 0x64, substate)
        put(data, a + 0x2e, face_a, 1)
        put(data, t + 0x2e, face_t, 1)
        yield f'engage-{substate}-{face_a}-{face_t}', data, 4


core = re.search(r'^CORE=(.*)$', (root / 'Makefile').read_text(), re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action49-') as folder:
    library = Path(folder) / 'action49.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Iinclude', *core, 'src/wf_r18_runtime_handoff.c',
                    'tests/r18/action0b_oracle_bridge.c', '-o', str(library)],
                   cwd=root, check=True)
    native = ctypes.CDLL(str(library))
    native.action0b_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32),
                                      ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32)]
    count, coverage = 0, set()
    for name, data, slot in fixtures():
        regs = [rng.getrandbits(32) for _ in range(8)]
        data[0xc0000:0xc4000] = rng.randbytes(0x4000)
        machine = mus.Machine(mus.CPUType.M68000, 2048)
        cpu, memory = machine.cpu, machine.mem
        memory.w_block(0, bytes(data))
        for index, value in enumerate(regs):cpu.w_reg(index, value)
        cpu.w_reg(8, actor[slot])
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1c03e)
        descriptor = 0
        for _ in range(20000):
            pc = cpu.r_pc()
            coverage.add(pc)
            if pc == 0x70000:break
            if pc == 0x1c0c8:descriptor = cpu.r_reg(9)
            cpu.execute(1)
        assert cpu.r_pc() == 0x70000, (name, 'ROM runaway', hex(cpu.r_pc()))
        ram = bytes(memory.r_block(0x1c0000, 0x4000))
        fg = bytes(memory.r_block(0xc0000, 0x4000))
        want_regs = [cpu.r_reg(index) for index in range(8)]
        machine.cleanup()
        buffer = ctypes.create_string_buffer(bytes(data))
        native_regs = (ctypes.c_uint32 * 8)(*regs)
        native_descriptor = ctypes.c_uint32()
        status = native.action0b_oracle(buffer, native_regs, slot,
                                        ctypes.byref(native_descriptor))
        assert status == 0, (name, 'native blocked', hex(-status))
        assert native_descriptor.value == descriptor, (name, 'descriptor',
                                                       hex(native_descriptor.value), hex(descriptor))
        for address, expected in ((0x1c0000, ram), (0xc0000, fg)):
            got = buffer.raw[address:address + len(expected)]
            assert got == expected, (name, 'memory', [(hex(address + index), x, y)
                                                      for index, (x, y) in enumerate(zip(got, expected))
                                                      if x != y][:12])
        assert list(native_regs) == want_regs, (name, 'D0-D7',
                                                list(native_regs), want_regs)
        count += 1
    required = {0x13444, 0x1349e, 0x13500, 0x13580, 0x1358c, 0x135be}
    assert required.issubset(coverage), [hex(pc) for pc in sorted(required - coverage)]
    report = {'status': 'PASS', 'cases': count, 'rom_sha1': hashlib.sha1(rom).hexdigest(),
              'compared': ['RAM16KiB', 'FG16KiB', 'D0-D7', 'descriptor'],
              'covered_branch_sites': [hex(pc) for pc in sorted(coverage)
                                       if 0x13444 <= pc <= 0x135be]}
    output = workspace / 'validation/local-20261025/action49-rom-report.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {count} action49 cases; raw RAM, FG, D0-D7, descriptor')
