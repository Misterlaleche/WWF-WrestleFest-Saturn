#!/usr/bin/env python3
"""Prove fault tests reject individual production regressions (not build errors).

Each mutant is a disposable source-tree copy. The original tree is never edited.
An assertion failure is required; an unavailable compiler, timeout, or build error
cannot make this gate pass. No emulator/hardware or acoustic claim is made.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
# Mutation name, production file, exact original fragment, replacement, probe.
MUTANTS = (
    ('skip-master-mute', 'sdk/jo_engine/audio.c',
     '    jo_audio_set_memory_and_volume(0);',
     '    /* MUTANT: master mute removed */', 'audio'),
    ('volume-can-reenable', 'sdk/jo_engine/audio.c',
     '(__jo_audio_emergency_muted ? 0u : (volume & JO_SCSP_MASTER_VOLUME_MASK))',
     '(volume & JO_SCSP_MASTER_VOLUME_MASK)', 'audio'),
    ('stale-update-can-poll-cd', 'sdk/jo_engine/audio.c',
     'jo_audio_music_update(void)\n{\n    if (__jo_audio_emergency_muted) return;',
     'jo_audio_music_update(void)\n{', 'audio'),
    ('leave-sdk-slots-pending', 'sdk/jo_engine/audio.c',
     '    JO_ZERO(__jo_audio_pending_count);\n    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL;',
     '    /* MUTANT: pending register writes retained */\n    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL;', 'audio'),
    ('leave-game-queue', 'port/wf_jo_audio.c',
     '    queue_read=queue_write=0;\n    wf_jo_audio_diagnostic.queue_depth=0;',
     '    /* MUTANT: pending commands retained */', 'audio'),
    ('leave-pcm-voices', 'port/wf_jo_audio.c',
     '        pcm[v].pending=pcm[v].playing=pcm[v].filling=0;',
     '        /* MUTANT: live PCM voices retained */', 'audio'),
    ('allow-adapter-restart', 'port/wf_jo_audio.c',
     '    if(emergency_stopped)return false;\n    initialized=false;',
     '    initialized=false;', 'audio'),
    ('overwrite-first-fault', 'port/main.c',
     '    if(failed)return;\n    loading_text_visible=0;',
     '    loading_text_visible=0;', 'main'),
    ('no-mute-before-drain', 'port/main.c',
     '    wf_jo_audio_emergency_stop();\n    /* Keep the original',
     '    /* Keep the original', 'main'),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT/'build/emergency-mutants')
    args = parser.parse_args()
    source, out = args.source.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    report = {'scope': 'Deliberate source mutations; host assertion failures only.',
              'runs': [], 'source_hashes': {}}
    for name, rel, old, new, probe in MUTANTS:
        data = (source/rel).read_text()
        if data.count(old) != 1:
            raise RuntimeError(f'{name}: expected exactly one production edit point')
        report['source_hashes'][rel] = hashlib.sha256((source/rel).read_bytes()).hexdigest()
        with tempfile.TemporaryDirectory(prefix='wf-p0-mutant-') as folder:
            root = Path(folder)/'source'
            shutil.copytree(source, root, ignore=shutil.ignore_patterns('.git', 'build', '__pycache__'))
            (root/rel).write_text(data.replace(old, new, 1))
            dest = out/name
            test = root/'tests'/('test_emergency_audio.py' if probe == 'audio' else 'test_main_frontend_fault.py')
            target = root if probe == 'audio' else root/'port/main.c'
            cmd = [sys.executable, str(test), '--source', str(target), '--output', str(dest)]
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=30,
                                  env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))
            text = proc.stdout + proc.stderr
            # Probe errors must be behavioral assertions, not compile failures.
            assertions = 'Assertion' in text or 'Assertion' in ''.join(
                p.read_text(errors='replace') for p in dest.glob('*.log'))
            compile_errors = any(p.stat().st_size for p in dest.glob('*compile.log'))
            if probe == 'audio':
                result = json.loads((dest/'report.json').read_text())
                compile_errors |= any('compile_returncode' in r for r in result['runs'])
                rejected = any(r.get('returncode', 0) < 0 for r in result['runs'])
            else:
                rejected = proc.returncode != 0
            detected = proc.returncode != 0 and assertions and rejected and not compile_errors
            (out/(name+'.log')).write_text(text)
            report['runs'].append({'name': name, 'file': rel, 'detected': detected,
                                   'returncode': proc.returncode, 'assertion_observed': assertions,
                                   'compilation_error': compile_errors})
            print(name, 'DETECTED' if detected else 'NOT DETECTED', flush=True)
            (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    report['status'] = 'PASS' if all(r['detected'] for r in report['runs']) else 'FAIL'
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return report['status'] != 'PASS'


if __name__ == '__main__':
    raise SystemExit(main())
