#!/usr/bin/env python3
"""Audit a native reference recording against the immutable R01 source PCM.

Reimplements the documented R01 audit: 44.1 kHz BGM correlation and gain,
background subtraction only for analysis, then 8 kHz cue correlations. The
preserved R01 lossless recording provides a same-method calibration.
"""
import os
os.environ['OPENBLAS_NUM_THREADS'] = '1'
os.environ['OMP_NUM_THREADS'] = '1'
from pathlib import Path
import argparse
import hashlib
import json
import wave
import numpy as np
from scipy import signal

RATE = 44100
CUE_RATE = 8000
TICK_SECONDS = 448 * 272 / 7000000


def match(y, q, low, high):
    """Best signed normalized correlation with start in inclusive [low, high]."""
    low = max(0, int(low)); high = min(int(high), len(y) - len(q))
    if high < low:
        raise ValueError('No complete match window')
    x = y[low:high + len(q)]
    c = signal.correlate(x, q, mode='valid', method='fft')
    energy = np.r_[0., np.cumsum(x * x)]
    energy = energy[len(q):] - energy[:-len(q)]
    qenergy = float(np.dot(q, q))
    ncc = c / np.sqrt(np.maximum(energy * qenergy, 1e-30))
    # FFT roundoff in a silent search window must not become a huge normalized
    # score when cumulative-energy subtraction reaches zero after BIOS audio.
    ncc[energy < len(q) * 1e-12] = -np.inf
    best = int(np.argmax(ncc))
    at = low + best
    gain = float(np.dot(y[at:at + len(q)], q) / qenergy)
    return at, float(ncc[best]), gain


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(8 * 1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('wav', type=Path)
    ap.add_argument('--label', required=True)
    ap.add_argument('--scene-origin', type=float, required=True)
    ap.add_argument('--scene-end', type=float)
    ap.add_argument('--calibration', type=Path)
    ap.add_argument('--source-media', type=Path)
    ap.add_argument('--runtime-metrics', type=Path)
    ap.add_argument('--source-dir', type=Path, default=here.parent / 'research_audio/integration')
    ap.add_argument('--cue-audit', type=Path, default=here.parent.parent / 'audit/r01_reference/audio/recording_cue_audit_v7_foreground.json')
    args = ap.parse_args()
    source_dir = args.source_dir
    source_path = source_dir / 'cd/WP02BGM.decoded.pcm'
    pack_path = source_dir / 'cd/WP02PCM.PAK'
    metadata_path = source_dir / 'audio_assets.json'
    cue_audit_path = args.cue_audit
    cue_audit = json.loads(cue_audit_path.read_text())
    metadata = json.loads(metadata_path.read_text())
    pack = pack_path.read_bytes()
    assert hashlib.sha256(pack).hexdigest() == metadata['pcm_pack_sha256']
    source = np.fromfile(source_path, dtype='>i2').astype(np.float64) / 32768.
    assert len(source) == 1278900
    with wave.open(str(args.wav), 'rb') as w:
        assert w.getframerate() == RATE and w.getsampwidth() == 2
        channels = w.getnchannels()
        raw = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').reshape(-1, channels)
    mono = raw.astype(np.float64).mean(axis=1) / 32768.

    # The initial match is independent of the video timing. Refine using the
    # original cue-free music interval, also used in the R01 report.
    opening, opening_ncc, _ = match(mono, source[:RATE], 0, len(mono) - RATE)
    quiet_first, quiet_last = round(26.2 * RATE), round(27.0 * RATE)
    quiet_source = source[quiet_first:quiet_last]
    quiet_at, quiet_ncc, bgm_gain = match(mono, quiet_source,
                                        opening + quiet_first - 128,
                                        opening + quiet_first + 128)
    origin = quiet_at - quiet_first
    quiet_actual = mono[quiet_at:quiet_at + len(quiet_source)]
    quiet_residual = quiet_actual - bgm_gain * quiet_source
    windows = []
    for first in range(0, 27 * RATE, RATE // 2):
        q = source[first:first + RATE // 2]
        expected = origin + first
        at, ncc, gain = match(mono, q, expected - 2048, expected + 2048)
        windows.append({'source_seconds': first / RATE, 'ncc': ncc,
                        'offset_samples': at - expected, 'gain': gain})

    # Locate the finite scene stop and inspect the uninterrupted interval.
    ending_low = origin + round(26.5 * RATE)
    ending_high = min(len(raw), origin + 30 * RATE)
    nonzero = np.flatnonzero(np.any(raw[ending_low:ending_high] != 0, axis=1))
    silence_at = ending_low + int(nonzero[-1]) + 1
    scene_raw = raw[origin:silence_at]
    inspected = mono[origin + 1024:silence_at - 1024]
    zero = inspected == 0
    transitions = np.diff(np.r_[False, zero, False].astype(np.int8))
    zero_starts = np.flatnonzero(transitions == 1)
    zero_ends = np.flatnonzero(transitions == -1)
    maximum_zero_run = int(np.max(zero_ends - zero_starts, initial=0))
    n10 = (silence_at - origin - 2048) // 441
    body10 = mono[origin + 1024:origin + 1024 + n10 * 441].reshape(-1, 441)
    source10 = source[1024:1024 + n10 * 441].reshape(-1, 441)
    rms10 = np.sqrt(np.mean(body10 * body10, axis=1))
    source_rms10 = np.sqrt(np.mean(source10 * source10, axis=1))
    dropouts = np.flatnonzero((source_rms10 > .001) & (rms10 < source_rms10 * .05))

    # Subtraction, resampling and filtering affect analysis arrays only.
    foreground = mono.copy()
    count = min(len(source), len(foreground) - origin)
    foreground[origin:origin + count] -= bgm_gain * source[:count]
    foreground8 = signal.resample_poly(foreground, 80, 441)
    sos = signal.butter(3, [100, 3400], btype='bandpass', fs=CUE_RATE, output='sos')
    foreground8 = signal.sosfiltfilt(sos, foreground8)
    assets = {(row['bank'], row['sample_id']): row for row in metadata['samples']}
    cues = []
    for row in cue_audit['saturn']['cues']:
        asset = assets[(row['bank'], row['sample'])]
        first = asset['sector'] * 2048
        data = pack[first:first + asset['pcm16_bytes']]
        assert hashlib.sha256(data).hexdigest() == asset['pcm16be_sha256']
        q = np.frombuffer(data, dtype='>i2').astype(np.float64) / 32768.
        q = signal.sosfiltfilt(sos, q)
        # The R01 capture audit verifies onsets. Original voice allocation can
        # replace a common cue before the end of its four-second source file.
        # Use up to one second: enough energy for the quiet beginning of common
        # sample17, while excluding later replacement of the long common cues.
        source_seconds = len(q) / CUE_RATE
        q = q[:CUE_RATE]
        expected = args.scene_origin + row['frame'] * TICK_SECONDS
        coarse_at, coarse_ncc, coarse_gain = match(
            foreground8, q, round((expected - .15) * CUE_RATE),
            round((expected + .15) * CUE_RATE))
        # A nearest8kHz-sample match can be62.5us from the true cue phase.
        # Refine the same filtered waveform locally at eightfold resolution;
        # retain the coarse result so the analysis improvement is explicit.
        refine_first = max(0, coarse_at - 4)
        refine_last = min(len(foreground8), coarse_at + len(q) + 4)
        fine_y = signal.resample_poly(foreground8[refine_first:refine_last], 8, 1)
        fine_q = signal.resample_poly(q, 8, 1)
        fine_at, ncc, gain = match(fine_y, fine_q, 0, len(fine_y) - len(fine_q))
        detected = (refine_first + fine_at / 8.) / CUE_RATE
        cues.append({'frame': row['frame'], 'command': row['command'],
                     'bank': row['bank'], 'sample': row['sample'],
                     'expected_seconds': expected, 'detected_seconds': detected,
                     'offset_ms': (detected - expected) * 1000.,
                     'ncc': ncc, 'source_to_capture_gain': gain,
                     'coarse_detected_seconds': coarse_at / CUE_RATE,
                     'coarse_ncc': coarse_ncc, 'coarse_source_to_capture_gain': coarse_gain,
                     'source_seconds': source_seconds, 'onset_template_seconds': len(q) / CUE_RATE,
                     'source_pcm_sha256': asset['pcm16be_sha256']})

    failures = []
    if quiet_ncc < .9999: failures.append('Quiet music correlation is below .9999')
    if np.sqrt(np.mean(quiet_residual ** 2)) > .0001:
        failures.append('Quiet music residual exceeds .0001 RMS')
    if any(w['offset_samples'] != 0 for w in windows): failures.append('BGM window sample lag')
    if min(w['ncc'] for w in windows) < .80: failures.append('BGM window correlation below .80')
    if maximum_zero_run >= 44: failures.append('An internal silent run lasts at least 1 ms')
    if len(dropouts): failures.append('A 10 ms window has less than 5% of expected music RMS')
    clipped = int(np.count_nonzero((scene_raw == 32767) | (scene_raw == -32768)))
    channel_delta = float(np.sqrt(np.mean((scene_raw[:, 0].astype(float) - scene_raw[:, -1]) ** 2))) / 32768.
    if clipped: failures.append('Clipped scene PCM values')
    if channel_delta != 0: failures.append('Scene channels differ')
    if len(cues) != 13 or min(c['ncc'] for c in cues) < .75:
        failures.append('At least one required cue correlation is below .75')
    if any(abs(c['offset_ms']) > 30 for c in cues[1:]):
        failures.append('A later cue is more than 30 ms from its original scene deadline')
    if args.scene_end is not None and abs(silence_at / RATE - args.scene_end) > .05:
        failures.append('Music stop differs from visible scene exit by more than 50 ms')

    report = {
        'status': 'PASS' if not failures else 'FAIL', 'failures': failures,
        'capture_wav': str(args.wav.resolve()), 'capture_wav_sha256': sha(args.wav),
        'sample_rate': RATE, 'duration_seconds': len(mono) / RATE,
        'visual_scene_origin_seconds': args.scene_origin, 'visual_scene_end_seconds': args.scene_end,
        'music_start_sample': origin, 'music_start_seconds': origin / RATE,
        'opening_match_start_sample': opening, 'opening_ncc': opening_ncc,
        'quiet_window_source_seconds': [26.2, 27.0], 'quiet_ncc': quiet_ncc,
        'quiet_native_source_gain': bgm_gain, 'quiet_native_source_db': float(20 * np.log10(bgm_gain)),
        'quiet_residual_rms': float(np.sqrt(np.mean(quiet_residual ** 2))),
        'background_windows': windows, 'background_window_count': len(windows),
        'background_max_abs_lag_samples': max(abs(w['offset_samples']) for w in windows),
        'silence_begins_seconds': silence_at / RATE,
        'silence_begins_source_elapsed': (silence_at - origin) / RATE,
        'completed_ring_cycles': (silence_at - origin) // 32768,
        'maximum_internal_zero_run_samples': maximum_zero_run,
        'low_energy_10ms_window_count': len(dropouts), 'clipped_values': clipped,
        'scene_channel_delta_rms': channel_delta, 'cue_count': len(cues), 'cues': cues,
        'analysis': {'bgm': 'Raw44.1kHz PCM; signed normalized correlation; half-second windows with ±2048-sample lag search',
                     'cues': 'Background subtraction, polyphase44.1kHz-to8kHz resampling, third-order zero-phase100..3400Hz bandpass; first1s original cue onset templates, or complete shorter sources; local8x interpolation refines the same comparison to64kHz timing resolution',
                     'calibration': 'Same analysis independently run on preserved R01 lossless MOV; the R01 final generator was not included in its package'},
        'inputs_sha256': {str(p): sha(p) for p in (source_path, pack_path, metadata_path, cue_audit_path)},
        'limits': ['Emulator output only; physical Saturn result remains pending.',
                   'Cue sample delivery retains VBlank/driver latency; first cue precedes the first visible frame.',
                   'Gap checks establish continuity at the documented analysis resolutions, not an analog-output guarantee.']
    }
    if args.calibration:
        calibration = json.loads(args.calibration.read_text())
        report['r01_same_method_comparison'] = {
            'path': str(args.calibration.resolve()),
            'background_gain_delta_db': float(20 * np.log10(bgm_gain / calibration['quiet_native_source_gain'])),
            'cue_gain_delta_db': [float(20 * np.log10(a['source_to_capture_gain'] / b['source_to_capture_gain']))
                                  for a, b in zip(cues, calibration['cues'])],
            'cue_ncc_delta': [a['ncc'] - b['ncc'] for a, b in zip(cues, calibration['cues'])]
        }
    if args.source_media:
        report['source_media'] = str(args.source_media.resolve())
        report['source_media_sha256'] = sha(args.source_media)
        report['source_media_bytes'] = args.source_media.stat().st_size
        report['audio_extraction_command'] = [
            'ffmpeg', '-v', 'error', '-i', str(args.source_media.resolve()),
            '-map', '0:a:0', '-vn', '-c:a', 'pcm_s16le', str(args.wav.resolve())]
    if args.runtime_metrics:
        report['runtime_metrics'] = json.loads(args.runtime_metrics.read_text())['metrics']
    output = here / (args.label + '.json')
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in (
        'status', 'failures', 'music_start_seconds', 'quiet_ncc', 'quiet_native_source_gain',
        'quiet_residual_rms', 'background_window_count', 'background_max_abs_lag_samples',
        'silence_begins_seconds', 'silence_begins_source_elapsed', 'completed_ring_cycles',
        'maximum_internal_zero_run_samples', 'low_energy_10ms_window_count', 'clipped_values',
        'scene_channel_delta_rms', 'cue_count')}, indent=2))
    print('Cue NCC range:', min(c['ncc'] for c in cues), max(c['ncc'] for c in cues))
    print('Report:', output)
    raise SystemExit(0 if not failures else 1)


if __name__ == '__main__':
    main()
