#!/usr/bin/env python3
"""Compare observed Saturn fields across identical sampled gameplay ticks.

Use the earliest common tick at/after --from-tick and the latest common tick
at/before --to-tick. Endpoints are real first observations, never interpolated
or selected for a favorable timing result. Captures are asynchronous, so small
differences near one field require a complete-run measurement.
"""
import argparse
import json
from pathlib import Path
import analyze_capture as capture


def points(folder):
    path = folder/'report.json' if folder.is_dir() else folder
    report = json.loads(path.read_text())
    assert report.get('status') == 'CAPTURED', 'A completed cold capture is required'
    assert report.get('cold_boot') and not report.get('guest_writes') and not report.get('state_loaded')
    result = {}
    for point in report['trace']:
        if point.get('stage') == 10 and 'live_field' in point:
            result.setdefault(point['ticks'], point)
    return path, report, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', required=True, type=Path)
    parser.add_argument('--after', required=True, type=Path)
    parser.add_argument('--from-tick', type=int, default=300)
    parser.add_argument('--to-tick', type=int, default=3000)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    before, after = points(args.before), points(args.after)
    shared = sorted(t for t in before[2].keys() & after[2].keys() if args.from_tick <= t <= args.to_tick)
    assert len(shared) >= 2 and shared[-1]-shared[0] >= (args.to_tick-args.from_tick)*0.9
    first, last = shared[0], shared[-1]
    result = {'scope': __doc__, 'requested_ticks': [args.from_tick, args.to_tick],
              'observed_common_ticks': [first, last], 'common_observations': len(shared),
              'hardware_verified': False}
    for key, data in [('before', before), ('after', after)]:
        path, report, samples = data
        a, b = samples[first], samples[last]
        fields = capture.delta(a['live_field'], b['live_field'])
        result[key] = {'report': str(path), 'report_sha256': capture.sha(path),
                       'elf_sha256': report['elf_sha256'],
                       'start': {k: a.get(k) for k in ('ticks', 'live_field', 'debt', 'video', 'slave_jobs')},
                       'end': {k: b.get(k) for k in ('ticks', 'live_field', 'debt', 'video', 'slave_jobs')},
                       'timing': capture.rate(last-first, fields)}
    a, b = result['before']['timing']['fields'], result['after']['timing']['fields']
    result['fields_saved'] = a-b
    result['elapsed_reduction_percent'] = (a-b)*100/a
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: result[k] for k in ('observed_common_ticks', 'fields_saved', 'elapsed_reduction_percent')}, indent=2))
    print(json.dumps({k: result[k]['timing'] for k in ('before', 'after')}, indent=2))


if __name__ == '__main__':
    main()
