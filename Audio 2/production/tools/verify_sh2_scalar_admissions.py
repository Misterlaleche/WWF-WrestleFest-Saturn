#!/usr/bin/env python3
"""Reproduce scalar9 RED/GREEN and full independent R01 sequence semantics.

These host tests do not measure Saturn performance or establish audio continuity.
The native checkpoint and known remaining fade failure are documented separately.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import time


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', required=True, type=Path, help='Original complete64KiB SOUND.ROM')
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--production', type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument('--seconds', type=int, default=600)
    p.add_argument('--cc', default='cc')
    a = p.parse_args()
    root, out, rom = a.production.resolve(), a.out.resolve(), a.rom.resolve()
    if out.is_relative_to(root) or not 1 <= a.seconds <= 1200:
        p.error('Use a private output outside production and seconds1..1200')
    if sha(rom) != 'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7':
        raise ValueError('Require the unchanged original complete sound ROM')
    original = root / 'tests/oracle/z80_native_before_scalar9.c'
    if sha(original) != '983bde9378737eeb6bdcf0ed9ff15a5b535ad55ed61ea87a84516b1613b04939':
        raise ValueError('Frozen pre-scalar9 native core changed')
    out.mkdir(parents=True, exist_ok=False)
    src, tests = root / 'src', root / 'tests'
    inputs = [rom, original, tests / 'test_sh2_scalar_admissions.c',
              tests / 'test_music_seq_service.c', tests / 'oracle/wf_music_seq_r01.c',
              src / 'wf_music_seq.c', src / 'wf_music_seq.h',
              *sorted((src / 'reference_driver').glob('z80*.c')),
              src / 'reference_driver/z80.h']
    before = {str(path): sha(path) for path in inputs}
    report = {'scope': 'HOST exact CPU/RAM/timer/event semantics; NOT Saturn performance or audio acceptance',
              'seconds_per_track': a.seconds, 'source_and_input_sha256': before,
              'commands': [], 'passed': False}

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
            raise RuntimeError(name + ': unexpected exit\n' + result.stdout + result.stderr)
        return result

    common = [a.cc, '-std=c11', '-O2', '-I' + str(src), '-I' + str(src / 'reference_driver')]
    aliases = ['z80_init=wf_ref_z80_init', 'z80_step=wf_ref_z80_step',
               'z80_gen_nmi=wf_ref_z80_gen_nmi', 'z80_gen_int=wf_ref_z80_gen_int',
               'z80_debug_output=wf_ref_z80_debug_output']
    defs = ['-D' + x for x in aliases]
    ref_z80, ref_seq = out / 'reference-z80.o', out / 'reference-r01.o'
    run('build_original_z80', [*common, *defs, '-c', src / 'reference_driver/z80.c', '-o', ref_z80])
    run('build_frozen_r01', [*common, *defs, '-Dwf_music_seq_init=wf_ref_init',
                           '-Dwf_music_seq_send=wf_ref_send', '-Dwf_music_seq_run=wf_ref_run',
                           '-c', tests / 'oracle/wf_music_seq_r01.c', '-o', ref_seq])
    for name, native in [('red', original), ('green', src / 'reference_driver/z80-native.c')]:
        exe = out / name
        run('build_' + name, [*common, tests / 'test_sh2_scalar_admissions.c', native,
                             src / 'reference_driver/z80-cold.c', ref_z80, '-o', exe])
        result = run(name, [exe], 2 if name == 'red' else 0)
        if name == 'red' and 'RED opcode01 was not admitted, count=0' not in result.stderr:
            raise ValueError('RED must fail the intended missing-admission assertion')
        if name == 'green' and 'PASS exact new scalar admissions total=1245190' not in result.stdout:
            raise ValueError('GREEN did not finish every unit case')
    seq_object = out / 'candidate-sequence.o'
    seq_options = ['-Os' if x == '-O2' else x for x in common]
    run('build_selected_sequence_Os', [*seq_options, '-DWF_MUSIC_NATIVE_CONTROL=1',
                                     '-c', src / 'wf_music_seq.c', '-o', seq_object])
    exe = out / 'sequence-oracle'
    run('build_sequence_oracle', [*common, '-DWF_MUSIC_NATIVE_CONTROL=1',
                                 tests / 'test_music_seq_service.c', seq_object,
                                 src / 'reference_driver/z80-native.c',
                                 src / 'reference_driver/z80-cold.c', ref_z80, ref_seq, '-o', exe])
    run('all18_and_live_transitions', [exe, rom, a.seconds])
    report['source_stable'] = before == {str(path): sha(path) for path in inputs}
    report['passed'] = report['source_stable']
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'seconds_per_track': a.seconds,
                      'scope': report['scope'], 'report': str(out / 'verification.json')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
