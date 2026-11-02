#!/usr/bin/env python3
"""Capture an explicitly built native lifecycle harness and preserve both epochs.

The API lifecycle gate and zero-loss audio gate are separate results. Cold
recovery must never erase the evidence from the first playback epoch.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('build', 'environment-root', 'bios', 'out'):
        p.add_argument('--' + name, required=True, type=Path)
    p.add_argument('--display', type=int, default=97)
    a = p.parse_args()
    for key, value in vars(a).items():
        if isinstance(value, Path):
            setattr(a, key, value.resolve())
    build = json.loads((a.build / 'saturn-build.json').read_text())
    if build.get('harness') != 'saturn_lifecycle.c':
        raise ValueError('Not an explicit lifecycle diagnostic build')
    for name, identity in build['files'].items():
        if sha(a.build / name) != identity['sha256']:
            raise ValueError('Build artifact changed before capture: ' + name)
    a.out.mkdir(parents=True, exist_ok=False)
    symbols = json.loads((a.build / 'symbols.json').read_text())
    address, diag = symbols['_audio2_lifecycle'], symbols['_audio2_diag']
    names = ('magic', 'phase', 'sfx_accepted', 'busy_retries', 'quiescent_stops',
             'recoveries', 'restarted_command', 'diagnostic_error', 'prior_api_error',
             'prior_fm_discard', 'prior_underruns', 'prior_late', 'prior_priority_rejections',
             'prior_accepted_commands', 'post_fm_discard', 'post_underruns', 'post_late',
             'api_state', 'api_error', 'worker_quiescent', 'video_frame',
             'post_priority_rejections', 'last_music_command', 'worker_state')
    command = [sys.executable, str(Path(__file__).with_name('capture_mednafen.py')),
               '--cue', str(a.build / 'AUDIO2.cue'), '--environment-root', str(a.environment_root),
               '--bios', str(a.bios), '--out', str(a.out / 'capture'),
               '--display', str(a.display), '--seconds', '90', '--audio-only', '--dump-wram',
               '--ready-watch', 'magic', '--ready-value', str(0x4c494631),
               '--stop-watch', 'completion', '--stop-value', '4',
               '--watch', f'completion:{diag+4:#x}:4']
    for index, name in enumerate(names):
        command += ['--watch', f'{name}:{address+index*4:#x}:4']
    with (a.out / 'capture.log').open('w') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
    report_path = a.out / 'capture/report.json'
    report = json.loads(report_path.read_text()) if report_path.exists() else {}
    values = report.get('final_watch_values', {})
    gates = {'native_capture_completed': result.returncode == 0 and report.get('stop_reason') == 'watch_threshold_reached',
             'all_original_sfx_words_accepted': values.get('sfx_accepted') == 98 and values.get('prior_accepted_commands') == 99,
             'busy_path_exercised': values.get('busy_retries', 0) > 0,
             'both_workers_returned_before_stop': values.get('quiescent_stops') == 2,
             'one_cold_recovery': values.get('recoveries') == 1,
             'original_music06_after_recovery': values.get('restarted_command') == 6 and values.get('last_music_command') == 6,
             'finished_stopped_and_quiescent': values.get('phase') == 7 and values.get('api_state') == 4 and values.get('worker_quiescent') == 1,
             'no_api_or_diagnostic_error': all(values.get(key) == 0 for key in ('diagnostic_error', 'prior_api_error', 'api_error'))}
    counter_keys = ('prior_fm_discard', 'prior_underruns', 'prior_late',
                    'post_fm_discard', 'post_underruns', 'post_late')
    zero_loss = all(values.get(key) == 0 for key in counter_keys)
    output = {'kind': 'NATIVE_BUSY_STOP_QUIESCENCE_COLD_RECOVERY',
              'scope': 'Direct98-command diagnostic, not gameplay or waveform-fidelity acceptance',
              'emulator': report.get('emulator'), 'emulator_sha256': report.get('emulator_sha256'),
              'build': build, 'firmware_sha256': sha(a.build / 'SCSP.ELF'), 'bios_sha256': sha(a.bios),
              'capture_command': command, 'capture_exit_code': result.returncode,
              'capture_report_sha256': sha(report_path) if report_path.exists() else None,
              'gates': gates, 'diagnostics': values, 'api_lifecycle_passed': all(gates.values()),
              'zero_loss_both_epochs': zero_loss, 'passed': all(gates.values()) and zero_loss,
              'hardware_status': 'HARDWARE PENDING'}
    (a.out / 'lifecycle-result.json').write_text(json.dumps(output, indent=2) + '\n')
    print(json.dumps({key: output[key] for key in ('api_lifecycle_passed', 'zero_loss_both_epochs', 'gates', 'diagnostics')}, indent=2))
    return 0 if output['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
