#!/usr/bin/env python3
"""Compare generated animation descriptors and dispatch tables to ROM bytes."""

import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM_PATH = ROOT / 'src/runtime/audit/runtime18-cd/ROM68K.BIN'
TABLE_PATH = ROOT / 'src/runtime/generated/wf_animation_tables.c'
REPORT_PATH = ROOT / 'validation/local-20261026/action-animation-table-audit.json'
EXPECTED_ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'


def array_body(source: str, name: str) -> str:
    match = re.search(r'\b' + re.escape(name) + r'\[[^\]]*\]\s*=\s*\{(.*?)\};',
                      source, re.S)
    assert match, name
    return match.group(1)


def numbers(text: str) -> list[int]:
    return [int(value.rstrip('uU'), 0)
            for value in re.findall(r'0x[0-9a-fA-F]+[uU]?|\b\d+[uU]?', text)]


def records(body: str, width: int) -> list[tuple[int, ...]]:
    result = []
    for match in re.finditer(r'\{([^{}]*)\}', body):
        row = tuple(numbers(match.group(1)))
        assert len(row) == width, row
        result.append(row)
    return result


def word(rom: bytes, address: int) -> int:
    assert 0 <= address <= len(rom) - 2, hex(address)
    return int.from_bytes(rom[address:address + 2], 'big')


def long(rom: bytes, address: int) -> int:
    assert 0 <= address <= len(rom) - 4, hex(address)
    return int.from_bytes(rom[address:address + 4], 'big')


rom = ROM_PATH.read_bytes()
source = TABLE_PATH.read_text()
assert hashlib.sha1(rom).hexdigest() == EXPECTED_ROM_SHA1
entries = records(array_body(source, 'wf_anim_entries'), 2)
descs = records(array_body(source, 'wf_anim_descs'), 6)
action = numbers(array_body(source, 'wf_anim_action_dispatch'))
state4 = numbers(array_body(source, 'wf_anim_state4_dispatch'))
main = numbers(array_body(source, 'wf_anim_main_dispatch'))
assert len(entries) == 519 and len(descs) == 180
assert len(action) == 143 and len(state4) == 43 and len(main) == 13
assert all(0 <= index < len(descs) for index in action + state4 + main)

short_records = []
callback_owned = 0
for index, (address, callback, kind, count, start, stored) in enumerate(descs):
    assert address < len(rom) - 8, (index, hex(address))
    assert callback == long(rom, address), (index, 'callback', hex(callback),
                                           hex(long(rom, address)))
    assert kind == word(rom, address + 4), (index, 'kind')
    if kind != 0:
        assert count == word(rom, address + 6), (index, 'count')
    else:
        # Type 0 transfers control to its callback. The next ROM word is
        # callback code, not an animation frame count.
        assert count == stored == 0, (index, 'callback-owned entry count')
        callback_owned += 1
    assert start + stored <= len(entries), (index, 'entry bounds')
    if stored != count:
        short_records.append({'index': index, 'rom': f'{address:06X}',
                              'count': count, 'stored': stored})
    for frame in range(stored):
        expected = (word(rom, address + 8 + 2 * count + 2 * frame),
                    word(rom, address + 8 + 2 * frame))
        assert entries[start + frame] == expected, (
            index, frame, 'pose/duration', entries[start + frame], expected)

for table_name, table, address in (('main', main, 0x11478),
                                    ('action', action, 0x12614),
                                    ('state4', state4, 0x1afd4)):
    for selector, desc_index in enumerate(table):
        rom_pointer = long(rom, address + selector * 4)
        assert descs[desc_index][0] == rom_pointer, (
            table_name, selector, desc_index, hex(descs[desc_index][0]),
            hex(rom_pointer))

action_selectors = []
for selector, desc_index in enumerate(action):
    address, callback, kind, count, start, stored = descs[desc_index]
    action_selectors.append({
        'id': f'{selector:02X}',
        'descriptor_rom': f'{address:06X}',
        'callback_rom': f'{callback:06X}',
        'kind': kind,
        'frame_count': count,
        'poses_raw': [f'{entries[start + frame][0]:04X}'
                      for frame in range(stored)],
        'durations_raw': [f'{entries[start + frame][1]:04X}'
                          for frame in range(stored)],
    })

report = {
    'status': 'PASS',
    'rom_sha1': EXPECTED_ROM_SHA1,
    'table_sha256': hashlib.sha256(TABLE_PATH.read_bytes()).hexdigest(),
    'descriptor_count': len(descs),
    'entry_count': len(entries),
    'action_dispatch_count': len(action),
    'state4_dispatch_count': len(state4),
    'main_dispatch_count': len(main),
    'callback_owned_descriptor_count': callback_owned,
    'callback_owned_action_selectors': [item['id'] for item in action_selectors
                                        if item['kind'] == 0],
    'action_selectors': action_selectors,
    'short_records': short_records,
    'compared': ['ROM callback/kind for every generated descriptor, count for frame-based descriptors',
                 'ROM duration and pose for every stored animation entry',
                 'all 13 main, 143 action, and 43 state-4 dispatch pointers'],
    'limits': ['This audits data tables, not reachable callback branches or on-screen sprites.',
               'Callback-owned descriptors contain executable code after kind, not a frame count.'],
}
REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
REPORT_PATH.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
