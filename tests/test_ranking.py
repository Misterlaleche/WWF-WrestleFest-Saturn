#!/usr/bin/env python3
"""Native records/initials compared with the intact original 68000 ROM.

Only physical pad levels and IRQ acknowledgement are supplied. Native
initial glyphs and records graphics are included; other external scene/fade
owners execute ROM instructions in both runs. Expected results never come
from a second implementation of the native ranking rules.
Requires machine68k==0.5.1; no emulator is linked into the game.
"""
import argparse
import ctypes as ct
import hashlib
import json
import random
import subprocess
import tempfile
from pathlib import Path

from test_credits import ROOT, REGIONS, Oracle, Shadow, Machine, Context, Rom, put

Store = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_uint32, ct.c_uint32, ct.c_uint)


class GlyphContext(ct.Structure):
    _fields_ = [('machine', ct.POINTER(Machine)), ('store', Store), ('user', ct.c_void_p),
                ('blocked_dependency', ct.c_uint32), ('blocked_address', ct.c_uint32)]


class RecordsContext(ct.Structure):
    _fields_ = [('machine', ct.POINTER(Machine)), ('read_rom', Rom), ('user', ct.c_void_p),
                ('store', Store), ('blocked_dependency', ct.c_uint32), ('blocked_address', ct.c_uint32)]


class Request(ct.Structure):
    _fields_ = [(name, ct.c_uint32) for name in
                ('kind', 'owner', 'call_pc', 'return_pc', 'address', 'value', 'width')]


class State(ct.Structure):
    _fields_ = [('pc', ct.c_uint32), ('entry_pc', ct.c_uint32),
                ('blocked_dependency', ct.c_uint32), ('return_stack', ct.c_uint32 * 8),
                ('saved_d', ct.c_uint32 * 5), ('saved_a2', ct.c_uint32),
                ('saved_d0', ct.c_uint32), ('io_value', ct.c_uint32),
                ('request', Request), ('depth', ct.c_uint8), ('pending', ct.c_uint8),
                ('fatal', ct.c_uint8), ('carry', ct.c_uint8)]


