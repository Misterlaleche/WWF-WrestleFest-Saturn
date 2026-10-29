#!/usr/bin/env python3
"""Reproducible, read-only observation of a newly linked WP1979 Saturn image."""
import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / 'native/src/runtime'
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('name')
p.add_argument('--frames', type=int, default=1800)
p.add_argument('--video', action='store_true')
p.add_argument('--snapshot-every', type=int, default=120)
p.add_argument('--wall-timeout', type=int, default=180)
p.add_argument('--cue', type=Path)
p.add_argument('--schedule', type=Path)
p.add_argument('--core', type=Path)
p.add_argument('--output-root', type=Path, default=ROOT / 'measure',
               help='Use a local temporary directory for streaming capture files')
p.add_argument('--probe-audio', action='store_true', help='Read-only native SCSP and music-state evidence')
p.add_argument('--actors', action='store_true', help='Read the four actor records without modifying emulated RAM')
a = p.parse_args()
out = a.output_root.resolve() / a.name
# Never mix an old report with a new recording in the same destination.
out.mkdir(parents=True, exist_ok=False)
elf = RUNTIME / 'audit/runtime18-build/wrestlefest-wp1979.elf'
cue = (a.cue or RUNTIME / 'audit/runtime18-disc/wrestlefest-wp1979.cue').resolve()
disc = out / 'disc'
disc.mkdir(exist_ok=True)
cue_text = cue.read_text()
tracks = re.findall(r'^FILE\s+"([^"]+)"\s+BINARY\s*$', cue_text, re.M)
assert len(tracks) == 1 and len(re.findall(r'^\s*TRACK\s', cue_text, re.M)) == 1
assert Path(tracks[0]).name == tracks[0], 'Only a local single-track CUE is supported'
for source, target in [(elf, out / 'program.elf'),
                       (elf.with_suffix('.map'), out / 'program.map'),
                       (elf.with_suffix('.bin'), out / 'program.bin'),
                       (cue, disc / cue.name),
                       (cue.parent / tracks[0], disc / tracks[0])]:
    shutil.copy2(source, target)
track = (disc / tracks[0]).read_bytes()
if 'MODE1/2352' in cue_text:
    assert len(track) % 2352 == 0
    iso = b''.join(track[i+16:i+2064] for i in range(0, len(track), 2352))
else:
    assert 'MODE1/2048' in cue_text and len(track) % 2048 == 0
    iso = track
sys.path.insert(0, str(ROOT / 'native/validation'))
from verify_disc_exact import directory_files
assert directory_files(iso)['A.BIN']['data'] == (out / 'program.bin').read_bytes(), \
       'Refusing capture: disc A.BIN differs from the linked binary'
identity = {str(path.relative_to(out)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in [out / 'program.elf', out / 'program.map', out / 'program.bin',
                         disc / cue.name, disc / tracks[0]]}
(out / 'identity.json').write_text(json.dumps(dict(a_bin_matches=True, sha256=identity), indent=2) + '\n')
elf = out / 'program.elf'
nm = ROOT / 'tooling/sh2eb-elf/bin/sh2eb-elf-nm'
symbols = {}
for line in subprocess.check_output([str(nm), '-n', str(elf)], text=True).splitlines():
    fields = line.split()
    if len(fields) == 3:
        symbols[fields[2].lstrip('_').split('.lto_priv')[0]] = int(fields[0], 16)
watch = [
    'wp1979_cd_init_rc', 'wp1979_cdfs_stage', 'wp1979_cdfs_lba',
    'wp1979_cdfs_rc', 'wp1979_started', 'wp1979_draws', 'wp1979_vblanks',
    'wp1979_max_gap', 'wp1979_demo_active', 'fatal_owner', 'fatal_dependency',
    'fatal_frame', 'video_clock', 'finished_frame', 'finished_result',
    'finished_count', 'finished_timer', 'finished_presentation',
    'finished_vblanks', 'finished_presentation_vblanks',
    'window_misses', 'window_sectors',
    'profile_frames', 'profile_vblanks', 'profile_start_vblank',
]
args = ['python', str(ROOT / 'tooling/capture_yabause.py'), '--cue',
        str(disc / cue.name),
        '--out', str(out), '--frames', str(a.frames), '--snapshot-every',
        str(a.snapshot_every), '--wall-timeout', str(a.wall_timeout), '--dump-memory']
if a.probe_audio:
    args += ['--probe-audio', '--watch', 'music_state:0x05c7f084:2',
             '--watch', 'music_error:0x05c7f088:4', '--watch', 'music_events:0x05c7f08c:4',
             '--watch', 'scsp_mail:0x05a0c040:32']
if not a.video:
    args.append('--no-video')
if a.schedule:
    args += ['--schedule', str(a.schedule)]
if a.core:
    args += ['--core', str(a.core.resolve())]
for name in watch:
    if name in symbols:
        args += ['--watch', f'{name}:{hex(symbols[name])}:4']
# The SH-2 ABI places frame_counter at byte24 of the first member (g) of rt.
args += ['--watch', f'game_frame:{hex(symbols["rt"] + 24)}:4']
for name, size in [('wp1979_cdfs_probe', 16), ('boot_timing', 32),
                   ('profile_totals', 96), ('profile_calls', 96)]:
    if name in symbols:
        args += ['--watch', f'{name}:{hex(symbols[name])}:{size}']
if a.actors:
    offset, stride, *_ = struct.unpack('>5I', (ROOT / 'measure/actors-layout.bin').read_bytes())
    assert stride == 0x10c
    for slot in (0, 1, 4, 5):
        args += ['--watch', f'actor{slot}:{hex(symbols["rt"] + offset + slot * stride)}:176']
(out / 'symbols.json').write_text(json.dumps(symbols, indent=2) + '\n')
(out / 'command.json').write_text(json.dumps(args, indent=2) + '\n')
with (out / 'capture.log').open('w') as log:
    code = subprocess.call(args, stdout=log, stderr=subprocess.STDOUT)
print('capture return:', code)
report = json.loads((out / 'report.json').read_text())
print(json.dumps({k: v for k, v in report.items() if k not in ['samples', 'events', 'audio_probe']}, indent=2))
print(json.dumps(report['samples'][-1], indent=2))
raise SystemExit(code)
