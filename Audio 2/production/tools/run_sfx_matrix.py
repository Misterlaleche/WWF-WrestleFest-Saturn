#!/usr/bin/env python3
"""Cold-boot each original SFX command in real standalone Mednafen 1.32.1.

Every command starts from its own unmodified Saturn boot and default bank.
The capture lasts the complete reference duration plus two seconds. The tool
never stops a tail early, normalizes audio, or treats a failed gate as success.
All generated discs, ROM resources and WAVs belong outside the source tree.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import threading

from build_saturn import build


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('resources', 'firmware', 'sdk', 'environment-root', 'bios',
                 'reference-report', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--package', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--first', type=lambda s: int(s, 0), default=0x20)
    parser.add_argument('--last', type=lambda s: int(s, 0), default=0x81)
    parser.add_argument('--jobs', type=int, choices=(1, 2), default=2)
    parser.add_argument('--display', type=int, default=90)
    args = parser.parse_args()
    if not 0x20 <= args.first <= args.last <= 0x81:
        parser.error('Original SFX range is 0x20..0x81')
    if not os.environ.get('MAKE_ISO_XORRISOFS'):
        parser.error('Source the reproduced environment env.sh before running this matrix')
    for key, value in vars(args).items():
        if isinstance(value, Path):
            setattr(args, key, value.resolve())
    args.out.mkdir(parents=True, exist_ok=False)
    reference = json.loads(args.reference_report.read_text())
    originals = {item['command']: item for item in reference['commands']}
    package = args.package
    source_paths = sorted(p for group in ('src', 'include', 'tests')
                          for p in (package / group).rglob('*') if p.is_file())
    source_hashes = {str(p.relative_to(package)): sha(p) for p in source_paths}
    report = {
        'kind': 'COLD_BOOT_NATIVE_SFX_MATRIX',
        'emulator': 'standalone Mednafen 1.32.1',
        'source_sha256_before': source_hashes,
        'firmware_sha256': sha(args.firmware),
        'reference_report_sha256': sha(args.reference_report),
        'range': [args.first, args.last],
        'reference_scope': reference['kind'],
        'ownership_scope': 'Direct diagnostic commands only; no new gameplay producer evidence',
        'commands': [],
        'hardware_status': 'HARDWARE PENDING',
        'listening_approval': 'PENDING',
    }
    lock = threading.Lock()
    displays = list(range(args.display, args.display + args.jobs))
    condition = threading.Condition()

    def capture_command(command):
        item = originals[command]
        source_wav = Path(item['wav'])
        if sha(source_wav) != item['wav_sha256']:
            raise ValueError('Reference WAV hash mismatch')
        seconds = math.ceil(item['seconds']) + 2
        work = args.out / f'command-{command:02X}'
        work.mkdir()
        build_report = build(package, work / 'build', args.resources,
                             args.firmware, args.sdk, command, seconds)
        with condition:
            condition.wait_for(lambda: bool(displays))
            display = displays.pop()
        try:
            invocation = [sys.executable, str(package / 'tools/run_native_test.py'),
                          '--build', str(work / 'build'), '--environment-root', str(args.environment_root),
                          '--bios', str(args.bios), '--out', str(work / 'capture'),
                          '--display', str(display), '--wall-seconds', str(120 + seconds * 4),
                          '--audio-only']
            with (work / 'capture.log').open('w') as output:
                result = subprocess.run(invocation, stdout=output, stderr=subprocess.STDOUT)
        finally:
            with condition:
                displays.append(display)
                condition.notify()
        gate_path = work / 'capture/native-gates.json'
        gates = json.loads(gate_path.read_text()) if gate_path.exists() else {'passed': False}
        compare_command = [sys.executable, str(package / 'tools/compare_capture.py'),
                           '--capture', str(work / 'capture'), '--reference', str(source_wav),
                           '--build', str(work / 'build'), '--out', str(args.out / 'pairs'),
                           '--command', hex(command)]
        comparison_code = None
        if gates.get('diagnostics', {}).get('state') == 4:
            with (work / 'comparison.log').open('w') as output:
                comparison_code = subprocess.run(compare_command, stdout=output,
                                                  stderr=subprocess.STDOUT).returncode
        summary = {
            'command': f'0x{command:02X}', 'window_seconds': seconds,
            'reference_seconds': item['seconds'],
            'build': build_report, 'capture_command': invocation,
            'capture_returncode': result.returncode, 'native_gates': gates,
            'comparison_command': compare_command, 'comparison_returncode': comparison_code,
            'passed_runtime_gates': result.returncode == 0 and gates['passed'],
            'listening_pair_available': comparison_code == 0,
        }
        with lock:
            report['commands'].append(summary)
            report['commands'].sort(key=lambda v: int(v['command'], 0))
            (args.out / 'matrix-progress.json').write_text(json.dumps(report, indent=2) + '\n')
            print(json.dumps({'command': summary['command'],
                              'runtime_passed': summary['passed_runtime_gates'],
                              'pair': summary['listening_pair_available'],
                              'completed': len(report['commands'])}), flush=True)
        return summary

    failures = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        pending = {pool.submit(capture_command, command): command
                   for command in range(args.first, args.last + 1)}
        for future in as_completed(pending):
            try:
                future.result()
            except Exception as error:
                failures.append({'command': hex(pending[future]), 'error': str(error)})
                print(json.dumps(failures[-1]), flush=True)
    report['source_sha256_after'] = {str(p.relative_to(package)): sha(p) for p in source_paths}
    report['source_stable'] = report['source_sha256_after'] == source_hashes
    report['errors'] = failures
    report['complete'] = len(report['commands']) == args.last - args.first + 1 and not failures
    report['all_runtime_gates_passed'] = (report['complete'] and report['source_stable'] and
                                        all(c['passed_runtime_gates'] for c in report['commands']))
    report['all_pairs_available'] = (report['complete'] and
                                    all(c['listening_pair_available'] for c in report['commands']))
    (args.out / 'native-sfx-matrix.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['all_runtime_gates_passed'] and report['all_pairs_available'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
