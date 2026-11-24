#!/usr/bin/env python3
"""Exercise the private resumable scene inflater with real resident assets."""
from pathlib import Path
import argparse, os, subprocess, tempfile

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-dir',type=Path,required=True)
    p.add_argument('--asset-dir',type=Path,default=ROOT/'cd')
    p.add_argument('--sanitize',action='store_true')
    a=p.parse_args()
    flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-implicit-fallthrough',
           '-DWF_JO_ASSETS_HOST','-DZ_SOLO','-DNO_GZIP','-I'+str(ROOT/'port')]
    if a.sanitize:flags+=['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer']
    source=['tests/test_scene_prepare.c','port/wf_jo_scene_prepare.c','port/wf_jo_assets.c']
    source+=['port/zlib/'+s for s in ('inflate.c','inffast.c','inftrees.c','adler32.c','zutil.c')]
    env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0')
    with tempfile.TemporaryDirectory(prefix='wf-scene-prepare-') as folder:
        binary=Path(folder)/'test_scene_prepare'
        subprocess.run([os.environ.get('CC','cc'),*flags,*[str(ROOT/s) for s in source],'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(a.source_dir.resolve())],cwd=a.asset_dir.resolve(),env=env,check=True)
if __name__=='__main__':main()
