#!/usr/bin/env python3
"""Compare independently generated fullfrontend traces, including every snapshot.

This is an exact output comparison, not a game runner or timing measurement.
Use the baseline and candidate with the same native trace fixture and assets.
"""
import argparse
import hashlib
import json
from pathlib import Path

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('reference',type=Path)
    p.add_argument('candidate',type=Path)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    result={'status':'PASS','reference':str(a.reference.resolve()),'candidate':str(a.candidate.resolve()),'routes':{}}
    for route in ['frontend-royal','frontend-tag']:
        left,right=a.reference/route,a.candidate/route
        names={f.name for f in left.iterdir() if f.suffix in ['.bin','.csv'] or f.name.endswith('-state.json')}
        other={f.name for f in right.iterdir() if f.suffix in ['.bin','.csv'] or f.name.endswith('-state.json')}
        r={'missing':sorted(names-other),'extra':sorted(other-names),'files':{},'snapshots':sum(n.endswith('-state.json') for n in names)}
        for name in sorted(names&other):
            x,y=left/name,right/name
            equal=x.read_bytes()==y.read_bytes()
            r['files'][name]={'equal':equal,'reference_sha256':sha(x),'candidate_sha256':sha(y),'reference_bytes':x.stat().st_size,'candidate_bytes':y.stat().st_size}
            if not equal:result['status']='FAIL'
        if r['missing'] or r['extra']:result['status']='FAIL'
        r['terminal_frame']=json.loads((right/'terminal-state.json').read_text())['frame']
        if (right/'commands.csv').exists():r['command_rows']=sum(1 for _ in (right/'commands.csv').open())-1
        result['routes'][route]=r
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    for route,r in result['routes'].items():
        print(result['status'],route,'terminal',r['terminal_frame'],'snapshots',r['snapshots'],'command rows',r.get('command_rows'), 'files',len(r['files']))
        for name,v in r['files'].items():
            if not v['equal']:print('DIFF',name,v['reference_sha256'],v['candidate_sha256'])
        if r['missing'] or r['extra']:print('file set mismatch',r['missing'],r['extra'])
    raise SystemExit(result['status']!='PASS')
if __name__=='__main__':main()
