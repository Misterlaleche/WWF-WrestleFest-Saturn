#!/usr/bin/env python3
"""Compare complete ordinary-input Royal/TAG logic and graphics traces."""
import argparse
import hashlib
import json
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', required=True, type=Path)
    parser.add_argument('--candidate', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    summaries = [json.loads((p/'summary.json').read_text()) for p in (args.baseline, args.candidate)]
    assert all(not s['sources_changed_during_run'] for s in summaries)
    result = {'scope': 'Complete native main with ordinary deterministic input; every command/pixel row and periodic full logic states. Native execution does not measure console speed.',
              'baseline_summary_sha256': digest(args.baseline/'summary.json'),
              'candidate_summary_sha256': digest(args.candidate/'summary.json'), 'routes': {}, 'equal': True}
    for mode in ('royal', 'tag'):
        route = 'frontend-'+mode
        a, b = args.baseline/route, args.candidate/route
        assert all(s['runs'][route]['returncode'] == 0 for s in summaries)
        names = sorted(p.name for p in a.glob('*-*.bin'))+['commands.csv']
        assert len(names) > 60, 'A full periodic snapshot trace is required'
        compared = {}
        for name in names:
            x, y = a/name, b/name
            same = y.is_file() and x.read_bytes() == y.read_bytes()
            compared[name] = {'equal': same, 'baseline_sha256': digest(x),
                              'candidate_sha256': digest(y) if y.is_file() else None,
                              'bytes': x.stat().st_size}
            result['equal'] &= same
        result['routes'][route] = {'files': compared,
                                   'snapshots': len(list(a.glob('*-state.json'))),
                                   'command_rows': len((a/'commands.csv').read_text().splitlines())-1,
                                   'terminal_frame': summaries[1]['runs'][route]['terminal']['frame']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({'equal': result['equal'], 'routes': {
        k: {n: v[n] for n in ('snapshots', 'command_rows', 'terminal_frame')}
        for k, v in result['routes'].items()}}, indent=2))
    return 0 if result['equal'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
