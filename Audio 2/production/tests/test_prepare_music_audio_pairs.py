"""Synthetic samples and metadata only: never original ROM or musical audio."""
from pathlib import Path
import copy
from concurrent.futures import ThreadPoolExecutor
import importlib.util
import json
import struct
import subprocess
import tempfile
import threading
import unittest
from unittest.mock import patch
import wave

import test_native_capture_gate as native_fixture

TOOL = Path(__file__).resolve().parents[1] / 'tools/prepare_music_audio_pairs.py'
SPEC = importlib.util.spec_from_file_location('prepare_music_audio_pairs', TOOL)
pairs = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(pairs)

NATIVE_GATES = (
    'capture_subprocess_succeeded', 'emulator_exited_successfully', 'native_capture_method',
    'normal_window_stop', 'real_module_entered', 'requested_window_completed',
    'no_reported_worker_error', 'no_reported_api_error', 'no_reported_fm_discard',
    'no_reported_rejected_events', 'no_reported_pcm_underrun', 'no_reported_transport_lateness',
    'worker_still_running', 'native_still_ready', 'api_still_running',
    'requested_scsp_duration_reached', 'build_and_capture_identities_match',
    'native_wav_finalized_and_unchanged', 'native_wav_covers_scsp_window',
    'observed_history_has_no_errors', 'observed_clock_did_not_reset',
    'native_f5_and_linked_elf_evidence_complete', 'embedded_firmware_matches_elf',
    'all_loaded_firmware_bytes_match', 'all_linked_sh2_function_bytes_match',
    'diagnostic_mirror_matches_f5_and_elf', 'no_actual_native_counter_errors',
    'all_replay_words_accepted')
ADDITIONAL_GATES = ('firmware_first512_matches', 'native_wav_finalized',
                    'selected_emulator_identity', 'no_expansion_cartridge',
                    'replay_words_results_and_admission_times')


def matrix_fixture(score_seconds=600, tail_seconds=8):
    cases = [{'id': f'command-{c:02X}', 'command': c, 'kind': 'music',
              'seconds': score_seconds + tail_seconds,
              'events': [{'sample': 0, 'word': c},
                         {'sample': score_seconds * 44100, 'word': 31}]}
             for c in range(1, 19)]
    cases.append({'id': 'transitions', 'command': 1, 'kind': 'transitions',
                  'seconds': 24 + tail_seconds,
                  'events': [{'sample': i * 132300, 'word': word}
                             for i, word in enumerate((1, 6, 18, 31, 3, 31, 2, 1, 31))]})
    return {'complete': True, 'source_stable': True, 'inputs_stable': True,
            'emulator_stable': True, 'all_runtime_gates_passed': True,
            'kind': 'LONG_NATIVE_MUSIC_FADE_MATRIX',
            'score_seconds_before_1F': score_seconds, 'tail_seconds': tail_seconds,
            'native_window_seconds': score_seconds + tail_seconds, 'range': [1, 18],
            'cases': cases,
            'commands': [dict(c, status='complete', passed_runtime_gates=True,
                              pair_available=True,
                              native_gates={'passed': True, 'gate_version': 2,
                                            'gates': dict.fromkeys(NATIVE_GATES, True),
                                            'evidence_errors': []},
                              additional_gates=dict.fromkeys(ADDITIONAL_GATES, True)) for c in cases]}


