#!/usr/bin/env python3
"""Summarize original emulator audio callbacks and read-only firmware watches."""
import argparse
import array
import hashlib
import json
import math
import sys
import wave
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('capture', type=Path)
a = p.parse_args()
report = json.loads((a.capture / 'report.json').read_text())
samples = [s for s in report['samples'] if s.get('wp1979_started')]
assert samples, 'No native gameplay observed'
first = samples[0]['emulator_frame']
probes = [s for s in report.get('audio_probe', []) if s['emulator_frame'] >= first]
assert len(probes) >= 2, 'Capture needs --probe-audio'
ticks = report['sample_rate'] / report['video_rate']
assert ticks.is_integer(), 'Expected integral SCSP sample ticks per video frame'
bad = []
for old, new in zip(probes, probes[1:]):
    delta = (new['dsp_clock'] - old['dsp_clock']) & 0xffff
    expected = int(ticks) * (new['emulator_frame'] - old['emulator_frame']) & 0xffff
    if delta != expected:
        bad.append({'emulator_frame': new['emulator_frame'], 'actual': delta, 'expected': expected})

wav_path = a.capture / 'native-audio.wav'
count = nonzero = peak = squares = 0
with wave.open(str(wav_path), 'rb') as wav:
    assert wav.getsampwidth() == 2 and wav.getcomptype() == 'NONE'
    channels, rate, frames = wav.getnchannels(), wav.getframerate(), wav.getnframes()
    while True:
        data = wav.readframes(65536)
        if not data:
            break
        values = array.array('h', data)
        if sys.byteorder != 'little':
            values.byteswap()
        count += len(values)
        for value in values:
            nonzero += value != 0
            peak = max(peak, abs(value))
            squares += value * value
assert count == channels * frames
states = sorted({s['music_state'] for s in samples})
errors = sorted({s['music_error'] for s in samples})
mail_states = sorted({int.from_bytes(bytes.fromhex(s['scsp_mail'])[6:8], 'big') for s in samples})
result = {
    'status': 'PASS' if not bad and states == [2] and errors == [0] and mail_states == [2] and nonzero else 'FAIL',
    'capture': a.capture.name,
    'source': 'Actual libretro audio callbacks; no replacement soundtrack or amplitude processing',
    'music_state_values_during_gameplay': states,
    'music_error_values_during_gameplay': errors,
    'scsp_mail_state_values_during_gameplay': mail_states,
    'music_events_final': samples[-1]['music_events'],
    'dsp_intervals_checked': len(probes) - 1,
    'dsp_ticks_per_emulated_video_frame': int(ticks),
    'dsp_sample_rate_hz': report['sample_rate'],
    'dsp_clock_mismatches': bad,
    'wav': {'channels': channels, 'sample_rate_hz': rate, 'samples_per_channel': frames,
            'duration_seconds': frames / rate, 'nonzero_channel_samples': nonzero,
            'peak_pcm16': peak, 'rms_pcm16': math.sqrt(squares / count) if count else 0,
            'sha256': hashlib.sha256(wav_path.read_bytes()).hexdigest()},
    'hardware_test': False,
}
(a.capture / 'audio-analysis.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(result['status'] != 'PASS')
