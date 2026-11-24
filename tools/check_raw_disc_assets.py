#!/usr/bin/env python3
"""Packaging contract for WF_JO_RAW_DISC_ASSETS; no target files are modified."""
from pathlib import Path
import argparse,hashlib,json

NATIVE_MAGIC={'SPRITES.WFP':b'WFSZ','SCENES.WFP':b'WFSC','SFX.DAT':b'WFSX'}
NAMES=('ROM68K.BIN',*NATIVE_MAGIC)

def check(directory:Path,manifest:Path):
    lock=json.loads(manifest.read_text())
    if lock.get('format')!='wf-disc-asset-lock' or lock.get('version')!=1:
        raise ValueError('Unsupported asset-lock format')
    result={}
    for name in NAMES:
        path=directory/name;data=path.read_bytes()
        if data[:4]==b'JOLZ':raise ValueError(name+': JoLZ is incompatible with raw ISO sizes; disable WF_RAW_DISC_ASSETS')
        if len(data)<4 or len(data)>0x7fffffff:raise ValueError(name+': unsupported physical length')
        if name=='ROM68K.BIN' and len(data)!=524288:raise ValueError(name+': invalid native ROM length')
        if name in NATIVE_MAGIC and data[:4]!=NATIVE_MAGIC[name]:raise ValueError(name+': invalid native format')
        digest=hashlib.sha256(data).hexdigest();expected=lock['files'][name]
        if expected.get('bytes')!=len(data) or expected.get('sha256')!=digest:
            raise ValueError(name+': does not match asset-lock.json')
        result[name]={'bytes':len(data),'sha256':digest,'raw':True}
    return {'status':'PASS','contract':'WF_JO_RAW_DISC_ASSETS=1','files':result}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cd',type=Path,default=Path('cd'))
    parser.add_argument('--manifest',type=Path,default=Path('asset-lock.json'))
    args=parser.parse_args()
    try:result=check(args.cd,args.manifest)
    except (OSError,ValueError,KeyError,TypeError) as error:parser.exit(1,'Raw WF asset gate failed: '+str(error)+'\n')
    print(json.dumps(result,sort_keys=True))

if __name__=='__main__':main()
