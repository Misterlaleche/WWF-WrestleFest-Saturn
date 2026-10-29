#!/usr/bin/env python3
"""Portable R02 audio regressions; package as verification/r02_audio/run_tests.py.

Uses the delivered source/research_audio/integration and local fixtures/r01.
The host C compiler is required. SH-2 compilation is optional and enabled by
YAUL_INSTALL_ROOT. Production files and fixtures are only read.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess


def main():
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--integration', type=Path,
                    default=here.parents[1] / 'research_audio/integration')
    ap.add_argument('--fixtures', type=Path, default=here / 'fixtures/r01')
    ap.add_argument('--build', type=Path, default=here / 'build-host')
    args = ap.parse_args()
    base, fixtures, build_dir = (p.resolve() for p in
                                 (args.integration, args.fixtures, args.build))
    for path in (base / 'wp02_bgm.c', base / 'wp02_bgm.h', fixtures / 'wp02_bgm.c',
                 fixtures / 'wp02_bgm.h', base / 'cd/WP02BGM.PAK',
                 base / 'cd/WP02BGM.decoded.pcm', base / 'cd/WP02PCM.PAK'):
        if not path.is_file():
            ap.error('Required input is missing: ' + str(path))
    build_dir.mkdir(parents=True, exist_ok=True)
    tests = [
        ('test_wp02_audio', [base / 'tests/test_wp02_audio.c', base / 'wp02_audio.c',
                             base / 'wf_audio_sfx.c', base / 'wp02_bgm.c',
                             base / 'wp02_bgm_codec.c'], [base / 'cd/WP02PCM.PAK'], 0),
        ('test_pcm16_format', [base / 'tests/test_pcm16_format.c', base / 'wp02_pcm16.c'], [], 0),
        ('test_pcm16_gain', [base / 'tests/test_pcm16_gain.c'], [], 0),
        ('test_bgm_codec', [base / 'tests/test_bgm_codec.c', base / 'wp02_bgm_codec.c'],
         [base / 'cd/WP02BGM.PAK', base / 'cd/WP02BGM.decoded.pcm'], 0),
        ('test_bgm_stream', [base / 'tests/test_bgm_stream.c', base / 'wp02_bgm.c',
                             base / 'wp02_bgm_codec.c'],
         [base / 'cd/WP02BGM.PAK', base / 'cd/WP02BGM.decoded.pcm'], 0),
        ('test_bgm_failure_snapshot', [here / 'test_bgm_failure_snapshot.c',
                                       base / 'wp02_bgm.c', base / 'wp02_bgm_codec.c'],
         [base / 'cd/WP02BGM.PAK'], 0),
    ]
    for cadence in (1, 2, 3, 4, 5):
        tests.append((f'test_bgm_cadence_{cadence}',
                      [here / 'test_bgm_service_cadence.c', base / 'wp02_bgm.c',
                       base / 'wp02_bgm_codec.c'],
                      [base / 'cd/WP02BGM.PAK', base / 'cd/WP02BGM.decoded.pcm',
                       cadence, 1 if cadence in (1, 4, 5) else 2], 0))
    tests.append(('test_r01_cadence_expected_failure',
                  [here / 'test_bgm_service_cadence.c', fixtures / 'wp02_bgm.c',
                   base / 'wp02_bgm_codec.c'],
                  [base / 'cd/WP02BGM.PAK', base / 'cd/WP02BGM.decoded.pcm', 2, 1], 1))
    for label, source in (('r01', fixtures / 'wp02_bgm.c'), ('r02', base / 'wp02_bgm.c')):
        for start_us, expected_unsafe in ((738000, 0), (740000, 1)):
            tests.append((f'test_duration_{label}_{start_us}',
                          [here / 'test_bgm_service_duration.c', source, base / 'wp02_bgm_codec.c'],
                          [base / 'cd/WP02BGM.PAK', expected_unsafe, start_us], 0))

    results = []
    for name, sources, parameters, expected in tests:
        executable = build_dir / name
        command = [os.environ.get('CC', 'cc'), '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-I' + str(base / 'tests'), '-I' + str(base),
                   *map(str, sources), '-o', str(executable)]
        subprocess.run(command, check=True)
        invocation = [str(executable), *map(str, parameters)]
        result = subprocess.run(invocation, capture_output=True, text=True)
        print(name + ': ' + result.stdout, end='', flush=True)
        if result.returncode != expected:
            raise RuntimeError(f'{name}: expected exit {expected}, got {result.returncode}\n'
                               + result.stderr)
        results.append({'name': name, 'build': command, 'command': invocation,
                        'expected_exit_code': expected, 'exit_code': result.returncode,
                        'stdout': result.stdout.strip(), 'stderr': result.stderr})

    sh2 = {'status': 'SKIPPED', 'reason': 'YAUL_INSTALL_ROOT not supplied'}
    sdk_env = os.environ.get('YAUL_INSTALL_ROOT')
    if sdk_env:
        sdk = Path(sdk_env).resolve()
        command = [str(sdk / 'bin/sh2eb-elf-gcc'), '-std=c11', '-O2', '-Wall', '-Wextra',
                   '-Werror=implicit-function-declaration', '-specs=yaul.specs',
                   '-specs=yaul-main.specs', '-fno-lto', '-I' + str(base),
                   '-I' + str(sdk / 'sh2eb-elf/include/yaul'), '-S',
                   str(base / 'wp02_bgm.c'), '-o', str(build_dir / 'wp02_bgm_sh2.s')]
        subprocess.run(command, check=True)
        sh2 = {'status': 'PASS', 'command': command}
    print('SH-2 compile: ' + sh2['status'], flush=True)

    def sha(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    inputs = [base / 'wp02_bgm.c', base / 'wp02_bgm.h', fixtures / 'wp02_bgm.c',
              fixtures / 'wp02_bgm.h', base / 'cd/WP02BGM.PAK',
              base / 'cd/WP02BGM.decoded.pcm', base / 'cd/WP02PCM.PAK']
    report = {'status': 'PASS', 'integration': str(base), 'fixtures': str(fixtures),
              'tests': results, 'sh2_compile': sh2,
              'sha256': {str(p): sha(p) for p in inputs}, 'snapshot_bytes': 24,
              'cadences': {'1': '59.826 Hz', '2': '29.913 Hz',
                           '3': '59.826 Hz; miss one refresh every ninth service',
                           '4': '45 Hz', '5': '50 Hz'},
              'limits': ['Host models do not establish physical Saturn execution-time bounds.',
                         'The 740 ms duration cases deliberately document the inherited one-block window.',
                         'Original R01 at 29.913 Hz is an expected failure, not a passing transport case.']}
    (build_dir / 'r02-audio-test-results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(tests)} host cases; report: {build_dir / "r02-audio-test-results.json"}')


if __name__ == '__main__':
    main()
