#!/usr/bin/env python3
"""Compare the three coin helpers with the bundled, hash-pinned B12 source.

Both shared libraries are rebuilt for every invocation. The reference is the
exact core/src/wf_r18_frontend_coin.c from baseline commit3eeb30d, included in
tests/native so an exported ZIP needs neither Git nor existing build outputs.

The ctypes ABI is imported from test_coin_irq_rom.py. That existing oracle
module requires machine68k (install with: python3 -m pip install machine68k==0.5.1).
This helper comparison does not instantiate or run the 68000 emulator.
"""
import argparse
import ctypes as ct
import hashlib
import importlib.metadata
import importlib.util
import json
import os
from pathlib import Path
import random
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / 'tests/native/reference_b12_frontend_coin.c'
REFERENCE_SHA256 = '39794cbb835bf38bdbe20dd162352b2a89e0500f97562cb758028043a4c3087d'
SEED = 0x434048C
CASES_PER_HELPER = 10000
NAMES = tuple('wf_r18_frontend_coin_' + suffix for suffix in
              ('pulse_048c', 'service_04bc', 'credit_04f2'))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def import_abi():
    spec = importlib.util.spec_from_file_location('coin_irq_rom_abi',
                                                  ROOT / 'tests/test_coin_irq_rom.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(lib, name, raw, regs, sr, word, failure, mutation, abi):
    ram = (ct.c_uint8 * abi.RAM_BYTES).from_buffer_copy(raw)
    shadow = abi.Shadow((ct.c_uint32 * 8)(*regs[:8]), 1)
    addr = (ct.c_uint32 * 7)(*regs[8:])
    events = []

    @abi.Effect
    def effect(_, pe, pc):
        e, c = pe.contents, pc.contents
        events.append((e.kind, e.owner_rom, e.address, e.value,
                       tuple(c.regs.contents.d), tuple(c.a[i] for i in range(7)),
                       c.sr, c.ssp, c.usp, bytes(ram[0x4B:0x79])))
        if e.kind == 2:
            e.value = word
        if mutation:
            for j in range(3):
                c.regs.contents.d[j] = (regs[j] ^ mutation) & 0xFFFFFFFF
            c.sr ^= mutation & 0xFFFF
            for j in range(6):
                ram[0x4E + j] ^= (mutation >> (j * 3)) & 255
        return int(len(events) == failure)

    context = abi.Context(ram, len(ram), ct.pointer(shadow), addr,
                          0x1C3BCE, 0x1C3FD4, sr, effect, None)
    status = getattr(lib, name)(ct.byref(context))
    return {'status': status, 'ram': bytes(ram), 'd': tuple(shadow.d),
            'a': tuple(addr), 'sr': context.sr, 'ssp': context.ssp,
            'usp': context.usp, 'events': events}


def differences(expected, actual):
    result = {}
    for key in expected:
        if expected[key] == actual[key]:
            continue
        if key == 'ram':
            changed = [i for i, (a, b) in enumerate(zip(expected[key], actual[key])) if a != b]
            result[key] = {'different_bytes': len(changed),
                           'first_differences': [[i, expected[key][i], actual[key][i]]
                                                 for i in changed[:32]]}
        elif key == 'events':
            result[key] = {'expected': repr(expected[key]), 'actual': repr(actual[key])}
        else:
            result[key] = {'expected': expected[key], 'actual': actual[key]}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/coin-helper-tests')
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'),
                        help='Compiler command, optionally including wrapper arguments.')
    args = parser.parse_args()
    directory = args.build_dir.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    report_path, log_path = directory / 'helper-comparison.json', directory / 'run.log'
    report = {'format': 'wf-coin-helper-regression-v1', 'status': 'RUNNING',
              'invocation': {'argv': [sys.executable, str(Path(__file__).resolve()), *sys.argv[1:]],
                             'cwd': str(Path.cwd()),
                             'environment': {key: os.environ[key] for key in ('CC', 'PYTHONPATH')
                                             if key in os.environ}},
              'reference': {'path': str(REFERENCE), 'baseline_commit': '3eeb30d',
                             'expected_sha256': REFERENCE_SHA256},
              'seed': SEED, 'cases_per_helper': CASES_PER_HELPER,
              'comparisons': 0, 'helpers': {}, 'commands': [],
              'compared': ['complete16KiB RAM', 'D0-D7/A0-A6', 'SR/SSP/USP',
                           'return status', 'ordered MMIO owner/address/value and callback state',
                           'read/write callback mutations', 'callback rejection and rollback'],
              'scope': 'Pure helpers against unchanged B12 source; no speed or complete IRQ claim.'}
    with log_path.open('w') as log:
        def note(message):
            print(message, flush=True)
            print(message, file=log, flush=True)

        try:
            report['reference']['actual_sha256'] = sha(REFERENCE)
            if report['reference']['actual_sha256'] != REFERENCE_SHA256:
                raise AssertionError('Bundled B12 reference hash mismatch')
            abi = import_abi()
            try:
                dependency_version = importlib.metadata.version('machine68k')
            except importlib.metadata.PackageNotFoundError:
                dependency_version = None
            report['dependencies'] = {'machine68k': {
                'version': dependency_version, 'module': str(abi.mus.__file__),
                'reason': 'Imported by the shared ctypes ABI module; emulator is not executed.',
                'install_command': [sys.executable, '-m', 'pip', 'install', 'machine68k==0.5.1']}}
            native = ROOT / 'core/src/wf_r18_frontend_coin.c'
            shadow = ROOT / 'core/src/wf_r17_m68k_shadow.c'
            sources = [Path(__file__).resolve(), REFERENCE, native, shadow,
                       ROOT / 'tests/test_coin_irq_rom.py',
                       *sorted((ROOT / 'core/include').glob('*.h'))]
            hashes = {str(p.relative_to(ROOT)): sha(p) for p in sources}
            report['source_sha256'] = hashes
            cc = shlex.split(args.cc)
            if not cc:
                raise ValueError('Empty compiler command')
            # Unique names prevent a stale ctypes loader handle or a previously
            # built .so from becoming either side of a later comparison.
            with tempfile.TemporaryDirectory(prefix='libraries-', dir=directory) as temporary:
                libraries = []
                for label, source in (('reference-b12', REFERENCE), ('current', native)):
                    library = Path(temporary) / (label + '.so')
                    command = [*cc, '-shared', '-fPIC', '-std=c11', '-O2',
                               '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'core/include'),
                               str(source), str(shadow), '-o', str(library)]
                    note('BUILD ' + shlex.join(command))
                    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
                    report['commands'].append({'argv': command, 'cwd': str(ROOT),
                                               'returncode': completed.returncode,
                                               'stdout': completed.stdout, 'stderr': completed.stderr})
                    if completed.stdout:
                        note(completed.stdout.rstrip())
                    if completed.stderr:
                        note(completed.stderr.rstrip())
                    completed.check_returncode()
                    report.setdefault('libraries_sha256', {})[label] = sha(library)
                    lib = ct.CDLL(str(library))
                    for name in NAMES:
                        getattr(lib, name).argtypes = [ct.POINTER(abi.Context)]
                        getattr(lib, name).restype = ct.c_int
                    libraries.append(lib)
                before, after = libraries
                randomizer = random.Random(SEED)
                for name in NAMES:
                    counts = {'comparisons': 0, 'callback_events': 0,
                              'cases_with_mutation': 0, 'mutated_read_events': 0,
                              'mutated_write_events': 0, 'rejected_callbacks': 0,
                              'rejected_second_callbacks': 0}
                    report['helpers'][name] = counts
                    for ordinal in range(CASES_PER_HELPER):
                        # Preserve the exact seed, cases and draw order of the
                        # original30,000-case compare_helpers.py experiment.
                        raw = bytearray(abi.RAM_BYTES)
                        raw[0x4B:0x79] = randomizer.randbytes(0x2E)
                        regs = [randomizer.getrandbits(32) for _ in range(15)]
                        if ordinal % 4:
                            regs[0] &= 0xFFFF0000
                        regs[2] = (regs[2] & 0xFFFFFF00) | (
                            0, 1, 3, 4, 9, 10, 127, 128, 255)[ordinal % 9]
                        sr = randomizer.getrandbits(16)
                        word = randomizer.getrandbits(16)
                        failure = ordinal % 3
                        mutation = randomizer.getrandbits(32) if ordinal % 5 == 0 else 0
                        expected = run(before, name, raw, regs, sr, word, failure, mutation, abi)
                        actual = run(after, name, raw, regs, sr, word, failure, mutation, abi)
                        if expected != actual:
                            report['failure'] = {'helper': name, 'ordinal': ordinal,
                                'input': {'ram_004b_0078_hex': raw[0x4B:0x79].hex(),
                                          'registers': regs, 'sr': sr, 'input_word': word,
                                          'failure_callback': failure, 'mutation': mutation},
                                'differences': differences(expected, actual)}
                            raise AssertionError(f'{name} case{ordinal}: helper states differ')
                        counts['comparisons'] += 1
                        report['comparisons'] += 1
                        counts['callback_events'] += len(expected['events'])
                        counts['cases_with_mutation'] += bool(mutation)
                        if mutation:
                            counts['mutated_read_events'] += sum(e[0] == 2 for e in expected['events'])
                            counts['mutated_write_events'] += sum(e[0] == 1 for e in expected['events'])
                        if failure and len(expected['events']) >= failure:
                            counts['rejected_callbacks'] += 1
                            counts['rejected_second_callbacks'] += failure == 2
                    note(f'PASS {name}: {counts["comparisons"]} helper comparisons')
            report['sources_changed_during_run'] = any(sha(ROOT / path) != value
                                                       for path, value in hashes.items())
            if report['sources_changed_during_run']:
                raise AssertionError('Test sources changed while the libraries were being compared')
            if report['comparisons'] != 30000:
                raise AssertionError('Incomplete helper corpus')
            report['status'] = 'PASS'
            note('PASS:30000 B12/current helper comparisons, including callback mutation and rejection')
        except Exception as error:
            report['status'] = 'FAIL'
            report['error'] = f'{type(error).__name__}: {error}'
            note(report['error'])
            raise
        finally:
            report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
