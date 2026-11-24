#!/usr/bin/env python3
"""Acceptance reports must expose slow windows and avoid false endpoint proof."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('capture_analysis', ROOT / 'tools/analyze_capture.py')
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)


def point(tick, field, stage=10, **extra):
    return dict(stage=stage, ticks=tick, fields=field, live_field=field,
                start_field=0, finish_field=0, debt=0, max_debt=0,
                draws=tick, video={}, audio={}, **extra)


class CaptureAnalysisTests(unittest.TestCase):
    def frontend_packet(self, folder, mode='royal', complete=1):
        fixture = {'tag': (9666, 0x11b6, 0x6e0f91a0, 4510),
                   'royal': (14409, 0x1afc, 0x9f7d9dae, 1771)}[mode]
        frame, owner, rng, frontend_ticks = fixture
        actors = (ROOT/'evidence'/('frontend-'+mode+'-terminal-actors.bin')).read_bytes()
        (folder/('frontend-'+mode+'-terminal-actors.bin')).write_bytes(actors)
        header = [0x57465431, 1, len(actors), frame, 20000, int(mode == 'royal'),
                  owner, rng, 1000, frontend_ticks, 2, 123, 20001, 20105,
                  frontend_ticks+97, complete]
        ram = bytearray(0x100000)
        struct.pack_into('>16I', ram, 0, *header)
        ram[64:64+len(actors)] = actors
        (folder/'WorkRAML-state.bin').write_bytes(ram)
        layout = {'values': dict(actors_bytes=2412, actor_stride=268, actor_count=9)}
        return ram, header, layout

    def test_clock_is_physical_and_rational(self):
        value = analysis.rate(37609, 39168)
        self.assertAlmostEqual(value['arcade_speed_percent'], 100.0)
        self.assertAlmostEqual(value['ticks_per_second'], 57.44487018813895)

    def test_slow_window_cannot_be_hidden_by_a_later_fast_window(self):
        trace = [point(t, t * 2 if t <= 120 else 240 + (t - 120) // 2) for t in range(0, 1201, 10)]
        trace[60]['debt'] = 37
        trace[60]['max_debt'] = 37
        segment = analysis.combat_segments(trace, (120, 600))[0]
        self.assertLess(segment['windows']['120']['minimum_percent'], 97)
        self.assertGreater(segment['windows']['120']['maximum_percent'], 190)
        self.assertGreater(segment['from_recorded_start']['arcade_speed_percent'], 100)
        self.assertEqual(segment['debt']['observed_peak'], 37)
        self.assertNotIn('accepted', segment)

    def test_terminal_transition_uses_live_field_after_finish_freezes(self):
        before = point(8536, 999)
        first = point(8537, 1000, 20)
        first.update(finish_field=1000, live_field=1001)
        waiting = dict(first, live_field=1133)
        end = dict(first, stage=21, live_field=1138, origin='save_state')
        result = analysis.result_transitions([before, first, waiting, end])[0]
        self.assertEqual(result['duration_fields_bounds'], [133, 138])
        self.assertEqual(result['finish_field'], 1000)
        self.assertEqual(result['completion_source'], 'save_state')

    def test_frontend_stage22_closes_combat_at_recorded_finish(self):
        before = point(14400, 19990)
        after = point(14409, 20105, 22)
        after.update(finish_field=20000, start_field=0)
        segment = analysis.combat_segments([before, after], (120,))[0]
        self.assertTrue(segment['completed_combat_boundary_observed'])
        self.assertEqual(segment['from_recorded_start']['fields'], 20000)
        self.assertEqual(segment['last']['field'], 20000)

    def test_frontend_actor_snapshot_survives_owner_mutation_and_uses_exact_fields(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            _, _, layout = self.frontend_packet(folder)
            # WorkRAMH/current rt is deliberately absent: the immutable WFT1
            # copy, not the later result owner's actors, is authoritative.
            result = analysis.inspect_frontend_terminal(folder, layout, folder)
            self.assertEqual(result['status'], 'MATCH')
            self.assertEqual(result['source'], 'WorkRAML/WFT1')
            self.assertEqual(result['header']['combat_ticks'], 14409)
            self.assertEqual(result['combat']['fields'], 19000)
            self.assertEqual(result['transition']['fields'], 104)
            self.assertEqual(result['transition']['arcade_ticks'], 97)
            self.assertEqual(result['transition']['terminal_to_owner_fields'], 1)

    def test_incomplete_post_owner_does_not_invalidate_published_actors(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            _, _, layout = self.frontend_packet(folder, complete=0)
            result = analysis.inspect_frontend_terminal(folder, layout, folder)
            self.assertEqual(result['status'], 'MATCH')
            self.assertEqual(result['transition']['status'], 'INCOMPLETE')
            self.assertNotIn('fields', result['transition'])

    def test_frontend_packet_must_be_published_complete_and_bounded(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            original, header, layout = self.frontend_packet(folder)
            for index, bad in [(0, 0), (1, 2), (2, 0xfffffff0), (5, 2), (15, 2)]:
                ram = original.copy()
                struct.pack_into('>I', ram, index*4, bad)
                (folder/'WorkRAML-state.bin').write_bytes(ram)
                result = analysis.inspect_frontend_terminal(folder, layout, folder)
                self.assertNotEqual(result['status'], 'MATCH', (index, result))
                self.assertNotIn('equal', result)
            (folder/'WorkRAML-state.bin').write_bytes(original[:64+header[2]-1])
            self.assertEqual(analysis.inspect_frontend_terminal(folder, layout, folder)['status'], 'INVALID_HEADER')

    def test_frontend_endpoint_and_reference_hash_are_not_guessed(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            original, _, layout = self.frontend_packet(folder)
            for index, bad in [(3, 8474), (6, 0x1b4e), (9, 4510)]:
                ram = original.copy()
                struct.pack_into('>I', ram, index*4, bad)
                (folder/'WorkRAML-state.bin').write_bytes(ram)
                result = analysis.inspect_frontend_terminal(folder, layout, folder)
                self.assertEqual(result['status'], 'NOT_REFERENCE_ENDPOINT')
                self.assertNotIn('equal', result)
            (folder/'WorkRAML-state.bin').write_bytes(original)
            reference = folder/'frontend-royal-terminal-actors.bin'
            reference.write_bytes(bytes(2412))
            self.assertEqual(analysis.inspect_frontend_terminal(folder, layout, folder)['status'], 'REFERENCE_INVALID')

    def test_frontend_comparison_reports_actor_and_rng_corruption(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            ram, _, layout = self.frontend_packet(folder)
            ram[64+270] ^= 1
            struct.pack_into('>I', ram, 7*4, 0)
            (folder/'WorkRAML-state.bin').write_bytes(ram)
            result = analysis.inspect_frontend_terminal(folder, layout, folder)
            self.assertEqual(result['status'], 'MISMATCH')
            self.assertFalse(result['rng_equal'])
            self.assertEqual(result['differences'][0]['slot'], 1)
            self.assertEqual(result['differences'][0]['actor_offset'], 2)

    def test_unobserved_frontend_boundary_is_left_unknown(self):
        trace = [dict(point(0, 20, 30), frontend={'phase': 2, 'pc': 0x8302,
                    'arcade_ticks': 1169, 'physical_fields': 1219})]
        result = analysis.frontend_summary(trace)
        self.assertIsNone(result['title_hold_first_observation'])
        trace.append(dict(point(0, 24, 30), frontend={'phase': 2, 'pc': 0x83A4,
                    'arcade_ticks': 1173, 'physical_fields': 1223}))
        result = analysis.frontend_summary(trace)
        self.assertEqual(result['title_hold_first_observation']['arcade_ticks'], 1173)
        self.assertEqual(result['title_hold_entry_bounds']['arcade_ticks'], [1169, 1173])
        self.assertNotIn('exact_intro_ticks', result)

    def test_zero_frontend_diagnostic_in_a_direct_build_is_not_a_frontend(self):
        self.assertFalse(analysis.frontend_active({'frontend': dict.fromkeys(analysis.FRONT_FIELDS, 0)}))
        self.assertTrue(analysis.frontend_active({'frontend': {'phase': 1, 'pc': 0x654}}))

    def test_snapshot_boundary_is_inserted_into_the_correct_match(self):
        trace = [dict(point(100, 1000), start_field=100),
                 dict(point(9000, 19000), start_field=100),
                 dict(point(0, 21000), start_field=20900)]
        terminal = {'status': 'MATCH', 'header': dict(combat_start_field=100,
                    terminal_field=20000, combat_ticks=9666, pending=2)}
        merged = analysis.with_frontend_boundary(trace, terminal)
        self.assertEqual(merged[2]['finish_field'], 20000)
        result = analysis.combat_segments(merged, (120,))
        self.assertEqual(len(result), 2)
        self.assertTrue(result[0]['completed_combat_boundary_observed'])
        self.assertFalse(result[1]['completed_combat_boundary_observed'])

    def test_missing_physical_counter_is_not_replaced_with_wall_seconds(self):
        trace = [{'stage': 10, 'ticks': 0, 'wall_seconds': 0},
                 {'stage': 10, 'ticks': 600, 'wall_seconds': 1}]
        self.assertEqual(analysis.combat_segments(trace, (120,)), [])

    def test_coin_cost_uses_recorded_calls_and_irqs_without_inventing_a_profile(self):
        self.assertEqual(analysis.coin_profile_summary([])['status'], 'NOT_OBSERVED')
        trace = [dict(point(300, 497), coin_profile=dict(calls=302, irq2s=8109,
                 total_frt=1412256, max_frt=30889, total_ms=1686, max_ms=36,
                 snapshots=289, last_frt=2816))]
        value = analysis.coin_profile_summary(trace)
        self.assertEqual(value['recorded_irq_periods'], 477)
        self.assertAlmostEqual(value['mean_ms_per_call'], 1686 / 302)
        self.assertAlmostEqual(value['mean_ms_per_irq_period'], 1686 / 477)
        self.assertEqual(value['last']['max_ms'], 36)
        self.assertNotIn('owner_only_ms', value)

    def test_actor_comparison_requires_exact_saved_terminal(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            layout = {'values': dict(runtime_bytes=4096, actors_offset=128,
                      actors_bytes=2412, actor_stride=268, actor_count=9,
                      frame_counter_offset=16, mode_offset=32, frame_stage_offset=4090)}
            ram = bytearray(0x100000)
            syms = {'wf_jo_diagnostic': 0x06000020, 'rt': 0x06002000,
                    'jo_core_vblank_count': 0x060000A0}
            actors = bytes((i * 17) & 255 for i in range(2412))
            ram[0x2080:0x2080 + 2412] = actors
            (folder / 'tag-terminal-actors.bin').write_bytes(actors)
            for name, value in {'magic': 0x574A3031, 'stage': 10, 'ticks': 8537,
                                'finish_field': 9000}.items():
                struct.pack_into('>I', ram, 0x20 + analysis.DIAGNOSTIC_FIELDS.index(name) * 4, value)
            struct.pack_into('>I', ram, 0x2010, 8537)
            struct.pack_into('>I', ram, 0xA0, 9138)
            (folder / 'WorkRAMH-state.bin').write_bytes(ram)
            result, snapshot = analysis.inspect_state({'symbols': syms}, folder, layout, folder)
            self.assertEqual(result['status'], 'NOT_TERMINAL')
            self.assertNotIn('equal', result)
            self.assertEqual(snapshot['live_field'], 9138)
            struct.pack_into('>I', ram, 0x24, 21)
            (folder / 'WorkRAMH-state.bin').write_bytes(ram)
            result, _ = analysis.inspect_state({'symbols': syms}, folder, layout, folder)
            self.assertEqual(result['status'], 'MATCH')
            ram[0x2080 + 270] ^= 1
            (folder / 'WorkRAMH-state.bin').write_bytes(ram)
            result, _ = analysis.inspect_state({'symbols': syms}, folder, layout, folder)
            self.assertEqual(result['status'], 'MISMATCH')
            self.assertEqual(result['differences'][0]['slot'], 1)
            self.assertEqual(result['differences'][0]['actor_offset'], 2)
            struct.pack_into('>I', ram, 0x2010, 8538)
            (folder / 'WorkRAMH-state.bin').write_bytes(ram)
            result, _ = analysis.inspect_state({'symbols': syms}, folder, layout, folder)
            self.assertEqual(result['status'], 'NOT_TERMINAL')


if __name__ == '__main__':
    unittest.main()