class CompletePairChecks(unittest.TestCase):
    def reference_pool(self):
        pool = getattr(pairs, 'reference_captures', None)
        self.assertTrue(callable(pool), 'Bounded reference captures must be available')
        return pool

    def test_reference_parallelism_bounds_unconsumed_outputs_and_returns_declared_order(self):
        pool = self.reference_pool()
        lock = threading.Lock()
        release_first, first_started, second_finished = (threading.Event() for _ in range(3))
        started, cleaned, active = set(), set(), set()
        maxima = {'unconsumed': 0, 'active': 0}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def capture(case):
                with lock:
                    started.add(case); active.add(case)
                    maxima['unconsumed'] = max(maxima['unconsumed'], len(started-cleaned))
                    maxima['active'] = max(maxima['active'], len(active))
                if case == 0:
                    first_started.set()
                    self.assertTrue(release_first.wait(5))
                elif case == 1:
                    self.assertTrue(first_started.wait(5))
                output = root/f'{case}.pcm'; output.write_bytes(bytes([case]))
                with lock: active.remove(case)
                if case == 1: second_finished.set()
                return output

            with pool(list(range(5)), capture, 2) as captures:
                self.assertTrue(second_finished.wait(5))
                with lock: self.assertEqual(started, {0, 1})
                release_first.set()
                observed = []
                for case, output in captures:
                    observed.append(case)
                    self.assertEqual(output.read_bytes(), bytes([case]))
                    output.unlink()
                    with lock: cleaned.add(case)
            self.assertEqual(observed, [0, 1, 2, 3, 4])
            self.assertEqual(maxima['unconsumed'], 2)
            self.assertEqual(maxima['active'], 2)
            self.assertEqual(active, set())
            self.assertEqual(list(root.iterdir()), [])

    def test_reference_parallelism_defaults_to_one_and_rejects_invalid_limits(self):
        pool = self.reference_pool()
        cleaned, started = [], []

        def capture(case):
            started.append(case)
            self.assertEqual(len(started)-len(cleaned), 1)
            return case

        with pool([0, 1, 2], capture) as captures:
            for case, result in captures:
                self.assertEqual(case, result)
                cleaned.append(case)
        self.assertEqual(cleaned, [0, 1, 2])
        for limit in (0, 5, -1, True, 1.5):
            with self.subTest(limit=limit), self.assertRaises(ValueError):
                with pool([0], capture, limit):
                    self.fail('Invalid parallelism must fail before starting a reference')

    def test_reference_failure_cancels_pending_cases_and_drains_running_work(self):
        pool = self.reference_pool()
        release_failure, release_pending, running_done = (threading.Event() for _ in range(3))
        started, submitted = [], []
        # A real executor with one worker makes pending cancellation observable.
        executor = ThreadPoolExecutor(max_workers=1)
        original_submit = executor.submit

        def submit(*args, **kwargs):
            future = original_submit(*args, **kwargs)
            submitted.append(future)
            return future

        def capture(case):
            started.append(case)
            if case == 0:
                self.assertTrue(release_failure.wait(5))
                raise RuntimeError('Synthetic reference failed')
            self.assertTrue(release_pending.wait(5))
            running_done.set()
            return case

        with patch.object(executor, 'submit', side_effect=submit), patch.object(pairs, 'ThreadPoolExecutor', return_value=executor):
            with self.assertRaisesRegex(RuntimeError, 'Synthetic reference failed'):
                with pool(list(range(8)), capture, 4) as captures:
                    self.assertEqual(len(submitted), 4)
                    submitted[3].add_done_callback(lambda future: release_pending.set() if future.cancelled() else None)
                    release_failure.set()
                    next(captures)
        self.assertTrue(submitted[2].cancelled())
        self.assertTrue(submitted[3].cancelled())
        self.assertTrue(all(f.done() for f in submitted))
        self.assertTrue(set(started).issubset({0, 1}))
        if 1 in started: self.assertTrue(running_done.is_set())

    def test_consumer_failure_does_not_submit_more_references(self):
        pool = self.reference_pool()
        started = []

        def capture(case):
            started.append(case)
            return case

        with self.assertRaisesRegex(ValueError, 'Serialization failed'):
            with pool(list(range(8)), capture, 2) as captures:
                case, result = next(captures)
                self.assertEqual((case, result), (0, 0))
                raise ValueError('Serialization failed')
        self.assertTrue(set(started).issubset({0, 1}))

    def test_reference_schedule_uses_admitted_fade_not_requested_fade(self):
        observation = {'passed': True, 'gates': {'all_rows_present': True},
                       'observations': [
                           {'word': 1, 'sample': 0, 'result': 0, 'recorded': True},
                           {'word': 31, 'sample': 1323159, 'result': 0, 'recorded': True,
                            'expected': {'sample': 1323000, 'word': 31}}]}
        self.assertEqual(pairs.accepted_events(observation),
                         [{'sample': 0, 'word': 1}, {'sample': 1323159, 'word': 31}])
        for mutate in ('missing', 'simultaneous', 'unaccepted'):
            broken = copy.deepcopy(observation)
            if mutate == 'missing':
                broken['passed'] = False
            elif mutate == 'simultaneous':
                broken['observations'][1]['sample'] = 0
            else:
                broken['observations'][1]['result'] = 1
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                pairs.accepted_events(broken)

    def test_native_origin_preserves_music_lead_and_wraps_clock(self):
        details = {'final_watch_values': {'magic': 0x57464132, 'state': 4,
                                          'commandDSPclock': 31066,
                                          'DSPclock16': 65530, 'SCSPelapsed': 100000}}
        window = pairs.native_window(100200, details, 4, 88200)
        self.assertEqual(window.get('start_frame'), 190)
        self.assertEqual(window.get('end_frame_exclusive'), 88390)
        self.assertEqual(window.get('diagnostic_staleness_frames'), 10)
        with self.assertRaises(ValueError):
            pairs.native_window(100200, details, 1500, 88200)
        with self.assertRaises(ValueError):
            pairs.native_window(1000, details, 4, 88200)

    def test_native_origin_uses_coherent_elapsed_clock_not_later_diagnostic_read(self):
        details = {'final_watch_values': {'magic': 0x57464132, 'state': 4,
                                          'commandDSPclock': 17070,
                                          'SCSPelapsed': 1677722, 'DSPclock16': 56393}}
        for later_clock in (56392, 56393, 56400):
            details['final_watch_values']['DSPclock16'] = later_clock
            with self.subTest(later_clock=later_clock):
                result = pairs.native_window(2339668, details, 56451, 1675800)
                self.assertEqual(result['start_frame'], 661887)
                self.assertEqual(result['end_frame_exclusive'], 2337687)
                self.assertEqual(result['diagnostic_staleness_frames'], 59)
                self.assertEqual(result['native_music_lead_removed_frames'], 0)

    def test_native_origin_rejects_missing_or_invalid_epoch_clock(self):
        details = {'final_watch_values': {'magic': 0x57464132, 'state': 4,
                                          'commandDSPclock': 31066,
                                          'DSPclock16': 65530, 'SCSPelapsed': 100000}}
        for invalid in (None, True, -1, 65536, 1.5):
            details['final_watch_values']['commandDSPclock'] = invalid
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                pairs.native_window(100200, details, 4, 88200)

    def test_extract_keeps_exact_stereo_pcm_and_all_leading_silence(self):
        samples = [(91, -91), (0, 0), (0, 0), (7, -9), (-32768, 32767), (11, 13)]
        data = b''.join(struct.pack('<hh', *v) for v in samples)
        with tempfile.TemporaryDirectory() as directory:
            source, target = Path(directory)/'raw.wav', Path(directory)/'native.wav'
            with wave.open(str(source), 'wb') as f:
                f.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
                f.writeframes(data)
            metadata = pairs.copy_wav_window(source, target, 1, 4)
            self.assertTrue(target.exists(), 'The real captured frames must be extracted')
            with wave.open(str(target)) as f:
                self.assertEqual((f.getnchannels(), f.getsampwidth(), f.getframerate(), f.getnframes()),
                                 (2, 2, 44100, 4))
                self.assertEqual(f.readframes(4), data[4:20])
            self.assertTrue(metadata.get('source_pcm_bytes_match'))
            with self.assertRaises(FileExistsError):
                pairs.copy_wav_window(source, target, 1, 4)

    def test_reference_pcm_conversion_preserves_signed_values(self):
        with tempfile.TemporaryDirectory() as directory:
            raw, target = Path(directory)/'raw.pcm', Path(directory)/'reference.wav'
            values = (-32768, -1, 0, 1, 32767)
            raw.write_bytes(struct.pack('>5h', *values))
            metadata = pairs.pcm16be_to_wav(raw, target, len(values))
            self.assertTrue(target.exists(), 'The captured PCM must be serialized as WAV')
            with wave.open(str(target)) as f:
                self.assertEqual(f.readframes(5), struct.pack('<5h', *values))
            self.assertTrue(metadata.get('source_pcm_bytes_match'))

    def test_full_delivery_requires_long_pass_and_complete_transition_case(self):
        self.assertEqual(pairs.select_cases(matrix_fixture(), None, False).__len__(), 19)
        for mutate in ('short', 'failed', 'missing_transition'):
            report = matrix_fixture(30 if mutate == 'short' else 600)
            if mutate == 'failed':
                report['commands'][3]['passed_runtime_gates'] = False
                report['all_runtime_gates_passed'] = False
            elif mutate == 'missing_transition':
                report['cases'].pop()
                report['commands'].pop()
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                pairs.select_cases(report, None, False)
        self.assertEqual([c['id'] for c in pairs.select_cases(matrix_fixture(30), 'command-01', True)],
                         ['command-01'])
        with self.assertRaises(ValueError):
            pairs.select_cases(matrix_fixture(30), None, True)

    def test_available_audio_and_silent_tail_never_erase_runtime_failure(self):
        report = matrix_fixture(30)
        failed = {'command-04', 'command-06', 'command-07', 'command-12', 'transitions'}
        for item in report['commands']:
            item['fade_tail_observation'] = {'final_window_all_channels_exact_zero': True}
            if item['id'] in failed:
                item['passed_runtime_gates'] = False
                item['native_gates'] = {'passed': False, 'gates': {'no_reported_transport_lateness': False}}
        inventory = pairs.runtime_inventory(report)
        self.assertEqual({i['id'] for i in inventory if i['runtime_status'] == 'FAIL'}, failed)
        self.assertEqual(len(inventory), 19)

    def test_long_report_label_cannot_hide_short_or_interrupted_music_windows(self):
        for mutate in ('short_window', 'early_fade', 'extra_command'):
            report = matrix_fixture()
            case = report['cases'][0]
            if mutate == 'short_window':
                case['seconds'] = 38
            elif mutate == 'early_fade':
                case['events'][1]['sample'] = 30 * 44100
            else:
                case['events'].insert(1, {'sample': 30 * 44100, 'word': 2})
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                pairs.select_cases(report, None, False)

    def test_full_collection_requires_eight_second_tail_and_exact_declared_durations(self):
        for tail in (1, 7):
            with self.subTest(tail=tail), self.assertRaises(ValueError):
                pairs.select_cases(matrix_fixture(tail_seconds=tail), None, False)
        for mutate in ('missing_tail', 'different_music_duration', 'different_transition_duration'):
            report = matrix_fixture()
            if mutate == 'missing_tail':
                del report['tail_seconds']
            elif mutate == 'different_music_duration':
                report['cases'][0]['seconds'] = 609
            else:
                report['cases'][-1]['seconds'] = 33
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                pairs.select_cases(report, None, False)

    def test_full_collection_requires_exact_transition_and_case_mapping(self):
        for mutate in ('short_transition', 'reordered_transition', 'wrong_runtime_command',
                       'wrong_runtime_kind', 'duplicate_runtime_id', 'extra_case', 'noninteger_event'):
            report = matrix_fixture()
            if mutate == 'short_transition':
                report['cases'][-1]['events'] = [{'sample': 0, 'word': 1}, {'sample': 44100, 'word': 31}]
            elif mutate == 'reordered_transition':
                report['cases'][-1]['events'][1]['word'] = 18
            elif mutate == 'wrong_runtime_command':
                report['commands'][0]['command'] = 18
            elif mutate == 'wrong_runtime_kind':
                report['commands'][0]['kind'] = 'transitions'
            elif mutate == 'duplicate_runtime_id':
                report['commands'][1]['id'] = report['commands'][0]['id']
            elif mutate == 'extra_case':
                report['cases'].append({'id': 'other', 'command': 1, 'kind': 'other', 'seconds': 608})
                report['commands'].append(dict(report['commands'][0], id='other', kind='other'))
            else:
                report['cases'][0]['events'][0]['word'] = True
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                pairs.select_cases(report, None, False)

    def test_full_collection_rejects_missing_extra_or_nonboolean_runtime_gates(self):
        for mutate in ('empty_native', 'missing_native', 'extra_native', 'nonboolean_native',
                       'empty_additional', 'nonboolean_additional', 'missing_version', 'evidence_error'):
            report = matrix_fixture()
            item = report['commands'][0]
            if mutate == 'empty_native':
                item['native_gates']['gates'].clear()
            elif mutate == 'missing_native':
                del item['native_gates']['gates']['native_f5_and_linked_elf_evidence_complete']
            elif mutate == 'extra_native':
                item['native_gates']['gates']['unknown_gate'] = True
            elif mutate == 'nonboolean_native':
                item['native_gates']['gates']['native_f5_and_linked_elf_evidence_complete'] = 1
            elif mutate == 'empty_additional':
                item['additional_gates'].clear()
            elif mutate == 'nonboolean_additional':
                item['additional_gates']['native_wav_finalized'] = 1
            elif mutate == 'missing_version':
                del item['native_gates']['gate_version']
            else:
                item['native_gates']['evidence_errors'] = [{'error': 'RAM does not match F5'}]
            with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                pairs.select_cases(report, None, False)

    def test_case_ids_cannot_escape_input_or_output_directories(self):
        for identity in ('../../escaped-output', '/tmp/absolute-output', 'command-1', 'command-02/child'):
            report = matrix_fixture()
            report['cases'][0]['id'] = report['commands'][0]['id'] = identity
            with self.subTest(identity=identity), self.assertRaises(ValueError):
                pairs.select_cases(report, None, False)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            out = root/'output'; out.mkdir()
            case = {'id': '../../escaped-output', 'command': 1, 'kind': 'music', 'seconds': 1}
            events = [{'sample': 0, 'word': 1}, {'sample': 1, 'word': 31}]
            # The subprocess is the external boundary; path creation remains real.
            with patch.object(pairs.subprocess, 'run', return_value=subprocess.CompletedProcess([], 1)):
                with self.assertRaises(ValueError):
                    pairs.capture_reference(root/'reference', out, case, events, 1)
            self.assertEqual(list(out.iterdir()), [])
            self.assertFalse((root/'escaped-output').exists())

    def make_native_fixture(self, runtime_error=False):
        fixture = native_fixture.NativeCaptureGateTests('test_complete_consistent_capture_passes')
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        matrix_root = fixture.root/'matrix'
        work = matrix_root/'command-01'; work.mkdir(parents=True)
        fixture.build.rename(work/'build'); fixture.build = work/'build'
        fixture.capture.rename(work/'capture'); fixture.capture = work/'capture'
        fixture.wav = fixture.capture/'native.wav'
        with wave.open(str(fixture.wav), 'wb') as output:
            output.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
            output.writeframes(b'\0' * 45100 * 4)
        fixture.report['wav_validation'].update(sha256=pairs.sha(fixture.wav),
                                                bytes=fixture.wav.stat().st_size, frames=45100)
        parsed = native_fixture.gate.read_elf(fixture.build/'AUDIO2.ELF', 42)
        loads = [(v[2], parsed['data'][v[1]:v[1]+v[4]], v[5], v[6]) for v in parsed['loads']]
        symbols = [(name, v['value'], v['size'], v['kind']) for name, v in parsed['symbols'].items()]
        loads.append((0x0600a000, b'', 32, 6))
        symbols.append(('_audio2_replay_observed', 0x0600a000, 32, 1))
        elf_path = fixture.build/'AUDIO2.ELF'
        elf_path.write_bytes(native_fixture.elf(42, loads, symbols))
        fixture.b['files']['AUDIO2.ELF'] = {'bytes': elf_path.stat().st_size, 'sha256': pairs.sha(elf_path)}
        (fixture.build/'symbols.json').write_text(json.dumps({'_audio2_replay_observed': 0x0600a000}))
        case = {'id': 'command-01', 'command': 1, 'kind': 'music', 'seconds': 1,
                'events': [{'sample': 0, 'word': 1}, {'sample': 22050, 'word': 31}]}
        replay = matrix_root/'inputs/replays/command-01.json'; replay.parent.mkdir(parents=True)
        replay.write_text(json.dumps({'events': case['events']}))
        fixture.b['replay'] = {'events': 2, 'sha256': pairs.sha(replay)}
        struct.pack_into('>8I', fixture.high, 0xa000, 1, 0, 100, 0, 31, 22050, 130, 0)
        struct.pack_into('>H', fixture.scsp, 0xb010, 44100)
        fixture.values.update(DSPclock16=44100, commandDSPclock=0, lastCommand=31,
                              replayAccepted=2, replayExpected=2, error=1 if runtime_error else 0)
        fixture.report.update(emulator_sha256='a'*64, cartridge='none')
        fixture.report['disc_images'][0]['path'] = str(fixture.build/'AUDIO2.iso')
        fixture.report['trace'] = [{**fixture.values, 'state': 2, 'wall_seconds': 1.0}]
        self.save_native_fixture(fixture)
        gate = native_fixture.gate.audit_capture(fixture.build, fixture.capture, 0)
        self.assertEqual(gate['evidence_errors'], [])
        self.assertEqual(gate['passed'], not runtime_error)
        (fixture.capture/'native-gates.json').write_text(json.dumps(gate))
        from run_music_matrix import replay_observation
        item = dict(case, build=fixture.b, native_gates=gate,
                    capture_report_sha256=pairs.sha(fixture.capture/'report.json'),
                    replay_observation=replay_observation(fixture.build, fixture.capture, case['events']),
                    status='complete', passed_runtime_gates=not runtime_error,
                    additional_gates=dict.fromkeys(ADDITIONAL_GATES, True))
        matrix = {'emulator_sha256': 'a'*64, 'firmware_sha256': pairs.sha(fixture.build/'SCSP.ELF'),
                  'source_sha256_before': {'tools/'+name: pairs.sha(TOOL.parent/name) for name in
                      ('run_native_test.py', 'capture_mednafen.py', 'analyze_audio_capture.py')}}
        return matrix_root, matrix, case, item, fixture

    @staticmethod
    def save_native_fixture(fixture):
        fixture.save_state()
        fixture.report['final_watch_values'] = dict(fixture.values)
        (fixture.build/'saturn-build.json').write_text(json.dumps(fixture.b))
        (fixture.capture/'report.json').write_text(json.dumps(fixture.report))

    def test_native_inspection_hashes_f5_and_every_audited_ram_and_build_input(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        inspected = pairs.inspect_native(root, matrix, case, item)
        self.assertEqual(inspected['window']['start_frame'], 1000)
        for relative in ('capture/mednafen-home/mcs/fixture.mc0', 'capture/WorkRAML-saturn-order.bin',
                         'build/AUDIO2.ELF', 'build/AUDIO2.iso', 'build/AUDIO2.cue'):
            path = root/'command-01'/relative
            self.assertEqual(inspected['identities'].get(str(path)), pairs.sha(path))

    def test_changed_scsp_clock_cannot_pass_with_stale_saved_audit(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        ram = fixture.capture/'SCSP-RAM-saturn-order.bin'
        data = bytearray(ram.read_bytes()); struct.pack_into('>H', data, 0xb010, 44200)
        ram.write_bytes(data)
        with self.assertRaises(ValueError):
            pairs.inspect_native(root, matrix, case, item)

    def test_native_evidence_cannot_change_between_audit_and_window_selection(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        original_audit = native_fixture.gate.audit_capture

        def audit_then_change_clock(*args):
            audited = original_audit(*args)
            ram = fixture.capture/'SCSP-RAM-saturn-order.bin'
            data = bytearray(ram.read_bytes()); struct.pack_into('>H', data, 0xb010, 44200)
            ram.write_bytes(data)
            return audited

        # The real audit runs; only the timing of a subsequent input edit is controlled.
        with patch.object(native_fixture.gate, 'audit_capture', side_effect=audit_then_change_clock):
            with self.assertRaises(ValueError):
                pairs.inspect_native(root, matrix, case, item)

    def test_missing_f5_cannot_pass_with_stale_saved_audit(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        (fixture.capture/'mednafen-home/mcs/fixture.mc0').unlink()
        with self.assertRaises(ValueError):
            pairs.inspect_native(root, matrix, case, item)

    def test_genuine_runtime_failure_remains_available_and_failed_in_diagnostic_metadata(self):
        root, matrix, case, item, fixture = self.make_native_fixture(runtime_error=True)
        inspected = pairs.inspect_native(root, matrix, case, item)
        self.assertEqual(inspected['window']['start_frame'], 1000)
        inventory = pairs.runtime_inventory({'commands': [item]})
        self.assertEqual(inventory[0]['runtime_status'], 'FAIL')
        self.assertIn('no_reported_worker_error', inventory[0]['failed_gates'])
        public = pairs.public_manifest({'runtime_inventory': inventory, 'diagnostic_only': True,
                                       'native_runtime_passed': False, 'pairs': []})
        self.assertFalse(public['native_runtime_passed'])
        self.assertFalse(public['fidelity_approved'])

    def test_diagnostic_rejects_matching_audit_with_evidence_errors(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        (fixture.capture/'mednafen-home/mcs/fixture.mc0').unlink()
        broken = native_fixture.gate.audit_capture(fixture.build, fixture.capture, 0)
        self.assertTrue(broken['evidence_errors'])
        item['native_gates'] = broken
        (fixture.capture/'native-gates.json').write_text(json.dumps(broken))
        with self.assertRaises(ValueError):
            pairs.inspect_native(root, matrix, case, item)

    def test_diagnostic_rejects_structural_failure_even_without_audit_errors(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        fixture.high[0x4002] ^= 1
        self.save_native_fixture(fixture)
        broken = native_fixture.gate.audit_capture(fixture.build, fixture.capture, 0)
        self.assertEqual(broken['evidence_errors'], [])
        self.assertFalse(broken['gates']['all_linked_sh2_function_bytes_match'])
        item.update(native_gates=broken, capture_report_sha256=pairs.sha(fixture.capture/'report.json'))
        (fixture.capture/'native-gates.json').write_text(json.dumps(broken))
        with self.assertRaises(ValueError):
            pairs.inspect_native(root, matrix, case, item)

    def test_native_audit_helper_must_match_frozen_matrix_source(self):
        root, matrix, case, item, fixture = self.make_native_fixture()
        matrix['source_sha256_before']['tools/run_native_test.py'] = '0'*64
        with self.assertRaises(ValueError):
            pairs.inspect_native(root, matrix, case, item)

    def test_public_export_filters_private_fields_inside_pair_metadata(self):
        report = {'pairs': [{'id': 'command-01', 'seconds': 38, 'runtime_status': 'FAIL',
                             'native_wav': {'sha256': 'a'*64, 'frames': 10,
                                            'raw_samples': [100, 200], 'path': '/private/capture.wav'},
                             'trace': 'protected event stream', 'RAM': [99],
                             'fade': {'native': {'final_window_peak': 0, 'samples': [4, 5]}}}]}
        public = pairs.public_manifest(report)
        self.assertEqual(public['pairs'][0]['runtime_status'], 'FAIL')
        self.assertEqual(public['pairs'][0]['native_wav']['sha256'], 'a'*64)
        text = json.dumps(public)
        for forbidden in ('/private/', 'raw_samples', 'protected event', '"RAM"', '"samples"'):
            self.assertNotIn(forbidden, text)

    def test_public_metadata_does_not_copy_private_material_or_approve_fidelity(self):
        report = {'schema_version': 1, 'tool_version': 'test', 'kind': 'DIAGNOSTIC',
                  'source_sha256': {'tool.py': 'a'*64}, 'matrix_sha256': 'b'*64,
                  'runtime_inventory': [], 'pairs': [], 'complete_capture_export': True,
                  'source_stable': True, 'native_runtime_passed': False,
                  'private_path': '/tmp/private/native.wav', 'ROM': [1, 2, 3],
                  'trace': 'protected trace', 'pcm': 'protected samples', 'native_gates': {'RAM': [4]}}
        public = pairs.public_manifest(report)
        self.assertFalse(public.get('fidelity_approved', True))
        self.assertFalse(public.get('native_loop_verified', True))
        self.assertFalse(public.get('native_runtime_passed', True))
        encoded = json.dumps(public)
        self.assertNotIn('/tmp/private', encoded)
        self.assertNotIn('protected', encoded)
        self.assertNotIn('ROM', public)


if __name__ == '__main__':
    unittest.main()
