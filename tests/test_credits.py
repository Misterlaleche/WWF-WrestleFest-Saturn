#!/usr/bin/env python3
"""Compare native Credits $1B4E with the intact 68000 ROM and explicit I/O.

Requires machine68k==0.5.1. The emulator is an offline oracle only; it is
never linked into the Saturn game. Original dependencies execute from ROM.
Only sound output and physical VBlank/input levels are supplied by this test.
"""
import argparse
import ctypes as ct
import hashlib
import json
import random
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

ROOT = Path(__file__).resolve().parents[1]
REGIONS = ((0x1C0000, 0x4000), (0x80000, 0x10000), (0xC0000, 0x4000),
           (0x180000, 0x9000), (0x100000, 0x20), (0x140000, 0x40))
# These complete native owners already exist; compare the exact call seams
# while the differential oracle executes their original ROM instructions.
EXTERNAL = {0x264E2, 0x1F6C, 0x1FDE, 0x1F9E, 0x2988, 0x2052, 0x2A06,
            0x26E66, 0x2AEA, 0x2503C, 0x27B8, 0x2836, 0x2B58, 0x2208,
            0x247C, 0x65C0, 0x21E6}
LEVELS = {0x1BAC: (1, 0x1BB6), 0x1BB6: (0, 0x1BC0),
          0x1C12: (1, 0x1C1C), 0x1C22: (0, 0x1C2C),
          0x1C70: (1, 0x1C7A), 0x1C7A: (0, 0x1C84),
          0x1D54: (1, 0x1D5E), 0x1D64: (0, 0x1D6E)}
READS = {0x1C34: (0x140020, 0x1C3A), 0x1C40: (0x140022, 0x1C46),
         0x1C4C: (0x140024, 0x1C52), 0x1C58: (0x140026, 0x1C5E),
         0x1D1A: (0x140020, 0x1D20), 0x1D26: (0x140022, 0x1D2C),
         0x1D32: (0x140024, 0x1D38), 0x1D3E: (0x140026, 0x1D44)}
WRITES = {0x1BC0: (0x140010, 0x78, 0x1BC8),
          0x1C1C: (0x140008, 0, 0x1C22),
          0x1D5E: (0x140008, 0, 0x1D64)}


class Shadow(ct.Structure):
    _fields_ = [('d', ct.c_uint32 * 8), ('valid', ct.c_uint8)]


class Machine(ct.Structure):
    _fields_ = [('ram', ct.POINTER(ct.c_uint8)), ('regs', ct.POINTER(Shadow)),
                ('a', ct.c_uint32 * 7), ('effect', ct.c_void_p),
                ('user', ct.c_void_p), ('dependency', ct.c_uint32)]


Rom = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_uint32, ct.c_void_p, ct.c_size_t)


class Context(ct.Structure):
    _fields_ = [('machine', ct.POINTER(Machine)), ('read_rom', Rom),
                ('user', ct.c_void_p)]


class Request(ct.Structure):
    _fields_ = [(name, ct.c_uint32) for name in
                ('kind', 'owner', 'call_pc', 'return_pc', 'address', 'value')]


class State(ct.Structure):
    _fields_ = [(name, ct.c_uint32) for name in
                ('pc', 'entry_pc', 'saved_d0', 'sub_return', 'blocked_dependency')]
    _fields_ += [('saved_d', ct.c_uint32 * 8), ('saved_a', ct.c_uint32 * 4),
                 ('request', Request), ('pending', ct.c_uint8),
                 ('fatal', ct.c_uint8), ('carry', ct.c_uint8),
                 ('callback_a0', ct.c_uint32)]


def put(mem, address, value, width=2):
    mem[address:address + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'big')


def digest_mem(read):
    # Physical input can change at the read itself, outside CPU-owned state.
    return hashlib.sha256(b''.join(read(a, n) for a, n in REGIONS[:-1])).hexdigest()


