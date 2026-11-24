#!/usr/bin/env python3
import hashlib
import importlib.util
import math
from pathlib import Path
import tempfile
import wave

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('wf_build_audio', ROOT / 'tools' / 'build_audio.py')
BUILD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILD)


def tone_amplitude(samples, rate, frequency):
    n = np.arange(len(samples), dtype=np.float64)
    phase = 2.0 * math.pi * frequency * n / rate
    # Least-squares sinusoid amplitude, insensitive to output phase/delay.
    c = np.cos(phase)
    s = np.sin(phase)
    return 2.0 * math.hypot(float(np.dot(samples, c)), float(np.dot(samples, s))) / len(samples)


def read_mono16(path):
    with wave.open(str(path), 'rb') as source:
        assert source.getnchannels() == 1
        assert source.getsampwidth() == 2
        assert source.getframerate() == 44100
        frames = source.getnframes()
        data = np.frombuffer(source.readframes(frames), dtype='<i2').astype(np.float64)
    return data


def main():
    assert hasattr(BUILD, 'resample_pcm16be'), 'band-limited PCM resampler is missing'
    in_rate = 176400
    seconds = 1
    n = np.arange(in_rate * seconds, dtype=np.float64)
    # 10 kHz must survive. 30 kHz is above the 44.1 kHz Nyquist limit and,
    # without antialias filtering, folds to 14.1 kHz at almost full level.
    source = (7000.0 * np.sin(2.0 * math.pi * 10000.0 * n / in_rate) +
              7000.0 * np.sin(2.0 * math.pi * 30000.0 * n / in_rate))
    source = np.rint(source).astype('>i2')

    with tempfile.TemporaryDirectory(prefix='wf-bandlimit-test-') as temporary:
        tmp = Path(temporary)
        raw = tmp / 'input.pcm'
        out1 = tmp / 'output1.wav'
        out2 = tmp / 'output2.wav'
        raw.write_bytes(source.tobytes())
        BUILD.resample_pcm16be(raw, out1, in_rate=in_rate, out_rate=44100)
        BUILD.resample_pcm16be(raw, out2, in_rate=in_rate, out_rate=44100)
        assert hashlib.sha256(out1.read_bytes()).digest() == hashlib.sha256(out2.read_bytes()).digest(), \
            'resampling is not deterministic'
        samples = read_mono16(out1)

    assert len(samples) == 44100, ('wrong output frame count', len(samples))
    passband = tone_amplitude(samples, 44100, 10000.0)
    alias = tone_amplitude(samples, 44100, 14100.0)
    passband_db = 20.0 * math.log10(max(passband, 1e-12) / 7000.0)
    alias_db = 20.0 * math.log10(max(alias, 1e-12) / 7000.0)
    assert -0.25 <= passband_db <= 0.25, ('10 kHz passband changed', passband_db)
    assert alias_db <= -80.0, ('30 kHz alias was not suppressed', alias_db)
    print('PASS band-limited resampler: passband=%.3f dB alias=%.1f dB' % (passband_db, alias_db))


if __name__ == '__main__':
    main()
