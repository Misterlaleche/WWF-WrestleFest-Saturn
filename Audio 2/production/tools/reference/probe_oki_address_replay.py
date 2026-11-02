#!/usr/bin/env python3
"""Reproduce the bounded host original-address/checkpoint experiment privately.

This installs no native code or resource ABI. ROM/PCM bytes and mutated test
builds remain in a fresh output outside the repository. The original mixed
trace is replayed at its original clock boundaries; FM is not synthesized again.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import zipfile

sys.dont_write_bytecode = True
from audit_oki_storage import coverage, ROM_SHA

PACKAGE = Path(__file__).resolve().parents[2]
REPOSITORY = PACKAGE.parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('rom-zip', 'pack', 'model-run', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--no-leak-check', action='store_true',
                        help='Disable LeakSanitizer where /proc tracing is unavailable; retain ASan/UBSan')
    args = parser.parse_args()
    original_out = args.out
    for name in ('rom_zip', 'pack', 'model_run', 'out'):
        setattr(args, name, getattr(args, name).resolve())
    if original_out.is_symlink() or args.out.exists() or args.out == REPOSITORY or REPOSITORY in args.out.parents:
        parser.error('Use a fresh private output outside the source repository')
    model_report = args.model_run / 'mix-reference-report.json'
    capture = json.loads(model_report.read_text())['reference_capture']
    trace = Path(capture['trace'])
    if sha(trace) != capture['output_sha256']['trace']:
        raise ValueError('Recorded mixed trace identity mismatch')
    with zipfile.ZipFile(args.rom_zip) as archive:
        rom = archive.read('31j10.ic73')
    if len(rom) != 0x80000 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError('Original sample ROM identity mismatch')
    args.out.mkdir(parents=True, exist_ok=False)
    fixtures = args.out / 'fixtures'
    fixtures.mkdir()
    (fixtures / 'SAMPLES.ROM').write_bytes(rom)
    feasibility = coverage(rom, args.pack.read_bytes(), fixtures)
    source = PACKAGE / 'tests/oki_address_replay/wf_oki_address.c'
    driver = PACKAGE / 'tests/test_oki_address_replay.c'
    watched = [source, source.with_suffix('.h'), driver, Path(__file__).resolve(),
               PACKAGE / 'tools/reference/reference_oki.h',
               PACKAGE / 'tools/reference/audit_oki_storage.py']
    watched += sorted((PACKAGE / 'src').rglob('*.c'))
    watched += sorted((PACKAGE / 'src').rglob('*.h'))
    before = {str(p.relative_to(PACKAGE)): sha(p) for p in watched}
    report = {
        'kind': 'HOST_ORIGINAL_ADDRESS_CHECKPOINT_PROTOTYPE',
        'scope': 'Original-address bytes and ADPCM/ring state only; no Saturn execution, native deadline, FM resynthesis or analog equivalence claim.',
        'resource_abi_installed': False,
        'native_runtime_integrated': False,
        'input_sha256': {'rom_zip': sha(args.rom_zip), 'sample_rom': ROM_SHA,
                         'pack': sha(args.pack), 'mixed_trace': sha(trace),
                         'mixed_reference_report': sha(model_report)},
        'source_sha256': before, 'coverage': feasibility,
        'sanitizers': args.sanitize, 'leak_detection': args.sanitize and not args.no_leak_check,
        'commands': [], 'tests': [], 'mutations': [],
    }
    report_path = args.out / 'oki-address-replay-report.json'

    def save():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    def run(name, command, expected=0):
        started = time.monotonic()
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
        if args.sanitize:
            env['ASAN_OPTIONS'] = 'detect_leaks=' + str(int(not args.no_leak_check)) + ':halt_on_error=1'
            env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        result = subprocess.run(list(map(str, command)), cwd=PACKAGE, env=env,
                                text=True, capture_output=True)
        item = dict(name=name, command=list(map(str, command)),
                    exit_code=result.returncode, expected_exit_code=expected,
                    seconds=time.monotonic() - started,
                    stdout=result.stdout, stderr=result.stderr)
        report['commands'].append(item)
        save()
        if result.returncode != expected:
            raise RuntimeError(name + ' returned an unexpected status\n' + result.stdout + result.stderr)
        return item

    common = [args.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
              '-I', source.parent]
    if args.sanitize:
        common += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    exe = args.out / 'test_oki_address_replay'
    run('build', [*common, driver, source, '-o', exe])
    inputs = [fixtures / 'SAMPLES.ROM', args.pack, fixtures / 'bankmap-extension.bin']
    for mode in ('resolver', 'decoder', 'ring', 'trace'):
        item = run(mode, [exe, mode, *inputs, *([trace] if mode == 'trace' else [])])
        parsed = json.loads(item['stdout'])
        if not parsed['passed'] or item['stderr']:
            raise RuntimeError('Test did not report a clean pass: ' + mode)
        report['tests'].append(parsed)
        save()
        print(mode + ': PASS', flush=True)
    # Private negative controls prove the same tests catch realistic omissions.
    # They never replace or edit a source file in the repository.
    mutations = (
        ('stale_bank_cache', 'resolver', 'cache->bank != bank ||', ''),
        ('discard_bank_boundary_history', 'ring',
         'if (position >= ring->anchor_position)', 'if (true)'),
        ('leave_prefetched_pcm_stale', 'trace',
         'if (!wf_oki_ring_fill(ring, map, pcm, target)) return false;',
         'ring->prepared = target;'),
    )
    for name, mode, old, new in mutations:
        text = source.read_text()
        if text.count(old) != 1:
            raise ValueError('Mutation target is no longer unique: ' + name)
        mutated = args.out / (name + '.c')
        mutated.write_text(text.replace(old, new))
        mutation_exe = args.out / name
        run('build_' + name, [*common, driver, mutated, '-o', mutation_exe])
        item = run(name, [mutation_exe, mode, *inputs, *([trace] if mode == 'trace' else [])], expected=1)
        if 'FAIL ' not in item['stderr']:
            raise RuntimeError('Negative control did not fail a semantic assertion: ' + name)
        report['mutations'].append(dict(name=name, caught=True, mode=mode,
                                         source_sha256=sha(mutated)))
        save()
    report['source_stable'] = all(sha(PACKAGE / p) == digest for p, digest in before.items())
    report['compiler'] = subprocess.run([args.cc, '--version'], check=True,
                                        capture_output=True, text=True).stdout.splitlines()[0]
    report['executable_sha256'] = sha(exe)
    report['passed'] = report['source_stable'] and len(report['tests']) == 4 and len(report['mutations']) == 3
    save()
    print(json.dumps({'passed': report['passed'], 'report': str(report_path)}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