class Oracle:
    def __init__(self, rom):
        self.rom = rom
        self.machine = mus.Machine(mus.CPUType.M68000, 2048)
        self.cpu, self.mem = self.machine.cpu, self.machine.mem
        self.mem.w_block(0, rom)
        self.coverage = set()

    def registers(self):
        return tuple(self.cpu.r_reg(i) for i in range(15))

    def load(self, mem, registers, owner):
        for address, size in REGIONS:
            self.mem.w_block(address, bytes(mem[address:address + size]))
        self.cpu.w_sr(0x2700)
        for i, value in enumerate(registers):
            self.cpu.w_reg(i, value)
        self.cpu.w_sp(0x1D0000)
        self.mem.w32(0x1D0000, 0x70000)
        self.cpu.w_pc(owner)

    def level(self, high):
        value = self.mem.r8(0x140026)
        self.mem.w8(0x140026, value | 4 if high else value & ~4)

    def poll(self, pc):
        if self.rom[pc:pc + 8] == bytes.fromhex('08390001001c006f'):
            self.mem.w8(0x1C006F, self.mem.r8(0x1C006F) | 2)
        if self.rom[pc:pc + 8] == bytes.fromhex('0839000200140026'):
            assert self.rom[pc + 8] in (0x66, 0x67)
            self.level(self.rom[pc + 8] == 0x67)
        if pc in (0x298C, 0x2A0A):
            self.level(1)
        elif pc in (0x29F6, 0x2A88):
            self.level(0)

    def execute(self, mem, registers, owner, trace=False, tape=None):
        self.load(mem, registers, owner)
        stop = 0x6FC if owner in (0x1AFC, 0x1B4E) else 0x70000
        events = []
        ignore_until = None
        instructions = 0
        while self.cpu.r_pc() != stop:
            pc = self.cpu.r_pc()
            instructions += 1
            assert instructions < 30_000_000, ('ROM runaway', hex(owner), hex(pc))
            self.coverage.add(pc)
            if pc == ignore_until:
                ignore_until = None
            if trace and ignore_until is None and \
                    (pc in EXTERNAL or pc in LEVELS or pc in READS or pc in WRITES):
                registers_at_call = self.registers()
                state_digest = digest_mem(self.mem.r_block)
                if pc in EXTERNAL:
                    ret = self.mem.r32(self.cpu.r_sp())
                    call = ret - (6 if self.rom[ret - 6:ret - 4] == b'\x4e\xb9' else 4)
                    events.append((0, pc, call, ret, 0, 0, registers_at_call, state_digest))
                    ignore_until = ret
                elif pc in LEVELS:
                    high, ret = LEVELS[pc]
                    events.append((5, pc, pc, ret, 0x140026, high, registers_at_call, state_digest))
                    self.level(high)
                elif pc in READS:
                    address, ret = READS[pc]
                    tape.apply(pc, self.mem.r16, self.mem.w16)
                    events.append((1, pc, pc, ret, address, 0, registers_at_call, state_digest))
                elif pc in WRITES:
                    address, value, ret = WRITES[pc]
                    events.append((2, pc, pc, ret, address, value, registers_at_call, state_digest))
            self.poll(pc)
            if pc == 0x2052:
                self.cpu.w_pc(self.mem.r32(self.cpu.r_sp()))
                self.cpu.w_sp(self.cpu.r_sp() + 4)
            else:
                self.cpu.execute(1)
        for address, size in REGIONS:
            mem[address:address + size] = self.mem.r_block(address, size)
        return self.registers(), events


class Tape:
    def __init__(self, phase, page=0, port=0, button=0x10):
        self.phase, self.page, self.port, self.button = phase, page, port, button
        self.pages = self.staff = 0

    def apply(self, pc, read, write):
        if pc not in (0x1C34, 0x1D1A):
            return
        if pc == 0x1C34:
            pressed = self.phase == 'page' and self.pages >= 129 * self.page
            self.pages += 1
        else:
            pressed = self.phase == 'staff'
            self.staff += 1
        if self.phase == 'start_only':
            pressed = True
        address = 0x140020 + self.port * 2
        value = read(address)
        write(address, value & ~self.button if pressed else value | self.button)


