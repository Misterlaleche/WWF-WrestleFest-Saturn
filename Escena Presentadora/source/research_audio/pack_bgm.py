#!/usr/bin/env python3
"""Lossless independent-block packing of original 44.1 kHz PCM16.

No gain, resampling, filtering, dither, or content repetition is performed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
import struct
import wave
from pathlib import Path
import numpy as np

BLOCK = 1024

class Bits:
    def __init__(self):
        self.output = bytearray()
        self.value = self.count = 0
    def put(self, value, count):
        self.value = (self.value << count) | int(value)
        self.count += count
        while self.count >= 8:
            self.count -= 8
            self.output.append((self.value >> self.count) & 255)
        self.value &= (1 << self.count) - 1
    def finish(self):
        if self.count:
            self.output.append((self.value << (8 - self.count)) & 255)
        return bytes(self.output)

def pack_block(x):
    n = len(x)
    choice = (16 * n, 0, 0, None)
    for order in (1, 2):
        if n < order:
            continue
        delta = np.diff(x.astype(np.int32), n=order)
        zig = np.where(delta < 0, -2 * delta - 1, 2 * delta).astype(np.uint32)
        for k in range(18):
            bits = int(np.sum(zig >> k)) + len(zig) * (k + 1)
            if bits < choice[0]:
                choice = bits, order, k, zig
    _, mode, k, zig = choice
    if mode == 0:
        body = x.astype('>i2').tobytes()
    else:
        b = Bits()
        for u in zig:
            u = int(u)
            b.put(1, (u >> k) + 1)
            b.put(u & ((1 << k) - 1), k)
        body = b.finish()
    assert len(body) <= n * 2
    first = int(x[0])
    second = int(x[1]) if n > 1 else first
    return struct.pack('>HHBBhhH', n, len(body), mode, k, first, second, 0) + body, mode, k

def pack_samples(x):
    body = bytearray()
    stats = {}
    for off in range(0, len(x), BLOCK):
        encoded, mode, k = pack_block(x[off:off + BLOCK])
        body.extend(encoded)
        key = f'order{mode}_k{k}'
        stats[key] = stats.get(key, 0) + 1
    header = struct.pack('>8s6I', b'WP02BGM1', 44100, len(x), BLOCK,
                         (len(x) + BLOCK - 1) // BLOCK, len(body), 0)
    return header + body, stats

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--pcm-be', action='store_true')
    p.add_argument('--start', type=float, default=0.0)
    p.add_argument('--seconds', type=float)
    a = p.parse_args()
    raw_source = a.source.read_bytes()
    if a.pcm_be:
        x = np.frombuffer(raw_source, dtype='>i2').astype(np.int16)
    else:
        with wave.open(str(a.source)) as w:
            assert w.getframerate() == 44100 and w.getsampwidth() == 2
            channels = w.getnchannels()
            x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').reshape(-1, channels)
            assert np.array_equal(x, np.repeat(x[:, :1], channels, axis=1)), 'source channels differ'
            x = x[:, 0].copy()
    first = math.ceil(a.start * 44100)
    count = round(a.seconds * 44100) if a.seconds is not None else len(x) - first
    assert first >= 0 and count > 0 and first + count <= len(x)
    x = x[first:first + count]
    packed, stats = pack_samples(x)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(packed)
    decoded = a.output.with_suffix('.decoded.pcm')
    decoded.write_bytes(x.astype('>i2').tobytes())
    meta = {
        'source': str(a.source), 'source_sha256': hashlib.sha256(raw_source).hexdigest(),
        'source_start_requested_seconds': a.start, 'source_first_sample': first,
        'source_start_actual_seconds': first / 44100, 'samples': count,
        'rate_hz': 44100, 'channels': 1, 'bits': 16,
        'seconds': count / 44100, 'packed_bytes': len(packed),
        'packed_sha256': hashlib.sha256(packed).hexdigest(),
        'decoded_sha256': hashlib.sha256(decoded.read_bytes()).hexdigest(),
        'blocks': stats, 'transforms': 'lossless predictor/Rice packing only; no audio processing',
    }
    a.output.with_suffix('.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(json.dumps({k: v for k, v in meta.items() if k != 'blocks'}, indent=2))

if __name__ == '__main__':
    main()
