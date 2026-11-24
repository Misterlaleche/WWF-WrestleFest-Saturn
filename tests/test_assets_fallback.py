#!/usr/bin/env python3
"""Sanitized differential of the >170-slot legacy cache, pinned to C13.

Pass --reference-source explicitly. The exact reference is verified against a
fixed SHA and copied into the report directory, so the evidence is reproducible
without depending on another ignored experiment directory.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE_COMMIT = '2c91b70'
REFERENCE_SHA = 'a0e983c2e71ce891fe31aac09c09c08cdf3f17dbff84eba9c1ff65c1be13d7a3'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-source', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/cache-slru-fallback')
    parser.add_argument('--assets-root', type=Path, default=ROOT)
    args = parser.parse_args()
    build = args.build_dir.resolve(); build.mkdir(parents=True, exist_ok=True)
    reference = args.reference_source.resolve(); assets = args.assets_root.resolve()
    assert sha(reference) == REFERENCE_SHA, 'Reference is not the pinned C13 source'
    source = ROOT/'port/wf_jo_assets.c'
    copied = {}
    for label, path in [('reference', reference), ('candidate', source)]:
        destination = build/(label+'-assets.c')
        if path != destination: shutil.copyfile(path, destination)
        copied[label] = destination
    fixtures = {'SPRITES.WFP': assets/'cd/SPRITES.WFP',
                'SCENES.WFP': assets/'cd/SCENES.WFP',
                'ROM68K.BIN': assets/'source_assets/ROM68K.BIN',
                'SPRITES.PAK': assets/'source_assets/SPRITES.PAK',
                'R18RING.BIN': assets/'source_assets/R18RING.BIN'}
    for name, path in fixtures.items(): shutil.copyfile(path, build/name)
    zsources = [ROOT/'port/zlib'/(name+'.c') for name in
                ('inflate', 'inffast', 'inftrees', 'adler32', 'zutil')]
    inputs = [Path(__file__).resolve(), ROOT/'tests/test_assets_fallback.c', source,
              ROOT/'port/wf_jo_assets.h', reference, *zsources,
              *sorted((ROOT/'port/zlib').glob('*.h'))]
    report = dict(status='RUNNING', reference_commit=REFERENCE_COMMIT,
                  reference_sha256=REFERENCE_SHA, commands=[],
                  source_sha256={str(p): sha(p) for p in inputs},
                  input_sha256={name: sha(build/name) for name in fixtures},
                  capacities=[171, 256], sanitizer='address,undefined',
                  comparison='public results, pixel bytes, returned physical slots, '
                             'forward/reverse maps, counters, callback order and state; '
                             'inactive ages intentionally excluded')

    def run(command, label):
        command = [str(p) for p in command]
        result = subprocess.run(command, cwd=build, capture_output=True,
                                env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',
                                         UBSAN_OPTIONS='halt_on_error=1'))
        (build/(label+'.stdout')).write_bytes(result.stdout)
        (build/(label+'.stderr')).write_bytes(result.stderr)
        report['commands'].append(dict(argv=command, cwd=str(build), returncode=result.returncode,
                                       stdout_sha256=sha(build/(label+'.stdout')),
                                       stderr=result.stderr.decode(errors='replace')[-3000:]))
        if result.returncode:
            print(result.stderr.decode(errors='replace')[-3000:], flush=True)
        result.check_returncode()
        return result.stdout

    try:
        flags = ['cc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                 '-Wno-implicit-fallthrough', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                 '-DWF_JO_ASSETS_HOST', '-DZ_SOLO', '-DNO_GZIP',
                 '-I'+str(ROOT/'port'), '-I'+str(ROOT/'port/zlib')]
        outputs = {}
        for label in ('reference', 'candidate'):
            binary = build/('fallback-'+label)
            run([*flags, '-DWF_FALLBACK_ASSETS_SOURCE="'+str(copied[label])+'"',
                 ROOT/'tests/test_assets_fallback.c', *zsources, '-o', binary], 'compile-'+label)
            outputs[label] = run([binary, build/'SPRITES.WFP', build/'ROM68K.BIN',
                                  build/'SPRITES.PAK'], 'run-'+label)
            report[label+'_binary_sha256'] = sha(binary)
        left = outputs['reference'].decode().splitlines()
        right = outputs['candidate'].decode().splitlines()
        differences = [(i+1, a, b) for i, (a, b) in enumerate(zip(left, right)) if a != b]
        report['reference_rows'] = len(left); report['candidate_rows'] = len(right)
        report['differing_rows'] = len(differences)
        report['first_differences'] = differences[:12]
        report['exact_trace_sha256'] = hashlib.sha256(outputs['reference']).hexdigest()
        assert left == right, 'Fallback trace differs: '+repr(differences[:1])
        report['status'] = 'PASS'
        print('PASS: exact sanitized 171/256-slot fallback; '+str(len(left))+' trace rows')
    except Exception as error:
        report['status'] = 'FAIL'; report['error'] = repr(error)
        raise
    finally:
        report['changed_sources'] = [str(p) for p in inputs if sha(p) != report['source_sha256'][str(p)]]
        if report['changed_sources']: report['status'] = 'FAIL'
        (build/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    assert not report['changed_sources']


if __name__ == '__main__': main()
