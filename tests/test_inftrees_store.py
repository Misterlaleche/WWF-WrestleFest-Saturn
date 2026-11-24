#!/usr/bin/env python3
"""Full-state inflate_table differential, including missing-endian fallback.

The C driver independently constructs trees and compares table/work/lens bytes,
returns, pointers and guards against immutable accepted C22 source. Native and
three absent-macro configurations run 80,688 cases each with ASan/UBSan and
strict aliasing. The deliberately wrong width and packed word must both fail.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT/'tests/native/reference_c23_inftrees'
CONFIGURATIONS = {
    'native': [],
    'no-byte-order': ['-U__BYTE_ORDER__'],
    'no-order-constants': ['-U__ORDER_BIG_ENDIAN__', '-U__ORDER_LITTLE_ENDIAN__'],
    'no-endian-macros': ['-U__BYTE_ORDER__', '-U__ORDER_BIG_ENDIAN__', '-U__ORDER_LITTLE_ENDIAN__'],
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/inftrees-store-final')
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--candidate-source', type=Path,
                        help='Read-only analysis/red source; default is production.')
    parser.add_argument('--configuration', choices=CONFIGURATIONS)
    parser.add_argument('--skip-mutant', action='store_true',
                        help='Only for baseline or an independently supplied red control.')
    args = parser.parse_args()
    assert not (args.baseline and args.candidate_source)
    assert not args.skip_mutant or args.baseline or args.candidate_source
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((REFERENCE/'manifest.json').read_text())
    reference = REFERENCE/'inftrees.c'
    source = reference if args.baseline else (args.candidate_source or ROOT/'port/zlib/inftrees.c').resolve()
    proof = {}
    for name, identity in manifest['files'].items():
        git_data = subprocess.check_output(['git', 'show', manifest['commit']+':'+name], cwd=ROOT)
        git_blob = subprocess.check_output(['git', 'rev-parse', manifest['commit']+':'+name], cwd=ROOT, text=True).strip()
        assert git_blob == identity['git_blob']
        assert hashlib.sha256(git_data).hexdigest() == identity['sha256']
        expected_file = reference if name == 'port/zlib/inftrees.c' else ROOT/name
        assert expected_file.read_bytes() == git_data, f'Pinned reference/header changed: {name}'
        proof[name] = dict(identity, current_path=str(expected_file), git_bytes_equal=True)
    protected = {str(p): sha(p) for p in [source, reference, ROOT/'tests/test_inftrees_store.c',
                 Path(__file__).resolve(), REFERENCE/'manifest.json',
                 *[ROOT/name for name in manifest['files'] if not name.endswith('inftrees.c')]]}
    copied = out/'candidate-inftrees.c'
    copied.write_bytes(source.read_bytes())
    common = ['cc', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
              '-Wno-implicit-fallthrough', '-fstrict-aliasing', '-Wstrict-aliasing=2',
              '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-pie', '-no-pie', '-DZ_SOLO', '-DNO_GZIP', '-I'+str(ROOT/'port/zlib')]
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    report = dict(status='RUNNING', scope='Direct builder equivalence and sanitizer checks; no target speed claim.',
                  baseline=args.baseline, reference_commit=manifest['commit'], reference_git_proof=proof,
                  candidate_path=str(source), candidate_sha256=sha(copied), protected_sha256=protected,
                  sanitizer_environment={key: environment[key] for key in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')},
                  sanitizer_scope='ASan/UBSan and strict aliasing enabled. Leak detection is disabled under the ptrace supervisor.',
                  commands=[], configurations={})

    def run(command, label, directory):
        with (directory/(label+'.log')).open('w') as stream:
            result = subprocess.run([str(x) for x in command], stdout=stream,
                                    stderr=subprocess.STDOUT, env=environment, cwd=ROOT)
        log = directory/(label+'.log')
        report['commands'].append(dict(label=label, argv=[str(x) for x in command],
                                       returncode=result.returncode, cwd=str(ROOT), log=str(log)))
        return result.returncode, log.read_text()

    try:
        ret, _ = run(common+['-Dinflate_table=wf_c23_reference_table',
                            '-Dinflate_copyright=wf_c23_reference_copyright',
                            '-c', reference, '-o', out/'reference.o'], 'reference-build', out)
        assert ret == 0, 'Reference compile failed'
        for configuration, defines in CONFIGURATIONS.items():
            if args.configuration and configuration != args.configuration:
                continue
            directory = out/configuration
            directory.mkdir(exist_ok=True)
            ret, preprocessed = run(['cc', '-E', '-P', '-DZ_SOLO', '-DNO_GZIP',
                '-I'+str(ROOT/'port/zlib'), *defines, copied], 'preprocessed', directory)
            assert ret == 0, 'Candidate preprocessing failed'
            helper = re.search(r'wf_table_pack\(code entry\)\s*\{(.*?)\n\}', preprocessed, re.S)
            selected = ('legacy-struct' if not helper else 'union-fallback' if 'union {' in helper[1]
                        else 'big' if 'entry.op << 24' in helper[1] else 'little')
            variants = ['candidate']
            if not args.skip_mutant:
                variants.append('negative-width')
                if not args.baseline:
                    variants.append('negative-packed-bit')
            results = {}
            for variant in variants:
                candidate = copied
                if variant != 'candidate':
                    text = copied.read_text()
                    if variant == 'negative-width':
                        original = '        here.bits = (unsigned char)(len - drop);'
                        assert text.count(original) == 2
                        text = text.replace(original,
                            '        here.bits = (unsigned char)((len - drop) ^ 1u); /* negative control */', 1)
                    else:
                        original = '        packed = wf_table_pack(here);'
                        assert text.count(original) == 1
                        text = text.replace(original,
                            '        packed = wf_table_pack(here) ^ 1u; /* negative control */')
                    candidate = directory/(variant+'-inftrees.c')
                    candidate.write_text(text)
                binary = directory/variant
                ret, _ = run(common+defines+[candidate, ROOT/'tests/test_inftrees_store.c',
                    out/'reference.o', '-o', binary], variant+'-build', directory)
                assert ret == 0, f'{configuration}/{variant} compile failed'
                ret, log = run([binary], variant+'-run', directory)
                if variant == 'candidate':
                    assert ret == 0, f'{configuration}: full-state differential failed: {log.strip()}'
                    result = json.loads(log)
                    assert result['status'] == 'PASS' and result['cases'] == 80688
                    if not args.baseline:
                        expected = sys.byteorder if configuration == 'native' else 'union-fallback'
                        assert selected == expected, (configuration, selected, expected)
                else:
                    assert ret == 90 and 'MISMATCH table bytes ' in log, (
                        f'{configuration}/{variant}: expected semantic byte mismatch, got {ret}: {log}')
                    result = dict(status='EXPECTED_TABLE_BYTE_MISMATCH', returncode=ret,
                                  diagnostic=log.strip(), source_sha256=sha(candidate))
                result['binary_sha256'] = sha(binary)
                results[variant] = result
            report['configurations'][configuration] = dict(defines=defines,
                selected_representation=selected, preprocessed_sha256=sha(directory/'preprocessed.log'), results=results)
            print(configuration+': PASS 80688 tables; representation='+selected, flush=True)
        report['status'] = 'PASS'
    except Exception as error:
        report['status'], report['error'] = 'FAIL', repr(error)
        raise
    finally:
        report['changed_sources'] = [path for path, digest in protected.items() if sha(Path(path)) != digest]
        report['sources_unchanged'] = not report['changed_sources']
        if report['changed_sources']:
            report['status'] = 'FAIL'
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    assert report['sources_unchanged'], 'Source/header changed during the gate'


if __name__ == '__main__':
    main()
