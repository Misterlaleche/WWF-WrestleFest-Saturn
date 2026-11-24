#!/usr/bin/env python3
"""Install the pinned Jo2026.3 compiler from the SDK supplied with this project.

Only compiler directories are restored. The project's adapted Jo source and
COMMON build files remain the ones delivered with this source package.
"""
from pathlib import Path, PurePosixPath
import argparse, hashlib, os, shutil, zipfile

SDK_SHA256='341209b0216f2e2247f26946a192cfe295d69a38df99c6639c86ee524eed5383'
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('sdk_zip',type=Path)
a=p.parse_args()
if hashlib.sha256(a.sdk_zip.read_bytes()).hexdigest()!=SDK_SHA256:
    p.error('SDK ZIP does not match the verified Jo Engine2026.3 archive')
target=ROOT/'sdk/Compiler'
for folder in ('LINUX','WINDOWS'):
    path=target/folder
    if path.is_symlink():path.unlink()
    elif path.exists():
        p.error(f'{path} already exists; use the existing installation or select a fresh source directory')
with zipfile.ZipFile(a.sdk_zip) as z:
    for info in z.infolist():
        parts=PurePosixPath(info.filename).parts
        if len(parts)<3 or parts[:2]!=('joengine-2026.3','Compiler') or parts[2] not in ('LINUX','WINDOWS'):
            continue
        if '..' in parts or PurePosixPath(info.filename).is_absolute():raise ValueError('Unsafe ZIP entry')
        dest=target.joinpath(*parts[2:])
        if info.is_dir():dest.mkdir(parents=True,exist_ok=True);continue
        dest.parent.mkdir(parents=True,exist_ok=True)
        with z.open(info) as source,dest.open('wb') as out:shutil.copyfileobj(source,out)
        mode=(info.external_attr>>16)&0o777
        if mode:dest.chmod(mode)
        elif dest.parent.name in ('bin','Other Utilities'):dest.chmod(0o755)
print('Pinned Jo2026.3 compiler installed; project Jo patches preserved.')
