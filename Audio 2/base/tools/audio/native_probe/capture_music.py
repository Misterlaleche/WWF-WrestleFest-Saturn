"""Capture finite diagnostic prefixes, validate A53 and preserve explicit limits."""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import time


def validate_trace(trace_path, expected_events):
    counts = Counter()
    last = -1
    with trace_path.open(newline='') as stream:
        for row in csv.DictReader(stream):
            cycle = int(row['cycle'])
            if cycle < last:
                raise RuntimeError(f'Nonmonotonic trace: {trace_path.name}')
            last = cycle
            counts[row['kind']] += 1
    if sum(counts.values()) != expected_events:
        raise RuntimeError(f'Trace event count mismatch: {trace_path.name}')
    return counts


def capture(probe, rom, prepared, out, command, fade=None, trace=True):
    tag = f'cmd{command:02x}' + ('-fade' if fade is not None else '')
    argv = [str(probe.resolve()), str(rom.resolve()),
            str((prepared / 'AUDPCM.PAK').resolve()),
            str((prepared / 'bank0.csv').resolve()), f'{command:02x}', '529200',
            str((out / (tag + '.pcm')).resolve()),
            str((out / (tag + '.csv')).resolve()) if trace else '-',
            str(fade) if fade is not None else '-1', '1' if trace else '0']
    start = time.perf_counter()
    process = subprocess.run(argv, capture_output=True, text=True)
    if process.returncode:
        raise RuntimeError(f'{tag}: {process.returncode}: {process.stdout} {process.stderr}')
    metadata = json.loads(process.stdout)
    if not metadata['ok'] or metadata['vm_error'] or metadata['samples'] != 529200:
        raise RuntimeError(f'Incomplete capture: {metadata}')
    metadata['host_wall_seconds'] = time.perf_counter() - start
    metadata['pcm_sha256'] = hashlib.sha256((out / (tag + '.pcm')).read_bytes()).hexdigest()
    if trace:
        counts = validate_trace(out / (tag + '.csv'), metadata['trace_events'])
        metadata['event_counts'] = dict(counts)
        metadata['trace_sha256'] = hashlib.sha256((out / (tag + '.csv')).read_bytes()).hexdigest()
    return tag, metadata


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--prepared', type=Path, required=True)
    parser.add_argument('--golden', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    with args.golden.open() as stream:
        golden = {int(r['command'], 16): r['expected_a53_sha256'] for r in csv.DictReader(stream)}
    results = {}
    with ThreadPoolExecutor(max_workers=2) as pool:
        tasks = [pool.submit(capture, args.probe, args.rom, args.prepared, args.out, c)
                 for c in range(1, 19)]
        for task in as_completed(tasks):
            tag, result = task.result()
            result['a53_match'] = result['pcm_sha256'] == golden[result['command']]
            results[tag] = result
            print(tag, 'A53 PASS' if result['a53_match'] else 'A53 FAIL', flush=True)
    tag, result = capture(args.probe, args.rom, args.prepared, args.out, 1, fade=132300)
    result['fade_injection_sample'] = 132300
    results[tag] = result
    summary = {'clock_hz': 3579545, 'samples_per_capture': 529200,
               'duration_seconds': 12, 'cycle_resolution': 'Z80 instruction, not bus T-state',
               'scope': '18 music prefixes and one 0x1f injection at 3 s; not full lifecycle/coverage',
               'captures': dict(sorted(results.items()))}
    (args.out / 'capture-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    if not all(r['a53_match'] for r in results.values() if 'a53_match' in r):
        raise SystemExit('A53 regression')
