#!/usr/bin/env python3
"""Verify and restore the five approved external packages to a NEW directory."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import stat
import zipfile

PACKAGES = {
    'intro': ('WF-WP0-R04-BIN-CUE-Y-FUENTES.zip', 'a6ef24b69de89abc25d4882829b36d7497beb1f0464318facc527f413ecde5d7'),
    'entrance': ('WF-WP01-ENTRADA-TAG-R02-AUDIO-NATIVO-FUENTES.zip', 'dcacfe5d4de8067067194a9c0117fad95ad912982ebbb0030bf1e09cc4a8024d'),
    'presenter': ('WP02-Saturn-TAG-R02.zip', '69673d389d872af5d66e37670c228f1b6ab72e7d737066ecc2fde28ad90f4730'),
    'royal': ('wp1980_hw1_fuentes_y_pruebas.zip', 'afa7132a241e06208bd47b10487e2361e9cba59e5085a17e4d3bc0a3c135eb70'),
    'tag_r3': ('wp2015-checkpoint-06.zip', '321c23f6aebb576edb33effa8f71da3d61e45570405e97fc628b5de4f9aab405'),
}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--packages', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    out = args.out.absolute()
    if out.exists() or out.is_symlink():
        raise SystemExit('Choose a new output directory; previous checkpoints are preserved')
    checked = []
    for key, (name, expected) in PACKAGES.items():
        archive = args.packages / name
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != expected:
            raise SystemExit('Package SHA256 mismatch: ' + name)
        with zipfile.ZipFile(archive) as bundle:
            seen = set()
            for entry in bundle.infolist():
                path = PurePosixPath(entry.filename)
                if (path.is_absolute() or '..' in path.parts or '\\' in entry.filename
                        or not path.parts or stat.S_ISLNK(entry.external_attr >> 16)
                        or entry.filename in seen):
                    raise SystemExit('Unsafe/duplicate ZIP member: ' + entry.filename)
                seen.add(entry.filename)
            checked.append({'key': key, 'name': name, 'sha256': digest,
                            'members': len(seen), 'bytes': sum(x.file_size for x in bundle.infolist())})
    out.mkdir(parents=True)
    for record in checked:
        with zipfile.ZipFile(args.packages / record['name']) as bundle:
            for entry in bundle.infolist():
                target = out / record['key'] / entry.filename
                if entry.is_dir():
                    target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with target.open('xb') as stream:
                        stream.write(bundle.read(entry))
        print('RESTORED', record['key'], record['members'], flush=True)
    (out / 'packages.json').write_text(json.dumps({'packages': checked}, indent=2) + '\n')

if __name__ == '__main__':
    main()
