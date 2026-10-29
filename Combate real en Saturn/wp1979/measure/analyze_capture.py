#!/usr/bin/env python3
"""Summarize native game progress against emulated Saturn VBlanks."""
import argparse
import collections
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('directory', type=Path)
p.add_argument('--warmup-frames', type=int, default=30)
p.add_argument('--output', type=Path)
a = p.parse_args()
assert a.warmup_frames >= 0
report = json.loads((a.directory / 'report.json').read_text())
samples = report['samples']
started = [s for s in samples if s.get('wp1979_started')]
result = dict(emulator=report['backend'], firmware=report['firmware'],
              hardware_test=False, video_rate=report['video_rate'],
              wall_seconds=report['wall_seconds'],
              emulated_seconds=report['emulated_seconds'],
              capture_status=report['status'],
              fatal=[s for s in samples if s.get('fatal_owner')][:1])
if started:
    first = started[0]
    terminal = next((s for s in started if s.get('finished_frame')), started[-1])
    window = [s for s in started if s['emulator_frame'] <= terminal['emulator_frame']]
    warm = next((s for s in window if s['game_frame'] >= a.warmup_frames), first)
    clocks = terminal['video_clock'] - warm['video_clock']
    frames = terminal['game_frame'] - warm['game_frame']
    draws = terminal['wp1979_draws'] - warm['wp1979_draws']
    hist = collections.Counter()
    previous = warm
    for s in window:
        if s['emulator_frame'] <= warm['emulator_frame']:
            continue
        if s['wp1979_draws'] != previous['wp1979_draws']:
            delta = s['wp1979_draws'] - previous['wp1979_draws']
            if delta == 1:
                hist[s['video_clock'] - previous['video_clock']] += 1
            previous = s
    result.update(
        boot_emulator_frames=first['emulator_frame'],
        first_game_frame=first['game_frame'], last_game_frame=terminal['game_frame'],
        measured_start_game_frame=warm['game_frame'],
        measured_native_frames=frames, measured_draws=draws,
        measured_vblanks=clocks,
        native_frames_per_second=frames * report['video_rate'] / clocks if clocks else None,
        completed_draws_per_second=draws * report['video_rate'] / clocks if clocks else None,
        draw_vblank_gap_histogram=dict(sorted(hist.items())),
        maximum_recorded_gap=terminal['wp1979_max_gap'],
        combat_cd_windows=(terminal['window_misses'] - first['window_misses'])
            if 'window_misses' in terminal and 'window_misses' in first else None,
        combat_cd_sectors=(terminal['window_sectors'] - first['window_sectors'])
            if 'window_sectors' in terminal and 'window_sectors' in first else None,
        finished_frame=terminal.get('finished_frame', 0),
        finished_result=terminal.get('finished_result', 0),
        finished_presentation=samples[-1].get('finished_presentation', 0),
    )
    # Use completed native steps over actual VBlank windows. This exposes
    # short slow periods hidden by a whole-match average.
    rolling = {}
    eligible = [s for s in window if s['emulator_frame'] >= warm['emulator_frame']]
    by_clock = {s['video_clock']: s for s in eligible}
    for span in (60, 300, 600):
        candidates = []
        for s in eligible:
            end = by_clock.get(s['video_clock'] + span)
            if end is not None:
                steps = end['game_frame'] - s['game_frame']
                candidates.append((steps * report['video_rate'] / span,
                                   s['game_frame'], end['game_frame'], s['emulator_frame']))
        if candidates:
            rate, begin, end, emulator_frame = min(candidates)
            rolling[str(span)] = dict(minimum_fps=rate, start_game_frame=begin,
                                      end_game_frame=end, emulator_frame=emulator_frame)
    result['rolling_vblank_windows'] = rolling
    if 'actor0' in first:
        actors = {}
        legal_changes = []
        last_roles = None
        for s in window:
            roles = []
            for slot in (0, 1, 4, 5):
                b = bytes.fromhex(s[f'actor{slot}'])
                word = lambda off: int.from_bytes(b[off:off+2], 'big')
                actor = actors.setdefault(str(slot), dict(ids=set(), active_samples=0,
                                                           legal_samples=0, first_position=None))
                actor['ids'].add(word(2))
                actor['active_samples'] += bool(b[0] & 128)
                actor['legal_samples'] += bool(b[0x33] & 1)
                if actor['first_position'] is None:
                    actor['first_position'] = [word(6), word(10)]
                if b[0x33] & 1:
                    roles.append(slot)
            if roles != last_roles:
                legal_changes.append(dict(observed_completed_frame_counter=s['game_frame'],
                                          emulator_frame=s['emulator_frame'],
                                          video_seconds=s['emulator_frame'] / report['video_rate'],
                                          legal_slots=roles))
                last_roles = roles
        for actor in actors.values():
            actor['ids'] = sorted(actor['ids'])
        result['actors'] = actors
        result['legal_participant_changes'] = legal_changes
else:
    result['last_observation'] = samples[-1]
(a.output or a.directory / 'analysis.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
