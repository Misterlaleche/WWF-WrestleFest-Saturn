#!/usr/bin/env python3
"""Analyze native counters using actual standalone MOV timestamp calibration.

Read-only offline analysis. Observed draw endpoints are not interpolated. A
different capture may supply the calibration for a --no-movie run; that reuse
is explicit in the report. Host elapsed time never determines native cadence.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from fractions import Fraction
import gzip
import hashlib
import json
from pathlib import Path


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1048576), b''):
            digest.update(block)
    return digest.hexdigest()


def scalar(row):
    return {name: row.get(name) for name in (
        'video_clock', 'start_clock', 'game_frame', 'wp1980_draws',
        'wp1980_vblanks', 'finished_frame', 'finished_vblanks',
        'finished_presentation', 'finished_presentation_vblanks',
        'window_misses', 'window_sectors', 'fatal_owner', 'fatal_dependency',
        'rng_state')}


def window(first, last, points, rate):
    if first not in points or last not in points:
        return {'draws_first': first, 'draws_last': last,
                'status': 'MISSING_OBSERVED_ENDPOINT'}
    fields = points[last] - points[first]
    if fields <= 0:
        raise ValueError('Native completed-draw fields are not increasing')
    updates = last - first
    return dict(draws_first=first, draws_last=last, updates=updates,
                vblanks=fields, updates_per_field=updates / fields,
                updates_per_second=float(Fraction(updates, fields) * rate))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--pts-check', type=Path)
    parser.add_argument('--reference', type=Path,
                        help='Earlier native execution report, used only for terminal state comparison')
    parser.add_argument('--expected-final-frame', type=int, default=8474)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    folder = args.capture.resolve()
    capture_report = json.loads((folder / 'report.json').read_text())
    if capture_report.get('status') != 'PASS':
        raise ValueError('Capture identity/execution report must pass first')
    pts_path = (args.pts_check or folder / 'video-pts-check.json').resolve()
    pts = json.loads(pts_path.read_text())
    if pts.get('status') != 'PASS' or not pts.get('all_video_pts_identical'):
        raise ValueError('Timestamp calibration must have a verified MOV-to-MP4 PTS vector')
    rate = Fraction(pts['mean_recorded_field_rate_fraction'])
    with gzip.open(folder / 'samples.jsonl.gz', 'rt') as stream:
        rows = [json.loads(line) for line in stream]
    rows.append(capture_report['last_sample'])
    live = [row for row in rows if row.get('wp1980_started') == 1]
    terminal = capture_report['last_sample']
    expected = args.expected_final_frame
    completed = [row for row in live if row.get('finished_frame') == expected
                 and row.get('finished_vblanks', 0) > 0
                 and row.get('wp1980_draws') == expected
                 and row.get('game_frame') == expected]
    if not live or not completed:
        raise ValueError('No completed native match with the requested final frame')
    begin, gameover = live[0], completed[0]
    total_fields = terminal['finished_vblanks']
    total_updates = terminal['finished_frame']
    stamps = defaultdict(set)
    for row in live:
        draw = row.get('wp1980_draws', -1)
        if (row.get('boundary_stable') and draw == row.get('game_frame')
                and 1 <= draw <= expected
                and row.get('start_clock') == begin.get('start_clock')):
            stamps[draw].add(row['wp1980_vblanks'])
    conflicts = {draw: sorted(values) for draw, values in stamps.items() if len(values) != 1}
    # Exclude any asynchronous torn counter pair; never select a convenient
    # timing value or interpolate an unobserved draw.
    points = {draw: next(iter(values)) for draw, values in stamps.items() if len(values) == 1}
    sliding = []
    for updates in (59, 99, 499, 999):
        candidates = [window(first, first + updates, points, rate)
                      for first in range(1, expected - updates + 1)
                      if first in points and first + updates in points]
        worst = min(candidates, key=lambda item: item['updates_per_second']) if candidates else None
        sliding.append(dict(updates=updates, possible_windows=expected-updates,
                            observed_endpoint_windows=len(candidates), worst_observed=worst))
    comparison = None
    if args.reference:
        reference = json.loads(args.reference.read_text())
        ref = reference.get('last_sample')
        if ref is None:
            ref = reference['samples'][-1]
        names = ['rng_state'] + [f'actor{i}' for i in range(9)]
        matches = {name: terminal.get(name) == ref.get(name)
                   and terminal.get(name) is not None for name in names}
        comparison = dict(path=str(args.reference.resolve()), sha256=sha(args.reference),
                          fields=matches, all_equal=all(matches.values()),
                          native_finished_frame_reference=ref.get('finished_frame'))
    fatal = {name: max(row.get(name, 0) for row in live)
             for name in ('fatal_owner', 'fatal_dependency')}
    cd_delta = {name: gameover[name] - begin[name]
                for name in ('window_misses', 'window_sectors')}
    combat = [row for row in live if row.get('finished_frame') == 0]
    cd_range = {name: [min(row[name] for row in combat), max(row[name] for row in combat)]
                for name in ('window_misses', 'window_sectors')}
    accepted = (total_updates == expected and terminal.get('finished_presentation') == 2
                and not any(fatal.values()) and not any(cd_delta.values())
                and (comparison is None or comparison['all_equal']))
    report = dict(
        status='PASS' if accepted else 'FAIL',
        scope='Standalone Mednafen native execution; no physical Saturn hardware claim',
        capture=str(folder), capture_report_sha256=sha(folder / 'report.json'),
        field_rate_calibration=dict(path=str(pts_path), sha256=sha(pts_path),
            reused_from_other_capture=pts_path.parent != folder,
            movie_recorded_for_this_capture=capture_report.get('movie_recorded', True),
            mean_recorded_field_rate=float(rate), fraction=str(rate),
            method='Recorded frame PTS span, not nominal container fps or host elapsed time'),
        full_match=dict(updates=total_updates, vblanks=total_fields,
            updates_per_field=total_updates / total_fields,
            native_seconds=float(Fraction(total_fields, 1) / rate),
            updates_per_second=float(Fraction(total_updates, total_fields) * rate)),
        first_windows=[window(1, last, points, rate) for last in (100, 500, 1000)],
        sliding_windows=sliding,
        endpoint_coverage=dict(observed_unambiguous_draws=len(points), possible_draws=expected,
            missing_or_conflicting_draws=[draw for draw in range(1, expected+1) if draw not in points],
            excluded_conflicting_stamps=conflicts,
            limitation='Asynchronous O_RDONLY samples. Worst windows cover observed endpoints only; no interpolation or instantaneous uniform-speed claim.'),
        first_sample=scalar(begin), gameover_sample=scalar(gameover), last_sample=scalar(terminal),
        fatal_max=fatal, combat_cd_delta=cd_delta, observed_preterminal_cd_ranges=cd_range,
        terminal_state_comparison=comparison,
        presentation=dict(completed=terminal.get('finished_presentation') == 2,
                          native_vblanks=terminal.get('finished_presentation_vblanks')),
        observer_stats=capture_report.get('observer_stats'),
        host_wall_seconds=capture_report.get('wall_seconds'),
        host_wall_note='Includes boot, host processing and optional video encoding; not used for cadence')
    output = args.output or folder / 'execution-analysis.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('status', 'full_match', 'first_windows',
          'sliding_windows', 'combat_cd_delta', 'terminal_state_comparison')}, indent=2))
    if not accepted:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
