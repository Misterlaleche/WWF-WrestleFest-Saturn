#!/usr/bin/env python3
"""Prove unchanged mapper/ring output when redundant YM times are deferred.

The RED is a saved module with the old eager timestamp path. Both versions
run the same synthetic state/clock/fault cases. This is a semantic/cost-count
test; it does not substitute for a measured native execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ORACLE_SHA = 'f7fe65540e2d6604b9c038d82f4426ed75f204ac9c1ff4b025f48f01653f2850'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate-source', type=Path, default=ROOT)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    candidate, out = args.candidate_source.resolve(), args.out.resolve()
    if args.out.is_symlink() or out.exists() or any(
            out == root or root in out.parents for root in (ROOT, candidate)):
        parser.error('Use a fresh output outside both source packages')
    oracle = ROOT / 'tests/oracle/wf_music_module_pre_timestamp_elision.c'
    if sha(oracle) != ORACLE_SHA:
        parser.error('Frozen pre-elision module changed')
    test = ROOT / 'tests/test_music_timestamp_elision.c'
    src = candidate / 'src'
    inputs = [Path(__file__).resolve(), oracle, test,
              ROOT / 'tests/test_music_module_deadline.c',
              *[src / name for name in ('wf_music_module.c', 'wf_music_time.h',
                'wf_music_clock.h', 'wf_music_seq.c', 'wf_music_seq.h',
                'wf_scsp_ym.c', 'wf_scsp_ym.h', 'reference_driver/z80.c',
                'reference_driver/z80.h')]]
    before = {str(path): sha(path) for path in inputs}
    out.mkdir(parents=True, exist_ok=False)
    report = {'scope': 'Host mapper/queue equality and actual conversion counts',
              'input_sha256': before, 'commands': [], 'passed': False}
    common = [args.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
              '-fno-pie', '-no-pie', '-I' + str(src),
              '-I' + str(ROOT / 'tests')]

    def run(name, command, expected):
        result = subprocess.run(list(map(str, command)), text=True,
                                capture_output=True)
        record = {'name': name, 'command': list(map(str, command)),
                  'returncode': result.returncode, 'stdout': result.stdout,
                  'stderr': result.stderr}
        report['commands'].append(record)
        (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
        if result.returncode != expected:
            raise RuntimeError(name + ': unexpected result\n' + result.stderr)
        return result

    for name, module in [('red', oracle), ('green', src / 'wf_music_module.c')]:
        exe = out / name
        run('build_' + name, [*common,
            '-DWF_MUSIC_MODULE_SOURCE="' + str(module) + '"', test,
            src / 'wf_music_seq.c', src / 'wf_scsp_ym.c',
            src / 'reference_driver/z80.c', '-o', exe], 0)
        result = run('run_' + name, [exe, out / (name + '.trace')],
                     2 if name == 'red' else 0)
        report[name] = json.loads(result.stdout)
    if report['red']['duplicate_time_advances'] != 1 or \
            report['green']['duplicate_time_advances'] != 0:
        raise ValueError('Expected eager/deferred conversion difference absent')
    equal = True
    with (out / 'red.trace').open('rb') as a, (out / 'green.trace').open('rb') as b:
        while True:
            x, y = a.read(1024 * 1024), b.read(1024 * 1024)
            if x != y:
                equal = False
                break
            if not x:
                break
    report['complete_trace_bytes_equal'] = equal
    report['trace_bytes'] = (out / 'green.trace').stat().st_size
    report['trace_sha256'] = sha(out / 'green.trace')
    report['source_stable'] = before == {str(path): sha(path) for path in inputs}
    report['passed'] = equal and report['source_stable']
    report['compiler'] = subprocess.run([args.cc, '--version'], text=True,
        capture_output=True, check=True).stdout.splitlines()[0]
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'],
                      'report': str(out / 'verification.json')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
