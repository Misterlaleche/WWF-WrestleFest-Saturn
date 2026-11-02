#!/usr/bin/env python3
"""Exercise two corrupt local inputs through the real Saturn API in Mednafen.

Expected failure is API FAULT/RESOURCE before a worker or command starts.
Original source and resources are frozen; failed capture is never a PASS.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys

from source_snapshot import snapshot_package, source_identity as file_hashes


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('package', 'resources', 'firmware', 'sdk', 'environment-root', 'bios', 'out'):
        p.add_argument('--' + name, required=True, type=Path)
    p.add_argument('--display', type=int, default=97)
    a = p.parse_args()
    a.out = a.out.resolve()
    a.out.mkdir(parents=True, exist_ok=False)
    frozen = a.out / 'source'
    # This module's helper verifies that all files survived a stable copy.
    source_manifest = snapshot_package(a.package.resolve(), frozen)
    spec = importlib.util.spec_from_file_location('frozen_saturn_builder', frozen / 'tools/build_saturn.py')
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    base = a.out / 'inputs'
    (base / 'resources').mkdir(parents=True)
    (base / 'firmware').mkdir()
    for name in ('SOUND.ROM', 'SFX.DAT'):
        shutil.copyfile(a.resources / name, base / 'resources' / name)
    shutil.copyfile(a.firmware, base / 'firmware/SCSP.ELF')
    shutil.copyfile(a.firmware.parent / 'symbols.json', base / 'firmware/symbols.json')
    shutil.copyfile(a.bios, base / 'bios.bin')
    before = file_hashes(base)
    results = []
    for name in ('pack_crc', 'firmware_magic'):
        case = a.out / name
        shutil.copytree(base, case / 'inputs')
        bad = case / ('inputs/resources/SFX.DAT' if name == 'pack_crc' else 'inputs/firmware/SCSP.ELF')
        data = bytearray(bad.read_bytes())
        offset = len(data) - 1 if name == 'pack_crc' else 0
        original_byte = data[offset]
        data[offset] ^= 1
        bad.write_bytes(data)
        build = builder.build(frozen, case / 'build', case / 'inputs/resources',
                              case / 'inputs/firmware/SCSP.ELF', a.sdk.resolve(), 1, 3)
        address = build['diag_address']
        watches = {'magic': 0, 'state': 1, 'workerError': 3, 'workerState': 5,
                   'commandStartFrame': 4, 'APIstate': 19, 'APIerror': 20}
        command = [sys.executable, str(frozen / 'tools/capture_mednafen.py'),
                   '--cue', str(case / 'build/AUDIO2.cue'), '--environment-root', str(a.environment_root.resolve()),
                   '--bios', str(base / 'bios.bin'), '--out', str(case / 'capture'),
                   '--display', str(a.display), '--seconds', '60', '--audio-only',
                   '--stop-watch', 'state', '--stop-value', '3',
                   '--ready-watch', 'magic', '--ready-value', str(0x57464132), '--dump-wram']
        for field, index in watches.items():
            command += ['--watch', f'{field}:{address+index*4:#x}:4']
        with (case / 'capture.log').open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        report_path = case / 'capture/report.json'
        report = json.loads(report_path.read_text()) if report_path.exists() else {}
        observed = report.get('final_watch_values', {})
        passed = (result.returncode == 0 and report.get('stop_reason') == 'watch_threshold_reached' and
                  observed.get('magic') == 0x57464132 and observed.get('state') == 3 and
                  observed.get('APIstate') == 5 and observed.get('APIerror') == 2 and
                  observed.get('commandStartFrame') == 0 and observed.get('workerState') == 0)
        results.append({'case': name, 'changed_input': str(bad.relative_to(a.out)),
                        'byte_offset': offset, 'before_byte': original_byte, 'after_byte': data[offset],
                        'corrupt_input_sha256': sha(bad), 'build': build, 'capture_command': command,
                        'capture_exit_code': result.returncode, 'observed': observed,
                        'expected_native_rejection_passed': passed,
                        'capture_report_sha256': sha(report_path) if report_path.exists() else None})
        (a.out / 'native-load-failures.json').write_text(json.dumps({'results': results}, indent=2) + '\n')
    stable = before == file_hashes(base)
    report = {'scope': 'Real Saturn/Mednafen1.32.1 negative resource-load tests',
              'source_snapshot': source_manifest, 'inputs_sha256': before, 'inputs_stable': stable,
              'source_stable': file_hashes(frozen) == source_manifest,
              'results': results, 'passed': stable and all(row['expected_native_rejection_passed'] for row in results),
              'hardware_status': 'HARDWARE PENDING'}
    report['passed'] &= report['source_stable']
    (a.out / 'native-load-failures.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'results': [dict(case=x['case'], passed=x['expected_native_rejection_passed'], observed=x['observed']) for x in results]}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
