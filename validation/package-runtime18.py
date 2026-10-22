#!/usr/bin/env python3
"""Package committed source, local assets, discs and selected validation evidence."""
import argparse
import hashlib
import io
from pathlib import Path
import subprocess
import tarfile
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('output', type=Path)
p.add_argument('--arcade-zip', type=Path, required=True)
p.add_argument('--replay-dir', choices=('replay', 'replay-noinput'), default='replay')
p.add_argument('--checkpoint-dir', choices=('checkpoint', 'checkpoint-noinput'), default='checkpoint')
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
def git(*args):
    return subprocess.check_output(['git', *args], cwd=root)
if git('status', '--porcelain').strip():
    raise SystemExit('Commit the source checkpoint before packaging')
revision = git('rev-parse', 'HEAD').decode().strip()
files = {}
executables = set()
with tarfile.open(fileobj=io.BytesIO(git('archive', '--format=tar', revision))) as archive:
    for entry in archive:
        if entry.isfile():
            files['source/' + entry.name] = archive.extractfile(entry).read()
            if entry.mode & 0o111:
                executables.add('source/' + entry.name)
for name in ('ROM68K.BIN', 'SPRITES.PAK', 'R18RING.BIN', 'HOTSPR.BIN'):
    path = root / 'src/runtime/audit/runtime18-cd' / name
    files['source/src/runtime/audit/runtime18-cd/' + name] = path.read_bytes()
files['reference/wwfwfest.zip'] = a.arcade_zip.read_bytes()
results = root / 'validation/results/runtime18'
for name in ('wrestlefest-runtime18.bin', 'wrestlefest-runtime18.cue',
             'build.log', 'unresolved.txt', 'disc-validation.json', 'conversion.json',
             'A.BIN', 'wrestlefest-runtime18.elf', 'wrestlefest-runtime18.map'):
    files['evidence/disc/' + name] = (results / name).read_bytes()
for folder in (a.checkpoint_dir, a.replay_dir):
    for path in sorted((results / folder).rglob('*')):
        if path.is_file():
            files['evidence/' + folder + '/' + path.relative_to(results / folder).as_posix()] = path.read_bytes()
for folder in ('rom', 'core-sanitizers', 'core-host'):
    for path in sorted((results / folder).iterdir()):
        if path.suffix in ('.json', '.log'):
            files['evidence/' + folder + '/' + path.name] = path.read_bytes()
files['CHECKPOINT.txt'] = (
    'WrestleFest Saturn RUNTIME-18\n'
    f'Commit: {revision}\n'
    'Source: source/ (committed tree plus canonical local assets).\n'
    'Playable disc: evidence/disc/wrestlefest-runtime18.cue (keep BIN beside it).\n'
    f'Validation replay: evidence/{a.replay_dir}/; test-only deterministic input/seed.\n'
    'Read source/CURRENT-STATE-LATEST.md and source/validation/RUNTIME18-CPU-CHECKPOINT.md.\n'
    'No physical Saturn validation or universal ROM-EXACT claim.\n'
    'HOST: make -C source/src/runtime test-runtime18-complete-match\n'
    'No-input HOST: make -C source/src/runtime test-runtime18-noinput-complete-match\n'
    'Build: cd source; DOCKER=/path/to/docker sh validation/build-runtime18.sh /path/to/sh2eb-elf\n'
    'ROM tests: validation/requirements-rom.txt; original ZIP in reference/wwfwfest.zip.\n'
    'SDK, BIOS and Docker are external dependencies. ISO timestamps may vary on rebuild.\n'
    'Check all delivered bytes with: shasum -a 256 -c SHA256SUMS\n'
).encode()
files['SHA256SUMS'] = ''.join(
    f'{hashlib.sha256(data).hexdigest()}  {name}\n'
    for name, data in sorted(files.items())).encode()
a.output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(a.output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for name, data in sorted(files.items()):
        info = zipfile.ZipInfo(name, (2026, 10, 23, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = (0o100755 if name in executables else 0o100644) << 16
        z.writestr(info, data, compresslevel=9)
with zipfile.ZipFile(a.output) as z:
    assert z.testzip() is None
print(f'{a.output}\ncommit={revision}\nfiles={len(files)}\nsha256={hashlib.sha256(a.output.read_bytes()).hexdigest()}')
