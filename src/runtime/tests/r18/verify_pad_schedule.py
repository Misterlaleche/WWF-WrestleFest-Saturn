#!/usr/bin/env python3
"""Verify HOST pad schedules against the same frozen match binary.

Usage: verify_pad_schedule.py BINARY [--frames 299]
Numeric CSV rows are frame,mask; no header. All paths are local HOST fixtures.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('binary', type=Path)
p.add_argument('--frames', type=int, default=299)
a = p.parse_args()
assert 184 <= a.frames < 300, 'Keep this input-observer check before unrelated later owners'
root = Path(__file__).resolve().parents[2]
binary = a.binary.resolve()
assets = root / 'audit/runtime18-cd'
arguments = [str(binary), *(str(assets / name) for name in
             ('ROM68K.BIN', 'SPRITES.PAK', 'R18RING.BIN', 'HOTSPR.BIN'))]

with tempfile.TemporaryDirectory(prefix='wf-pad-schedule-') as temporary:
    temporary = Path(temporary)

    def run(name, rows=None, attack=False, frames=a.frames):
        out = temporary / name
        out.mkdir()
        env = dict(os.environ)
        for key in ('WF_PROBE_PAD_SCHEDULE', 'WF_PROBE_ATTACK',
                    'WF_PROBE_REQUIRE_COMPLETE', 'WF_PROBE_ROM_TRACE',
                    'WF_PROBE_SPRITE_TRACE', 'WF_PROBE_ROM_DEMAND',
                    'WF_PROBE_TILE_DEMAND', 'WF_PROBE_REQUIRE_RESIDENT'):
            env.pop(key, None)
        env.update(WF_PROBE_FRAMES=str(frames), WF_ALLIES_OUT=str(out))
        if rows is not None:
            schedule = out / 'pad.csv'
            schedule.write_text(rows)
            env['WF_PROBE_PAD_SCHEDULE'] = str(schedule)
        if attack:
            env['WF_PROBE_ATTACK'] = '1'
        result = subprocess.run(arguments, cwd=root, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return result, out

    invalid = {
        'duplicate': '0,255\n0,254\n',
        'descending': '1,255\n0,254\n',
        'mask-range': '0,256\n',
        'negative-mask': '0,-1\n',
        'negative-frame': '-1,255\n',
        'frame-range': '4294967296,255\n',
        'malformed': '0,255,1\n',
        'header': 'frame,mask\n',
        'empty': '',
    }
    for name, rows in invalid.items():
        result, _ = run(name, rows, frames=1)
        assert result.returncode == 2, (name, result.returncode, result.stdout, result.stderr)
        assert 'pad schedule' in result.stderr.lower(), (name, result.stderr)

    def normalized(result):
        assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
        return '\n'.join(line for line in result.stdout.splitlines()
                         if not line.startswith('INPUT '))

    def compare(name, left, right):
        lr, lp = left
        rr, rp = right
        assert normalized(lr) == normalized(rr), name + ': gameplay stdout differs'
        for filename in ('trace.csv',):
            if (lp / filename).exists() or (rp / filename).exists():
                assert (lp / filename).read_bytes() == (rp / filename).read_bytes(), name + ': ' + filename
        ls = sorted(x.name for x in lp.glob('snapshot-*.bin'))
        rs = sorted(x.name for x in rp.glob('snapshot-*.bin'))
        assert ls == rs
        for filename in ls:
            assert (lp / filename).read_bytes() == (rp / filename).read_bytes(), name + ': ' + filename
        return len(ls)

    baseline = run('default-attack', attack=True)
    scheduled = run('scheduled-attack', '60,254\n180,239\n184,255\n', attack=True)
    snapshots = compare('fixed RIGHT/A schedule', baseline, scheduled)
    idle = run('default-idle')
    compare('initial FF and frame-zero FF', idle, run('zero', '0,255\n'))
    normalized(run('zero-mask', '0,0\n', frames=1)[0])
    compare('initial FF before first future event', idle, run('future', '4294967295,254\n'))
    compare('hold last mask', run('held', '0,254\n'),
            run('held-explicit', '0,254\n183,254\n'))
    print(json.dumps({'result': 'pass', 'invalid_cases': list(invalid),
                      'frames': a.frames, 'fixed_schedule_snapshots': snapshots,
                      'stdout_and_actor_traces_identical': True}, indent=2))
