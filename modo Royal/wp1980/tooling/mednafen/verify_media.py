#!/usr/bin/env python3
"""Verify native Mednafen MOV/MP4 video PTS and derive rate from recorded time."""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import subprocess


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1048576), b''):
            digest.update(block)
    return digest.hexdigest()


def inspect(path):
    command = ['ffprobe', '-v', 'error', '-select_streams', 'v:0',
               '-show_entries', 'stream=time_base,width,height,nb_frames:packet=pts,duration',
               '-show_streams', '-show_packets', '-of', 'json', str(path)]
    result = json.loads(subprocess.check_output(command, text=True))
    stream = result['streams'][0]
    timescale = Fraction(stream['time_base'])
    packets = sorted(result['packets'], key=lambda row: int(row['pts']))
    times = [int(packet['pts']) * timescale for packet in packets]
    if len(times) < 2 or any(b <= a for a, b in zip(times, times[1:])):
        raise ValueError('Video PTS must be strictly increasing')
    return stream, packets, times


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    folder = args.capture.resolve()
    source, delivery = folder / 'native.mov', folder / 'native.mp4'
    a, ap, at = inspect(source)
    b, bp, bt = inspect(delivery)
    equal = at == bt
    mean_field_rate = Fraction(len(at) - 1, 1) / (at[-1] - at[0])
    report = dict(status='PASS' if equal else 'FAIL', scope='Recorded video timestamps; not gameplay update rate',
                  source_sha256=sha(source), mp4_sha256=sha(delivery),
                  source_frames=len(at), mp4_frames=len(bt), all_video_pts_identical=equal,
                  source_time_base=a['time_base'], mp4_time_base=b['time_base'],
                  source_first_pts=int(ap[0]['pts']), source_last_pts=int(ap[-1]['pts']),
                  mean_recorded_field_rate=float(mean_field_rate),
                  mean_recorded_field_rate_fraction=str(mean_field_rate),
                  source_geometry=[a['width'], a['height']], mp4_geometry=[b['width'], b['height']],
                  final_source_packet_duration=ap[-1].get('duration'),
                  final_mp4_packet_duration=bp[-1].get('duration'),
                  duration_note='Last-packet duration metadata may differ; each actual frame PTS is compared exactly.')
    (folder / 'video-pts-check.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    if not equal:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
