#!/usr/bin/env python3
"""Produce a reproducible WIP source/history checkpoint, excluding disc/SDK binaries.

The checkpoint records exactly the bytes read, including uncommitted work.
It is explicitly not a release or a claim that concurrent work passed gates.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--evidence', action='append', type=Path, default=[])
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    paths = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=root).decode().split('\0')
    entries = {root.name+'/'+p: root/p for p in paths if p and (root/p).is_file()}
    for folder in sorted(root.parent.glob('recovery-*')):
        if folder.is_dir():
            entries.update((str(p.relative_to(root.parent)), p) for p in folder.rglob('*') if p.is_file())
    for folder in args.evidence:
        folder = folder.resolve()
        for p in folder.rglob('*'):
            if (p.is_file() and p.stat().st_size <= 5*1024*1024 and
                p.suffix in ('.json', '.bin', '.csv', '.c', '.h', '.py', '.sh', '.s', '.txt', '.log', '.patch', '.md') and
                'profile' not in p.parts):
                entries[root.name+'/'+str(p.relative_to(root))] = p
    manifest = {}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Write a separate completed file before replacing the prior checkpoint.
    temp = args.output.with_suffix('.partial.zip')
    with tempfile.TemporaryDirectory(prefix='wf-history-') as d:
        bundle = Path(d)/'recovery-history.bundle'
        subprocess.run(['git', 'bundle', 'create', str(bundle), '--all'], cwd=root, check=True)
        with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for name, path in sorted(entries.items()):
                data = path.read_bytes()
                z.writestr(name, data)
                manifest[name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
            z.write(bundle, 'recovery-history.bundle')
            z.writestr('CHECKPOINT.json', json.dumps({
                'status': 'WIP source recovery; not release-validated',
                'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
                'files': manifest}, indent=2)+'\n')
    temp.replace(args.output)
    print(json.dumps({'file': str(args.output.resolve()), 'bytes': args.output.stat().st_size,
                      'sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(), 'files': len(entries)}))


if __name__ == '__main__':
    main()
