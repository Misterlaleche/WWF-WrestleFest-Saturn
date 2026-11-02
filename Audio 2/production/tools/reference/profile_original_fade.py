#!/usr/bin/env python3
"""Read-only scalar PC profile: original command02 for6s, then original1F for8s.

This snapshots the sequencer and the pinned original core, disables native
batch admission, and retains the existing optimized idle skip. The helper
compares all callback fields and complete CPU/RAM/timer state against a second
run with a4096-instruction budget. Counts are not native CPU timings.
Protected input bytes, trace values and opcode details stay in private output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
PACKAGE = HERE.parents[1]
ROM_SHA = 'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7'
CORE_SHA = 'd82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    rom, out = args.rom.resolve(), args.out.resolve()
    if out == PACKAGE or PACKAGE in out.parents:
        parser.error('Private opcode/trace output must stay outside the source package')
    if sha(rom) != ROM_SHA:
        parser.error('Original SOUND.ROM identity mismatch')
    core = PACKAGE / 'src/reference_driver/z80.c'
    if sha(core) != CORE_SHA:
        parser.error('Pinned independent original Z80 core identity mismatch')
    out.mkdir(parents=True, exist_ok=False)
    source = out / 'frozen-src'
    originals = [PACKAGE / name for name in ['src/wf_music_seq.c', 'src/wf_music_seq.h',
                 'src/reference_driver/z80.c', 'src/reference_driver/z80.h']]
    manifest = {'kind': 'ORIGINAL_SCALAR_PC_COUNTS_NOT_NATIVE_COST', 'rom_sha256': ROM_SHA,
                'source_sha256': {}, 'native_runtime_modified': False}
    for path in originals:
        data = path.read_bytes()
        target = source / path.relative_to(PACKAGE / 'src')
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        manifest['source_sha256'][str(path)] = hashlib.sha256(data).hexdigest()
    helper = HERE / 'profile_original_fade.c'
    manifest['source_sha256'][str(helper)] = sha(helper)
    manifest['source_sha256'][str(Path(__file__).resolve())] = sha(Path(__file__).resolve())
    helper_copy = out / 'profile_original_fade.c'
    helper_copy.write_bytes(helper.read_bytes())
    executable = out / 'profile_original_fade'
    commands = [
        [args.cc, '-O3', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(source),
         '-Dz80_step=wf_profile_core_step', '-c', str(source / 'reference_driver/z80.c'),
         '-o', str(out / 'original-z80.o')],
        [args.cc, '-O3', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(source),
         str(helper_copy), str(source / 'wf_music_seq.c'), str(out / 'original-z80.o'),
         '-o', str(executable)],
        [str(executable), str(rom), str(out)],
    ]
    manifest['commands'] = commands
    with (out / 'run-manifest.json').open('x') as stream:
        json.dump(manifest, stream, indent=2)
        stream.write('\n')
    for index, command in enumerate(commands):
        result = subprocess.run(command, capture_output=True, text=True)
        with (out / f'command-{index}.log').open('x') as stream:
            stream.write(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f'Profile build/run failed; see command-{index}.log')
    report = json.loads((out / 'profile.json').read_text())
    report['executable_sha256'] = sha(executable)
    report['input_sha256'] = {str(rom): sha(rom)}
    report['source_sha256'] = manifest['source_sha256']
    report['sources_still_match_snapshot'] = all(sha(path) == digest for path, digest in manifest['source_sha256'].items())
    report['output_sha256'] = {name: sha(out / name) for name in
                              ['profile.json', 'seconds.csv', 'pc-counts.csv', 'verified-events.csv']}
    with (out / 'profile-report.json').open('x') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'complete': report['complete'],
                      'exact_callback_events_compared': report['exact_callback_events_compared'],
                      'exact_cpu_ram_timer_state_checkpoints': report['exact_cpu_ram_timer_state_checkpoints'],
                      'output': str(out / 'profile-report.json')}), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
