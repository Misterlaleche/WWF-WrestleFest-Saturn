#!/usr/bin/env python3
"""Check that the recorded combat interval has audio in both delivery streams."""
import argparse
import json
import math
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--start', type=float, default=45.5)
    parser.add_argument('--duration', type=float, default=120)
    args = parser.parse_args()
    if args.start < 0 or args.duration <= 0:
        parser.error('The selected combat interval must have positive duration')
    result = {'scope': 'Signal non-silence in the selected combat interval; not a listening-quality judgement',
              'start_seconds': args.start, 'duration_seconds': args.duration, 'files': []}
    for filename in ('native.mov', 'native.mp4'):
        command = ['ffmpeg', '-hide_banner', '-loglevel', 'info', '-nostdin',
                   '-ss', str(args.start), '-i', str(args.capture / filename),
                   '-t', str(args.duration), '-map', '0:a:0',
                   '-af', 'astats=metadata=0:reset=0', '-f', 'null', '-']
        completed = subprocess.run(command, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, text=True, check=True)
        (args.capture / (filename + '-combat-audio.log')).write_text(completed.stderr)
        overall = completed.stderr.rsplit('] Overall', 1)[-1]
        def value(name):
            match = re.search(r'\] ' + re.escape(name) + r':\s*([^\r\n]+)', overall)
            if not match:
                raise ValueError('Missing audio statistic: ' + name)
            return float(match[1])
        peak, rms, samples = value('Peak level dB'), value('RMS level dB'), value('Number of samples')
        good = math.isfinite(peak) and math.isfinite(rms) and samples > 0
        result['files'].append(dict(path=str((args.capture / filename).resolve()),
                                    status='PASS' if good else 'FAIL', command=command,
                                    peak_dbfs=peak if math.isfinite(peak) else None,
                                    rms_dbfs=rms if math.isfinite(rms) else None,
                                    samples_per_channel=int(samples)))
    result['status'] = 'PASS' if all(item['status'] == 'PASS' for item in result['files']) else 'FAIL'
    (args.capture / 'audio-combat-validation.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    if result['status'] != 'PASS':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
