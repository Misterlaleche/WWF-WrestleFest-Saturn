#!/usr/bin/env python3
"""Generate the archived SH2 benchmark fixture from source and supplied inputs.

Build only: never starts Mednafen. The frozen diagnostic observer is deliberately
separate from the selected runtime. No private prebuilt directory, object, ROM,
firmware binary or game source is embedded in this package.
"""
from pathlib import Path
from types import SimpleNamespace
import argparse
import hashlib
import importlib.util
import json
import os
import struct
import subprocess
import sys


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('rom-zip', 'game-source', 'sdk', 'out'):
        p.add_argument('--' + name, required=True, type=Path)
    p.add_argument('--production', type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument('--runtime-source', type=Path,
                   help='Pinned GCC13.3 base68000 helper source; default SDK sibling src/gcc-lb1sf68.S')
    a = p.parse_args()
    root, rom, game, sdk, out = (getattr(a, n).resolve() for n in
                                ('production', 'rom_zip', 'game_source', 'sdk', 'out'))
    fixture = root / 'tests/benchmarks/sh2_checkpoint'
    manifest = json.loads((fixture / 'source-manifest.json').read_text())
    for relative, expected in manifest['files'].items():
        path = fixture / relative
        if sha(path) != expected:
            raise ValueError('Frozen checkpoint source changed: ' + relative)
    protected = [root, game, sdk, fixture]
    for parent in root.parents:
        if (parent / '.git').exists():
            protected.append(parent)
            break
    if a.out.is_symlink() or out.exists() or any(out == x or x in out.parents for x in protected):
        p.error('Use a fresh private output outside source, game and SDK trees')
    runtime = (a.runtime_source.resolve() if a.runtime_source else sdk.parent / 'src/gcc-lb1sf68.S')
    if sha(runtime) != '851f082a547137b70dfe686764c9312aed489df0ff00d60e2dd6c6fe64d5ba00':
        raise ValueError('Require the pinned base68000 GCC runtime source from the restored SDK environment')
    tape = game / 'src/runtime/src/wp1979_tape.inc'
    if sha(tape) != manifest['expected']['input_tape_sha256']:
        raise ValueError('Require the original unchanged WP1979 TAG tape')
    if not (game / 'src/runtime/audit/runtime18-cd').is_dir():
        raise ValueError('Original game source must include its archived runtime18-cd assets')
    out.mkdir(parents=True)
    env = dict(os.environ, YAUL_INSTALL_ROOT=str(sdk),
               YAUL_ARCH_SH_PREFIX='sh2eb-elf', YAUL_PROG_SH_PREFIX='sh2eb-elf')
    report = {'kind': 'wf_sh2_checkpoint_fixture_v1', 'built_from_sources': True,
              'native_emulator_started': False, 'passed': False,
              'scope': manifest['scope'], 'source_manifest_sha256': sha(fixture / 'source-manifest.json'),
              'rom_zip_sha256': sha(rom), 'game_source': str(game),
              'input_tape_sha256': sha(tape), 'sdk': str(sdk),
              'runtime_source_sha256': sha(runtime), 'build_directory': 'build', 'commands': []}

    def save():
        (out / 'fixture.json').write_text(json.dumps(report, indent=2) + '\n')

    def run(name, command):
        command = list(map(str, command))
        with (out / (name + '.log')).open('w') as log:
            result = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        report['commands'].append({'name': name, 'command': command, 'returncode': result.returncode})
        save()
        if result.returncode:
            raise RuntimeError(name + ' failed; exact command/log preserved')

    save()
    run('resources', [sys.executable, fixture / 'tools/generate_resources.py',
                      '--rom-zip', rom, '--out', out / 'resources'])
    run('firmware10', [sys.executable, fixture / 'tools/build_firmware.py',
                       '--package', fixture / 'audio2', '--out', out / 'firmware',
                       '--prefix', str(sdk / 'bin/m68k-linux-gnu-'),
                       '--runtime-source', runtime, '--no-timing'])
    firmware = out / 'firmware/SCSP.ELF'
    report['firmware_sha256'] = sha(firmware)
    save()
    if report['firmware_sha256'] != manifest['expected']['firmware_sha256']:
        raise ValueError('Fresh firmware does not reproduce the archived firmware10 bytes')
    run('game_and_module', [sys.executable, fixture / 'integration/build_combat_pair.py',
                            '--scenario', 'tag', '--game-source', game,
                            '--audio2', fixture / 'audio2', '--sdk', sdk,
                            '--resources', out / 'resources', '--firmware', firmware,
                            '--out', out / 'build'])
    build = out / 'build'
    report['baseline_module_sha256'] = sha(build / 'cd/MUSIC4.ELF')
    report['master_bin_sha256'] = sha(build / 'profiles/audio2/AUDIO2TAG.bin')
    save()
    for name in ('baseline_module_sha256', 'master_bin_sha256'):
        if report[name] != manifest['expected'][name]:
            raise ValueError('Fresh fixture differs from archived benchmark: ' + name)

    # Declare observations from this freshly linked master/module, never copy
    # absolute addresses from an old private capture command.
    spec = importlib.util.spec_from_file_location('checkpoint_capture', root / 'integration/capture_combat_pair.py')
    capture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture)
    args = SimpleNamespace(build=build, game_layout=build / 'game-layout.bin',
        capture_tool=root / 'tools/capture_mednafen.py', environment_root=sdk.parent,
        bios=out / 'BIOS_REQUIRED_AT_RUN', seconds=120, display=96, steps=3000, profiles=['audio2'])
    command = capture.commands(args)['audio2']
    command[command.index('--environment-root') + 1] = '{ENVIRONMENT_ROOT}'
    command[command.index('--bios') + 1] = '{BIOS}'
    command += ['--audio-only']
    master = json.loads((build / 'master-symbols.json').read_text())
    worker = json.loads((build / 'module-symbols.json').read_text())
    for prefix, address, count in [('master', master['_wf_master_cost_profile'], 9),
                                    ('worker', worker['_wf_sh2_cost_profile'], 17)]:
        for index in range(count):
            command += ['--watch', f'{prefix}_{index}:{address + index * 4:#x}:4']
    (build / 'capture-profile-command.json').write_text(json.dumps(command, indent=2) + '\n')
    report['capture_inputs_required_only_for_run'] = ['environment_root', 'bios', 'unused_display']
    report['original_game_files_unchanged'] = json.loads((build / 'integration-build.json').read_text())['original_game_files_unchanged']
    report['source_stable'] = all(sha(fixture / relative) == expected
                                 for relative, expected in manifest['files'].items())
    report['passed'] = report['source_stable']
    save()
    if not report['passed']:
        raise ValueError('Frozen fixture sources changed during generation')
    print(json.dumps({'fixture': str(out / 'fixture.json'), 'passed': True,
                      'firmware_sha256': report['firmware_sha256'],
                      'baseline_module_sha256': report['baseline_module_sha256'],
                      'master_bin_sha256': report['master_bin_sha256'],
                      'native_emulator_started': False}, indent=2))


if __name__ == '__main__':
    main()
