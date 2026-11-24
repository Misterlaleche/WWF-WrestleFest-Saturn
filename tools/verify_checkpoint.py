#!/usr/bin/env python3
"""Verify the files explicitly frozen in this source checkpoint; no execution."""
import argparse,hashlib,json
from pathlib import Path,PurePosixPath

def verify(root):
    root=Path(root).resolve()
    manifest=json.loads((root/'source-manifest.json').read_text())
    if manifest.get('format')!='wf-r03-source-checkpoint' or manifest.get('version')!=1:
        raise ValueError('Unknown checkpoint manifest')
    checked=0
    for name,identity in manifest['files'].items():
        rel=PurePosixPath(name)
        if rel.is_absolute() or '..'in rel.parts:raise ValueError('Unsafe manifest path')
        path=root.joinpath(*rel.parts)
        if path.is_symlink() or not path.is_file():raise ValueError('Missing/nonregular source: '+name)
        data=path.read_bytes()
        if len(data)!=identity['bytes'] or hashlib.sha256(data).hexdigest()!=identity['sha256']:
            raise ValueError('Source identity differs: '+name)
        checked+=1
    return {'status':'PASS_SOURCE_IDENTITIES','files':checked,'commit':manifest['commit'],
            'scope':'Frozen files only; restored cd/compiler/build outputs are outside this manifest. No rebuild or hardware claim.'}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);a=p.parse_args()
    print(json.dumps(verify(a.root),indent=2))

if __name__=='__main__':main()
