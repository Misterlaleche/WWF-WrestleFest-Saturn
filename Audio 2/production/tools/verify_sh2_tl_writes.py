#!/usr/bin/env python3
"""Reproduce exact TL-prefix RED/GREEN checks against the original scalar Z80.

This checks host CPU/RAM, write-callback state and admission semantics only.
Native placement, throughput, timer scheduling and audio fidelity require
their separate gates. Supply the unchanged private SOUND.ROM and a fresh
output directory outside the source package. No original data is embedded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time


ROM_SHA256 = 'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7'
SCALAR_SHA256 = 'd82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698'
BEFORE_SHA256 = '983bde9378737eeb6bdcf0ed9ff15a5b535ad55ed61ea87a84516b1613b04939'
OPERATIONS = ((0x0c09, 139, 623), (0x0c16, 130, 605),
              (0x0c21, 130, 605), (0x0c2c, 123, 591))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out, rom = args.out.resolve(), args.rom.resolve()
    if out == root or root in out.parents:
        parser.error('Use a new private output outside the source package')
    if out.exists():
        parser.error('Output already exists; use a new private directory')
    src, core = root / 'src', root / 'src/reference_driver'
    old = root / 'tests/oracle/z80_native_before_scalar9.c'
    scalar = core / 'z80.c'
    test = root / 'tests/test_sh2_tl_writes.c'
    for path, expected, label in ((rom, ROM_SHA256, 'complete original SOUND.ROM'),
                                  (scalar, SCALAR_SHA256, 'untouched original scalar Z80'),
                                  (old, BEFORE_SHA256, 'frozen pre-correction native core')):
        try:
            actual = sha(path)
        except OSError as error:
            parser.error(str(error))
        if actual != expected:
            parser.error('Require the unchanged ' + label)
    inputs = [Path(__file__).resolve(), test, old, scalar, rom,
              src / 'wf_music_seq.h', core / 'z80.h',
              core / 'z80-native.c', core / 'z80-cold.c']
    before = {str(path): sha(path) for path in inputs}
    out.mkdir(parents=True, exist_ok=False)
    report = {
        'scope': 'Host exact CPU/RAM/adapter, YM-write callback and admission semantics only',
        'started_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'source_and_input_sha256': before,
        'expected': {'accepted': 790920, 'rejected': 2424, 'total_cases': 793344,
                     'callback_snapshots_per_engine': 1581840,
                     'instructions_per_admission': 18, 'final_pc': '0F5F'},
        'commands': [], 'red_operations': [], 'green_complete': False,
        'passed': False,
    }
    report_path = out / 'verification.json'

    def save():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    def run(name, command, expected=0):
        command = list(map(str, command))
        started = time.monotonic()
        result = subprocess.run(command, text=True, capture_output=True)
        report['commands'].append({
            'name': name, 'command': command, 'expected_returncode': expected,
            'returncode': result.returncode, 'seconds': time.monotonic() - started,
            'stdout': result.stdout, 'stderr': result.stderr,
        })
        save()
        if result.returncode != expected:
            raise RuntimeError(name + ': unexpected result\n' + result.stdout + result.stderr)
        return result

    save()
    try:
        report['compiler'] = run('compiler', [args.cc, '--version']).stdout.splitlines()[0]
        common = [args.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                  '-Wno-unused-const-variable', '-Wno-unused-function',
                  '-I' + str(src), '-I' + str(core)]
        aliases = ('z80_init', 'z80_step', 'z80_gen_nmi', 'z80_gen_int', 'z80_debug_output')
        ref = out / 'original-z80.o'
        run('build_original_z80', [*common, *['-D' + name + '=wf_ref_' + name for name in aliases],
                                   '-c', scalar, '-o', ref])
        test_object = out / 'tl-tests.o'
        run('build_tests_strict', [*common, '-c', test, '-o', test_object])
        for name, native in (('red', old), ('green', core / 'z80-native.c')):
            executable = out / name
            # The legacy cores keep their original compact formatting. The
            # new test above is compiled strictly without this suppression.
            run('build_' + name, [*common, '-Wno-misleading-indentation', test_object,
                                 native, core / 'z80-cold.c', ref, '-o', executable])
            if name == 'red':
                for pc, _, _ in OPERATIONS:
                    result = run('red_' + format(pc, '04x'), [executable, rom, hex(pc)], 2)
                    expected = f'RED TL operation {pc:04X} expected 18 original instructions, admitted 0'
                    if result.stderr.strip() != expected or result.stdout.strip():
                        raise ValueError('RED failed for a different reason at ' + format(pc, '04X'))
                    report['red_operations'].append(format(pc, '04X'))
            else:
                result = run('green_all_cases', [executable, rom])
                expected = [
                    f'PASS TL {pc:04X} exact differential accepted=197730 rejected={rejects} '
                    f'instructions=18 T={ticks} R=18 final-PC=0F5F'
                    for pc, ticks, rejects in OPERATIONS
                ]
                expected.append('PASS TL total accepted=790920 rejected=2424 '
                                'callback-snapshots-per-engine=1581840 mode=differential')
                if result.stdout.splitlines() != expected or result.stderr:
                    raise ValueError('GREEN did not finish the complete 793344-case matrix')
                report['green_complete'] = True
        report['artifact_sha256'] = {
            name: sha(out / name) for name in ('original-z80.o', 'tl-tests.o', 'red', 'green')
        }
    except (OSError, RuntimeError, ValueError) as error:
        report['error'] = str(error)
    finally:
        after = {str(path): sha(path) if path.is_file() else None for path in inputs}
        report['source_and_input_sha256_after'] = after
        report['source_stable'] = before == after
        report['passed'] = (report['green_complete'] and len(report['red_operations']) == 4
                            and report['source_stable'] and 'error' not in report)
        report['completed_utc'] = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())
        save()
    print(json.dumps({'passed': report['passed'], 'report': str(report_path)}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
