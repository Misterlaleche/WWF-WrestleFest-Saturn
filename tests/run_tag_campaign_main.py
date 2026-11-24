#!/usr/bin/env python3
"""Run uninterrupted real-main matches past their post-match adapters.

Uses ordinary existing autorun input, with no actor/result/progression writes.
Reports every visited stage and scene; a bounded route is not a full-campaign
acceptance. A successful default run requires reaching the second credited TAG F8C at
progression one; falling back to attract does not pass.
Royal mode instead requires a completed original credits owner 0x1B4E.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def input_driver_sources(path):
    """Include local policy headers in the immutable input provenance."""
    pending=[path.resolve()];found=set()
    while pending:
        current=pending.pop()
        if current in found:continue
        found.add(current)
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"',current.read_text(),re.M):
            child=(current.parent/name).resolve()
            if child.is_file():pending.append(child)
    return sorted(found)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/tag-campaign-main')
    parser.add_argument('--output', type=Path, default=ROOT/'build/tag-campaign-main/results')
    parser.add_argument('--entries', type=int, default=2)
    parser.add_argument('--mode', choices=('tag', 'royal'), default='tag')
    parser.add_argument('--require-owner', type=lambda text:int(text,0),
                        help='Required completed result owner; Royal defaults to original credits 0x1B4E')
    parser.add_argument('--field-limit', type=int, default=200000)
    parser.add_argument('--require-stage', type=int, default=1)
    parser.add_argument('--require-scenes', type=lambda text:int(text,0), default=0)
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--input-driver', type=Path, help='Test-only C header defining campaign_pad_buttons(port); returns physical Jo key bits')
    parser.add_argument('--pad-tape', type=Path, help='Record observed port0 inputs as original active-low bus bytes for portable replay')
    args = parser.parse_args()
    required_owner = args.require_owner if args.require_owner is not None else (0x1b4e if args.mode == 'royal' else 0)
    build, output = args.build_dir.resolve(), args.output.resolve()
    build.mkdir(parents=True, exist_ok=True); output.mkdir(parents=True, exist_ok=True)
    if not args.skip_build:
        with (output/'build.log').open('w') as log:
            result = subprocess.run([sys.executable, str(ROOT/'tools/build_runtime_tests.py'),
                                     '--build-dir', str(build)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            print((output/'build.log').read_text())
        result.check_returncode()
    manifest = json.loads((build/'runtime-build.json').read_text())
    assert not manifest['sources_changed_during_build']
    for name, digest in manifest['sources'].items():
        assert sha(ROOT/name) == digest, ('stale source', name)
    objects = []
    for name, digest in sorted(manifest['objects'].items()):
        assert sha(build/name) == digest, ('stale object', name)
        objects.append(str(build/name))
    flags = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-DWF_JO_ENGINE',
             '-DWF_JO_ASSETS_HOST', '-DWF_JO_FRONTEND_TEST', '-DWF_JO_AUTORUN',
             '-DWF_JO_AUTORUN_MODE='+('2' if args.mode == 'royal' else '1'),
             '-DWF_JO_FRONTEND', '-DZ_SOLO', '-DNO_GZIP']
    flags += ['-I'+str(ROOT/p) for p in ['tests/native/jo_host', 'tests', 'core/include', 'port', 'port/zlib', 'tests/native']]
    if args.input_driver:
        flags.append('-DWF_CAMPAIGN_INPUT_DRIVER="'+str(args.input_driver.resolve())+'"')
    binary = build/(args.mode+'-campaign-main')
    command = [os.environ.get('CC', 'cc'), *flags, '-Wl,--wrap=wf_jo_frontend_run',
               '-Wl,--wrap=wf_jo_frontend_after_combat_at_level', '-Wl,--wrap=wf_r18_ring_bind',
               '-Wl,--wrap=wf_r18_post_tag_step',
               str(ROOT/'tests/test_tag_campaign_main.c'), str(ROOT/'tests/native/assets_fixture.c'),
               str(ROOT/'port/wf_jo_frontend.c'), str(ROOT/'port/wf_jo_clock.c'), *objects, '-o', str(binary)]
    sources = [ROOT/'tests/test_tag_campaign_main.c', ROOT/'tests/test_tag_campaign_game.c', ROOT/'tests/native/jo_main_probe.c', ROOT/'port/main.c', ROOT/'port/wf_jo_frontend.c', Path(__file__).resolve()]
    if args.input_driver:
        sources.extend(input_driver_sources(args.input_driver))
    sources_before = {str(p):sha(p) for p in sources}
    with (output/'link.log').open('w') as log:
        result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((output/'link.log').read_text())
    result.check_returncode()
    env = dict(os.environ, WF_JO_TEST_CACHE_BYTES='565248')
    if args.pad_tape:
        tape=args.pad_tape.resolve();tape.parent.mkdir(parents=True,exist_ok=True)
        assert not tape.exists(), 'Never overwrite a recorded physical input tape'
        env['WF_POLICY_TAPE']=str(tape)
    with (output/'run.log').open('w') as log:
        result = subprocess.run([str(binary), str(args.entries), str(args.field_limit), str(output), str(args.require_stage), str(args.require_scenes), str(required_owner)],
                                cwd=ROOT/'cd', env=env, stdout=log, stderr=subprocess.STDOUT)
    print((output/'run.log').read_text())
    summary = dict(returncode=result.returncode, mode=args.mode, required_owner=required_owner, entries_requested=args.entries,
                   field_limit=args.field_limit, binary_sha256=sha(binary),
                   sources_sha256=sources_before, sources_changed_during_run=[str(p) for p in sources if sha(p)!=sources_before[str(p)]],
                   required_stage=args.require_stage, required_scenes=args.require_scenes,
                   scope='Continuous real-main and native post-match adapter, existing normal autorun inputs. Host fields do not measure Saturn performance.')
    events = output/'campaign-events.log'
    if events.exists():
        summary['events']=events.read_text().splitlines()
    if args.pad_tape and tape.exists():
        summary['pad_tape']=dict(path=str(tape),sha256=sha(tape),rows=len(tape.read_text().splitlines())-1)
    (output/'summary.json').write_text(json.dumps(summary, indent=2)+'\n')
    result.check_returncode()


if __name__ == '__main__':
    main()
