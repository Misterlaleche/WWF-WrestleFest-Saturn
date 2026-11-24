#!/usr/bin/env python3
"""Freeze source and hard-link immutable disc assets for an isolated build.

The application binary in cd/ is deliberately excluded. Another build can
never change this variant's running disc or overwrite its executable asset.
"""
from pathlib import Path
import argparse, hashlib, json, os, shutil

root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('name')
p.add_argument('--source',type=Path,default=root)
a=p.parse_args()
source=a.source.resolve()
if not a.name.replace('-','').replace('_','').isalnum():p.error('simple variant name required')
dest=root/'build/variants'/a.name
dest.mkdir(parents=True,exist_ok=False)
def ignore(folder,names):
    return [n for n in names if n.endswith(('.o','.d','.pyc')) or n=='__pycache__']
for folder in ('core','port','sdk'):
    shutil.copytree(source/folder,dest/folder,symlinks=True,ignore=ignore)
for name in ('makefile','core_sources.mk'):
    shutil.copy2(source/name,dest/name)
(dest/'cd').mkdir()
for src in (source/'cd').iterdir():
    if src.is_file() and src.name.lower()!='0.bin':os.link(src,dest/'cd'/src.name)
hashes={name:hashlib.sha256((dest/name).read_bytes()).hexdigest() for name in ('makefile','core_sources.mk')}
for folder in ('core','port','sdk/jo_engine','sdk/Compiler/COMMON'):
    for src in (dest/folder).rglob('*'):
        if src.is_file():hashes[str(src.relative_to(dest))]=hashlib.sha256(src.read_bytes()).hexdigest()
(dest/'source-identity.json').write_text(json.dumps(hashes,indent=2)+'\n')
print(dest)
