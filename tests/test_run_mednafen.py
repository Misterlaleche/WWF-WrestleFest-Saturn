#!/usr/bin/env python3
"""Read-only capture-reader tests; no emulator process is started."""
import argparse
import importlib.util
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import run_mednafen as reader


def diagnostic(**fields):
    words = [0] * 217
    values = dict(magic=0x57464350, version=1, bytes=868, sequence=2, policy_mode=2)
    values.update(fields)
    for key, value in values.items():
        words[reader.CAMPAIGN.index(key)] = value
    for serial in range(max(0, values.get('events_written', 0)-32), values.get('events_written', 0)):
        start = 25 + (serial % 32) * 6
        words[start:start+6] = [1 + serial % 7, serial+100, serial+10, serial//4, serial%7, serial]
    return struct.pack('>217I', *words)


class CampaignReaderTests(unittest.TestCase):
    def test_abi_and_wrapped_event_order_match_harness_decoder(self):
        spec = importlib.util.spec_from_file_location('campaign_decoder', ROOT/'tests/saturn/read_diagnostic.py')
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        data = diagnostic(events_written=39, credits_completed=1, capture_ready=1)
        self.assertEqual(reader.decode_campaign(data), module.decode(data))
        self.assertEqual([event['serial'] for event in reader.decode_campaign(data)['events']], list(range(7, 39)))

    def test_invalid_or_torn_blocks_are_rejected(self):
        for data in (b'', diagnostic()[:-1], diagnostic(magic=0), diagnostic(version=2),
                     diagnostic(bytes=864), diagnostic(sequence=3), diagnostic(policy_mode=0)):
            with self.subTest(data=data[:20]), self.assertRaises(ValueError):
                reader.decode_campaign(data)

    def test_live_read_checks_sequence_before_inside_and_after(self):
        def word(value): return reader.swap(struct.pack('>I', value))
        block = reader.swap(diagnostic(sequence=4))
        # First read changes during the copy; the second is stable.
        with mock.patch.object(reader.os, 'pread', side_effect=[word(2), block, word(4), word(4), block, word(4)]):
            self.assertEqual(reader.read_campaign(9, 0x1234)['sequence'], 4)
        with mock.patch.object(reader.os, 'pread', return_value=word(3)):
            self.assertIsNone(reader.read_campaign(9, 0x1234))
        with mock.patch.object(reader.os, 'pread', side_effect=OSError('unmapped')):
            self.assertIsNone(reader.read_campaign(9, 0x1234))

    def test_locates_separate_low_ram_and_rejects_duplicate_candidates(self):
        data = bytearray(0x200000)
        data[0xa00:0xa00+868] = reader.swap(diagnostic())
        def pread(mem, count, address): return bytes(data[address-0x400000:address-0x400000+count])
        with mock.patch.object(Path, 'read_text', return_value='00400000-00600000 rw-p 00000000 00:00 0\n'), \
             mock.patch.object(reader.os, 'pread', side_effect=pread):
            self.assertEqual(reader.locate_campaign(1, 2), 0x400a00)
            data[0x100a00:0x100a00+868] = reader.swap(diagnostic())
            with self.assertRaisesRegex(RuntimeError, 'Ambiguous'):
                reader.locate_campaign(1, 2)

    def test_campaign_only_stops_at_capture_ready(self):
        args = argparse.Namespace(campaign=True, stop_ticks=1, stop_stage=6, stop_post_combat=True)
        for campaign in (None, {'capture_ready': 0}, {'capture_ready': 0, 'credits_completed': 1}):
            sample = dict(ticks=99, stage=6, terminal_completed=7, campaign=campaign)
            self.assertIsNone(reader.capture_stop_reason(args, sample))
        self.assertEqual(reader.capture_stop_reason(args, {'campaign': {'capture_ready': 1}}),
                         'campaign credits and terminal return completed')
        args.campaign = False
        self.assertEqual(reader.capture_stop_reason(args, dict(ticks=99, stage=0)), 'requested tick threshold')


class ColdCheckTests(unittest.TestCase):
    def test_return_title_requires_boundary_and_sixty_consecutive_fields(self):
        args = argparse.Namespace(campaign=True, return_title=True)
        state = {}
        sample = {'live_field': 100, 'frontend': {'phase': 2, 'pc': 0x83a4, 'arcade_ticks': 10},
                  'campaign': {'capture_ready': 0}}
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        self.assertEqual(state, {})
        sample['campaign']['capture_ready'] = 1
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        sample['live_field'] = 159
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        # A title interrupted by another scene must restart the observation.
        sample['frontend']['phase'] = 3
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        sample['frontend']['phase'] = 6
        sample['live_field'] = 200
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        sample['live_field'] = 260
        # A stationary controller does not pass on physical VBlank alone.
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        sample['live_field'] = 320
        sample['frontend']['arcade_ticks'] = 70
        self.assertEqual(reader.capture_stop_reason(args, sample, state),
                         'result completed and title observed for 60 fields')
        self.assertTrue(state['confirmed'])

    def test_return_title_after_ordinary_loss_requires_completed_result(self):
        args = argparse.Namespace(campaign=False, return_title=True)
        state = {}
        sample = {'live_field': 100, 'frontend': {'phase': 6, 'pc': 0x83a4, 'arcade_ticks': 10}}
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        sample['terminal_completed'] = 1
        self.assertIsNone(reader.capture_stop_reason(args, sample, state))
        sample['live_field'] = 160
        sample['frontend']['arcade_ticks'] = 70
        self.assertIsNotNone(reader.capture_stop_reason(args, sample, state))
        self.assertTrue(state['confirmed'])

    def test_fault_gate_catches_main_stop_without_owner(self):
        self.assertEqual(reader.capture_fault_reason({'failed': 1, 'fault_owner': 0}),
                         'main entered failed state')
        for sample in ({'fault_owner': 0x52be}, {'campaign': {'fault_owner': 1}},
                       {'audio': {'fault': 1}}, {'smpc_timing_fault': 1},
                       {'null_access': {'count': 1}}):
            self.assertIsNotNone(reader.capture_fault_reason(sample))
        self.assertIsNone(reader.capture_fault_reason({'failed': 0, 'fault_owner': 0,
                         'audio': {'fault': 0}, 'smpc_timing_fault': 0,
                         'null_access': {'count': 0}}))

    def test_cold_check_validates_identity_without_starting_emulator(self):
        with tempfile.TemporaryDirectory(prefix='wf-reader-') as temp:
            base = Path(temp)
            for name in ('game.elf', 'game.raw', 'bios.bin', 'emulator', 'game.bin'):
                (base/name).write_bytes(b'test identity')
            (base/'game.cue').write_text('FILE "game.bin" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n')
            args = ['run_mednafen.py', '--campaign', '--cold-check', '--out', str(base/'capture'), '--libs', str(base)]
            for option, name in [('cue','game.cue'), ('elf','game.elf'), ('raw','game.raw'), ('bios','bios.bin'), ('emulator','emulator')]:
                args.extend(['--'+option, str(base/name)])
            markers = dict(wf_campaign_core_run=0x6001000, wf_campaign_key=0x6002000,
                           wf_campaign_observer_callback=0x6003000, wf_jo_terminal_completed=0x6004000)
            with mock.patch.object(sys, 'argv', args), mock.patch.object(reader, 'symbols', return_value=markers), \
                 mock.patch.object(reader, 'symbol_sizes', return_value={'wf_jo_video_stats':len(reader.VIDEO)*4}), \
                 mock.patch.object(reader.subprocess, 'Popen') as launch, mock.patch('sys.stdout', new_callable=io.StringIO):
                reader.main()
            launch.assert_not_called()
            report = json.loads((base/'capture/report.json').read_text())
            self.assertEqual(report['status'], 'COLD_CHECKED')
            self.assertTrue(report['cold_boot']); self.assertFalse(report['guest_writes'])
            self.assertFalse(report['state_loaded']); self.assertFalse(report['emulator_started'])
            self.assertEqual(report['campaign']['guest_address'], '0x00200a00')
            self.assertNotIn('-loadstate', report['command'])
            self.assertEqual(report['command'][-1], str((base/'game.cue').resolve()))
            self.assertEqual(report['disc_images'][0]['sha256'], reader.sha(base/'game.bin'))

    def test_campaign_rejects_partial_stop_options(self):
        args = ['run_mednafen.py', '--campaign', '--stop-post-combat']
        for option in ('cue','elf','raw','bios','out','emulator','libs'):
            args.extend(['--'+option, 'unused'])
        with mock.patch.object(sys, 'argv', args), mock.patch('sys.stderr', new_callable=io.StringIO), \
             mock.patch.object(reader.subprocess, 'Popen') as launch, self.assertRaises(SystemExit):
            reader.main()
        launch.assert_not_called()

    def test_capture_loop_records_events_and_continues_past_first_terminal(self):
        with tempfile.TemporaryDirectory(prefix='wf-reader-loop-') as temp:
            base = Path(temp)
            for name in ('game.elf', 'game.raw', 'bios.bin', 'emulator', 'game.bin'):
                (base/name).write_bytes(b'test identity')
            (base/'game.cue').write_text('FILE "game.bin" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n')
            args = ['run_mednafen.py', '--campaign', '--seconds', '30', '--out', str(base/'capture'), '--libs', str(base)]
            for option, name in [('cue','game.cue'), ('elf','game.elf'), ('raw','game.raw'), ('bios','bios.bin'), ('emulator','emulator')]:
                args.extend(['--'+option, str(base/name)])
            syms = {name:0x06001000+i*0x100 for i, name in enumerate(
                ('wf_campaign_core_run','wf_campaign_key','wf_campaign_observer_callback',
                 'wf_jo_terminal_completed','wf_jo_diagnostic','wf_jo_video_stats',
                 'wf_jo_audio_diagnostic','jo_core_vblank_count'))}
            ram = 0x10000000; low = 0x20000000
            data = bytearray(0x100000)
            def put(name, words):
                offset = syms[name]-0x06000000
                data[offset:offset+len(words)*4] = reader.swap(struct.pack('>'+str(len(words))+'I', *words))
            put('wf_jo_diagnostic', [0x574a3031, 6, 20, 10]+[0]*(len(reader.FIELDS)-4))
            put('wf_jo_terminal_completed', [1])
            put('jo_core_vblank_count', [20])
            def pread(fd, count, address):
                return bytes(data[address-ram:address-ram+count]) if ram <= address < ram+len(data) else bytes(count)
            process = mock.Mock(pid=42, stdin=io.BytesIO(), returncode=0)
            process.poll.return_value = None
            process.wait.side_effect = lambda **kwargs: setattr(process.poll, 'return_value', 0)
            samples = [reader.decode_campaign(diagnostic(terminals=1, events_written=3)),
                       reader.decode_campaign(diagnostic(terminals=2, credits_completed=1, capture_ready=1, events_written=6))]
            with mock.patch.object(sys, 'argv', args), mock.patch.object(reader, 'symbols', return_value=syms), \
                 mock.patch.object(reader, 'symbol_sizes', return_value={'wf_jo_video_stats':len(reader.VIDEO)*4}), \
                 mock.patch.object(reader.subprocess, 'Popen', return_value=process) as launch, \
                 mock.patch.object(reader.os, 'open', return_value=9) as open_mem, \
                 mock.patch.object(reader.os, 'close'), mock.patch.object(reader.os, 'pread', side_effect=pread), \
                 mock.patch.object(reader, 'locate_ram', return_value=ram), \
                 mock.patch.object(reader, 'locate_campaign', return_value=low+0xa00), \
                 mock.patch.object(reader, 'read_campaign', side_effect=samples), \
                 mock.patch.object(reader.time, 'monotonic', side_effect=[n*.25 for n in range(100)]), \
                 mock.patch.object(reader.time, 'sleep'), mock.patch('sys.stdout', new_callable=io.StringIO):
                reader.main()
            launch.assert_called_once()
            open_mem.assert_called_once_with('/proc/42/mem', reader.os.O_RDONLY)
            self.assertEqual(process.stdin.getvalue(), b'exit\n')
            report = json.loads((base/'capture/report.json').read_text())
            self.assertEqual(report['status'], 'CAPTURED')
            self.assertEqual(report['stop_reason'], 'campaign credits and terminal return completed')
            self.assertEqual(len(report['trace']), 2)
            self.assertEqual(report['trace'][0]['terminal_completed'], 1)
            self.assertEqual(report['trace'][0]['campaign']['capture_ready'], 0)
            self.assertEqual(report['trace'][1]['campaign']['events'][-1]['kind'], 'capture_ready')

    def test_campaign_requires_harness_symbols_before_launch(self):
        with tempfile.TemporaryDirectory(prefix='wf-reader-negative-') as temp:
            base = Path(temp); (base/'game.raw').write_bytes(b'raw')
            args = ['run_mednafen.py', '--campaign', '--cold-check', '--out', str(base/'capture'),
                    '--raw', str(base/'game.raw')]
            for option in ('cue','elf','bios','emulator','libs'): args.extend(['--'+option, 'unused'])
            with mock.patch.object(sys, 'argv', args), mock.patch.object(reader, 'symbols', return_value={}), \
                 mock.patch.object(reader, 'symbol_sizes', return_value={'wf_jo_video_stats':len(reader.VIDEO)*4}), \
                 mock.patch.object(reader.subprocess, 'Popen') as launch, \
                 mock.patch('sys.stderr', new_callable=io.StringIO) as errors, self.assertRaises(SystemExit):
                reader.main()
            launch.assert_not_called()
            self.assertIn('validation harness ELF', errors.getvalue())


class CaptureReturnTitleTests(unittest.TestCase):
    def run_capture(self, snapshots, *, control_exit=False):
        """Run the actual capture loop; only process, clock and RAM I/O are fake."""
        with tempfile.TemporaryDirectory(prefix='wf-reader-title-') as temp:
            base = Path(temp)
            for name in ('game.elf', 'game.raw', 'bios.bin', 'emulator', 'game.bin'):
                (base/name).write_bytes(b'test identity')
            (base/'game.cue').write_text('FILE "game.bin" BINARY\n TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n')
            args = ['run_mednafen.py', '--campaign', '--return-title', '--seconds', '3',
                    '--out', str(base/'capture'), '--libs', str(base)]
            for option, name in [('cue','game.cue'), ('elf','game.elf'), ('raw','game.raw'),
                                 ('bios','bios.bin'), ('emulator','emulator')]:
                args.extend(['--'+option, str(base/name)])
            syms = {name:0x06001000+i*0x100 for i, name in enumerate(
                ('wf_campaign_core_run', 'wf_campaign_key', 'wf_campaign_observer_callback',
                 'wf_jo_terminal_completed', 'wf_jo_diagnostic', 'wf_jo_video_stats',
                 'wf_jo_audio_diagnostic', 'jo_core_vblank_count', 'wf_jo_front_diagnostic'))}
            ram = 0x10000000; low = 0x20000000
            data = bytearray(0x100000)

            def put(name, words):
                offset = syms[name]-0x06000000
                data[offset:offset+len(words)*4] = reader.swap(struct.pack('>'+str(len(words))+'I', *words))

            def frame(sample):
                field, tick, phase, pc = sample
                put('wf_jo_diagnostic', [0x574a3031, 6, field, 14369]+[0]*(len(reader.FIELDS)-4))
                put('wf_jo_terminal_completed', [1])
                put('jo_core_vblank_count', [field])
                put('wf_jo_front_diagnostic', [phase, pc, tick, field, 0, 0, 0, 0])

            frame(snapshots[0])
            sampled = 0

            def campaign(*unused):
                nonlocal sampled
                sampled += 1
                frame(snapshots[min(sampled, len(snapshots)-1)])
                if control_exit:
                    (base/'capture/control.txt').write_text('exit\n')
                return reader.decode_campaign(diagnostic(terminals=1, credits_completed=1,
                                                          capture_ready=1, events_written=6))

            def pread(fd, count, address):
                return bytes(data[address-ram:address-ram+count]) if ram <= address < ram+len(data) else bytes(count)

            process = mock.Mock(pid=42, stdin=io.BytesIO(), returncode=0)
            process.poll.return_value = None
            process.wait.side_effect = lambda **kwargs: setattr(process.poll, 'return_value', 0)
            with mock.patch.object(sys, 'argv', args), mock.patch.object(reader, 'symbols', return_value=syms), \
                 mock.patch.object(reader, 'symbol_sizes', return_value={'wf_jo_video_stats':len(reader.VIDEO)*4}), \
                 mock.patch.object(reader.subprocess, 'Popen', return_value=process) as launch, \
                 mock.patch.object(reader.os, 'open', return_value=9) as open_mem, \
                 mock.patch.object(reader.os, 'close'), mock.patch.object(reader.os, 'pread', side_effect=pread), \
                 mock.patch.object(reader, 'locate_ram', return_value=ram), \
                 mock.patch.object(reader, 'locate_campaign', return_value=low+0xa00), \
                 mock.patch.object(reader, 'read_campaign', side_effect=campaign), \
                 mock.patch.object(reader.time, 'monotonic', side_effect=[n*.25 for n in range(100)]), \
                 mock.patch.object(reader.time, 'sleep'), mock.patch('sys.stdout', new_callable=io.StringIO):
                reader.main()
            launch.assert_called_once()
            open_mem.assert_called_once_with('/proc/42/mem', reader.os.O_RDONLY)
            self.assertEqual(process.stdin.getvalue(), b'exit\n')
            report = json.loads((base/'capture/report.json').read_text())
            self.assertTrue(report['trace'])
            self.assertTrue(all(sample['campaign']['capture_ready'] for sample in report['trace']))
            return report

    def test_timeout_after_capture_ready_does_not_confirm_title(self):
        report = self.run_capture([(100, 10, 1, 0x6fc)])
        self.assertEqual(report['status'], 'TITLE_RETURN_INCOMPLETE')
        self.assertFalse(report['title_return']['confirmed'])
        self.assertEqual(report['stop_reason'], 'title return not confirmed before capture ended')

    def test_control_exit_after_capture_ready_does_not_confirm_title(self):
        report = self.run_capture([(100, 10, 1, 0x6fc)], control_exit=True)
        self.assertEqual(report['status'], 'TITLE_RETURN_INCOMPLETE')
        self.assertFalse(report['title_return']['confirmed'])
        self.assertEqual(report['stop_reason'], 'control exit')
        self.assertEqual(len(report['trace']), 1)

    def test_capture_ready_continues_until_advancing_title_completes(self):
        report = self.run_capture([(100, 10, 1, 0x6fc), (120, 20, 2, 0x83a4),
                                   (180, 80, 6, 0x83a4)])
        self.assertEqual(report['status'], 'CAPTURED')
        self.assertEqual(report['stop_reason'], 'result completed and title observed for 60 fields')
        self.assertTrue(report['title_return']['confirmed'])
        self.assertEqual(len(report['trace']), 3)
        self.assertEqual(report['title_return']['first_field'], 120)
        self.assertEqual(report['title_return']['last_field'], 180)

    def test_title_controller_stalled_after_one_tick_is_incomplete(self):
        report = self.run_capture([(100, 10, 2, 0x83a4), (101, 11, 2, 0x83a4),
                                   (160, 11, 2, 0x83a4)])
        self.assertEqual(report['status'], 'TITLE_RETURN_INCOMPLETE')
        self.assertFalse(report['title_return']['confirmed'])
        self.assertGreaterEqual(len(report['trace']), 3)

    def test_title_with_enough_early_ticks_but_later_stall_is_incomplete(self):
        report = self.run_capture([(100, 10, 2, 0x83a4), (130, 40, 2, 0x83a4),
                                   (160, 40, 2, 0x83a4)])
        self.assertEqual(report['status'], 'TITLE_RETURN_INCOMPLETE')
        self.assertFalse(report['title_return']['confirmed'])
        self.assertGreaterEqual(len(report['trace']), 3)


if __name__ == '__main__':
    unittest.main()
