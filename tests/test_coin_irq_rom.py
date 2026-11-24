#!/usr/bin/env python3
"""Compare the complete native coin IRQ with unchanged 68000 instructions.

machine68k is an offline oracle and is never linked into the Saturn image.
The only supplied hardware behaviour is the input word at each original
BTST. The oracle executes the real MOVEM/BSR/RTS/RTE and exception frame.
Native READ16 events are compared with the corresponding high-byte BTST,
not with speculative/dummy bus cycles internal to the 68000 emulator.
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
RAM_BASE, RAM_BYTES = 0x1C0000, 0x4000
RETURN_PC = 0x70000
READ_PCS = {0x4BC, 0x48C}
WRITE_PCS = {0x438: (0x140016, 0), 0x480: (0x140002, 0),
             0x500: (0x14000C, 0x312A)}
# Native contexts expose quiescent SSP, omitting temporary BSR return words.
CALL_DEPTH = {0x438: 0, 0x480: 0, 0x4BC: 4, 0x48C: 4, 0x500: 8}


class Shadow(ct.Structure):
    _fields_ = [('d', ct.c_uint32 * 8), ('valid', ct.c_uint8)]


class Event(ct.Structure):
    _fields_ = [(name, ct.c_uint32) for name in
               ('kind', 'owner_rom', 'address', 'value')]


class Context(ct.Structure):
    pass


Effect = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.POINTER(Event), ct.POINTER(Context))
Context._fields_ = [('ram', ct.POINTER(ct.c_uint8)), ('ram_bytes', ct.c_size_t),
                   ('regs', ct.POINTER(Shadow)), ('a', ct.POINTER(ct.c_uint32)),
                   ('ssp', ct.c_uint32), ('usp', ct.c_uint32), ('sr', ct.c_uint16),
                   ('effect', Effect), ('user', ct.c_void_p)]


class State(ct.Structure):
    _fields_ = [('pc', ct.c_uint32), ('blocked_dependency', ct.c_uint32),
                ('saved_d', ct.c_uint32 * 3), ('return_pc', ct.c_uint32),
                ('return_sr', ct.c_uint16), ('frame_valid', ct.c_uint8),
                ('registers_saved', ct.c_uint8)]


def put(raw, address, value, width=2):
    raw[address:address + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'big')


def event_state(kind, pc, address, value, registers, sr, ssp, usp):
    return (kind, pc, address, value, tuple(registers), sr, ssp, usp)


class Oracle:
    def __init__(self, rom):
        self.rom = rom
        self.machine = mus.Machine(mus.CPUType.M68000, 2048)
        self.cpu, self.mem = self.machine.cpu, self.machine.mem
        self.mem.w_block(0, rom)
        self.coverage = set()

    def execute(self, raw, registers, interrupted_sr, ssp, usp, words, high):
        cpu, mem = self.cpu, self.mem
        mem.w_block(RAM_BASE, bytes(raw))
        mem.w_block(0x140000, bytes(0x40))
        mem.w16(0x140026, 0xFFFF if high else 0xFBFF)
        cpu.w_sr(0x2700)
        for i, value in enumerate(registers):
            cpu.w_reg(i, value)
        cpu.w_usp(usp)
        cpu.w_sp(ssp)
        cpu.w_sr(0x2200 | (interrupted_sr & 31))
        cpu.w_pc(0x434)
        events, reads, instructions = [], 0, 0
        while cpu.r_pc() != RETURN_PC:
            pc = cpu.r_pc()
            assert 0x434 <= pc <= 0x558, ('ROM left coin owner', hex(pc))
            instructions += 1
            assert instructions <= 300, ('ROM did not RTE', hex(pc))
            self.coverage.add(pc)
            if pc in READ_PCS:
                assert reads < 3
                word = words[reads]
                reads += 1
                mem.w16(0x140020, word)
                kind, address, value = 2, 0x140020, word
            elif pc in WRITE_PCS:
                kind, (address, value) = 1, WRITE_PCS[pc]
            else:
                cpu.execute(1)
                continue
            events.append(event_state(kind, pc, address, value,
                                      (cpu.r_reg(i) for i in range(15)), cpu.r_sr(),
                                      cpu.r_isp() + CALL_DEPTH[pc], cpu.r_usp()))
            cpu.execute(1)
        assert reads == 3
        return {'ram': bytes(mem.r_block(RAM_BASE, RAM_BYTES)),
                'registers': tuple(cpu.r_reg(i) for i in range(15)),
                'sr': cpu.r_sr(), 'ssp': cpu.r_isp(), 'usp': cpu.r_usp(),
                'pc': cpu.r_pc(), 'events': events, 'instructions': instructions,
                'writes': tuple(mem.r16(a) for a in (0x140016, 0x140002, 0x14000C))}


class Native:
    def __init__(self, library):
        self.library = ct.CDLL(str(library))
        self.library.wf_r18_frontend_coin_enter.argtypes = [ct.POINTER(State), ct.c_uint16, ct.c_uint32]
        for name in ('step', 'run'):
            if hasattr(self.library, 'wf_r18_frontend_coin_' + name):
                getattr(self.library, 'wf_r18_frontend_coin_' + name).argtypes = [ct.POINTER(State), ct.POINTER(Context)]

    def execute(self, name, raw, registers, interrupted_sr, ssp, usp, words):
        buf = (ct.c_uint8 * RAM_BYTES).from_buffer_copy(raw)
        shadow = Shadow((ct.c_uint32 * 8)(*registers[:8]), 1)
        addresses = (ct.c_uint32 * 7)(*registers[8:])
        events, reads, failures = [], [], []
        writes = {0x140016: 0, 0x140002: 0, 0x14000C: 0}

        @Effect
        def effect(_, event, context):
            e, c = event.contents, context.contents
            if e.kind == 2:
                if len(reads) >= 3:
                    failures.append('extra native input read')
                    return 1
                e.value = words[len(reads)]
                reads.append(e.owner_rom)
            elif e.kind == 1:
                writes[e.address] = e.value
            else:
                failures.append(('unexpected callback', e.kind, e.owner_rom))
                return 1
            events.append(event_state(e.kind, e.owner_rom, e.address, e.value,
                                      tuple(c.regs.contents.d) + tuple(c.a[i] for i in range(7)),
                                      c.sr, c.ssp, c.usp))
            return 0

        context = Context(buf, RAM_BYTES, ct.pointer(shadow), addresses, ssp, usp,
                          0x2200 | (interrupted_sr & 31), effect, None)
        state = State()
        self.library.wf_r18_frontend_coin_enter(ct.byref(state), interrupted_sr, RETURN_PC)
        function = getattr(self.library, 'wf_r18_frontend_coin_' + name)
        for _ in range(20):
            status = function(ct.byref(state), ct.byref(context))
            if status != 0:
                break
        assert status == 3, ('native did not RTE', name, status, hex(state.pc), failures)
        assert not state.frame_valid and not state.registers_saved and not failures
        assert len(reads) == 3
        return {'ram': bytes(buf), 'registers': tuple(shadow.d) + tuple(addresses),
                'sr': context.sr, 'ssp': context.ssp, 'usp': context.usp,
                'pc': state.pc, 'events': events,
                'writes': tuple(writes[a] for a in (0x140016, 0x140002, 0x14000C))}


def compare(case, actual, expected, frame_ssp):
    for key in ('pc', 'registers', 'sr', 'ssp', 'usp', 'events', 'writes'):
        assert actual[key] == expected[key], (case, key, actual[key], expected[key])
    # The native contract deliberately omits the deepest BSR return words and
    # MOVEM's D0-D2 spill. Every other byte, including the real exception frame
    # and the remainder of the stack region, must agree with the ROM.
    scratch = range(frame_ssp - RAM_BASE - 20, frame_ssp - RAM_BASE)
    differences = [i for i, (a, b) in enumerate(zip(actual['ram'], expected['ram']))
                   if a != b and i not in scratch]
    assert not differences, (case, 'RAM', [(hex(i), actual['ram'][i], expected['ram'][i])
                                         for i in differences[:12]])


def cases(randomizer):
    # Pulse durations below/at/above both debounce and rejection thresholds,
    # including byte/word overflow; both physical coin slots and service.
    for duration in (0, 1, 2, 0x35F, 0x360, 0x361, 0x7FFF, 0xFFFF):
        for release in (0, 1, 3, 4, 9, 10, 127, 255):
            for inputs in range(8):
                word = 0xF8FF | inputs << 8
                yield 'pulse', {0x54: (duration, 2), 0x78: (duration, 2),
                                0x10: (release, 1), 0x11: (release, 1)}, (word,) * 3
    for duration in (0, 1, 29, 30, 31, 127, 128, 254, 255):
        for credit in (0, 98, 99, 127, 255):
            for active in (False, True):
                word = 0xFBFF if active else 0xFFFF
                yield 'service', {0x56: (duration, 1), 0x4F: (credit, 1)}, (word,) * 3
    for price in (0, 1, 2, 3, 255):
        for award in (0, 1, 2, 9, 99, 255):
            for credit in (0, 1, 98, 99, 100, 255):
                for partial in sorted({0, (price - 1) & 255, price, 255}):
                    yield 'credit_ratio', {0x4B: (price, 1), 0x4D: (award, 1),
                                           0x4E: (partial, 1), 0x4F: (credit, 1),
                                           0x50: (0xFFFF, 2), 0x52: (0xFFFF, 2),
                                           0x54: (1, 2), 0x78: (1, 2),
                                           0x10: (4, 1), 0x11: (4, 1)}, (0xFFFF,) * 3
    for _ in range(1000):
        edits = {a: (randomizer.randrange(256), 1) for a in
                 (0x10, 0x11, 0x4B, 0x4D, 0x4E, 0x4F, 0x56)}
        edits.update({a: (randomizer.randrange(65536), 2) for a in (0x50, 0x52, 0x54, 0x78)})
        yield 'independent_input_reads', edits, tuple(randomizer.randrange(65536) for _ in range(3))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / 'cd/ROM68K.BIN')
    parser.add_argument('--report', type=Path, default=ROOT / 'evidence/coin-irq-rom-oracle.json')
    parser.add_argument('--entry', choices=('step', 'run', 'both'), default='both')
    parser.add_argument('--raw-state', type=Path, action='append', default=[],
                        help='Optional native raw16KiB file or target-endian WorkRAML1MiB capture.')
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
    randomizer = random.Random(0x434)
    initial = bytearray(randomizer.randbytes(RAM_BYTES))
    for a, value, width in ((0x4B, 1, 1), (0x4D, 1, 1), (0x4E, 0, 1),
                            (0x4F, 0, 1), (0x50, 0, 2), (0x52, 0, 2),
                            (0x54, 0, 2), (0x78, 0, 2), (0x10, 10, 1),
                            (0x11, 10, 1), (0x56, 0, 1)):
        put(initial, a, value, width)
    seeds = [('explicit_boundaries', bytes(initial))]
    snapshots = []
    for path in args.raw_state:
        data = path.read_bytes()
        assert len(data) in (RAM_BYTES, 0x100000), ('raw snapshot size', path, len(data))
        raw = data[-RAM_BYTES:] if len(data) == 0x100000 else data
        seeds.append((str(path), raw))
        snapshots.append({'path': str(path), 'raw_sha256': hashlib.sha256(raw).hexdigest()})
    entries = ('step', 'run') if args.entry == 'both' else (args.entry,)
    counts, instruction_range, comparisons = {}, [1000, 0], 0
    event_count, sound_count = 0, 0
    with tempfile.TemporaryDirectory(prefix='wf-coin-rom-') as directory:
        library = Path(directory) / 'coin.so'
        subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'core/include'), str(ROOT / 'core/src/wf_r18_frontend_coin.c'),
                        str(ROOT / 'core/src/wf_r17_m68k_shadow.c'), '-o', str(library)], check=True)
        native, oracle = Native(library), Oracle(rom)
        for seed_name, seed in seeds:
            for ordinal, (kind, edits, words) in enumerate(cases(randomizer)):
                raw = bytearray(seed)
                for address, (value, width) in edits.items():
                    put(raw, address, value, width)
                registers = tuple(randomizer.getrandbits(32) for _ in range(15))
                interrupted_sr = (0, 0x1F, 0x2000, 0x201F, 0x8115, 0xA10A)[ordinal % 6]
                ssp = RAM_BASE + (0x3BD4 if ordinal & 1 else 0x3DD4) - 6
                usp = RAM_BASE + 0x3FD4
                put(raw, ssp - RAM_BASE, interrupted_sr)
                put(raw, ssp - RAM_BASE + 2, RETURN_PC, 4)
                expected = oracle.execute(raw, registers, interrupted_sr, ssp, usp, words, ordinal & 1)
                for entry in entries:
                    actual = native.execute(entry, raw, registers, interrupted_sr, ssp, usp, words)
                    compare((seed_name, kind, ordinal, entry, words), actual, expected, ssp)
                    comparisons += 1
                counts[kind] = counts.get(kind, 0) + 1
                instruction_range[0] = min(instruction_range[0], expected['instructions'])
                instruction_range[1] = max(instruction_range[1], expected['instructions'])
                event_count += len(expected['events'])
                sound_count += sum(e[2] == 0x14000C for e in expected['events'])
            print('PASS coin IRQ ROM differential:', seed_name, entries, flush=True)
        assert oracle.mem.r_block(0, len(rom)) == rom
        coverage = sorted(oracle.coverage)
        oracle.machine.cleanup()
    report = {'status': 'PASS', 'rom_sha1': hashlib.sha1(rom).hexdigest(),
              'entries': entries, 'cases_by_kind': counts, 'native_rom_comparisons': comparisons,
              'ordered_mmio_events': event_count, 'original_312a_writes': sound_count,
              'rom_instructions_per_irq': instruction_range, 'rom_instruction_addresses': coverage,
              'native_snapshots': snapshots,
              'compared': ['all D0-D7/A0-A6 at every MMIO and RTE', 'SR at every MMIO and RTE',
                           'quiescent SSP/USP at every MMIO and RTE', 'all RAM except 20 temporary stack bytes',
                           'ordered MMIO owner/address/value', 'real exception frame and interrupted PC'],
              'scope': 'Owner $434 only; physical scheduling/debounce periods are tested separately.'}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS:', comparisons, 'native/ROM comparisons;', event_count, 'ordered MMIO events;',
          sound_count, 'original coin sound writes')


if __name__ == '__main__':
    main()
