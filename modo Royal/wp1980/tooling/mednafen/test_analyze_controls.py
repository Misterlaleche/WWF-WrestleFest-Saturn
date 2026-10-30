#!/usr/bin/env python3
"""Small synthetic regressions for the read-only controller report.

Fixtures are observation records, never emulator state. They exercise acceptance
boundaries (sampling gaps, combat states, reset markers and partial chords).
"""
from copy import deepcopy
import gzip
import json
from pathlib import Path
import tempfile
import unittest

from analyze_controls import analyze_capture, analyze_data


def actor_bytes(*, x=576, y=300, state=1, direction=0, buttons=0):
    raw = bytearray(268)
    raw[0] = 0x80
    raw[2:4] = (5).to_bytes(2, 'big')
    raw[6:10] = round(x * 65536).to_bytes(4, 'big', signed=True)
    raw[10:14] = round(y * 65536).to_bytes(4, 'big', signed=True)
    raw[0x20:0x22] = state.to_bytes(2, 'big')
    raw[0xa6:0xa8] = buttons.to_bytes(2, 'big')
    raw[0xa8:0xaa] = direction.to_bytes(2, 'big')
    return raw.hex()


def sample(clock, frame, *, pad=0, input0=255, demo=0, start=90, stage=3,
           draws=None, ready=1, actors=None, **actor_fields):
    row = dict(video_clock=clock, game_frame=frame, wp1980_draws=frame if draws is None else draws,
               wp1980_started=1, wp1980_demo_active=demo, pad_connected=1,
               pad_level=pad, input0=input0, start_clock=start, royal_stage=stage,
               reset_armed=0 if pad == 0x0f00 else 1, frame_ready=ready,
               frame_stage=ready, fatal_owner=0, fatal_dependency=0, fatal_frame=0)
    for slot in range(9):
        row['actor' + str(slot)] = (actors if actors is not None else actor_bytes(**actor_fields))
    guard = {key: row[key] for key in ('video_clock', 'game_frame', 'wp1980_draws',
             'wp1980_started', 'frame_ready', 'frame_stage')}
    row.update(boundary_stable=True, boundary_before=guard.copy(), boundary_after=guard.copy())
    return row


def event(field, *, press=(), release=()):
    return dict(native_clock=100 + field, relative_field=field,
                press=list(press), release=list(release), host_elapsed_seconds=field / 60)


def left_fixture():
    rows = [sample(101, 1, demo=1), sample(102, 2, demo=1)]
    rows += [sample(100 + frame, frame, x=576 - frame, pad=0x4000, input0=0xfd, direction=2)
             for frame in range(3, 8)]
    rows.append(sample(108, 8, x=569))
    actions = dict(scenario='movement-reset', ranges=[dict(button='left', first=3, last=7)])
    events = [event(3, press=['left']), event(8, release=['left'])]
    report = dict(status='PASS', controller_start_clock=100)
    return rows, actions, events, report


def reset_fixture():
    chord = ['a', 'b', 'c', 'start']
    rows = [sample(100 + frame, frame, demo=1) for frame in range(1, 4)]
    rows += [sample(104, 4, pad=0x0f00),
             sample(105, 0, pad=0x0f00, draws=4, stage=1, ready=0),
             sample(106, 0, pad=0x0f00, draws=4, stage=2, ready=0),
             sample(107, 0, pad=0x0f00, draws=4, start=107, ready=0),
             sample(108, 1, pad=0x0f00, draws=5, start=107),
             sample(109, 2, pad=0x0f00, draws=6, start=107),
             sample(116, 9, draws=13, start=107),
             sample(117, 10, draws=14, start=107)]
    actions = dict(scenario='reset-first', ranges=[dict(button=b, first=4, last=15) for b in chord])
    events = [event(4, press=chord), event(16, release=chord)]
    report = dict(status='PASS', controller_start_clock=100)
    return rows, actions, events, report


