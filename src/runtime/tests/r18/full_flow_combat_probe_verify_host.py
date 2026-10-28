"""Observe real manual frontend -> paid F8C -> combat without changing owners."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[2]
PROBE = 'tests/r18/full_flow_combat_probe.c'
p = argparse.ArgumentParser()
p.add_argument('output', type=Path)
p.add_argument('--frames', type=int, default=6000)
p.add_argument('--profile', choices=['idle', 'basic', 'a600', 'pressure', 'punch', 'grapple', 'rush', 'pin'], default='basic')
p.add_argument('--royal', action='store_true')
p.add_argument('--binary', type=Path)
p.add_argument('--captured-handoff', type=Path, help='Read-only Beetle directory with verified stage4 F8C capture; separate exploration')
p.add_argument('--capture-pre-f4c2-frame', type=int,
               help='Capture one unmodified HOST state immediately before the wrestler phase')
a = p.parse_args()
assert (ROOT / PROBE).is_file(), 'Missing full-flow combat probe (RED)'
assert 1 <= a.frames <= 20000
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
assert not (out / 'host.log').exists(), 'Use a new evidence directory'
make = (ROOT / 'Makefile').read_text()
core = re.search(r'^CORE=(.*)$', make, re.M)[1].split()
host = re.search(r'^R18_HOST=(.*)$', make, re.M)[1].split()
for source in ('src/wf_cpu_runtime.c', 'src/wf_r17_dispatch_1c03e.c',
               'src/wf_r17_dependencies.c', 'src/wf_r17_referee.c'):
    core.remove(source)
wrappers = ['tests/r18/allies_cpu_trace.c', 'tests/r18/allies_dispatch_trace.c',
            'tests/r18/allies_trace.c', 'tests/r18/transition_trace.c',
            'tests/r18/referee_trace.c', PROBE]
binary = a.binary.resolve() if a.binary else out / 'full_flow_combat_probe.exe'
if not a.binary:
    with tempfile.TemporaryDirectory(prefix='wf-full-combat-', ignore_cleanup_errors=os.name == 'nt') as t:
        frozen = Path(t)
        for name in ('src', 'include', 'generated'):
            shutil.copytree(ROOT / name, frozen / name)
        for name in [*wrappers, 'tests/r18/frontend_driver_host.c',
                     'tests/r18/allies_trace.h', str(Path(__file__).relative_to(ROOT))]:
            target = frozen / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, target)
        sources = {str(f.relative_to(frozen)): hashlib.sha256(f.read_bytes()).hexdigest()
                   for f in sorted(frozen.rglob('*')) if f.is_file()}
        command = ['cc', '-std=c11', '-O2', '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
                   '-Iinclude', *core, *host, *wrappers, '-o', str(binary)]
        subprocess.run(command, cwd=frozen, check=True)
        (out / 'source-manifest.json').write_text(json.dumps({
            'sources_sha256': sources, 'compile_command': command,
            'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()}, indent=2) + '\n')
env = {**os.environ, 'WF_ALLIES_OUT': str(out), 'WF_TRACE_ALL_ACTORS': '1'}
if a.capture_pre_f4c2_frame is not None:
    assert 0 <= a.capture_pre_f4c2_frame < a.frames
    env['WF_CAPTURE_PRE_F4C2_FRAME'] = str(a.capture_pre_f4c2_frame)
captured = None
if a.captured_handoff:
    cap = a.captured_handoff.resolve()
    native = json.loads((cap / 'report.json').read_text())
    captured = native['handoff_capture']
    assert captured['valid'], 'Native capture was overwritten or inconsistent'
    for name,key in [('handoff-after-f8c-ram.bin','ram_sha256'),('handoff-after-f8c-dregs.bin','dregs_sha256')]:
        assert hashlib.sha256((cap/name).read_bytes()).hexdigest()==captured[key]
    env['WF_CAPTURED_HANDOFF_RAM']=str(cap/'handoff-after-f8c-ram.bin')
    env['WF_CAPTURED_HANDOFF_DREGS']=str(cap/'handoff-after-f8c-dregs.bin')
command = [str(binary), 'audit/runtime18-cd/ROM68K.BIN',
           'audit/runtime18-cd/SPRITES.PAK', 'audit/runtime18-cd/R18RING.BIN',
           'audit/runtime18-cd/HOTSPR.BIN', str(a.frames), a.profile,
           str(int(a.royal)), str(out)]
with (out / 'host.log').open('w') as log:
    result = subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
stdout = (out / 'host.log').read_text()
print(stdout[-7000:], end='', flush=True)
block = re.search(r'BLOCK frame=.*', stdout)
report = {'status': 'PASS' if result.returncode == 0 else 'BLOCKED' if block else 'ERROR',
          'returncode': result.returncode, 'first_block': block[0] if block else None,
          'frames_requested': a.frames, 'profile': a.profile, 'royal': a.royal,
          'command': command, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
          'rom_sha1': hashlib.sha1((ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest(),
          'captured_native_handoff': captured,
          'pre_f4c2_snapshot_sha256': (hashlib.sha256((out / 'pre-f4c2-snapshot.bin').read_bytes()).hexdigest()
                                        if (out / 'pre-f4c2-snapshot.bin').is_file() else None),
          'limits': ['HOST device tape; no physical Saturn timing claim',
                     'Audio is an explicit silent output device',
                     'Frontend RAM dump stops being live after typed runtime handoff',
                     'runtime.bin is the compiler-specific typed HOST structure, not arcade RAM',
                     'allies snapshots project documented gameplay fields into RAM8KB; remaining bytes zero']}
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
raise SystemExit(result.returncode)
