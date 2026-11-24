#!/usr/bin/env python3
"""STN-52 workspace bounds with the actual cache and both supported block sizes."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', type=Path, default=ROOT/'port')
    parser.add_argument('--assets-dir', type=Path, default=ROOT/'source_assets',
                        help='Canonical ROM68K.BIN and SPRITES.PAK')
    parser.add_argument('--stock-pack', type=Path, default=ROOT/'cd/SPRITES.WFP')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    port = args.source_dir.resolve()
    assets = args.assets_dir.resolve()
    stock = args.stock_pack.resolve()
    with tempfile.TemporaryDirectory(prefix='wf-save-workspace-') as temporary:
        build = args.build_dir.resolve() if args.build_dir else Path(temporary)
        build.mkdir(parents=True, exist_ok=True)
        spec = importlib.util.spec_from_file_location('wf_build_assets', ROOT/'tools/build_assets.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        raw = (assets/'SPRITES.PAK').read_bytes()
        legacy = build/'SPRITES-8192.WFP'
        legacy.write_bytes(module.build_sprite_pack(raw, 8192, 'zlib'))
        assert int.from_bytes(stock.read_bytes()[8:12], 'big') == 4096
        zlib = ROOT/'port/zlib'
        command = [os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-Wno-implicit-fallthrough', '-DWF_JO_ASSETS_HOST', '-DZ_SOLO', '-DNO_GZIP',
                   '-I'+str(port), '-I'+str(ROOT/'port'), '-I'+str(ROOT/'core/include'), '-I'+str(zlib)]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        command += [str(ROOT/'tests/test_save_workspace.c'), str(port/'wf_jo_save.c'),
                    str(ROOT/'core/src/wf_r18_save.c'), str(ROOT/'port/wf_jo_assets.c')]
        command += [str(zlib/(name+'.c')) for name in ('inflate','inffast','inftrees','adler32','zutil')]
        subprocess.run(command+['-o', str(build/'test')], check=True)
        report = {'physical_hardware_verified': False, 'bios_footprint_bytes': 24696, 'cases': {}}
        for size, pack in [(4096, stock), (8192, legacy)]:
            run = subprocess.run([str(build/'test'), str(assets/'ROM68K.BIN'), str(pack),
                                  str(assets/'SPRITES.PAK')], capture_output=True, text=True,
                                 env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
            (build/(str(size)+'.log')).write_text(run.stdout+run.stderr)
            print(run.stdout+run.stderr, end='')
            report['cases'][str(size)] = {'returncode': run.returncode,
                                         'pack_sha256': hashlib.sha256(pack.read_bytes()).hexdigest()}
        report['candidate_files'] = {name: hashlib.sha256((port/name).read_bytes()).hexdigest()
                                     for name in ('wf_jo_save.c','wf_jo_save.h')}
        (build/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        return int(any(case['returncode'] for case in report['cases'].values()))


if __name__ == '__main__':
    raise SystemExit(main())
