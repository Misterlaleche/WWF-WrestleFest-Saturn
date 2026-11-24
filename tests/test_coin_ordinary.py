#!/usr/bin/env python3
"""Compare ordinary coin paths with the unchanged, pinned C18 source.

The C harness exhausts all16-bit SR values and eight live coin selectors,
checks the complete enter object, and exercises legal RAM aliases, callback
mutation/replacement/reentry, rejection, and general arithmetic boundaries.
The optional leaf-entry codegen gate verifies the intended removed call.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / 'tests/native/reference_c18_frontend_coin.c'
REFERENCE_SHA = '13fbfe3f4ae27e4f7576e1823a535d5cd2b43de2697390de9e6eaf3cdca05c15'
SUFFIXES = ('enter', 'step', 'run', 'pulse_048c', 'service_04bc', 'credit_04f2')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'core/src/wf_r18_frontend_coin.c')
    parser.add_argument('--reference-source', type=Path, default=REFERENCE)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/coin-ordinary-tests')
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--require-leaf-entry', action='store_true')
    parser.add_argument('--mutants', action='store_true')
    args = parser.parse_args()
    out = args.build_dir.resolve();out.mkdir(parents=True, exist_ok=True)
    assert sha(args.reference_source) == REFERENCE_SHA, 'Pinned C18 reference mismatch'
    sources = [args.source, args.reference_source, ROOT / 'tests/test_coin_ordinary.c',
               Path(__file__).resolve(), *sorted((ROOT / 'core/include').glob('*.h'))]
    inputs = {str(path): sha(path) for path in sources}
    report = {'format': 'wf-coin-ordinary-differential-v1', 'status': 'RUNNING',
              'reference_commit': '24c23192541925e7356295db611b7b96e64b4252',
              'reference_sha256': REFERENCE_SHA, 'source_sha256': inputs,
              'sanitize': args.sanitize, 'commands': [], 'mutants': {},
              'scope': 'Native exact old/new state and events, plus optional standalone SH-2 call-removal check. No target timing claim.'}
    for label, path in [('reference.c', args.reference_source), ('current.c', args.source),
                        ('driver.c', ROOT / 'tests/test_coin_ordinary.c'), ('runner.py', Path(__file__).resolve())]:
        (out / label).write_bytes(path.read_bytes())
    env = os.environ.copy();env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1';env['UBSAN_OPTIONS'] = 'halt_on_error=1'

    def run(command, label, expected=0):
        completed = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
        (out / (label + '.log')).write_text(completed.stdout + completed.stderr)
        report['commands'].append({'argv': command, 'cwd': str(ROOT), 'label': label,
                                   'returncode': completed.returncode,
                                   'log_sha256': sha(out / (label + '.log'))})
        if completed.returncode != expected:
            raise AssertionError(f'{label}: exit {completed.returncode}, expected {expected}; {completed.stderr[-2000:]}')
        return completed

    try:
        cc = shlex.split(args.cc)
        flags = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-fno-builtin', '-I' + str(ROOT / 'core/include')]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        renames = ['-Dwf_r18_frontend_coin_' + name + '=reference_coin_' + name for name in SUFFIXES]
        reference_obj = out / 'reference.o'
        run([*cc, *flags, *renames, '-c', str(args.reference_source), '-o', str(reference_obj)], 'reference-build')

        def compare(source, label, expected=0):
            binary = out / label
            run([*cc, *flags, str(ROOT / 'tests/test_coin_ordinary.c'), str(source), str(reference_obj),
                 '-o', str(binary)], label + '-build')
            completed = run([str(binary)], label, expected)
            if expected:
                assert completed.stderr.startswith('FAIL '), 'Mutant must fail a semantic assertion'
                report['mutants'][label] = {'detected': True, 'failure': completed.stderr.strip(), 'source_sha256': sha(source)}
            else:
                report['counts'] = json.loads(completed.stdout.splitlines()[-1])
                report['binary_sha256'] = sha(binary)
                print(completed.stdout, end='', flush=True)

        compare(args.source, 'ordinary')
        if args.mutants:
            original = args.source.read_text()
            changes = {
                'mutant-drop-x': original.replace('c->sr&0xfff0u', 'c->sr&0xffe0u', 1),
                'mutant-stale-selector': original.replace('(c->regs->d[1]&7u)', '(0u)', 1),
                'mutant-incomplete-enter': re.sub(r'(?:__builtin_)?memset\(s,0,sizeof \*s\);', '(void)s;', original, count=1),
            }
            for label, content in changes.items():
                assert content != original
                source = out / (label + '.c');source.write_text(content)
                compare(source, label, 86)
        if args.require_leaf_entry:
            assembly = out / 'coin-sh2.s'
            command = [str(ROOT / 'sdk/Compiler/LINUX/bin/sh-none-elf-gcc-8.2.0'), '-m2', '-std=gnu99',
                       '-O2', '-fno-builtin', '-fno-peephole2', '-Wall', '-Wextra', '-Werror',
                       '-I' + str(ROOT / 'core/include'), '-I' + str(ROOT / 'sdk/Compiler/WINDOWS/sh-elf/include'),
                       '-S', str(args.source), '-o', str(assembly)]
            run(command, 'codegen-build')
            text = assembly.read_text();start = text.index('_wf_r18_frontend_coin_enter:')
            end = text.index('\n\t.size\t_wf_r18_frontend_coin_enter,', start)
            body = text[start:end]
            (out / 'coin-enter-sh2.s').write_text(body + '\n')
            calls = re.findall(r'^\s*(?:jsr|bsr)\s+.*$', body, re.M)
            report['leaf_entry'] = {'calls': calls, 'assembly_sha256': sha(assembly), 'status': 'FAIL' if calls else 'PASS'}
            assert not calls, 'SH-2 enter retains a helper call'
        report['inputs_unchanged'] = all(sha(Path(path)) == digest for path, digest in inputs.items())
        assert report['inputs_unchanged']
        report['status'] = 'PASS'
    except Exception as error:
        report['status'] = 'FAIL';report['error'] = str(error)
        raise
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS ordinary coin differential; inputs unchanged', flush=True)


if __name__ == '__main__':
    main()