def seed(rom):
    randomizer = random.Random(0x1B4E)
    mem = bytearray(0x200000)
    mem[:len(rom)] = rom
    # Explicit valid allocator/reset state, with nonzero incoming register
    # high halves and preserved actor bytes to detect accidental clearing.
    mem[0x1C1CD4:0x1C4000] = randomizer.randbytes(0x4000 - 0x1CD4)
    put(mem, 0x1C007C, 1)
    put(mem, 0x1C0076, 0x8000)
    for i in range(4):
        p = 0x1C05B0 + i * 0x10C
        put(mem, p, 0x8000)
        put(mem, p + 0xA1, 1 if i in (0, 2) else 0, 1)
    for i in range(4):
        put(mem, 0x140020 + i * 2, 0xFFFF)
    return mem, [randomizer.getrandbits(32) for _ in range(15)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / 'cd/ROM68K.BIN')
    parser.add_argument('--report', type=Path, default=ROOT / 'evidence/credits-rom-oracle.json')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'

    @Rom
    def read_rom(_, address, destination, size):
        if address > len(rom) or size > len(rom) - address:
            return 1
        ct.memmove(destination, rom[address:address + size], size)
        return 0

    with tempfile.TemporaryDirectory(prefix='wf-credits-') as directory:
        library = Path(directory) / 'credits.so'
        subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall',
                        '-Wextra', '-Werror', '-I' + str(ROOT / 'core/include'),
                        str(ROOT / 'core/src/wf_r18_post_tag.c'),
                        str(ROOT / 'core/src/wf_r17_m68k_shadow.c'), '-o', str(library)], check=True)
        native = ct.CDLL(str(library))
        native.wf_r18_post_tag_init.argtypes = [ct.POINTER(State), ct.c_uint32]
        native.wf_r18_post_tag_step.argtypes = [ct.POINTER(State), ct.POINTER(Context)]
        native.wf_r18_post_tag_complete.argtypes = [ct.POINTER(State), ct.POINTER(Context), ct.c_int, ct.c_uint16]
        base, registers = seed(rom)
        buf = (ct.c_uint8 * len(base)).from_buffer_copy(base)
        shadow = Shadow((ct.c_uint32 * 8)(*registers[:8]), 1)
        machine = Machine(ct.cast(ct.addressof(buf) + 0x1C0000, ct.POINTER(ct.c_uint8)),
                          ct.pointer(shadow), (ct.c_uint32 * 7)(*registers[8:]), None, None, 0)
        context = Context(ct.pointer(machine), read_rom, None)
        state = State()
        native.wf_r18_post_tag_init(ct.byref(state), 0x1B4E)
        for _ in range(8):
            status = native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context))
            if status != 0:
                break
        assert status == 1 and state.request.owner == 0x264E2, \
            ('Credits entry must execute original first fade', status, hex(state.pc))

        oracle = Oracle(rom)
        cases = [('page', 0, 0, 0x10)] if args.smoke else \
            [('page', 0, p, b) for p in range(4) for b in (0x10, 0x20)] + \
            [('page', 8, 2, 0x20), ('staff', 0, 3, 0x10),
             ('never', 0, 0, 0x10), ('start_only', 0, 0, 0x80)]
        reports = []
        for spec in cases:
            wanted = bytearray(base)
            expected_regs, expected_events = oracle.execute(wanted, registers, 0x1B4E, True, Tape(*spec))
            ct.memmove(buf, bytes(base), len(base))
            shadow.d[:] = registers[:8]
            machine.a[:] = registers[8:]
            native.wf_r18_post_tag_init(ct.byref(state), 0x1B4E)
            tape = Tape(*spec)
            seen = 0
            def read(address, size):
                return ct.string_at(ct.addressof(buf) + address, size)
            def read16(address):
                return int.from_bytes(read(address, 2), 'big')
            def write16(address, value):
                ct.memmove(ct.addressof(buf) + address, (value & 0xFFFF).to_bytes(2, 'big'), 2)
            while True:
                status = native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context))
                assert status >= 0, ('native blocked', spec, hex(state.pc), hex(state.blocked_dependency))
                if status == 0:
                    continue
                if status == 4:
                    assert state.pc == 0x6FC, ('wrong final boundary', hex(state.pc))
                    break
                assert status == 1, ('unexpected status', status, hex(state.pc))
                q = state.request
                regs = tuple(shadow.d) + tuple(machine.a)
                event = tuple(getattr(q, name) for name in
                              ('kind', 'owner', 'call_pc', 'return_pc', 'address', 'value')) + \
                    (regs, digest_mem(read))
                assert seen < len(expected_events) and event == expected_events[seen], \
                    ('request mismatch', spec, seen, event,
                     expected_events[seen] if seen < len(expected_events) else None)
                seen += 1
                before = bytes(state), digest_mem(read), regs
                assert native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context)) == 1
                assert before == (bytes(state), digest_mem(read), tuple(shadow.d) + tuple(machine.a))
                value = 0
                if q.kind == 0:
                    current = bytearray(buf)
                    returned, _ = oracle.execute(current, regs, q.owner)
                    for address, size in REGIONS:
                        ct.memmove(ct.addressof(buf) + address, bytes(current[address:address + size]), size)
                    shadow.d[:] = returned[:8]
                    machine.a[:] = returned[8:]
                elif q.kind == 1:
                    tape.apply(q.call_pc, read16, write16)
                    value = read16(q.address)
                elif q.kind == 2:
                    write16(q.address, q.value)
                elif q.kind == 5:
                    v = buf[0x140026]
                    buf[0x140026] = v | 4 if q.value else v & ~4
                else:
                    raise AssertionError(('unknown request', q.kind))
                assert native.wf_r18_post_tag_complete(ct.byref(state), ct.byref(context), 0, value) == 0
            assert seen == len(expected_events), (seen, len(expected_events))
            for address, size in REGIONS:
                actual = read(address, size)
                assert actual == wanted[address:address + size], \
                    ('terminal memory', spec, hex(address),
                     [(hex(address + i), x, y) for i, (x, y) in enumerate(zip(actual, wanted[address:address + size])) if x != y][:12])
            assert tuple(shadow.d) + tuple(machine.a) == expected_regs
            audio = [e[6][0] & 0xFFFF for e in expected_events if e[0] == 0 and e[1] == 0x2052]
            assert audio == [0x11, 0x1F], audio
            reports.append({'input': spec, 'requests': seen, 'page_input_groups': tape.pages,
                            'staff_updates': tape.staff, 'audio': audio,
                            'terminal_raw_sha256': hashlib.sha256(read(0x1C0000, 0x4000)).hexdigest()})
            print('PASS credits', spec, 'requests', seen, 'pages', tape.pages, 'staff', tape.staff, flush=True)
        negative_cases = []
        def reset_cursor(pc):
            ct.memmove(buf, bytes(base), len(base))
            shadow.d[:] = registers[:8]
            machine.a[:] = registers[8:]
            machine.dependency = 0
            native.wf_r18_post_tag_init(ct.byref(state), pc)

        reset_cursor(0x1BAC)
        buf[0x1C006F] = 0
        assert native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context)) == 1
        assert native.wf_r18_post_tag_complete(ct.byref(state), ct.byref(context), 0, 0) == 0
        assert state.pc == 0x1BB6 and not buf[0x1C006F]
        negative_cases.append('physical level completes with ACK006F clear')
        assert native.wf_r18_post_tag_complete(ct.byref(state), ct.byref(context), 0, 0) == -2
        negative_cases.append('duplicate completion rejected')

        reset_cursor(0x1B52)
        assert native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context)) == 1
        assert native.wf_r18_post_tag_complete(ct.byref(state), ct.byref(context), 1, 0) == -1
        assert state.fatal and state.blocked_dependency == 0x264E2
        before = bytes(state), bytes(buf)
        assert native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context)) == -1
        assert before == (bytes(state), bytes(buf))
        negative_cases.append('failed dependency fatal and subsequent step inert')

        @Rom
        def missing_rom(_, address, destination, size):
            return 1
        absent_context = Context(ct.pointer(machine), missing_rom, None)
        reset_cursor(0x1BE8)
        machine.a[0], machine.a[1] = 0x1C05B0, 0x1DE8
        before = bytes(buf)
        assert native.wf_r18_post_tag_step(ct.byref(state), ct.byref(absent_context)) == -1
        assert state.blocked_dependency == 0x1BE8 and bytes(buf) == before
        negative_cases.append('missing page ROM blocked before RAM write')

        reset_cursor(0x1BE8)
        machine.a[0], machine.a[1] = 0x1C3FF0, 0x1DE8
        before = bytes(buf)
        assert native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context)) == -1
        assert state.blocked_dependency == 0x1BE8 and bytes(buf) == before
        negative_cases.append('out-of-range actor blocked before ROM or RAM write')
        assert native.wf_r18_post_tag_step(None, ct.byref(context)) == -2
        assert native.wf_r18_post_tag_step(ct.byref(state), None) == -2
        negative_cases.append('invalid context rejected')
        assert oracle.mem.r_block(0, len(rom)) == rom
        oracle.machine.cleanup()
        report = {'status': 'PASS', 'rom_sha1': hashlib.sha1(rom).hexdigest(),
                  'cases': reports, 'negative_cases': negative_cases,
                  'coverage_pcs': len(oracle.coverage),
                  'compared': ['raw16KB', 'BG/FG/palettes/MMIO', 'D0-D7/A0-A6',
                               'ordered call/read/write/level seams', 'inert pending requests'],
                  'rom_modifications': [], 'intercepts': ['2052 silent sound device'],
                  'limits': ['Explicit physical input and VBlank levels; no physical Saturn timing claim.']}
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
