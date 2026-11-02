#!/usr/bin/env python3
"""Fail closed on incomplete/late/lost work and detached ELF symbol metadata."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import tempfile


def main():
    spec = importlib.util.spec_from_file_location('combat_analysis',
        Path(__file__).with_name('analyze_combat_pair.py'))
    analyzer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(analyzer)
    zero_names = ('fm_discarded_events', 'fm_discarded_batches', 'fm_rejected_events',
                  'ring_rejected_events', 'sfx_underruns_32bit', 'sfx_fault',
                  'transport_late_16bit', 'invalid_fm_batch_16bit', 'music_shared_error')
    healthy = {
        'diagnostics': dict(enabled=1, boot_success=1, api_state=2, worker_state=2,
            api_error=0, worker_error=0, first_audio_fault_vblank=0,
            produced=2, accepted=2, queue_completed=2, queue_used=0,
            queue_overflows=0, unsupported=0, skipped=0),
        'producer_events': [{'frame': 0, 'raw_word': 0x3104}, {'frame': 10, 'raw_word': 0x3136}],
        'native_audio_counters': dict.fromkeys(zero_names, 0),
        'loaded_master_executable_sections': [{'loaded_bytes_match': True}],
        'loaded_module_segments': [{'loaded_bytes_match': True}, {'loaded_bytes_match': True}],
        'loaded_firmware_segments': [{'loaded_bytes_match': True}] * 3,
    }
    healthy['native_audio_counters'].update(sfx_bound=1, mail_state=2, music_shared_state=2)
    assert all(analyzer.comparable_audio_work(healthy).values())
    cases = 1
    for key in zero_names:
        for mutation in ('nonzero', 'missing'):
            case = copy.deepcopy(healthy)
            if mutation == 'nonzero':
                case['native_audio_counters'][key] = 1
            else:
                del case['native_audio_counters'][key]
            result = analyzer.comparable_audio_work(case)
            assert result['audio_remained_operational']
            assert not result['zero_loss_and_deadlines'], (key, mutation, result)
            assert not all(result.values())
            cases += 1
    for key, value in [('accepted', 1), ('queue_completed', 1), ('queue_used', 1),
                       ('queue_overflows', 1), ('unsupported', 1), ('skipped', 1),
                       ('produced', 3)]:
        case = copy.deepcopy(healthy)
        case['diagnostics'][key] = value
        result = analyzer.comparable_audio_work(case)
        assert not result['requested_work_integrity'], (key, result)
        cases += 1
    for key, value in [('worker_state', 3), ('api_state', 5), ('worker_error', 15),
                       ('api_error', 5), ('first_audio_fault_vblank', 123), ('enabled', 0)]:
        case = copy.deepcopy(healthy)
        case['diagnostics'][key] = value
        assert not analyzer.comparable_audio_work(case)['audio_remained_operational']
        cases += 1
    for key in ('loaded_master_executable_sections', 'loaded_module_segments',
                'loaded_firmware_segments'):
        for mutation in ('bad_byte', 'empty', 'missing'):
            case = copy.deepcopy(healthy)
            if mutation == 'bad_byte':
                case[key][0]['loaded_bytes_match'] = False
            elif mutation == 'empty':
                case[key] = []
            else:
                del case[key]
            result = analyzer.comparable_audio_work(case)
            assert not result['immutable_code_verified'], (key, mutation, result)
            cases += 1

    # This minimal linked ELF contains a local symbol, as real LTO game state
    # does. The sidecar is deliberately independent of the actual symbol bytes.
    def fixture(values):
        name = b'\0_observed_counter\0'
        text = b'\0' * (4 * len(values))
        text_at = 84
        names_at = text_at + len(text)
        symbols_at = (names_at + len(name) + 3) & ~3
        symbols = bytes(16) + b''.join(struct.pack('>IIIBBH', 1, v, 4, 1, 0, 1) for v in values)
        sections_at = symbols_at + len(symbols)
        ident = b'\x7fELF\x01\x02\x01' + bytes(9)
        data = bytearray(struct.pack('>16sHHIIIIIHHHHHH', ident, 2, 42, 1,
            0x06004000, 52, sections_at, 0, 52, 32, 1, 40, 4, 0))
        data += struct.pack('>8I', 1, text_at, 0x06004000, 0x06004000,
                            len(text), len(text), 5, 4)
        data += text + name
        data += bytes(symbols_at - len(data)) + symbols
        sections = [(0,) * 10,
                    (0, 1, 6, 0x06004000, text_at, len(text), 0, 0, 4, 0),
                    (0, 3, 0, 0, names_at, len(name), 0, 0, 1, 0),
                    (0, 2, 0, 0, symbols_at, len(symbols), 2, 2, 4, 16)]
        data += b''.join(struct.pack('>10I', *row) for row in sections)
        return data

    with tempfile.TemporaryDirectory(prefix='combat-elf-') as directory:
        elf = Path(directory) / 'native.elf'
        elf.write_bytes(fixture([0x06004000]))
        symbols, proof = analyzer.verify_symbols(elf, 42,
            {'_observed_counter': 0x06004000}, ('_observed_counter',))
        assert symbols['_observed_counter'] == 0x06004000 and proof['verified']
        assert analyzer.immutable_master_sections(elf) == [(0x06004000, bytes(4))]
        cases += 1
        failures = [({'_observed_counter': 0x06004004}, ('_observed_counter',), 42),
                    ({}, ('_observed_counter',), 42),
                    ({'_observed_counter': 0x06004000}, ('_missing',), 42),
                    ({'_observed_counter': 0x06004000}, ('_observed_counter',), 4)]
        for sidecar, names, machine in failures:
            try:
                analyzer.verify_symbols(elf, machine, sidecar, names)
            except ValueError:
                cases += 1
            else:
                raise AssertionError('Detached/missing/wrong-machine symbol accepted')
        elf.write_bytes(fixture([0x06004000, 0x06004004]))
        try:
            analyzer.verify_symbols(elf, 42, {'_observed_counter': 0x06004000}, ('_observed_counter',))
        except ValueError:
            cases += 1
        else:
            raise AssertionError('Ambiguous ELF symbol accepted')
    print(json.dumps({'passed': True, 'contract_cases': cases,
                      'scope': 'Analysis gate and actual ELF symbol binding; no emulator or audio runtime changes'}))


if __name__ == '__main__':
    main()
