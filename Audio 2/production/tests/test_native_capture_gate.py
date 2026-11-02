"""Synthetic evidence for native capture gates; no ROM or emulator is required."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import wave

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import run_native_test as gate


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def elf(machine, loads, symbols):
    """Emit small, valid ELF32 MSB files with independent symbol/section tables."""
    data = bytearray(0x200)
    sections = [(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
    headers = []
    spans = []
    for address, payload, memory_size, flags in loads:
        offset = len(data)
        data.extend(payload)
        data.extend(b'\0' * (-len(data) % 4))
        headers.append((1, offset, address, address, len(payload), memory_size, flags, 4))
        sections.append((0, 1, 2 | (4 if flags & 1 else 0), address,
                         offset, len(payload), 0, 0, 4, 0))
        spans.append((address, address + len(payload), len(sections) - 1))
        if memory_size > len(payload):
            sections.append((0, 8, 3, address + len(payload), len(data),
                             memory_size - len(payload), 0, 0, 4, 0))
            spans.append((address + len(payload), address + memory_size, len(sections) - 1))
    strings = bytearray(b'\0')
    table = bytearray(16)
    for name, address, size, kind in symbols:
        name_at = len(strings)
        strings.extend(name.encode() + b'\0')
        section = next((i for start, end, i in spans if start <= address < end), 0xfff1)
        table.extend(struct.pack('>IIIBBH', name_at, address, size, 0x10 | kind, 0, section))
    table_at = len(data)
    data.extend(table)
    string_at = len(data)
    data.extend(strings)
    data.extend(b'\0' * (-len(data) % 4))
    sections.append((0, 2, 0, 0, table_at, len(table), len(sections) + 1, 1, 4, 16))
    sections.append((0, 3, 0, 0, string_at, len(strings), 0, 0, 1, 0))
    shoff = len(data)
    for section in sections:
        data.extend(struct.pack('>10I', *section))
    ident = b'\x7fELF\x01\x02\x01' + b'\0' * 9
    struct.pack_into('>16sHHIIIIIHHHHHH', data, 0, ident, 2, machine, 1,
                     loads[0][0], 52, shoff, 0, 52, 32, len(headers), 40, len(sections), 0)
    for i, header in enumerate(headers):
        struct.pack_into('>8I', data, 52 + i * 32, *header)
    return bytes(data)


def native_order(data):
    return b''.join(data[i:i+2][::-1] for i in range(0, len(data), 2))


class NativeCaptureGateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.build = self.root / 'build'
        self.capture = self.root / 'capture'
        self.build.mkdir()
        self.capture.mkdir()
        self.scsp = bytearray(512 * 1024)
        self.high = bytearray(1024 * 1024)
        self.low = bytearray(1024 * 1024)
        self.diag = 0x26008000
        self.engine = 0xd000
        m68k_loads = [(0, bytes(range(64)), 64, 5),
                      (0xc100, b'\x23' * 64, 0x1100, 7),
                      (0x10000, b'\x35' * 64, 64, 4)]
        firmware = elf(4, m68k_loads, [('wf_sfx_engine', self.engine, 262, 1)])
        (self.build / 'SCSP.ELF').write_bytes(firmware)
        for address, payload, _, _ in m68k_loads:
            self.scsp[address:address + len(payload)] = payload
        counters = ['_wf_music_fm_discarded_events', '_wf_music_fm_discarded_batches',
                    '_wf_music_fm_rejected_events', '_wf_music_ring_rejected_events']
        sh2_symbols = [('_fixture_function', 0x06004000, 32, 2),
                       ('_audio2_firmware', 0x06005000, 0, 0),
                       ('_audio2_firmware_end', 0x06005000 + len(firmware), 0, 0),
                       ('_audio2_diag', self.diag, 128, 1)]
        sh2_symbols.extend((name, 0x06009000 + 4 * i, 4, 1) for i, name in enumerate(counters))
        sh2_loads = [(0x06004000, b'\xab\xcd' * 16, 32, 5),
                     (0x06005000, firmware, len(firmware), 4),
                     (self.diag, b'', 128, 6),
                     (0x06009000, b'', 16, 6)]
        (self.build / 'AUDIO2.ELF').write_bytes(elf(42, sh2_loads, sh2_symbols))
        for address, payload, _, _ in sh2_loads:
            at = (address & 0x1fffffff) - 0x06000000
            self.high[at:at + len(payload)] = payload
        (self.build / 'AUDIO2.iso').write_bytes(b'synthetic disc identity\n')
        (self.build / 'AUDIO2.cue').write_text('FILE "AUDIO2.iso" BINARY\n TRACK 01 MODE1/2048\n INDEX 01 00:00:00\n')
        self.b = {'command': 1, 'seconds': 1, 'diag_address': self.diag, 'replay': None,
                  'files': {name: {'bytes': (self.build / name).stat().st_size,
                                   'sha256': digest(self.build / name)}
                            for name in ('AUDIO2.ELF', 'AUDIO2.iso', 'AUDIO2.cue')}}
        self.values = dict.fromkeys(gate.FIELDS, 0)
        self.values.update(magic=0x57464132, state=4, musicState=2, mailState=2,
                           APIstate=2, commandStartFrame=100, SCSPelapsed=44100,
                           diagAddress=self.diag, lastCommand=1)
        struct.pack_into('>H', self.scsp, 0xc046, 2)
        self.report = {'exit_code': 0, 'emulator': 'standalone Mednafen 1.32.1',
                       'frame_skip': False, 'emulated_hardware_clock_change': False,
                       'memory_access': 'read only', 'stop_reason': 'watch_threshold_reached',
                       'cue_sha256': digest(self.build / 'AUDIO2.cue'),
                       'disc_images': [{'path': str(self.build / 'AUDIO2.iso'),
                                        'bytes': (self.build / 'AUDIO2.iso').stat().st_size,
                                        'sha256': digest(self.build / 'AUDIO2.iso')}],
                       'trace': [{**self.values, 'state': 2, 'wall_seconds': 1.0}]}
        self.wav = self.capture / 'native.wav'
        with wave.open(str(self.wav), 'wb') as output:
            output.setnchannels(2)
            output.setsampwidth(2)
            output.setframerate(44100)
            output.writeframes(b'\0' * 44100 * 4)
        self.report['wav_validation'] = dict(sha256=digest(self.wav), bytes=self.wav.stat().st_size,
                                             frames=44100, channels=2, sample_rate=44100,
                                             bits=16, finalized=True)
        self.save_state()

    def save_state(self):
        for i, field in enumerate(gate.FIELDS):
            struct.pack_into('>I', self.high, (self.diag & 0x1fffffff) - 0x06000000 + i * 4,
                             self.values[field])
        blocks = bytearray()
        sections = {'SCSP': {'RAM': native_order(self.scsp)},
                    'MAIN': {'WorkRAMH': native_order(self.high), 'WorkRAML': native_order(self.low)}}
        for name, fields in sections.items():
            payload = bytearray()
            for key, value in fields.items():
                payload += bytes([len(key)]) + key.encode() + struct.pack('<I', len(value)) + value
            blocks += name.encode().ljust(32, b'\0') + struct.pack('<I', len(payload)) + payload
        header = bytearray(32)
        header[:8] = b'MDFNSVST'
        struct.pack_into('<4I', header, 16, 1061377, 32 + len(blocks), 0, 0)
        state_dir = self.capture / 'mednafen-home/mcs'
        state_dir.mkdir(parents=True, exist_ok=True)
        state = state_dir / 'fixture.mc0'
        state.write_bytes(header + blocks)
        self.report['native_state'] = {'path': str(state), 'version_numeric': 1061377,
                                      'file_sha256': digest(state), 'data_byte_order': 'little'}
        (self.capture / 'SCSP-RAM-saturn-order.bin').write_bytes(self.scsp)
        (self.capture / 'WorkRAMH-saturn-order.bin').write_bytes(self.high)
        (self.capture / 'WorkRAML-saturn-order.bin').write_bytes(self.low)

    def run_gate(self, child_exit=0):
        self.report['final_watch_values'] = dict(self.values)
        (self.build / 'saturn-build.json').write_text(json.dumps(self.b))
        (self.capture / 'report.json').write_text(json.dumps(self.report))
        argv = ['run_native_test.py', '--build', str(self.build), '--out', str(self.capture),
                '--environment-root', str(self.root), '--bios', str(self.root / 'unused-bios')]
        def child(*args, **kwargs):
            if child_exit and kwargs.get('check'):
                raise subprocess.CalledProcessError(child_exit, args[0])
            return subprocess.CompletedProcess([], child_exit)
        with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            with patch.object(gate.subprocess, 'run', side_effect=child):
                result = gate.main()
        output = json.loads((self.capture / 'native-gates.json').read_text())
        return result, output

    def assert_rejected(self):
        result, evidence = self.run_gate()
        self.assertEqual(result, 1)
        self.assertFalse(evidence['passed'])
        return evidence

    def test_complete_consistent_capture_passes(self):
        result, evidence = self.run_gate()
        self.assertEqual(result, 0, evidence)
        self.assertTrue(evidence['passed'])

    def test_emulator_nonzero_exit_is_not_a_pass(self):
        self.report['exit_code'] = 7
        self.assert_rejected()

    def test_completed_flag_does_not_prove_requested_seconds(self):
        self.b['seconds'] = 600
        self.assert_rejected()

    def test_missing_wav_cannot_pass(self):
        self.wav.unlink()
        self.assert_rejected()

    def test_unfinalized_riff_cannot_pass(self):
        with self.wav.open('r+b') as output:
            output.seek(4)
            output.write(b'\0' * 4)
        self.assert_rejected()

    def test_changed_valid_wav_does_not_match_capture_record(self):
        with self.wav.open('r+b') as output:
            output.seek(44)
            output.write(b'\x01\x00')
        self.assert_rejected()

    def test_finalized_but_short_wav_cannot_cover_the_window(self):
        with wave.open(str(self.wav), 'wb') as output:
            output.setnchannels(2)
            output.setsampwidth(2)
            output.setframerate(44100)
            output.writeframes(b'\0' * 100 * 4)
        self.report.pop('wav_validation')  # Older captures can be independently revalidated.
        evidence = self.assert_rejected()
        self.assertTrue(evidence['gates']['native_wav_finalized_and_unchanged'])
        self.assertFalse(evidence['gates']['native_wav_covers_scsp_window'])

    def test_earlier_error_cannot_be_hidden_by_final_zero(self):
        self.report['trace'][0]['lateB028'] = 1
        self.assert_rejected()

    def test_boot_garbage_is_not_an_active_capture_error(self):
        self.report['trace'].insert(0, {**self.values, 'magic': 0, 'lateB028': 19,
                                        'sfxUnderruns32': 7, 'SCSPelapsed': 0xffffffff})
        self.assertEqual(self.run_gate()[0], 0)

    def test_real_pcm_counter_overrules_zero_mirror(self):
        struct.pack_into('>I', self.scsp, self.engine + 32, 19)
        self.save_state()
        self.assert_rejected()

    def test_real_transport_counter_overrules_zero_mirror(self):
        struct.pack_into('>H', self.scsp, 0xb028, 2)
        self.save_state()
        self.assert_rejected()

    def test_real_sh2_discard_counter_overrules_zero_mirror(self):
        struct.pack_into('>I', self.high, 0x9000, 1)
        self.save_state()
        evidence = self.assert_rejected()
        self.assertEqual(evidence['evidence']['native_counters']['FMdiscardEvents'], 1)

    def test_embedded_firmware_must_match_the_supplied_elf(self):
        firmware = self.build / 'SCSP.ELF'
        data = bytearray(firmware.read_bytes())
        data[0x200] ^= 1
        firmware.write_bytes(data)
        evidence = self.assert_rejected()
        self.assertFalse(evidence['gates']['embedded_firmware_matches_elf'])

    def test_firmware_bytes_must_match_all_loaded_segments(self):
        self.scsp[0xc110] ^= 1
        self.save_state()
        self.assert_rejected()

    def test_sh2_function_bytes_must_match_linked_elf(self):
        self.high[0x4002] ^= 1
        self.save_state()
        self.assert_rejected()

    def test_state_hash_mismatch_cannot_pass(self):
        self.report['native_state']['file_sha256'] = '0' * 64
        self.assert_rejected()

    def test_ram_dump_must_match_native_f5(self):
        dump = self.capture / 'SCSP-RAM-saturn-order.bin'
        with dump.open('r+b') as output:
            output.write(b'\xff')
        self.assert_rejected()

    def test_changed_disc_cannot_pass_with_old_capture_identity(self):
        (self.build / 'AUDIO2.iso').write_bytes(b'other disc\n')
        self.assert_rejected()

    def test_claimed_diagnostic_address_must_match_linked_elf(self):
        self.b['diag_address'] += 4
        self.assert_rejected()

    def test_capture_subprocess_failure_is_preserved(self):
        result, evidence = self.run_gate(child_exit=1)
        self.assertEqual(result, 1)
        self.assertFalse(evidence['passed'])

    def test_audit_only_runs_no_process_and_preserves_all_inputs(self):
        self.assertEqual(self.run_gate()[0], 0)
        before = {str(path): digest(path) for folder in (self.build, self.capture)
                  for path in folder.rglob('*') if path.is_file()}
        destination = self.root / 'new-audit.json'
        argv = ['run_native_test.py', '--audit-only', '--build', str(self.build),
                '--out', str(self.capture), '--audit-output', str(destination)]
        with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            with patch.object(gate.subprocess, 'run', side_effect=AssertionError('Audit invoked a process')):
                self.assertEqual(gate.main(), 0)
        self.assertTrue(json.loads(destination.read_text())['passed'])
        after = {str(path): digest(path) for folder in (self.build, self.capture)
                 for path in folder.rglob('*') if path.is_file()}
        self.assertEqual(before, after)
        identity = digest(destination)
        with patch.object(sys, 'argv', argv), contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as caught:
                gate.main()
        self.assertEqual(caught.exception.code, 2)
        self.assertEqual(identity, digest(destination))

    def test_audit_output_cannot_replace_a_capture_or_build_file(self):
        for protected in (self.build / 'saturn-build.json', self.capture / 'report.json'):
            argv = ['run_native_test.py', '--audit-only', '--build', str(self.build),
                    '--out', str(self.capture), '--audit-output', str(protected)]
            with patch.object(sys, 'argv', argv), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as caught:
                    gate.main()
            self.assertEqual(caught.exception.code, 2)


if __name__ == '__main__':
    unittest.main()
