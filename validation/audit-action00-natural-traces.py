#!/usr/bin/env python3
"""Match observed HOST selector-$00 sequences to direct-ROM action timelines.

This checks the port's natural combat trace against the already verified
single-action oracle reports. It does not itself execute the arcade ROM.
"""
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'validation/local-20261026/action-interleaved'
ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'
FAMILY = ROOT / 'src/runtime/tests/r18/action_reversal_family_verify_rom.py'
BRIDGE = ROOT / 'src/runtime/tests/r18/action_reversal_family_oracle_bridge.c'
TRACES = [ROOT / 'validation/local-20261027/tag-pressure-after-f8d8/trace.csv',
          ROOT / 'validation/local-20261027/royal-pressure-after-kind2-wrap/trace.csv']
OUT = ROOT / 'validation/local-20261027/action00-natural-timeline-audit.json'
EXPECTED = {}
for name in ('base', 'alternate'):
    path = EVIDENCE / f'action00-{name}.json'
    report = json.loads(path.read_text())
    assert report['status'] == 'PASS' and report['cases'] == 12
    assert report['rom_sha1'] == ROM_SHA1
    assert report['idle_descriptor'] == name and report['idle_edge'] == 'center'
    assert report['test_sha256'] == hashlib.sha256(FAMILY.read_bytes()).hexdigest()
    assert report['bridge_sha256'] == hashlib.sha256(BRIDGE.read_bytes()).hexdigest()
    assert len({case['steps'] for case in report['sequence_outcomes']}) == 1
    history = report['sequence_outcomes'][0]['frame_history']
    assert all(case['frame_history'] == history for case in report['sequence_outcomes'])
    poses = []
    current = None
    for step in range(1, report['sequence_outcomes'][0]['steps'] + 1):
        for entry_step, _frame, pose in history:
            if entry_step == step:
                current = pose
        poses.append(current)
    EXPECTED[name] = (poses, hashlib.sha256(path.read_bytes()).hexdigest())

def active(row):
    return (row['kind'] == 'dispatch' and int(row['state'], 16) & 255 == 5
            and int(row['selector'], 16) & 255 == 0)

results = []
for path in TRACES:
    rows = defaultdict(list)
    with path.open(newline='') as stream:
        for row in csv.DictReader(stream):
            if row['kind'] == 'dispatch':
                rows[int(row['slot'])].append(row)
    groups = []
    for slot, stream in rows.items():
        current = []
        for row in stream:
            frame = int(row['frame'])
            if active(row) and (not current or frame == int(current[-1]['frame']) + 1):
                current.append(row)
                continue
            if current:
                groups.append((slot, current))
                current = []
            if active(row):
                current = [row]
        if current:
            groups.append((slot, current))
    complete = interrupted = 0
    for slot, group in sorted(groups, key=lambda item: int(item[1][0]['frame'])):
        first, last = group[0], group[-1]
        terminal = int(last['after_state'], 16) & 255 == 0
        observed = [int(row['after_pose'], 16) & 0x7fff for row in group]
        matching = [name for name, (poses, _) in EXPECTED.items() if observed == poses]
        if terminal:
            assert len(matching) == 1, (path, slot, first['frame'], len(group),
                                        observed, matching)
            assert all(row['status'] == '0' for row in group)
            complete += 1
        else:
            interrupted += 1
        results.append({'trace': str(path.relative_to(ROOT)).replace('\\', '/'),
                        'slot': slot, 'first_frame': int(first['frame']),
                        'last_frame': int(last['frame']), 'dispatches': len(group),
                        'terminal': terminal,
                        'timeline': matching[0] if terminal else None})
    assert complete > 0, (path, 'no complete selector-$00 route')

report = {'status': 'PASS', 'rom_sha1': ROM_SHA1,
          'complete_routes': sum(item['terminal'] for item in results),
          'interrupted_or_partial_routes': sum(not item['terminal'] for item in results),
          'routes': results,
          'oracle_reports_sha256': {name: digest for name, (_, digest) in EXPECTED.items()},
          'trace_sha256': {str(path.relative_to(ROOT)).replace('\\', '/'):
                           hashlib.sha256(path.read_bytes()).hexdigest() for path in TRACES},
          'verifier_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          'limits': ['HOST combat dispatches, not arcade full-frame execution',
                     'The comparison uses graphics pose order and terminal state; RAM and D registers are covered by the separate direct-ROM action reports',
                     'Interrupted and partial routes are listed but not declared equivalent',
                     'No Saturn hardware timing or VDP1 output is measured']}
OUT.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'status': report['status'], 'complete_routes': report['complete_routes'],
                  'interrupted_or_partial_routes': report['interrupted_or_partial_routes']}))
