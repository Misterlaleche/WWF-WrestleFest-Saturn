#!/usr/bin/env python3
"""Analyze observer CSV with exact integer comparison to Sega's 300 us limit."""
import argparse
import csv
import gzip
import json
from pathlib import Path

def analyze(path):
    groups = {}
    count = 0
    for row in csv.DictReader((gzip.open(path,'rt') if str(path).endswith('.gz') else Path(path).open())):
        if row['kind'] != 'C':
            continue
        count += 1
        command = int(row['value'])
        pc = int(row['master_pc'], 16) & 0x1fffffff
        # PC is an observation of the master pipeline, not a disassembled caller.
        origin = 'high_ram' if 0x06000000 <= pc < 0x06100000 else 'other'
        key = f'{origin}:0x{command:02x}'
        g = groups.setdefault(key, {'commands': 0, 'unmeasured': 0, 'under_300us': 0,
                                    'min_us': None, 'max_us': None, 'first_violation': None})
        g['commands'] += 1
        delta, hz = int(row['delta_master']), int(row['master_hz'])
        if int(row['last_vb_master']) < 0 or delta < 0 or hz <= 0:
            g['unmeasured'] += 1
            continue
        us = delta * 1000000 / hz
        g['min_us'] = us if g['min_us'] is None else min(g['min_us'], us)
        g['max_us'] = us if g['max_us'] is None else max(g['max_us'], us)
        # Classification is exact; floats above are presentation only.
        if delta * 1000000 < 300 * hz:
            g['under_300us'] += 1
            if g['first_violation'] is None:
                g['first_violation'] = row
    return {'commands': count, 'groups': groups, 'clock': 'emulated VDP2/SMPC master time',
            'hardware_verified': False}

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('csv', type=Path)
    a = p.parse_args()
    print(json.dumps(analyze(a.csv), indent=2))
