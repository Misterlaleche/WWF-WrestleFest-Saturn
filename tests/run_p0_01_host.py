#!/usr/bin/env python3
"""Reproducible, asset-independent P0-01 regression selection.

Not the project's full initial-load suite, target link, emulator or Saturn gate.
Only the startup fixture is synthesized here; no original music is generated.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import struct
import subprocess
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/p0-01-host')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    fixture = out/'SYNTHETIC-STARTUP-FIXTURE.wav'
    with wave.open(str(fixture), 'wb') as wav:
        wav.setparams((1, 2, 44100, 0, 'NONE', 'not compressed'))
        wav.writeframes(b''.join(struct.pack('<h', ((i*109) & 65535)-32768) for i in range(352800)))
    py, cc = sys.executable, os.environ.get('CC', 'cc')
    def test(name, *arguments):
        return [py, str(ROOT/'tests'/name), *map(str, arguments)]
    flags = ['-std=c11', '-fsyntax-only', '-Wall', '-Wextra', '-Werror',
             '-DWF_JO_ENGINE', '-DWF_JO_ASSETS_HOST', '-DWF_JO_FRONTEND_TEST',
             '-DWF_JO_FRONTEND', '-DWF_JO_AUTORUN', '-DWF_JO_AUTORUN_MODE=2', '-DZ_SOLO', '-DNO_GZIP']
    flags += ['-I'+str(ROOT/p) for p in ('tests/native/jo_host', 'tests', 'core/include', 'port', 'port/zlib', 'tests/native')]
    # Read the shipped target macros; omit target ISA and compiler/link flags.
    sdk_flags = [s for s in shlex.split((ROOT/'sdk/jo_engine/jo_options.txt').read_text())
                 if s.startswith('-D') or s.startswith('-I')]
    sdk_flags += ['-std=gnu99', '-fms-extensions', '-fsyntax-only',
                  '-Wno-pointer-to-int-cast', '-Wno-int-to-pointer-cast']
    commands = [
        ('emergency-audio', test('test_emergency_audio.py', '--output', out/'audio')),
        ('frontend-fault', test('test_main_frontend_fault.py', '--output', out/'main')),
        ('emergency-negative-controls', test('test_emergency_audio_mutants.py', '--output', out/'mutants')),
        ('normal-music-stream', test('test_music_stream.py')),
        ('music-deadline-model', test('test_music_deadline.py')),
        ('startup-synthetic-pcm', test('test_music_startup.py', '--wav', fixture, '--sanitize', '--build-dir', out/'startup')),
        ('cd-diagnostics', test('test_cd_diagnostics.py', '--source-dir', ROOT, '--build-dir', out/'cd-diagnostics')),
        ('cd-ui', test('test_cd_loading_ui.py', '--source-dir', ROOT, '--build-dir', out/'cd-ui')),
        ('cd-negative-controls', test('test_cd_diagnostic_mutants.py', '--source-dir', ROOT, '--input-dir', out/'cd-diagnostics', '--build-dir', out/'cd-negatives')),
        ('boot-progress', test('test_boot_load_progress.py')),
        ('main-host-syntax', [cc, *flags, str(ROOT/'tests/native/jo_main_probe.c')]),
        ('sdk-host-syntax', [cc, *sdk_flags, 'sdk/jo_engine/audio.c']),
    ]
    report = {'scope': __doc__, 'runs': [], 'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
              'target_build': 'NOT_RUN: no SH-2 toolchain or complete disc resources available',
              'physical_saturn': 'NOT_RUN', 'fix2_integration': 'NOT_RUN: FIX2 sources unavailable'}
    for name, command in commands:
        try:
            run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=80,
                                 env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))
            code, text = run.returncode, run.stdout+run.stderr
        except subprocess.TimeoutExpired as exc:
            code, text = 124, 'TIMEOUT\n'+str(exc)
        log = out/(name+'.log')
        log.write_text(text)
        report['runs'].append({'name': name, 'command': command, 'returncode': code,
                               'log': log.name, 'log_sha256': hashlib.sha256(log.read_bytes()).hexdigest()})
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        print(name, 'PASS' if code == 0 else 'FAIL', flush=True)
        if code:
            print(text[-2500:], flush=True)
    report['status'] = 'PASS_SELECTED_HOST_GATES' if all(r['returncode'] == 0 for r in report['runs']) else 'FAIL'
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return report['status'] == 'FAIL'


if __name__ == '__main__':
    raise SystemExit(main())
