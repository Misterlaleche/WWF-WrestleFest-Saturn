#!/usr/bin/env python3
"""Read an X11 Mednafen window as raw bgr0 video; preserve elapsed time.

Run alongside Mednafen with DISPLAY set; pipe stdout to FFmpeg rawvideo.
Only xwd reads the window. Missed sample deadlines hold the previous image
instead of speeding up the recording. The sidecar records actual samples.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('window', type=int)
p.add_argument('--seconds', type=float, default=12)
p.add_argument('--fps', type=int, default=30)
p.add_argument('--metadata', type=Path, required=True)
p.add_argument('--stop-file', type=Path, help='Finish when this file appears; no emulator interaction')
a = p.parse_args()
assert a.seconds > 0 and 1 <= a.fps <= 60


def capture():
    start = time.monotonic()
    data = subprocess.check_output(['xwd', '-silent', '-id', str(a.window)])
    end = time.monotonic()
    h = struct.unpack('>25I', data[:100])
    assert h[1] == 7 and h[2] == 2 and h[7] == 0 and h[11] == 32
    assert h[14:17] == (0xff0000, 0xff00, 0xff)
    assert h[12] == h[4] * 4
    offset = h[0] + h[19] * 12
    raw = data[offset:]
    assert len(raw) == h[12] * h[5]
    return raw, (start + end) / 2, end - start, (h[4], h[5])


raw, origin, cost, size = capture()
count = round(a.seconds * a.fps)
assert count >= 1
samples = [{'index': 0, 'time': 0.0, 'read_seconds': cost}]
sys.stdout.buffer.write(raw)
index = 0
held = 0
while index < count - 1:
    if a.stop_file and a.stop_file.exists():
        break
    deadline = origin + (index + 1) / a.fps
    remaining = deadline - time.monotonic()
    if remaining > 0:
        time.sleep(remaining)
    current, timestamp, cost, current_size = capture()
    assert current_size == size
    sampled_index = min(count - 1, max(index + 1, round((timestamp - origin) * a.fps)))
    for _ in range(sampled_index - index - 1):
        sys.stdout.buffer.write(raw)
        held += 1
    sys.stdout.buffer.write(current)
    samples.append({'index': sampled_index, 'time': timestamp - origin, 'read_seconds': cost})
    raw = current
    index = sampled_index
sys.stdout.buffer.flush()
a.metadata.write_text(json.dumps({
    'source': 'xwd read-only capture of actual Mednafen X11 window',
    'window': a.window, 'size': size, 'pixel_format': 'bgr0',
    'fps': a.fps, 'encoded_frames': index + 1, 'encoded_seconds': (index + 1) / a.fps,
    'maximum_seconds': a.seconds,
    'stop_file_triggered': bool(a.stop_file and a.stop_file.exists()),
    'actual_sample_span_seconds': samples[-1]['time'],
    'samples': samples, 'held_frames_for_missed_capture_deadlines': held,
    'scope': 'Screen recording, not a measurement of native game FPS; no emulator speed changes by this tool'
}, indent=2) + '\n')
