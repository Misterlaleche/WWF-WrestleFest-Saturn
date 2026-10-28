#!/usr/bin/env python3
"""Check the ROM's normal human and CPU choice tables for $2D/$2E/$2F."""

import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / 'src/runtime/audit/runtime18-cd/ROM68K.BIN'
MATRIX = ROOT / 'src/runtime/generated/wf_action_matrix.c'
parser = argparse.ArgumentParser()
parser.add_argument('--selector', choices=('2d', '2e', '2f'), required=True)
args = parser.parse_args()
target = int(args.selector, 16)
OUTPUT = ROOT / f'validation/local-20261026/action{args.selector}-selection-sources.json'
rom = ROM.read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
rows = [tuple(int(value, 16) for value in triple)
        for triple in re.findall(r'\{(0x[0-9A-F]+),(0x[0-9A-F]+),(0x[0-9A-F]+)\}',
                                 MATRIX.read_text())]
assert len(rows) == 12 * 21

human_hits = []
for character in range(12):
    offset = 0xe52e + 64 * character
    generated = bytes(value for row in rows[character*21:(character+1)*21] for value in row)
    assert rom[offset:offset+63] == generated
    assert rom[offset+63] == 0xff
    for category, triple in enumerate(rows[character*21:(character+1)*21]):
        for button, selector in enumerate(triple):
            if selector == target:
                human_hits.append({'character': character, 'category': category,
                                   'button': button + 1, 'rom_address': f'{offset+category*3+button:06X}'})
expected_characters = ({0, 5, 6, 10, 11} if target == 0x2d
                       else {1, 5, 9} if target == 0x2e else {1, 5})
assert {item['character'] for item in human_hits} == expected_characters
assert len(human_hits) == (15 if target == 0x2d else 6 if target == 0x2e else 5)


def pointer(address):
    assert 0 <= address <= len(rom)-4
    result = int.from_bytes(rom[address:address+4], 'big')
    assert 0 <= result < len(rom)
    return result


# The port's $1DEA6 chooser uses lanes 0..8 at $219B4 and lanes 0..2
# at $2307C. Each lane begins with a count followed by selector bytes.
cpu_hits = []
cpu_tables = []
for base, lanes in ((0x219b4, 9), (0x2307c, 3)):
    for character in range(12):
        directory = pointer(base + 4*character)
        for lane in range(lanes):
            address = pointer(directory + 4*lane)
            count = rom[address]
            assert count <= 6 and address + 1 + count <= len(rom)
            choices = rom[address+1:address+1+count]
            cpu_tables.append({'base': f'{base:06X}', 'character': character,
                               'lane': lane, 'rom_address': f'{address:06X}', 'choices': choices.hex()})
            if target in choices:
                cpu_hits.append({'base': f'{base:06X}', 'character': character,
                                 'lane': lane, 'rom_address': f'{address:06X}'})
assert len(cpu_tables) == 144
if target in (0x2d, 0x2e):
    assert len(cpu_hits) == (14 if target == 0x2d else 11)
    assert {item['character'] for item in cpu_hits} == expected_characters
else:
    assert not cpu_hits

report = {
    'status': 'PASS',
    'selector': args.selector,
    'rom_sha1': hashlib.sha1(rom).hexdigest(),
    'generated_matrix_sha256': hashlib.sha256(MATRIX.read_bytes()).hexdigest(),
    'human_table_base': '00E52E',
    'human_table_character_stride': 64,
    'human_table_rows_checked': len(rows),
    'human_selector_hits': human_hits,
    'cpu_tables_checked': len(cpu_tables),
    'cpu_selector_hits': cpu_hits,
    'cpu_tables': cpu_tables,
    'limits': [
        'This checks the normal human action matrix and two ROM CPU choice-table families used by $1DEA6.',
        'Indirect selector assignments outside these tables, scripts, and corrupted states are not excluded.',
        'Non-matrix wrestler substitutions in the action continuation are controlled tests, not natural captures.',
    ],
}
OUTPUT.write_text(json.dumps(report, indent=2) + '\n')
print('PASS', args.selector.upper(), '252 human matrix rows and 144 CPU choice tables;',
      len(human_hits), 'human and', len(cpu_hits), 'CPU entries')
