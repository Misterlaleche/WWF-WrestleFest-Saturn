#!/usr/bin/env python3
"""Rebuild B12/current decoders, compare exact outputs and worker lifetimes.

Uses the bundled, hash-pinned B12 source fixture. No Git metadata, build tree,
archived binary or earlier performance result is used. Host tests exercise immediate,
deferred and pthread worker schedules plus the compiled serial fallback.

The explicit --cache-policy-change mode permits B12/current replacement-state
and callback-count differences. Every binary still checks canonical tile bytes
and lifetime assertions, and all four current scheduling paths must agree exactly.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT/'tests/native/reference_b12_assets'
REFERENCE_MANIFEST_SHA256 = '595c120c1b4efea4cf256d626655ad3d8a681e0acffaec1925809edfbfd721fb'
SCENES = ('R18RING.BIN', 'R18MENU.BIN', 'R18MAP1.BIN', 'R18MAP2.BIN',
          'R18MAP4.BIN', 'R18MAP5.BIN', 'R18MAP6.BIN')
SOURCES = ('port/wf_jo_assets.c', 'port/zlib/inflate.c', 'port/zlib/inffast.c',
           'port/zlib/inftrees.c', 'port/zlib/adler32.c', 'port/zlib/zutil.c')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-root', type=Path, default=REFERENCE)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/recovery-assets/paired')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--cache-policy-change', action='store_true',
                        help='Classify B12/current cache-policy differences; keep exact current scheduling parity')
    args = parser.parse_args()
    build, reference = args.build_dir.resolve(), args.reference_root.resolve()
    build.mkdir(parents=True, exist_ok=True)
    for name in ('SPRITES.WFP', 'SCENES.WFP'):
        shutil.copyfile(ROOT/'cd'/name, build/name)
    for name in ('ROM68K.BIN', *SCENES):
        shutil.copyfile(ROOT/'source_assets'/name, build/name)
    report = {'format': 'wf-assets-paired-recovery-v2', 'status': 'RUNNING',
              'cache_policy_change': args.cache_policy_change,
              'commands': [], 'runs': {}, 'source_sha256': {},
              'input_sha256': {name: sha(build/name) for name in
                               ('SPRITES.WFP', 'SCENES.WFP', 'ROM68K.BIN', *SCENES)},
              'scope': 'Fresh host byte/state equivalence and lifetime gates; SH2 assembly and physical timing need target execution.'}
    inputs = [Path(__file__).resolve(), ROOT/'tests/test_assets_paired.c', ROOT/'tests/test_assets.c']
    for tree in (reference, ROOT):
        inputs += [tree/name for name in SOURCES]
        inputs += list((tree/'port/zlib').glob('*.h'))+[tree/'port/wf_jo_assets.h']
    report['source_sha256'] = {str(path): sha(path) for path in inputs}
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')

    def command(argv, name, cwd=ROOT):
        print(name, flush=True)
        done = subprocess.run([str(v) for v in argv], cwd=cwd, env=env,
                              text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        logfile = build/(name+'.log'); logfile.write_text(done.stdout)
        report['commands'].append({'argv': [str(v) for v in argv], 'cwd': str(cwd),
                                  'environment': {'ASAN_OPTIONS': 'detect_leaks=0'},
                                  'returncode': done.returncode, 'log': str(logfile)})
        if done.returncode:
            print(done.stdout, file=sys.stderr)
        done.check_returncode()
        return done.stdout

    try:
        manifest_file = REFERENCE/'manifest.json'
        if sha(manifest_file) != REFERENCE_MANIFEST_SHA256:
            raise AssertionError('B12 fixture manifest hash mismatch')
        manifest = json.loads(manifest_file.read_text())
        for name, expected_hash in manifest['files'].items():
            if sha(reference/name) != expected_hash:
                raise AssertionError('B12 source fixture hash mismatch: '+name)
        report['reference_manifest_sha256'] = REFERENCE_MANIFEST_SHA256
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                 '-Wno-implicit-fallthrough', '-DZ_SOLO', '-DNO_GZIP',
                 '-DWF_JO_ASSETS_HOST', '-pthread', '-I'+str(ROOT/'port')]
        if args.sanitize:
            flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        for name, tree, extra in (
            ('reference', reference, ['-DWF_ASSETS_REFERENCE', '-DWF_JO_ASSETS_TEST_WORKER']),
            ('paired', ROOT, ['-DWF_JO_ASSETS_TEST_WORKER']),
            ('serial', ROOT, ['-DWF_ASSETS_SERIAL_FALLBACK'])):
            command([*flags, *extra, ROOT/'tests/test_assets_paired.c',
                     *(tree/source for source in SOURCES), '-o', build/name], 'compile-'+name)
        expected = reference_result = candidate_result = None
        for name, binary, schedule in (('reference', 'reference', 1),
                                       ('immediate', 'paired', 0), ('deferred', 'paired', 1),
                                       ('pthread', 'paired', 2), ('serial-fallback', 'serial', 1)):
            output = command([build/binary, build/'SPRITES.WFP', ROOT/'source_assets/ROM68K.BIN',
                              ROOT/'source_assets/SPRITES.PAK', schedule], name, build)
            match = re.search(r'^EQUIVALENCE (\w+) callbacks=(\d+)$', output, re.M)
            if not match:
                raise AssertionError('Missing complete corpus result: '+name)
            result = match.groups()
            if name == 'reference':
                reference_result = result
                expected = result
            elif candidate_result is None:
                candidate_result = result
                if args.cache_policy_change:
                    expected = result
            if result != expected:
                scope = 'Current scheduling' if args.cache_policy_change else 'B12/current'
                raise AssertionError(scope+' output, slot, replacement, stats or hook order mismatch: '+name)
            jobs = re.search(r'PASS worker scheduling mode\d+: (\d+) launches/(\d+) joins', output)
            report['runs'][name] = {'status': 'PASS', 'digest': result[0],
                                   'service_callbacks': int(result[1]),
                                   'launches': int(jobs[1]), 'joins': int(jobs[2])}
        report['cross_version'] = {
            'equal': reference_result == candidate_result,
            'equality_required': not args.cache_policy_change,
            'reference_digest': reference_result[0], 'candidate_digest': candidate_result[0],
            'service_callback_delta': int(candidate_result[1])-int(reference_result[1]),
            'classification': ('Intentional replacement-policy differences are allowed only between '
                               'B12 and current; canonical bytes and lifetime assertions remain required '
                               'for every run, and all current schedules must match exactly.'
                               if args.cache_policy_change else 'Exact B12/current replacement state required.')}
        report['sources_changed_during_run'] = any(sha(path) != value for path, value in
                                                   report['source_sha256'].items())
        if report['sources_changed_during_run']:
            raise AssertionError('Sources changed during this gate')
        report['status'] = 'PASS'
        print('PASS: canonical corpus/lifetimes and exact current cache state across four schedules'
              if args.cache_policy_change else
              'PASS: B12/current corpus, ordered cache state and all four scheduling paths', flush=True)
    except Exception as error:
        report['status'] = 'FAIL'; report['error'] = repr(error)
        raise
    finally:
        (build/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
