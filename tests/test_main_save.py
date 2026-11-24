#!/usr/bin/env python3
"""Real application save wiring, from cold RESET through a native Royal loss."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=ROOT/'build/main-save-tests')
parser.add_argument('--skip-build', action='store_true')
parser.add_argument('--faults', type=int, nargs='+', default=[0, 1, 2])
args = parser.parse_args()
build = args.build_dir.resolve();build.mkdir(parents=True, exist_ok=True)
if not args.skip_build:
    subprocess.run([sys.executable, str(ROOT/'tools/build_runtime_tests.py'), '--build-dir', str(build)], check=True)
manifest = json.loads((build/'runtime-build.json').read_text())
objects = [str(build/name) for name in manifest['objects']]
flags = ['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE','-DWF_JO_ASSETS_HOST',
         '-DWF_JO_FRONTEND_TEST','-DWF_JO_FRONTEND','-DWF_JO_AUTORUN','-DWF_JO_AUTORUN_MODE=2','-DZ_SOLO','-DNO_GZIP']
flags += ['-I'+str(ROOT/p) for p in ['tests/native/jo_host','tests','core/include','port','port/zlib','tests/native']]
wraps = ['wf_jo_save_init','wf_jo_frontend_run','wf_jo_frontend_after_combat_at_level','wf_jo_video_drain_overlap']
command = [os.environ.get('CC','cc'),*flags,*['-Wl,--wrap='+name for name in wraps],
           str(ROOT/'tests/test_main_save.c'),str(ROOT/'tests/native/assets_fixture.c'),
           str(ROOT/'port/wf_jo_frontend.c'),str(ROOT/'port/wf_jo_clock.c'),*objects]
if 'wf_jo_save.o' not in manifest['objects']:
    command += [str(ROOT/'port/wf_jo_save.c')]
binary = build/'main-save';command += ['-o',str(binary)]
subprocess.run(command,check=True)
for fault in args.faults:
    output=build/('fault'+str(fault));output.mkdir(exist_ok=True)
    subprocess.run([str(binary),str(output),str(fault)],cwd=ROOT/'cd',
                   env=dict(os.environ,WF_JO_TEST_CACHE_BYTES='565248'),check=True)
