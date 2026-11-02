#!/usr/bin/env python3
"""Record a same-input native TAG or Royal pair through read-only capture tools.

The supplied game owns its original logical-frame input tape and endpoint.
No debugger memory writes or synthesized gameplay inputs are performed.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys


def symbol_address(symbols, name):
    if name in symbols:
        return symbols[name]
    # HW1 uses LTO, which may rename a private observer symbol. Reject
    # ambiguity rather than guessing an address from another game build.
    matches = [value for key, value in symbols.items()
               if key.startswith(name + '.lto_priv.')]
    if len(matches) != 1:
        raise ValueError('Missing or ambiguous native symbol: ' + name)
    return matches[0]


def commands(a):
    build = a.build.resolve()
    profile_info = json.loads((build / 'integration-build.json').read_text())
    royal = profile_info.get('scenario', 'tag') == 'royal'
    prefix = '_wp1980_' if royal else '_wp1979_'
    artifact = profile_info.get('artifact_basename', 'AUDIO2TAG')
    symbols = json.loads((build / 'master-symbols.json').read_text())
    layout = struct.unpack('>7I', a.game_layout.read_bytes())
    watches = {name: symbol_address(symbols, symbol) for name, symbol in (
        ('started', prefix+'started'), ('draws', prefix+'draws'),
        ('vblanks', prefix+'vblanks'), ('fatal_owner', '_fatal_owner'),
        ('fatal_dependency', '_fatal_dependency'), ('finished_frame', '_finished_frame'),
        ('video_clock', '_video_clock'))}
    if royal:
        watches['finished_presentation'] = symbols['_finished_presentation']
    watches['game_frame'] = symbols['_rt'] + layout[3]
    watches.update({name: symbols['_wf_audio2_cadence'] + index * 4 for name, index in (
        ('cadence_steps', 5), ('cadence_vblanks', 6), ('cadence_max_gap', 9), ('cadence_missed', 8))})
    watches.update({name: symbols['_wf_audio2_integration_diag'] + index * 4 for name, index in (
        ('enabled', 0), ('boot_stage', 1), ('api_state', 2), ('api_error', 3),
        ('worker_state', 4), ('worker_error', 5), ('accepted', 6), ('queued', 7),
        ('overflows', 9), ('unsupported', 10), ('skipped', 11), ('produced', 12),
        ('command_hash', 13), ('audio_fault_frame', 14))})
    common = [sys.executable, str(a.capture_tool.resolve()), '--environment-root',
              str(a.environment_root.resolve()), '--bios', str(a.bios.resolve()),
              '--seconds', str(a.seconds), '--display', str(a.display),
              '--cartridge', profile_info.get('emulator_cartridge', 'none'),
              '--stop-watch', 'finished_presentation' if royal and a.steps is None else 'cadence_steps',
              '--stop-value', '2' if royal and a.steps is None else str(a.steps or 7372),
              '--ready-watch', 'started', '--ready-value', '1', '--dump-wram']
    for name, address in watches.items():
        common += ['--watch', f'{name}:{address:#x}:4']
    return {profile: common + ['--cue', str(build / 'profiles' / profile / (artifact+'.cue')),
                              '--out', str(build / (profile + '-fullmatch'))]
            for profile in a.profiles}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('build', 'game-layout', 'capture-tool', 'environment-root', 'bios'):
        p.add_argument('--' + name, required=True, type=Path)
    p.add_argument('--display', type=int, default=96)
    p.add_argument('--seconds', type=int, default=210, help='Wall timeout including CD boot')
    p.add_argument('--steps', type=int, help='Optional partial cadence window; otherwise original TAG endpoint or completed Royal result')
    p.add_argument('--profiles', nargs='+', choices=('control', 'audio2'), default=['control', 'audio2'])
    p.add_argument('--prepare-only', action='store_true')
    a = p.parse_args()
    runs = commands(a)
    (a.build / 'capture-commands.json').write_text(json.dumps(runs, indent=2) + '\n')
    if a.prepare_only:
        print(json.dumps(runs, indent=2))
        return
    for profile, command in runs.items():
        destination = a.build / (profile + '-fullmatch')
        if destination.exists():
            raise ValueError('Refuse to overwrite existing capture: ' + str(destination))
        print('Recording native combat profile: ' + profile, flush=True)
        with (a.build / (profile + '-capture.log')).open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)


if __name__ == '__main__':
    main()
