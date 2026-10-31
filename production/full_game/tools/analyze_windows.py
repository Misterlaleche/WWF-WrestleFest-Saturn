#!/usr/bin/env python3
"""Measure requested windows from native counters and verified original video PTS.

Does not interpolate missing endpoints or use host execution time. Time windows
cover the smallest whole number of physical fields spanning 1 or 5 seconds;
the actual duration and observation coverage are always reported explicitly.
"""
import argparse
from collections import defaultdict
from fractions import Fraction
import gzip
import json
import math
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    folder = args.capture
    report = json.loads((folder / 'report.json').read_text())
    pts = json.loads((folder / 'video-pts-check.json').read_text())
    if report['status'] != 'PASS' or pts['status'] != 'PASS' or not pts['all_video_pts_identical']:
        raise SystemExit('Execution identity and video timestamp checks must pass first')
    rate = Fraction(pts['mean_recorded_field_rate_fraction'])
    terminal = report['last_sample']
    total = terminal['finished_frame']
    if not total or terminal['finished_presentation'] != 2:
        raise SystemExit('A completed match and completed result presentation are required')
    draws, fields = defaultdict(set), defaultdict(set)
    with gzip.open(folder / 'samples.jsonl.gz', 'rt') as stream:
        for line in stream:
            row = json.loads(line)
            draw = row.get('wp1980_draws', -1)
            if (row.get('wp1980_started') == 1 and row.get('boundary_stable')
                    and row.get('game_frame') == draw and 1 <= draw <= total):
                field = row['wp1980_vblanks']
                draws[draw].add(field)
                fields[field].add(draw)
    points = {key: next(iter(values)) for key, values in draws.items() if len(values) == 1}
    field_points = {key: next(iter(values)) for key, values in fields.items() if len(values) == 1}
    update_windows, time_windows = [], []
    for length in (60, 99):
        candidates = []
        for first in range(1, total - length + 1):
            last = first + length
            if first not in points or last not in points:
                continue
            elapsed = points[last] - points[first]
            if elapsed <= 0:
                raise SystemExit('Non-increasing native draw timestamps')
            candidates.append({'first_update': first, 'last_update': last,
                               'updates': length, 'fields': elapsed,
                               'seconds': float(elapsed / rate),
                               'updates_per_second': float(length * rate / elapsed)})
        update_windows.append({'updates': length, 'possible_windows': total - length,
                               'observed_windows': len(candidates),
                               'worst_observed': min(candidates, key=lambda x:x['updates_per_second']) if candidates else None})
    for seconds in (1, 5):
        span = math.ceil(seconds * rate)
        candidates = []
        for first, first_draw in field_points.items():
            if first + span not in field_points:
                continue
            updates = field_points[first + span] - first_draw
            if updates < 0:
                raise SystemExit('Non-increasing native update counter')
            candidates.append({'first_field': first, 'last_field': first + span,
                               'first_update': first_draw, 'last_update': first_draw + updates,
                               'updates': updates, 'updates_per_second': float(updates * rate / span)})
        time_windows.append({'requested_seconds': seconds, 'physical_fields': span,
                             'actual_seconds': float(span / rate),
                             'unambiguous_field_endpoints': len(field_points),
                             'observed_windows': len(candidates),
                             'worst_observed': min(candidates, key=lambda x:x['updates_per_second']) if candidates else None})
    output = {'scope':'Independent Royal reconstruction, without audio; not integrated universal game',
              'field_rate_fraction': str(rate), 'full_match_updates': total,
              'full_match_fields': terminal['finished_vblanks'],
              'mean_updates_per_second': float(total * rate / terminal['finished_vblanks']),
              'update_windows': update_windows, 'time_windows': time_windows,
              'limits':'Asynchronous read-only samples; ambiguous/missing endpoints excluded, never interpolated. Worst observed windows are not a guarantee of the absolute worst window.',
              'graphics_preparation_upload_timing':'OPEN: not provided by these native counters'}
    (folder / 'requested-windows.json').write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps(output,indent=2))

if __name__ == '__main__':
    main()