class ControllerAnalysisTests(unittest.TestCase):
    def test_qualified_consecutive_walking_and_single_input(self):
        result = analyze_data(*left_fixture())
        self.assertEqual(result['status'], 'PASS')
        self.assertEqual(result['buttons']['left']['expected_single_input0'], '0xfd')
        motion = result['pulses'][0]['movement']
        self.assertEqual(motion['status'], 'PASS')
        self.assertEqual(motion['qualifying_pairs'], 4)
        self.assertEqual(motion['axis_pixels_per_native_update']['mean'], -1)
        self.assertEqual(motion['total_dx_pixels'], -4)
        self.assertEqual(result['manual_takeover']['status'], 'PASS')

    def test_motion_does_not_interpolate_gaps_or_count_combat_states(self):
        fixture = list(left_fixture())
        fixture[0] = [row for row in fixture[0] if row['game_frame'] % 2]
        result = analyze_data(*fixture)
        self.assertEqual(result['buttons']['left']['status'], 'PASS')
        self.assertEqual(result['pulses'][0]['movement']['qualifying_pairs'], 0)
        self.assertEqual(result['pulses'][0]['movement']['status'], 'INCONCLUSIVE')
        fixture = list(left_fixture())
        for row in fixture[0]:
            if row['pad_level']:
                row['actor0'] = actor_bytes(state=11, direction=2, x=576 - row['game_frame'])
        result = analyze_data(*fixture)
        self.assertEqual(result['buttons']['left']['status'], 'PASS')
        self.assertEqual(result['pulses'][0]['movement']['qualifying_pairs'], 0)
        self.assertGreater(result['pulses'][0]['movement']['rejected']['not_free_walking_state1'], 0)

    def test_unstable_guard_rejected_and_old_single_sample_phase_inconclusive(self):
        fixture = list(left_fixture())
        for row in fixture[0]:
            row['boundary_before'].pop('frame_ready')
            row['boundary_before'].pop('frame_stage')
            row['boundary_after'].pop('frame_ready')
            row['boundary_after'].pop('frame_stage')
        result = analyze_data(*fixture)
        self.assertEqual(result['buttons']['left']['status'], 'PASS')
        self.assertEqual(result['pulses'][0]['movement']['qualifying_pairs'], 0)
        for row in fixture[0]:
            if row['pad_level']:
                row['boundary_after']['video_clock'] += 1
                row['boundary_stable'] = False
        result = analyze_data(*fixture)
        self.assertEqual(result['buttons']['left']['status'], 'INCONCLUSIVE')

    def test_reset_first_requires_full_first_mask_and_reload_progress(self):
        result = analyze_data(*reset_fixture())
        self.assertEqual(result['status'], 'PASS')
        reset = result['resets'][0]
        self.assertEqual(reset['first_native_input_is_full_chord'], 'PASS')
        self.assertEqual(reset['observed_new_start_clocks'], [107])
        self.assertTrue(reset['game_frame_decrease_observed'])
        self.assertEqual(reset['resumed_frame_range'], [1, 10])
        fixture = list(reset_fixture())
        fixture[0].insert(3, sample(104, 4, pad=0x0400, input0=0xef))
        result = analyze_data(*fixture)
        self.assertEqual(result['resets'][0]['status'], 'PASS')
        self.assertEqual(result['resets'][0]['first_native_input_is_full_chord'], 'INCONCLUSIVE')
        self.assertEqual(result['status'], 'INCONCLUSIVE')
        fixture = list(reset_fixture())
        fixture[0] = [row for row in fixture[0] if row['royal_stage'] == 3]
        result = analyze_data(*fixture)
        self.assertEqual(result['resets'][0]['status'], 'INCONCLUSIVE')

    def test_reversion_and_fatal_are_explicit_failures(self):
        fixture = list(left_fixture())
        fixture[0][-1]['wp1980_demo_active'] = 1
        result = analyze_data(*fixture)
        self.assertEqual(result['manual_takeover']['status'], 'FAIL')
        self.assertEqual(result['status'], 'FAIL')
        fixture = list(left_fixture())
        fixture[0][-1]['fatal_owner'] = 7
        result = analyze_data(*fixture)
        self.assertEqual(result['fatal']['status'], 'FAIL')
        self.assertEqual(result['fatal']['first_fatal']['fatal_owner'], 7)

    def test_incomplete_capture_and_malformed_ninth_actor_cannot_pass(self):
        fixture = left_fixture()
        self.assertEqual(analyze_data(*fixture, complete=False)['status'], 'INCONCLUSIVE')
        bad = deepcopy(fixture)
        bad[0][-1]['actor8'] = '00'
        result = analyze_data(*bad)
        self.assertEqual(result['status'], 'INCONCLUSIVE')
        self.assertEqual(result['actor_records']['malformed_or_missing_samples'], {'actor8': 1})

    def test_file_reader_handles_truncated_metadata_without_touching_inputs(self):
        rows, actions, events, report = left_fixture()
        with tempfile.TemporaryDirectory(prefix='wp1980-controller-analyzer-') as temporary:
            directory = Path(temporary)
            with gzip.open(directory / 'samples.jsonl.gz', 'wt') as output:
                for row in rows:output.write(json.dumps(row) + '\n')
            for name, value in [('actions', actions), ('input-events', events), ('report', report)]:
                (directory / (name + '.json')).write_text(json.dumps(value))
            before = {path.name: path.read_bytes() for path in directory.iterdir()}
            self.assertEqual(analyze_capture(directory)['status'], 'PASS')
            self.assertEqual(before, {path.name: path.read_bytes() for path in directory.iterdir()})
            (directory / 'report.json').write_text('{"status":')
            result = analyze_capture(directory)
            self.assertEqual(result['status'], 'INCONCLUSIVE')
            self.assertFalse(result['capture_complete'])
            self.assertIn(str(directory / 'report.json'), result['json_read_errors'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
