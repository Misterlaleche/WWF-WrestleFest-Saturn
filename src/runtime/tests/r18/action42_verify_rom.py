"""Compare Royal victim reaction $12614[$42] with the intact 68000 ROM."""
import ctypes
import hashlib
import json
import random
import re
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

root = Path(__file__).resolve().parents[2]
workspace = root.parents[1]
rom = (root / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x1271c:0x12720] == bytes.fromhex('00017180')
assert rom[0x17180:0x17194] == bytes.fromhex('00017194000100030008000c0004015701580158')
actor = [0x1c05b0 + i * 0x10c for i in range(9)]
rng = random.Random(0x17194)


def put(data, address, value, width=2):
    data[address:address + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'big')


def seed(slot=8, linked=5):
    data = bytearray(0x200000)
    data[:len(rom)] = rom
    put(data, 0x1c0161, 1, 1)  # Royal mode bypasses the TAG partner helper.
    for index, address in enumerate(actor):
        put(data, address, 0x8000)
        put(data, address + 2, index)
        put(data, address + 6, 0x260)
        put(data, address + 10, 0x160)
        put(data, address + 14, 0x140)
        put(data, address + 0x20, 0x8005)
        put(data, address + 0x26, actor[(index + 1) % 9], 4)
    a, t = actor[slot], actor[linked]
    put(data, a + 0x1c, 5)
    put(data, a + 0x20, 0x8005)
    put(data, a + 0x26, t, 4)
    put(data, a + 0x60, 0x42)
    put(data, t + 0x20, 0x8005)
    put(data, t + 0x60, 0x7b)
    return data, a, t


def fixtures():
    for slot, linked in ((8, 5), (4, 0), (0, 7)):
        data, a, t = seed(slot, linked)
        yield f'initial-{slot}', data, slot
        already = bytearray(data)
        put(already, a + 0x60, 0x8042)
        yield f'initial-flagged-{slot}', already, slot
        for frame, timer, count in ((0, 0, 3), (0, 8, 3), (1, 0, 3),
                                    (1, 0, 2), (1, 0, 1), (1, 1, 1),
                                    (2, 0, 1), (0xfe, 0, 1)):
            case = bytearray(data)
            put(case, a + 0x1c, 0x8005)
            put(case, a + 0x24, frame)
            put(case, a + 0x22, timer)
            put(case, a + 0x44, count)
            put(case, a + 0x60, 0x8042)
            put(case, a + 0x2e, 0x80 if slot == 4 else 0)
            yield f'phase-{slot}-{frame}-{timer}-{count}', case, slot


core = re.search(r'^CORE=(.*)$', (root / 'Makefile').read_text(), re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action42-') as folder:
    library = Path(folder) / 'action42.so'
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
        for index, value in enumerate(regs):
            cpu.w_reg(index, value)
        cpu.w_reg(8, actor[slot])
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1c03e)
        descriptor = 0
        for _ in range(20000):
            pc = cpu.r_pc()
            coverage.add(pc)
            if pc == 0x70000:
                break
            if pc == 0x1c0c8:
                descriptor = cpu.r_reg(9)
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
    required = {0x17194, 0x171ba, 0x171d0, 0x171f2, 0x17200, 0x17210, 0x17218, 0x17264}
    assert required.issubset(coverage), [hex(pc) for pc in sorted(required - coverage)]
    report = {'status': 'PASS', 'cases': count, 'rom_sha1': hashlib.sha1(rom).hexdigest(),
              'compared': ['RAM16KiB', 'FG16KiB', 'D0-D7', 'descriptor'],
              'covered_branch_sites': [hex(pc) for pc in sorted(coverage)
                                       if 0x17194 <= pc <= 0x17264]}
    output = workspace / 'validation/local-20261025/action42-rom-report.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {count} action42 cases; raw RAM, FG, D0-D7, descriptor')
