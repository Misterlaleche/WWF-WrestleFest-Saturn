#!/usr/bin/env python3
"""Original Royal loss $1AFC..$1B48, including every external owner seam."""
import ctypes as ct
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path
from test_credits import ROOT, REGIONS, Oracle, Tape, Rom, Shadow, Machine, Context, State, seed, digest_mem


def main():
    rom = (ROOT / 'cd/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
    @Rom
    def read_rom(_, address, destination, size):
        if address > len(rom) or size > len(rom) - address:
            return 1
        ct.memmove(destination, rom[address:address + size], size)
        return 0
    with tempfile.TemporaryDirectory(prefix='wf-royal-loss-') as directory:
        library = Path(directory) / 'loss.so'
        subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'core/include'), str(ROOT / 'core/src/wf_r18_post_tag.c'),
                        str(ROOT / 'core/src/wf_r17_m68k_shadow.c'), '-o', str(library)], check=True)
        native = ct.CDLL(str(library))
        native.wf_r18_post_tag_init.argtypes = [ct.POINTER(State), ct.c_uint32]
        native.wf_r18_post_tag_step.argtypes = [ct.POINTER(State), ct.POINTER(Context)]
        native.wf_r18_post_tag_complete.argtypes = [ct.POINTER(State), ct.POINTER(Context), ct.c_int, ct.c_uint16]
        oracle = Oracle(rom)
        reports = []
        for initial_high in (0, 1):
            base, registers = seed(rom)
            base[0x140026] = base[0x140026] | 4 if initial_high else base[0x140026] & ~4
            wanted = bytearray(base)
            expected_regs, expected_events = oracle.execute(wanted, registers, 0x1AFC, True, Tape('never'))
            buf = (ct.c_uint8 * len(base)).from_buffer_copy(base)
            shadow = Shadow((ct.c_uint32 * 8)(*registers[:8]), 1)
            machine = Machine(ct.cast(ct.addressof(buf) + 0x1C0000, ct.POINTER(ct.c_uint8)),
                              ct.pointer(shadow), (ct.c_uint32 * 7)(*registers[8:]), None, None, 0)
            context, state = Context(ct.pointer(machine), read_rom, None), State()
            native.wf_r18_post_tag_init(ct.byref(state), 0x1AFC)
            calls = []
            def read(address, size):
                return ct.string_at(ct.addressof(buf) + address, size)
            while True:
                status = native.wf_r18_post_tag_step(ct.byref(state), ct.byref(context))
                assert status >= 0, ('Royal loss blocked', hex(state.pc), hex(state.blocked_dependency))
                if status == 0:
                    continue
                if status == 4:
                    assert state.pc == 0x6FC
                    break
                assert status == 1
                q = state.request
                registers_now = tuple(shadow.d) + tuple(machine.a)
                event = tuple(getattr(q, name) for name in ('kind', 'owner', 'call_pc', 'return_pc', 'address', 'value')) + \
                        (registers_now, digest_mem(read))
                assert event == expected_events[len(calls)], ('call mismatch', event, expected_events[len(calls)])
                assert q.kind == 0
                if q.owner == 0x21E6:
                    assert shadow.d[0] & 0xFFFF == 0x60
                calls.append(q.owner)
                current = bytearray(buf)
                returned, _ = oracle.execute(current, registers_now, q.owner)
                for address, size in REGIONS:
                    ct.memmove(ct.addressof(buf) + address, bytes(current[address:address + size]), size)
                shadow.d[:], machine.a[:] = returned[:8], returned[8:]
                assert native.wf_r18_post_tag_complete(ct.byref(state), ct.byref(context), 0, 0) == 0
            assert calls == [0x2A06, 0x2503C, 0x21E6, 0x65C0]
            assert len(calls) == len(expected_events)
            assert tuple(shadow.d) + tuple(machine.a) == expected_regs
            for address, size in REGIONS:
                assert read(address, size) == wanted[address:address + size], ('memory mismatch', hex(address))
            reports.append({'initial_vblank_high': initial_high, 'calls': calls,
                            'raw_sha256': hashlib.sha256(read(0x1C0000, 0x4000)).hexdigest()})
            print('PASS Royal loss ROM differential: initial VBlank', 'HIGH' if initial_high else 'LOW')
        assert oracle.mem.r_block(0, len(rom)) == rom
        oracle.machine.cleanup()
        (ROOT / 'evidence/royal-loss-rom-oracle.json').write_text(json.dumps({
            'status': 'PASS', 'rom_sha1': hashlib.sha1(rom).hexdigest(), 'cases': reports,
            'compared': ['all D/A registers', 'raw RAM, video/palette/MMIO', 'ordered owner calls'],
            'timing_scope': 'The native frontend fixture separately tests conditional HIGH entry and the following 97 periods.'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
