#!/usr/bin/env python3
"""Cold-boot all18 music commands from one immutable source/input snapshot.

Uses only the selected real standalone Mednafen1.32.1. Corrected board-pcm16
references are required. Runtime failures remain recorded even when a WAV
pair is available; metrics never stand in for listening or hardware approval.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
import threading
import time
import wave

from source_snapshot import snapshot_package, source_identity


def make_cases(first, last, seconds, score_seconds=None, tail_seconds=8, transitions=False):
    cases = []
    for command in range(first, last + 1):
        events = None if score_seconds is None else [
            {'sample': 0, 'word': command}, {'sample': score_seconds * 44100, 'word': 0x1f}]
        cases.append({'id': f'command-{command:02X}', 'command': command, 'kind': 'music',
                      'seconds': seconds if score_seconds is None else score_seconds + tail_seconds,
                      'events': events})
    if transitions:
        words = [1, 6, 0x12, 0x1f, 3, 0x1f, 2, 1, 0x1f]
        cases.append({'id': 'transitions', 'command': 1, 'kind': 'transitions',
                      'seconds': 3 * (len(words) - 1) + tail_seconds,
                      'events': [{'sample': i * 3 * 44100, 'word': word} for i, word in enumerate(words)]})
    return cases


def inspect_replay_rows(rows, events, maximum_delay=1475):
    observations = []
    for expected, row in zip(events, rows):
        word, sample, frame, result = row
        observations.append({'expected': expected, 'word': word, 'sample': sample,
                             'video_frame': frame, 'result': result,
                             'admission_delay_samples': sample - expected['sample']})
    gates = {
        'all_rows_present': len(rows) == len(events),
        'exact_original_words': all(e['word'] == r[0] for e, r in zip(events, rows)),
        'all_results_accepted': all(r[3] == 0 for r in rows),
        'first_admission_is_epoch': bool(rows) and rows[0][1] == 0,
        'no_early_admission': all(r[1] >= e['sample'] for e, r in zip(events, rows)),
        'monotonic_admission': all(a[1] <= b[1] for a, b in zip(rows, rows[1:])),
        'within_declared_admission_tolerance': all(0 <= r[1] - e['sample'] <= maximum_delay
                                                  for e, r in zip(events, rows)),
    }
    return {'gates': gates, 'passed': all(gates.values()), 'observations': observations,
            'admission_tolerance_samples': maximum_delay,
            'maximum_admission_delay_samples': max((r['admission_delay_samples'] for r in observations), default=None),
            'scope': 'Actual public API admission observed in native RAM. The video-frame harness admits at or after each requested SCSP sample; this does not prove cycle-exact Z80 delivery.'}


def replay_observation(build, capture, events):
    symbols = json.loads((build / 'symbols.json').read_text())
    ram = (capture / 'WorkRAMH-saturn-order.bin').read_bytes()
    address = (symbols['_audio2_replay_observed'] & 0x1fffffff) - 0x06000000
    if not 0 <= address <= len(ram) - len(events) * 16:
        raise ValueError('Replay observation is outside native high RAM')
    rows = [struct.unpack_from('>4I', ram, address + i * 16) for i in range(len(events))]
    return inspect_replay_rows(rows, events)


def fade_tail_observation(capture, details, scsp_ram, replay, duration):
    """Measure unmodified stereo samples at a documented native-clock origin."""
    import numpy as np
    diagnostics = details['final_watch_values']
    clock = struct.unpack_from('>H', scsp_ram, 0xb010)[0]
    residual = (clock - diagnostics['DSPclock16']) & 0xffff
    if residual > 1475:
        raise ValueError('Final diagnostic clock is more than two video frames stale')
    fades = [r for r in replay['observations'] if (r['word'] & 255) == 0x1f]
    if not fades:
        raise ValueError('No original fade was observed')
    fade = fades[-1]['sample']
    with wave.open(str(capture / 'native.wav')) as f:
        if f.getframerate() != 44100 or f.getsampwidth() != 2:
            raise ValueError('Expected native PCM16/44100 soundrecord WAV')
        origin = f.getnframes() - diagnostics['SCSPelapsed'] - residual
        begin, end = origin + fade, origin + duration * 44100
        if not 0 <= begin < end <= f.getnframes():
            raise ValueError('Capture does not include the entire requested fade tail')
        f.setpos(begin)
        data = np.frombuffer(f.readframes(end - begin), dtype='<i2').reshape(-1, f.getnchannels())
    active = np.flatnonzero(np.any(data != 0, axis=1))
    last = int(active[-1]) if len(active) else None
    final = data[-min(len(data), 2 * 44100):].astype(np.int64)
    return {'raw_source_begin_frame': begin, 'raw_source_end_frame_exclusive': end,
            'origin_method': 'Final SCSP clock minus elapsed since first API admission; no waveform fitting',
            'residual_DAC_alignment': 'Not independently calibrated',
            'final_diagnostic_staleness_samples': residual, 'last_fade_admission_sample': fade,
            'captured_after_fade_seconds': len(data) / 44100,
            'last_nonzero_after_fade_seconds': last / 44100 if last is not None else None,
            'trailing_exact_zero_frames': len(data) - 1 - last if last is not None else len(data),
            'final_window_frames': len(final), 'final_window_peak': int(np.abs(final).max()),
            'final_window_rms_raw_pcm_units': float(np.sqrt(np.mean(final.astype(float) ** 2))),
            'final_window_all_channels_exact_zero': not bool(np.any(final)),
            'gain': 1, 'normalization': False,
            'scope': 'Audible digital tail observation only; no natural composition-end or PCM-loop claim'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def freeze_file(source, destination, expected=None):
    source, destination = Path(source), Path(destination)
    data = source.read_bytes()
    identity = hashlib.sha256(data).hexdigest()
    if expected is not None and identity != expected:
        raise ValueError('Input identity mismatch: ' + str(source))
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    if source.read_bytes() != data or destination.read_bytes() != data:
        raise RuntimeError('Input changed while making snapshot: ' + str(source))
    return identity


def firmware_prefix(firmware):
    data = Path(firmware).read_bytes()
    offset = struct.unpack_from('>I', data, 28)[0]
    size, count = struct.unpack_from('>HH', data, 42)
    headers = [struct.unpack_from('>8I', data, offset + i * size) for i in range(count)]
    text = next(h for h in headers if h[0] == 1 and h[2] == 0)
    if text[4] < 512:
        raise ValueError('Firmware text is too small for identity check')
    return data[text[1]:text[1] + 512]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('resources', 'firmware', 'sdk', 'environment-root', 'bios', 'reference-report', 'out'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--package', type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument('--first', type=lambda s: int(s, 0), default=1)
    p.add_argument('--last', type=lambda s: int(s, 0), default=18)
    p.add_argument('--seconds', type=int, default=35)
    p.add_argument('--score-seconds', type=int, help='Play this long, admit original1F, then capture --tail-seconds')
    p.add_argument('--tail-seconds', type=int, default=8)
    p.add_argument('--transitions', action='store_true', help='Also run the declared original-command transition schedule')
    p.add_argument('--presentation-speed', type=int, choices=(1,4), default=1)
    p.add_argument('--displays', default='86,88', help='One to four distinct X display numbers')
    p.add_argument('--wall-seconds', type=int, default=150)
    p.add_argument('--profile-label', required=True, help='Exact experimental firmware profile; never reuse its PASS for another build')
    a = p.parse_args()
    displays = [int(v) for v in a.displays.split(',')]
    if not 1 <= len(displays) <= 4 or len(set(displays)) != len(displays):
        p.error('Need one to four distinct display numbers')
    if not 1 <= a.first <= a.last <= 18 or a.seconds < 1:
        p.error('Original music command range is1..18')
    if a.score_seconds is not None and not 1 <= a.score_seconds <= 1800:
        p.error('Long score duration must be1..1800 seconds')
    if not 1 <= a.tail_seconds <= 60 or (a.transitions and a.score_seconds is None):
        p.error('Need1..60 tail seconds; transitions require the explicit long score profile')
    cases = make_cases(a.first,a.last,a.seconds,a.score_seconds,a.tail_seconds,a.transitions)
    if not os.environ.get('MAKE_ISO_XORRISOFS'):
        p.error('Source the reproduced environment env.sh before running')
    for name, value in vars(a).items():
        if isinstance(value, Path):
            setattr(a, name, value.resolve())
    reference = json.loads(a.reference_report.read_text())
    if (reference.get('profile') != 'board-pcm16' or not reference.get('complete') or
            reference.get('external_ym_clock_hz') != 3579545 or reference.get('opm_pipeline_divider') != 2):
        raise ValueError('Require complete corrected board-pcm16 reference report; historical A53 is separate')
    tracks = {t['command']: t for t in reference['tracks']}
    if set(tracks) != set(range(1, 19)):
        raise ValueError('Reference report must identify all18 original commands')
    if any(c['seconds'] < tracks[c['command']]['seconds'] + 2 for c in cases if c['kind']=='music'):
        raise ValueError('Native window must exceed each reference by at least two seconds')
    if a.score_seconds is not None and any(a.score_seconds < tracks[c]['seconds'] for c in range(a.first,a.last+1)):
        raise ValueError('The original fade must follow the complete prefix used for comparison')
    if a.out == a.package or a.package in a.out.parents:
        raise ValueError('Private captures must stay outside the source package')
    a.out.mkdir(parents=True, exist_ok=False)
    package, inputs = a.out / 'source', a.out / 'inputs'
    sources = snapshot_package(a.package, package)
    frozen_firmware = inputs / 'firmware/SCSP.ELF'
    freeze_file(a.firmware, frozen_firmware)
    freeze_file(a.firmware.parent / 'symbols.json', frozen_firmware.parent / 'symbols.json')
    for name in ('SOUND.ROM', 'SFX.DAT'):
        freeze_file(a.resources / name, inputs / 'resources' / name)
    bios = inputs / 'bios.bin'
    freeze_file(a.bios, bios)
    freeze_file(a.reference_report, inputs / 'reference-report.json')
    for command in range(a.first, a.last + 1):
        freeze_file(tracks[command]['wav'], inputs / 'reference' / f'ARCADE-REFERENCIA-{command:02d}.wav',
                    tracks[command]['wav_sha256'])
    for case in cases:
        if case['events'] is not None:
            replay=inputs/'replays'/(case['id']+'.json');replay.parent.mkdir(exist_ok=True)
            replay.write_text(json.dumps({'scope':'Declared isolated long music/fade/transition diagnostic; original commands only',
                                          'events':case['events']},indent=2)+'\n')
    input_identity = source_identity(inputs)
    expected_prefix = firmware_prefix(frozen_firmware)
    emulator = a.environment_root / 'mednafen/bin/mednafen'
    emulator_sha = sha(emulator)
    report = {
        'kind': 'LONG_NATIVE_MUSIC_FADE_MATRIX' if a.score_seconds is not None else 'COLD_BOOT_NATIVE_MUSIC_MATRIX_BOARD_CLOCK',
        'profile_label': a.profile_label,
        'emulator': 'standalone Mednafen1.32.1', 'emulator_sha256': emulator_sha,
        'source_origin': str(a.package), 'source_snapshot': str(package), 'source_sha256_before': sources,
        'input_snapshot': str(inputs), 'input_sha256_before': input_identity,
        'firmware_sha256': sha(frozen_firmware), 'reference_report_sha256': sha(inputs / 'reference-report.json'),
        'reference_profile': 'board-pcm16', 'range': [a.first, a.last],
        'native_window_seconds': cases[0]['seconds'], 'score_seconds_before_1F':a.score_seconds,
        'tail_seconds':a.tail_seconds if a.score_seconds is not None else None,
        'presentation_speed_requested':a.presentation_speed,'emulated_hardware_clock_change':False,
        'cases':cases,'reference_window_seconds': tracks[a.first]['seconds'],
        'output_naming': 'WAV suffixes are decimal01..18; command directories are hexadecimal01..12',
        'capture_mode': 'Actual soundrecord/native state; ss.cart none; no memory writes or emulator modifications',
        'scope': 'One frozen isolated public-API profile; no gameplay integration or waveform fidelity certification',
        'hardware_status': 'HARDWARE PENDING', 'listening_approval': 'PENDING',
        'started_unix': time.time(), 'commands': [], 'errors': [], 'complete': False,
    }
    lock, condition = threading.Lock(), threading.Condition()
    available = displays.copy()

    def save():
        temporary = a.out / 'matrix-progress.tmp.json'
        temporary.write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
        temporary.replace(a.out / 'matrix-progress.json')

    def run_stage(arguments, log, timeout):
        with log.open('w') as stream:
            child = subprocess.Popen(list(map(str, arguments)), stdout=stream, stderr=subprocess.STDOUT,start_new_session=True)
            timed_out=False
            try:child.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out=True;os.killpg(child.pid,signal.SIGTERM)
                try:child.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(child.pid,signal.SIGKILL);child.wait()
        return {'command': list(map(str, arguments)), 'exit_code': child.returncode,'timed_out':timed_out,
                'log': str(log), 'log_sha256': sha(log)}

    def capture(case):
        command=case['command'];identity=case['id']
        work = a.out / identity
        work.mkdir()
        build, native, pair = work / 'build', work / 'capture', a.out / 'pairs'
        item = {'id':identity,'kind':case['kind'],'command': command, 'hex': f'0x{command:02X}', 'status': 'running', 'stages': []}
        with lock:
            # Publish only an immutable running marker; the job's detailed
            # dictionary stays private until completion to avoid save races.
            report['commands'].append({'id':identity,'kind':case['kind'],'command': command, 'hex': item['hex'], 'status': 'running'})
            report['commands'].sort(key=lambda row: row['id']); save()
        try:
            build_args=[sys.executable, package / 'tools/build_saturn.py', '--package', package,
                '--out', build, '--resources', inputs / 'resources', '--firmware', frozen_firmware,
                '--sdk', a.sdk, '--command', command, '--seconds', case['seconds']]
            if case['events'] is not None:build_args+=['--replay',inputs/'replays'/(identity+'.json')]
            stage = run_stage(build_args, work / 'build.log', 180)
            item['stages'].append(stage)
            if stage['exit_code']:
                raise RuntimeError('Native build failed')
            item['build'] = json.loads((build / 'saturn-build.json').read_text())
            with condition:
                condition.wait_for(lambda: bool(available))
                display = available.pop(0)
            try:
                stage = run_stage([sys.executable, package / 'tools/run_native_test.py', '--build', build,
                    '--environment-root', a.environment_root, '--bios', bios, '--out', native,
                    '--display', display, '--wall-seconds', a.wall_seconds, '--audio-only',
                    '--presentation-speed',a.presentation_speed],
                    work / 'capture.log', a.wall_seconds + 45)
            finally:
                with condition:
                    available.append(display); condition.notify()
            item['stages'].append(stage)
            gate = json.loads((native / 'native-gates.json').read_text()) if (native / 'native-gates.json').exists() else {}
            details = json.loads((native / 'report.json').read_text()) if (native / 'report.json').exists() else {}
            command_line = details.get('command', [])
            ram = (native / 'SCSP-RAM-saturn-order.bin').read_bytes() if (native / 'SCSP-RAM-saturn-order.bin').exists() else b''
            extra = {'firmware_first512_matches': ram[:512] == expected_prefix,
                     'native_wav_finalized': details.get('wav_validation', {}).get('finalized') is True,
                     'selected_emulator_identity': details.get('emulator_sha256') == emulator_sha,
                     'no_expansion_cartridge': any(command_line[i:i+2] == ['-ss.cart', 'none'] for i in range(len(command_line) - 1))}
            if case['events'] is not None and details.get('final_watch_values',{}).get('state')==4:
                item['replay_observation']=replay_observation(build,native,case['events'])
                extra['replay_words_results_and_admission_times']=item['replay_observation']['passed']
                item['fade_tail_observation']=fade_tail_observation(native,details,ram,item['replay_observation'],case['seconds'])
            elif case['events'] is not None:
                extra['replay_words_results_and_admission_times']=False
            item.update(native_gates=gate, additional_gates=extra,
                        passed_runtime_gates=stage['exit_code'] == 0 and gate.get('passed') is True and all(extra.values()),
                        capture_report_sha256=sha(native / 'report.json') if details else None)
            item['pair_available'] = False if case['kind']=='music' else None
            if case['kind']=='music' and extra['native_wav_finalized'] and details.get('final_watch_values', {}).get('state') == 4:
                stage = run_stage([sys.executable, package / 'tools/compare_capture.py', '--capture', native,
                    '--reference', inputs / 'reference' / f'ARCADE-REFERENCIA-{command:02d}.wav', '--reference-profile', 'board-pcm16',
                    '--build', build, '--out', pair, '--command', command], work / 'compare.log', 90)
                item['stages'].append(stage); item['pair_available'] = stage['exit_code'] == 0
                if item['pair_available']:
                    comparison = pair / f'COMPARACION-{command:02d}.json'
                    item['comparison_sha256'] = sha(comparison)
                    item['metrics'] = json.loads(comparison.read_text())['metrics']
            item['status'] = 'complete'
        except Exception as error:
            item.update(status='failed', error=str(error), passed_runtime_gates=False)
            with lock:
                report['errors'].append({'id':identity,'command': command, 'error': str(error)})
        with lock:
            item['finished_unix'] = time.time()
            report['commands'] = [item if row['id'] == identity else row for row in report['commands']]
            save()
            print(json.dumps({'id':identity,'command': item['hex'], 'status': item['status'],
                              'runtime_passed': item.get('passed_runtime_gates'),
                              'pair': item.get('pair_available', False)}), flush=True)

    save()
    with ThreadPoolExecutor(max_workers=len(displays)) as pool:
        for result in as_completed([pool.submit(capture, case) for case in cases]):
            result.result()
    report['source_sha256_after'] = source_identity(package)
    report['input_sha256_after'] = source_identity(inputs)
    report['source_stable'] = sources == report['source_sha256_after']
    report['inputs_stable'] = input_identity == report['input_sha256_after']
    report['emulator_stable'] = emulator_sha == sha(emulator)
    report['complete'] = len(report['commands']) == len(cases) and not report['errors']
    report['all_runtime_gates_passed'] = (report['complete'] and report['source_stable'] and report['inputs_stable'] and
                                         report['emulator_stable'] and all(t.get('passed_runtime_gates') for t in report['commands']))
    report['all_pairs_available'] = report['complete'] and all(t.get('pair_available') for t in report['commands'] if t['kind']=='music')
    if a.score_seconds is not None:
        report['all_final_tails_exact_silence_observed']=report['complete'] and all(
            t.get('fade_tail_observation',{}).get('final_window_all_channels_exact_zero') for t in report['commands'])
        report['tail_acceptance_scope']='Measured separately from runtime gates; compare any remaining tail with original synthesis before classifying a fidelity defect.'
    report['finished_unix'] = time.time()
    save()
    (a.out / 'native-music-matrix.json').write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
    return 0 if report['all_runtime_gates_passed'] and report['all_pairs_available'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
