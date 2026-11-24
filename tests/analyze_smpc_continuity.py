#!/usr/bin/env python3
"""Audit cold-boot, read-only observer CSV; preserve bootstrap gaps explicitly.

The caller supplies the reviewed runtime boundary. The analyzer never moves it
past a missing request. Source/ELF review is still needed to infer publication
ordering; this script proves COMREG timing and completed hardware transactions.
"""
import argparse
import collections
import csv
import gzip
import hashlib
import json
from pathlib import Path


def analyze(rows, capture, runtime_start):
    command = capture['symbols']['jo_smpc_command']
    handoff = next(i for i, row in enumerate(rows)
                   if row['kind'] == 'C' and row['value'] == '3'
                   and command <= int(row['master_pc'], 16) < command + 256)
    requests = [row for row in rows[handoff:] if row['kind'] == 'C'
                and row['value'] == '16' and int(row['ireg1']) & 8]
    ins = {int(row['field']): row for row in rows if row['kind'] == 'B'}
    outs = {int(row['field']): row for row in rows if row['kind'] == 'O'}
    errors, after_in, before_out = [], [], []
    final = collections.Counter()
    for row in requests:
        field, hz = int(row['field']), int(row['master_hz'])
        delta = int(row['delta_master']) * 1e6 / hz
        after_in.append(delta)
        if delta < 300:
            errors.append({'kind': 'minimum', 'field': field, 'us': delta})
        if row['ireg0'] != '0':
            errors.append({'kind': 'not_peripheral_only', 'field': field})
        if field in outs:
            margin = (int(outs[field]['absolute_master']) - int(row['absolute_master'])) * 1e6 / hz
            before_out.append(margin)
            if margin < 100:
                errors.append({'kind': 'out_margin', 'field': field, 'us': margin})
        elif field < max(ins):
            errors.append({'kind': 'missing_out_observation', 'field': field})
        if field + 1 in ins:
            state = tuple(ins[field + 1][key] for key in ('sf', 'sr', 'executing'))
            final[state] += 1
            if state != ('0', '192', '-1'):
                errors.append({'kind': 'not_final_next_in', 'field': field, 'state': state})
    if not requests or not before_out:
        raise ValueError('No auditable peripheral requests')
    fields = [int(row['field']) for row in requests]
    gaps = collections.Counter(b - a for a, b in zip(fields, fields[1:]))
    # Anchor the end to the independent VDP2 observation, never the last query:
    # otherwise stopping requests permanently would shorten the audited scope.
    runtime_end = max(ins)
    if runtime_start < int(rows[handoff]['field']) or runtime_start > runtime_end:
        raise ValueError('Runtime boundary outside captured SDK fields')
    runtime = collections.Counter(field for field in fields if runtime_start <= field <= runtime_end)
    missing = [field for field in range(runtime_start, runtime_end + 1) if not runtime[field]]
    duplicates = [field for field, count in runtime.items() if count != 1]
    if missing or duplicates:
        errors.append({'kind': 'runtime_cadence', 'missing': missing, 'duplicates': duplicates})
    last = capture['trace'][-1]
    return {
        'pass': not errors,
        'pass_scope': 'Command deadlines, fixed runtime request cadence, and hardware final state when next IN is captured; pad/audio/null observations are reported separately.',
        'handoff_field': int(rows[handoff]['field']),
        'peripheral_requests': len(requests),
        'first_field': fields[0], 'last_field': fields[-1],
        'minimum_after_in_us': min(after_in),
        'minimum_before_out_us': min(before_out),
        'overall_gaps_in_fields': dict(sorted(gaps.items())),
        'overall_maximum_gap_fields': max(gaps, default=0),
        'final_packet_state_counts': {str(key): value for key, value in final.items()},
        'runtime': {'start_field': runtime_start, 'end_field': runtime_end,
                    'fields': runtime_end - runtime_start + 1,
                    'requests': sum(runtime.values()),
                    'missing_fields': missing, 'duplicate_fields': duplicates},
        'errors': errors,
        'pad_data_counts': dict(collections.Counter(x.get('pad', {}).get('data') for x in capture['trace'])),
        'last_ticks': last.get('ticks'), 'fault': last.get('smpc_timing_fault'),
        'null_access': last.get('null_access'),
        'audio_errors': {key: last.get('audio', {}).get(key) for key in
                         ('fault', 'pcm_underruns', 'music_underruns', 'music_errors', 'unsupported_fades')},
        'limits': 'Cold boot only. Last transaction may lack a following IN at capture end. '
                  'Published pad changes require the capture/normal-key evidence; hardware final state alone is insufficient. '
                  'Video publication ordering requires independent source/ELF review.',
    }


def read_json(path):
    with (gzip.open(path, 'rt') if path.suffix == '.gz' else path.open()) as handle:
        return json.load(handle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--runtime-start-field', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with (gzip.open(args.csv, 'rt') if args.csv.suffix == '.gz' else args.csv.open()) as handle:
        result = analyze(list(csv.DictReader(handle)), read_json(args.capture), args.runtime_start_field)
    result['input_sha256'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                              for path in (args.csv, args.capture)}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['pass'] else 1)


if __name__ == '__main__':
    main()
