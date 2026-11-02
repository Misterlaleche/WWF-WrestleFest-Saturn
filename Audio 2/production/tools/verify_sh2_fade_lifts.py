#!/usr/bin/env python3
"""Reproduce RED/GREEN exact fade operations against the untouched original Z80.

These host checks cannot certify native throughput, PCM fidelity or listening
approval. Run the separate native music matrix with the resulting source hash.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--cc', default='cc')
    a = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    out, rom = a.out.resolve(), a.rom.resolve()
    if out == root or root in out.parents:
        p.error('Use a new private output outside the source package')
    if sha(rom) != 'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7':
        raise ValueError('Require the complete, unchanged original SOUND.ROM')
    old = root / 'tests/oracle/z80_native_before_scalar9.c'
    if sha(old) != '983bde9378737eeb6bdcf0ed9ff15a5b535ad55ed61ea87a84516b1613b04939':
        raise ValueError('Frozen pre-correction native core changed')
    out.mkdir(parents=True, exist_ok=False)
    src, core = root / 'src', root / 'src/reference_driver'
    test = root / 'tests/test_sh2_fade_lifts.c'
    inputs = [Path(__file__).resolve(), test, old, rom, src / 'wf_music_seq.h',
              *[core / name for name in ('z80.c', 'z80-native.c', 'z80-cold.c', 'z80.h')]]
    before = {str(path): sha(path) for path in inputs}
    report = {'scope': 'Host exact CPU/RAM/cycle/refresh/admission semantics only',
              'source_and_input_sha256': before, 'commands': [], 'passed': False}

    def run(name, command, expected=0):
        command = list(map(str, command))
        started = time.monotonic()
        result = subprocess.run(command, text=True, capture_output=True)
        report['commands'].append({'name': name, 'command': command,
                                   'returncode': result.returncode,
                                   'seconds': time.monotonic() - started,
                                   'stdout': result.stdout, 'stderr': result.stderr})
        (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
        if result.returncode != expected:
            raise RuntimeError(name + ': unexpected result\n' + result.stdout + result.stderr)
        return result

    common = [a.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
              '-Wno-unused-const-variable', '-Wno-unused-function',
              '-I' + str(src), '-I' + str(core)]
    aliases = ['z80_init', 'z80_step', 'z80_gen_nmi', 'z80_gen_int', 'z80_debug_output']
    ref = out / 'original-z80.o'
    run('build_original_z80', [*common, *['-D' + x + '=wf_ref_' + x for x in aliases],
                               '-c', core / 'z80.c', '-o', ref])
    test_object = out / 'fade-tests.o'
    run('build_tests_strict', [*common, '-c', test, '-o', test_object])
    for name, native in [('red', old), ('green', core / 'z80-native.c')]:
        exe = out / name
        # The frozen native core predates this test and retains compact legacy
        # formatting. Keep that oracle unchanged; the new test is strict above.
        run('build_' + name, [*common, '-Wno-misleading-indentation', test_object,
                             native, core / 'z80-cold.c', ref, '-o', exe])
        if name == 'red':
            for operation in ('0x0be7', '0x0f3a', '0x0bd7', '0x0b68', '0x0f5f'):
                result = run('red_' + operation, [exe, rom, operation], 2)
                if 'admitted 0' not in result.stderr:
                    raise ValueError('RED failed for a different reason')
        else:
            result = run('green_all_cases', [exe, rom])
            if 'cases=1507643 ' not in result.stdout:
                raise ValueError('GREEN did not finish the complete case matrix')
    report['source_stable'] = before == {str(path): sha(path) for path in inputs}
    report['passed'] = report['source_stable']
    report['compiler'] = subprocess.run([a.cc, '--version'], text=True,
                                        capture_output=True, check=True).stdout.splitlines()[0]
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'report': str(out / 'verification.json')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