class Inputs:
    def __init__(self, policy='confirm'):
        self.policy, self.ticks = policy, 0

    def word(self, port):
        if self.policy == 'timeout':
            return 0xffff
        if self.policy == 'held':
            return 0xffef
        # Move both directions before confirming. Every participant sees an
        # independently phased button; holding it cannot confirm twice.
        if self.ticks < 20:
            return 0xfffe if port & 1 else 0xfffd
        pressed = ((self.ticks + port) // 2) & 1
        return 0xffff & ~(0x10 << (port & 1)) if pressed else 0xffff


class RankingOracle(Oracle):
    def execute_ranking(self, mem, registers, owner, inputs):
        self.load(mem, registers, owner)
        instructions = 0
        while self.cpu.r_pc() != 0x70000:
            pc = self.cpu.r_pc()
            instructions += 1
            assert instructions < 15_000_000, ('ranking ROM runaway', hex(owner), hex(pc))
            if pc == 0x6cf2:
                at = self.cpu.r_reg(10) + (self.cpu.r_reg(7) & 0xffff)
                self.mem.w16(at, inputs.word((at - 0x140020) // 2))
            if pc in (0x673e, 0x6776) and not self.mem.r8(0x1c006f) & 2:
                inputs.ticks += 1
            self.poll(pc)
            if pc == 0x2052:
                self.cpu.w_pc(self.mem.r32(self.cpu.r_sp()))
                self.cpu.w_sp(self.cpu.r_sp() + 4)
            else:
                self.cpu.execute(1)
        for address, size in REGIONS:
            mem[address:address + size] = self.mem.r_block(address, size)
        return self.registers()


def seed(rom, royal=0):
    rng = random.Random(0x65c0 + royal)
    memory = bytearray(0x200000)
    memory[:len(rom)] = rom
    memory[0x1c00c2:0x1c0146] = rom[0x6db0:0x6e34]
    put(memory, 0x1c007c, 1)
    put(memory, 0x1c0076, 0x8000)
    put(memory, 0x1c0161, royal, 1)
    put(memory, 0x1c0164, 9)
    for slot in range(4):
        actor = 0x1c05b0 + slot * 0x10c
        put(memory, actor, 0x8000)
        put(memory, actor + 0x57, slot, 1)
        put(memory, actor + 0xc4, 30 + slot)
        put(memory, 0x140020 + slot * 2, 0xffff)
        put(memory, 0x1c0092 + slot * 4, actor, 4)
    registers = [rng.getrandbits(32) for _ in range(15)]
    return memory, registers


def fixtures(rom):
    for royal in (0, 1):
        for score in (0, 1, 9, 30, 99, 100):
            mem, regs = seed(rom, royal)
            put(mem, 0x1c0164, score)
            put(mem, 0x1c05b0 + 0xc4, score)
            regs[0], regs[8] = 0, 0x1c05b0
            yield f'qualify_{royal}_{score}', 0x67b2, mem, regs, 'timeout'
        for count in (0, 1, 2, 4):
            mem, regs = seed(rom, royal)
            for slot in range(count, 4):
                put(mem, 0x1c0092 + slot * 4, 0, 4)
            yield f'full_{royal}_{count}', 0x65c0, mem, regs, 'confirm'
        for policy in ('timeout', 'held'):
            mem, regs = seed(rom, royal)
            for slot in range(1, 4):
                put(mem, 0x1c0092 + slot * 4, 0, 4)
            yield f'full_{royal}_{policy}', 0x65c0, mem, regs, policy
        rng = random.Random(6830 + royal)
        for case in range(12):
            mem, regs = seed(rom, royal)
            start = 0x1c0104 if royal else 0x1c00c2
            for slot in range(11):
                put(mem, start + slot * 6, rng.randrange(4), 1)
                put(mem, start + slot * 6 + 2, 0x5c if case & 1 else 0x42, 1)
            yield f'sort_ties_{royal}_{case}', 0x683e, mem, regs, 'timeout'
        for port in range(4):
            for char in (0x31, 0x4b):
                for direction in (1, 2):
                    mem, regs = seed(rom, royal)
                    regs[0], regs[1] = port, port
                    put(mem, 0x1c00ba + port * 2, port)
                    put(mem, 0x1c00a2 + port * 2, char)
                    put(mem, 0x1c00aa + port * 2, 7)
                    # Direct helper has no IRQ; fixed physical direction.
                    put(mem, 0x140020 + port * 2, 0xffff ^ direction)
                    yield f'initial_wrap_{royal}_{port}_{char}_{direction}', 0x6b6e, mem, regs, f'direction{direction}'


def native_run(lib, rom, original, registers, owner, inputs, oracle):
    memory = (ct.c_uint8 * len(original)).from_buffer_copy(original)
    shadow = Shadow((ct.c_uint32 * 8)(*registers[:8]), 1)
    machine = Machine(ct.cast(ct.addressof(memory) + 0x1c0000, ct.POINTER(ct.c_uint8)),
                      ct.pointer(shadow), (ct.c_uint32 * 7)(*registers[8:]), None, None, 0)
    @Rom
    def read_rom(_, address, destination, size):
        if address > len(rom) or size > len(rom) - address:
            return 1
        ct.memmove(destination, rom[address:address + size], size)
        return 0
    context, state = Context(ct.pointer(machine), read_rom, None), State()
    @Store
    def store(_, address, value, width):
        if width not in (1, 2, 4) or address + width > len(memory):
            return 1
        ct.memmove(ct.addressof(memory) + address, value.to_bytes(4, 'big')[-width:], width)
        return 0
    glyph = GlyphContext(ct.pointer(machine), store, None, 0, 0)
    records = RecordsContext(ct.pointer(machine), read_rom, None, store, 0, 0)
    lib.wf_r18_ranking_init(ct.byref(state), owner)
    pending_checked = set()
    for _ in range(2_000_000):
        status = lib.wf_r18_ranking_step(ct.byref(state), ct.byref(context))
        assert status >= 0, ('native ranking blocked', hex(state.pc), hex(state.blocked_dependency))
        if status == 0:
            continue
        if status == 2:
            return bytearray(memory), tuple(shadow.d) + tuple(machine.a)
        assert status == 1
        q = state.request
        if q.call_pc not in pending_checked:
            before = bytes(state), ct.string_at(ct.addressof(memory) + 0x1c0000, 0x4000)
            assert lib.wf_r18_ranking_step(ct.byref(state), ct.byref(context)) == 1
            assert before == (bytes(state), ct.string_at(ct.addressof(memory) + 0x1c0000, 0x4000)), 'pending step changed state'
            pending_checked.add(q.call_pc)
        value = 0
        if q.kind == 0:
            if q.owner in (0x6cfa, 0x6d76):
                assert lib.wf_r18_ranking_glyph_call(ct.byref(glyph), q.owner) == 0
            elif q.owner in (0xa4d4, 0x7874):
                assert lib.wf_r18_records_aux_call(ct.byref(records), q.owner) == 0
            else:
                current = bytearray(memory)
                regs, _ = oracle.execute(current, tuple(shadow.d) + tuple(machine.a), q.owner)
                for address, size in REGIONS:
                    ct.memmove(ct.addressof(memory) + address, bytes(current[address:address + size]), size)
                shadow.d[:], machine.a[:] = regs[:8], regs[8:]
        elif q.kind == 1:
            if q.call_pc == 0x6cf2:
                word = inputs.word((q.address - 0x140020) // 2)
                memory[q.address], memory[q.address + 1] = word >> 8, word & 255
            value = int.from_bytes(bytes(memory[q.address:q.address + q.width]), 'big')
        elif q.kind == 2:
            raw = int(q.value).to_bytes(4, 'big')[-q.width:]
            ct.memmove(ct.addressof(memory) + q.address, raw, q.width)
        elif q.kind == 3:
            inputs.ticks += 1
            memory[0x1c006f] |= 2
        else:
            raise AssertionError(('unknown request', q.kind))
        assert lib.wf_r18_ranking_complete(ct.byref(state), ct.byref(context), 0, value) >= 0
    raise AssertionError(('native runaway', hex(state.pc)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', help='Run cases containing this substring')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    rom = (ROOT / 'cd/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
    reports = []
    with tempfile.TemporaryDirectory(prefix='wf-rank-') as directory:
        path = Path(directory) / 'ranking.so'
        subprocess.run(['cc', '-std=c11', '-O2', '-shared', '-fPIC', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'core/include'), str(ROOT / 'core/src/wf_r18_ranking.c'),
                        str(ROOT / 'core/src/wf_r18_ranking_glyph.c'),
                        str(ROOT / 'core/src/wf_r18_records_aux.c'),
                        str(ROOT / 'core/src/wf_r17_m68k_shadow.c'), '-o', str(path)], check=True)
        lib = ct.CDLL(str(path))
        lib.wf_r18_ranking_init.argtypes = [ct.POINTER(State), ct.c_uint32]
        lib.wf_r18_ranking_step.argtypes = [ct.POINTER(State), ct.POINTER(Context)]
        lib.wf_r18_ranking_complete.argtypes = [ct.POINTER(State), ct.POINTER(Context), ct.c_int, ct.c_uint32]
        lib.wf_r18_ranking_glyph_call.argtypes = [ct.POINTER(GlyphContext), ct.c_uint32]
        lib.wf_r18_records_aux_call.argtypes = [ct.POINTER(RecordsContext), ct.c_uint32]
        # machine68k owns one process-global CPU/memory; reuse the instance
        # for dependency calls after the expected run has completed.
        oracle = RankingOracle(rom)
        for name, owner, mem, regs, policy in fixtures(rom):
            if args.case and args.case not in name:
                continue
            print('RUN ranking ROM:', name, flush=True)
            expected = bytearray(mem)
            expected_inputs, actual_inputs = Inputs(policy), Inputs(policy)
            if policy.startswith('direction'):
                direction = int(policy[-1])
                expected_inputs.word = actual_inputs.word = lambda port, d=direction: 0xffff ^ d
            expected_regs = oracle.execute_ranking(expected, regs, owner, expected_inputs)
            actual, actual_regs = native_run(lib, rom, mem, regs, owner, actual_inputs, oracle)
            for reg, (a, b) in enumerate(zip(actual_regs, expected_regs)):
                assert a == b, (name, f'register {reg}', hex(a), hex(b))
            for address, size in REGIONS:
                for offset in range(size):
                    at = address + offset
                    assert actual[at] == expected[at], (name, hex(at), actual[at], expected[at])
            assert actual_inputs.ticks == expected_inputs.ticks, (name, 'IRQ count', actual_inputs.ticks, expected_inputs.ticks)
            reports.append({'case': name, 'owner': hex(owner), 'ticks': actual_inputs.ticks,
                            'rankings_sha256': hashlib.sha256(actual[0x1c00c2:0x1c0146]).hexdigest()})
            print('PASS ranking ROM:', name, 'ticks', actual_inputs.ticks, flush=True)
        oracle.machine.cleanup()
    assert reports
    if args.report:
        args.report.write_text(json.dumps({'status': 'PASS', 'cases': reports}, indent=2) + '\n')
    print('PASS ranking ROM differential:', len(reports), 'cases')


if __name__ == '__main__':
    main()
