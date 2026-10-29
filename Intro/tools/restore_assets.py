#!/usr/bin/env python3
"""Restore the exact external R04 assets from the user's delivery ZIP.

Uses only the Python standard library. Validates every input before creating
files, refuses conflicting files, and never extracts arbitrary ZIP paths.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def destination(relative):
    path = PurePosixPath(relative)
    if path.is_absolute() or '..' in path.parts or not path.parts:
        raise ValueError('Invalid asset path: ' + relative)
    target = ROOT.joinpath(*path.parts)
    target.resolve().relative_to(ROOT)
    return target


def check_bytes(data, entry):
    if len(data) != entry['bytes'] or digest(data) != entry['sha256']:
        raise ValueError('Asset identity mismatch: ' + entry['path'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', nargs='?', type=Path)
    parser.add_argument('--check', action='store_true', help='Verify existing assets without writing')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'ASSETS-MANIFEST.json').read_text())
    entries = manifest['files']
    if len({entry['path'] for entry in entries}) != len(entries):
        raise ValueError('Duplicate manifest destination')
    if args.check:
        for entry in entries:
            check_bytes(destination(entry['path']).read_bytes(), entry)
        print(json.dumps({'result': 'PASS', 'verified': len(entries), 'written': 0}))
        return
    if args.archive is None:
        parser.error('Provide WF-WP0-R04-BIN-CUE-Y-FUENTES.zip, or use --check')
    archive_data = args.archive.read_bytes()
    expected = manifest['archive']
    if len(archive_data) != expected['bytes'] or digest(archive_data) != expected['sha256']:
        raise ValueError('Wrong archive: expected the original WP0 R04 delivery SHA-256')
    import io
    pending = []
    reused = 0
    with zipfile.ZipFile(io.BytesIO(archive_data)) as archive:
        names = archive.namelist()
        for entry in entries:
            if names.count(entry['archive_member']) != 1:
                raise ValueError('Missing or duplicate archive member: ' + entry['archive_member'])
            data = archive.read(entry['archive_member'])
            check_bytes(data, entry)
            target = destination(entry['path'])
            if target.exists():
                check_bytes(target.read_bytes(), entry)
                reused += 1
            else:
                pending.append((target, data))
    # All source bytes and existing destinations have passed before any write.
    for target, data in pending:
        target.parent.mkdir(parents=True, exist_ok=True)
        destination(target.relative_to(ROOT).as_posix())
        with target.open('xb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
    for entry in entries:
        check_bytes(destination(entry['path']).read_bytes(), entry)
    print(json.dumps({'result': 'PASS', 'verified': len(entries),
                      'written': len(pending), 'reused': reused}))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        print('Asset restoration failed: ' + str(error), file=sys.stderr)
        sys.exit(1)
