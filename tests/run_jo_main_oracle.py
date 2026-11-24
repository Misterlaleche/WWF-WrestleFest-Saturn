#!/usr/bin/env python3
"""Run real port/main.c with host hardware and compare direct/frontend entries.

This is a native logic oracle, not an emulator or speed measurement. A linked
observer records the fullfrontend terminal at the real post-ACK handoff before
the result owner executes. Input, clock admission and gameplay callbacks are
the application's own C functions. No initial actors/RNG are imported for the
frontend route. Direct mode still uses the application's explicit test entry.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def difference(a, b):
    aa, bb = a.read_bytes(), b.read_bytes()
    offsets = [i for i, (x, y) in enumerate(zip(aa, bb)) if x != y]
    return dict(equal=aa == bb, bytes_a=len(aa), bytes_b=len(bb),
                differing_bytes=len(offsets) + abs(len(aa) - len(bb)),
                first_differing_offsets=offsets[:24], sha256_a=sha(a), sha256_b=sha(b))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/fullfrontend-audit')
    parser.add_argument('--output', type=Path, default=ROOT/'evidence/fullfrontend-oracle')
    parser.add_argument('--limit', type=int, default=30000)
    parser.add_argument('--cache-bytes', type=int, default=565248)
    parser.add_argument('--stride', type=int, default=1)
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--worker', action='store_true',
                        help='Enable deferred asset jobs and require positive, balanced worker counts in every route')
    parser.add_argument('--trace', action='store_true',
                        help='Record every command/pixel digest and periodic complete logic snapshots')
    parser.add_argument('--routes', nargs='+', choices=['direct-tag', 'direct-royal', 'frontend-tag', 'frontend-royal'],
                        default=['direct-tag', 'direct-royal', 'frontend-tag', 'frontend-royal'])
    args = parser.parse_args()
    build, output = args.build_dir.resolve(), args.output.resolve()
    build.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    commands = []
    if not args.skip_build:
        command = [sys.executable, str(ROOT/'tools/build_runtime_tests.py'), '--build-dir', str(build)]
        if args.worker:
            command.append('--worker')
        log = output/'build.log'
        with log.open('w') as stream:
            built = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
        commands.append(dict(argv=command, cwd=str(ROOT), returncode=built.returncode, log=str(log)))
        if built.returncode:
            print(log.read_text())
        built.check_returncode()
    build_manifest_path = build/'runtime-build.json'
    build_manifest = json.loads(build_manifest_path.read_text()) if build_manifest_path.exists() else None
    if build_manifest is not None:
        if build_manifest['configuration']['worker'] != args.worker:
            raise SystemExit('Native objects use a different --worker mode; rebuild without --skip-build')
        stale = [name for name, value in build_manifest['sources'].items()
                 if not (ROOT/name).is_file() or sha(ROOT/name) != value]
        if build_manifest['sources_changed_during_build'] or stale:
            raise SystemExit('Native build sources changed; rebuild without --skip-build: '+', '.join(stale))
        objects = [str(build/name) for name in sorted(build_manifest['objects'])]
        if any(not Path(p).is_file() or sha(Path(p)) != build_manifest['objects'][Path(p).name]
               for p in objects):
            raise SystemExit('Native object hashes changed; rebuild without --skip-build')
    else:
        if args.worker or (build/'assets_worker_host.o').exists():
            raise SystemExit('--worker requires a matching runtime-build.json; rebuild without --skip-build')
        objects = sorted(str(p) for p in build.glob('*.o'))
    source_files = [ROOT/'port/main.c', ROOT/'port/wf_jo_frontend.c', ROOT/'port/wf_jo_clock.c',
                    ROOT/'port/wf_jo_scene_prepare.c', ROOT/'port/wf_jo_scene_prepare.h',
                    ROOT/'core/src/wp1979_input.c', ROOT/'core/src/wp1979_tape.inc',
                    ROOT/'core/src/wp1980_input.c', ROOT/'tests/native/jo_main_probe.c',
                    ROOT/'tests/native/jo_host/jo/jo.h', ROOT/'tests/native/assets_fixture.c',
                    Path(__file__).resolve()]
    if build_manifest:
        source_files = sorted(set(source_files+[ROOT/name for name in build_manifest['sources']]))
    sources_before = {str(p.relative_to(ROOT)): sha(p) for p in source_files}
    flags = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-DWF_JO_ENGINE',
             '-DWF_JO_ASSETS_HOST', '-DWF_JO_FRONTEND_TEST', '-DWF_JO_AUTORUN', '-DZ_SOLO', '-DNO_GZIP']
    if args.worker:
        flags.append('-DWF_JO_ASSETS_TEST_WORKER')
    flags += ['-I'+str(ROOT/p) for p in ['tests/native/jo_host', 'tests', 'core/include', 'port', 'port/zlib', 'tests/native']]
    results = {}
    for route in args.routes:
        royal = route.endswith('royal')
        front = route.startswith('frontend')
        binary = build/('main-'+route)
        command = [os.environ.get('CC', 'cc'), *flags, '-DWF_JO_AUTORUN_MODE='+str(2 if royal else 1)]
        if front:
            command += ['-DWF_JO_FRONTEND', '-Wl,--wrap=wf_jo_frontend_run',
                        '-Wl,--wrap=wf_jo_frontend_after_combat_at_level']
        command += [str(ROOT/'tests/native/jo_main_probe.c'), str(ROOT/'tests/native/assets_fixture.c'),
                    str(ROOT/'port/wf_jo_frontend.c'), str(ROOT/'port/wf_jo_clock.c'), *objects, '-o', str(binary)]
        folder = output/route
        folder.mkdir(exist_ok=True)
        link_log = folder/'link.log'
        with link_log.open('w') as stream:
            linked = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
        commands.append(dict(argv=command, cwd=str(ROOT), returncode=linked.returncode, log=str(link_log)))
        if linked.returncode:
            print(link_log.read_text())
        linked.check_returncode()
        # A failed rerun must never borrow an old successful terminal.
        for stage in ['f8c', 'prepared', 'terminal', 'bounded', 'blocked', 'tick*']:
            for old in folder.glob(stage+'-*'):
                if old.is_file():
                    old.unlink()
        env = dict(os.environ, WF_JO_TEST_CACHE_BYTES=str(args.cache_bytes))
        worker_stats = folder/'worker.json'
        if worker_stats.exists():
            worker_stats.unlink()
        # Do not inherit a caller's output path into a serial control run.
        env.pop('WF_JO_TEST_WORKER_STATS', None)
        if args.worker:
            env['WF_JO_TEST_WORKER_STATS'] = str(worker_stats)
        if args.trace:
            env.update(WF_JO_TEST_SNAPSHOTS='1', WF_JO_TEST_COMMANDS=str(folder/'commands.csv'),
                       WF_JO_TEST_STATS=str(folder/'assets.csv'), WF_JO_TEST_TILES=str(folder/'tiles.bin'))
        run_command = [str(binary), str(int(royal)), str(args.limit), str(folder), str(args.stride)]
        with (folder/'run.log').open('w') as log:
            run = subprocess.run(run_command,
                                 cwd=ROOT/'cd', env=env, stdout=log, stderr=subprocess.STDOUT)
        commands.append(dict(argv=run_command, cwd=str(ROOT/'cd'), returncode=run.returncode,
                             log=str(folder/'run.log'), environment={k: v for k, v in env.items()
                                                                  if k.startswith('WF_JO_TEST_')}))
        result = dict(returncode=run.returncode, binary_sha256=sha(binary), limit=args.limit,
                      cache_bytes=args.cache_bytes, host_field_stride=args.stride,
                      application_defines={'WF_JO_FRONTEND': front, 'WF_JO_AUTORUN': True,
                                           'WF_JO_AUTORUN_MODE': 2 if royal else 1,
                                           'WF_JO_ASSETS_TEST_WORKER': args.worker,
                                           'JO_COMPILE_WITH_PROFILER_SUPPORT': False})
        if args.worker:
            result['worker'] = json.loads(worker_stats.read_text()) if worker_stats.exists() else {}
            stats = result['worker']
            result['worker_valid'] = (stats.get('enabled') is True and stats.get('submitted', 0) > 0
                                      and stats.get('submitted') == stats.get('joined')
                                      and stats.get('pending') == 0)
        for stage in ['f8c', 'prepared', 'terminal', 'bounded', 'blocked']:
            meta = folder/(stage+'-state.json')
            if meta.exists():
                result[stage] = json.loads(meta.read_text())
                result[stage]['actor_sha256'] = sha(folder/(stage+'-actors.bin'))
                result[stage]['canonical_ram_sha256'] = sha(folder/(stage+'-arcade-ram.bin'))
                result[stage]['core_host_sha256'] = sha(folder/(stage+'-core-host.bin'))
        results[route] = result
        if args.trace:
            result['trace_files'] = {p.name: {'sha256': sha(p), 'bytes': p.stat().st_size}
                                     for p in sorted(folder.iterdir())
                                     if p.suffix in ('.bin', '.csv')}
        print(route, 'exit', run.returncode, 'entry_rng', hex(result.get('f8c', {}).get('rng', 0)),
              'terminal', result.get('terminal', {}).get('frame'), flush=True)
    comparisons = {}
    for mode in ['tag', 'royal']:
        direct, front = output/('direct-'+mode), output/('frontend-'+mode)
        if not (direct/'f8c-state.json').exists() or not (front/'f8c-state.json').exists():
            continue
        comparisons[mode] = {part: difference(direct/('f8c-'+part), front/('f8c-'+part))
                             for part in ['actors.bin', 'arcade-ram.bin', 'globals-host.bin', 'core-host.bin']}
        # Per-member comparisons distinguish gameplay from ABI padding in g.
        sections = json.loads((front/'f8c-state.json').read_text())['sections']
        a, b = (direct/'f8c-core-host.bin').read_bytes(), (front/'f8c-core-host.bin').read_bytes()
        comparisons[mode]['core_members'] = {
            s['name']: {'equal': a[s['offset']:s['offset']+s['bytes']] == b[s['offset']:s['offset']+s['bytes']],
                        'differing_bytes': sum(x != y for x, y in zip(a[s['offset']:s['offset']+s['bytes']],
                                                                      b[s['offset']:s['offset']+s['bytes']]))}
            for s in sections}
    changed = [str(p.relative_to(ROOT)) for p in source_files
               if sha(p) != sources_before[str(p.relative_to(ROOT))]]
    # B12 TAG golden follows the ROM-verified20CCC correction; Royal is unchanged.
    known_direct = {'direct-tag': (9101, '08015d9988c77a0d9bfa5b6001675dbd3cd1d3b3420052423e7bf32c50ea194a'),
                    'direct-royal': (8474, '656f7934c1c1b1cbac5afef25e4d17c8503c050c011effc734a0e00e1fb6f884')}
    for route, (frame, actors) in known_direct.items():
        if route in results and args.limit >= frame:
            terminal = results[route].get('terminal', {})
            results[route]['historical_direct_control'] = terminal.get('frame') == frame and terminal.get('actor_sha256') == actors
    report = {'scope': 'Actual application main/adapter/native owners; deterministic host hardware only. '
                       'Host fields do not measure Saturn execution speed. First fullfrontend terminal is observed '
                       'before its post-match owner, while direct terminals retain the legacy bounded result call. '
                       'arcade-ram.bin is the typed runtime exported into an initially zero image, not all unrepresented original RAM. '
                       'core-host.bin contains explicit named members, no pointers or tracing; globals remain host endian.',
              'sources': sources_before, 'sources_changed_during_run': changed,
              'commands': commands, 'worker_enabled': args.worker,
              'worker_scope': ('Deferred host jobs at the actual paired decoder joins. Real pthread scheduling '
                               'is a separate asset gate; this does not measure physical SH2 concurrency.'),
              'build_manifest_sha256': sha(build_manifest_path) if build_manifest else None,
              'build_manifest': build_manifest,
              'asset_inputs': {p.name: sha(p) for p in (ROOT/'cd').iterdir()
                               if p.name in ('ROM68K.BIN', 'SPRITES.WFP', 'SCENES.WFP')},
              'linked_objects': {str(Path(p).name): sha(Path(p)) for p in objects},
              'runs': results, 'entry_comparison': comparisons}
    (output/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if not changed and all(r['returncode'] == 0 and r.get('historical_direct_control', True)
                                   and r.get('worker_valid', True)
                                   for r in results.values()) else 1


if __name__ == '__main__':
    raise SystemExit(main())
