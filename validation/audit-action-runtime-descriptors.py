#!/usr/bin/env python3
"""Compare every literal action descriptor in the port with arcade ROM data."""

import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM_PATH = ROOT / 'src/runtime/audit/runtime18-cd/ROM68K.BIN'
SOURCE_PATH = ROOT / 'src/runtime/src/wf_r17_actions_r11.c'
GENERATED_AUDIT = ROOT / 'validation/local-20261026/action-animation-table-audit.json'
OUTPUT = ROOT / 'validation/local-20261026/action-runtime-descriptor-audit.json'

rom = ROM_PATH.read_bytes()
source = SOURCE_PATH.read_text()
generated = json.loads(GENERATED_AUDIT.read_text())
sha1 = hashlib.sha1(rom).hexdigest()
assert sha1 == generated['rom_sha1'] == 'a726c46f350723ea73158bc78055d17d418b14e2'

record = re.compile(
    r'static\s+const\s+desc_t\s+D_([0-9A-Fa-f]+)(?:_[A-Z0-9]+)?\s*=\s*'
    r'\{\s*([^,{}]+),\s*([^,{}]+),\s*([^,{}]+),\s*'
    r'\{([^{}]*)\},\s*\{([^{}]*)\}\s*\};', re.S)


def number(token: str) -> int:
    return int(token.strip().rstrip('uU'), 0)


def word(address: int) -> int:
    assert 0 <= address <= len(rom) - 2, hex(address)
    return int.from_bytes(rom[address:address + 2], 'big')


descriptors = []
for match in record.finditer(source):
    address, kind, count = map(number, match.group(2, 3, 4))
    durations = [number(value) for value in match.group(5).split(',') if value.strip()]
    poses = [number(value) for value in match.group(6).split(',') if value.strip()]
    assert address == int(match.group(1), 16), match.group(1)
    assert kind == word(address + 4), (hex(address), 'kind')
    assert 0 <= count <= 9 and count <= len(durations) and count <= len(poses)
    if kind == 0:
        assert count == 0, hex(address)
    else:
        assert count == word(address + 6), (hex(address), 'frame count')
        for frame in range(count):
            assert durations[frame] == word(address + 8 + 2 * frame), (
                hex(address), frame, 'duration')
            assert poses[frame] == word(address + 8 + 2 * count + 2 * frame), (
                hex(address), frame, 'pose')
    descriptors.append({'rom': f'{address:06X}', 'kind': kind, 'frames': count,
                        'poses_raw': [f'{value:04X}' for value in poses[:count]],
                        'durations_raw': [f'{value:04X}' for value in durations[:count]]})

declarations = re.findall(r'static\s+const\s+desc_t\s+D_', source)
assert len(declarations) == len(descriptors) == 169
addresses = [int(item['rom'], 16) for item in descriptors]
assert len(set(addresses)) == len(addresses)
generated_source = (ROOT / 'src/runtime/generated/wf_animation_tables.c').read_text()
generated_body = generated_source.split('wf_anim_descs[] = {', 1)[1].split('};', 1)[0]
generated_addresses = {int(value, 16) for value in re.findall(
    r'^\{(0x[0-9a-fA-F]+)u,', generated_body, re.M)}
extra = [item for item in descriptors if int(item['rom'], 16) not in generated_addresses]
assert len(extra) == 35

# Conservative source call graph: a helper's descriptor may be chosen by any
# raw action that calls that helper. These are candidates, not proven branches.
function = re.compile(
    r'(?:static\s+)?(?:const\s+)?'
    r'(?:wf_status_t|desc_t|void|int|unsigned|uint8_t|uint16_t|uint32_t|wf_actor_raw_t)'
    r'\s*\*?\s*(\w+)\s*\([^;{}]*\)\s*\{', re.S)
functions = {}
for match in function.finditer(source):
    cursor = match.end()
    depth = 1
    while depth and cursor < len(source):
        if source[cursor] == '{':
            depth += 1
        elif source[cursor] == '}':
            depth -= 1
        cursor += 1
    assert depth == 0, match.group(1)
    functions[match.group(1)] = source[match.end():cursor - 1]
for item in extra:
    descriptor_name = 'D_' + item['rom'].lstrip('0')
    owners = {function_name for function_name, body in functions.items()
              if re.search(r'\b' + re.escape(descriptor_name) + r'\b', body)}
    assert owners, item['rom']
    reachable = set(owners)
    while True:
        callers = {name for name, body in functions.items() if name not in reachable
                   and any(re.search(r'\b' + re.escape(callee) + r'\s*\(', body)
                           for callee in reachable)}
        if not callers:
            break
        reachable.update(callers)
    selectors = sorted(name[3:].upper() for name in reachable
                       if re.fullmatch(r'raw[0-9a-f]{2}', name))
    assert selectors, item['rom']
    item['possible_action_selectors'] = selectors

report = {
    'status': 'PASS',
    'rom_sha1': sha1,
    'source_sha256': hashlib.sha256(SOURCE_PATH.read_bytes()).hexdigest(),
    'literal_action_descriptors_checked': len(descriptors),
    'additional_to_generated_table': len(extra),
    'additional_descriptors': extra,
    'limits': [
        'Only statically declared desc_t literals are compared.',
        'This does not establish that every callback path selects the right descriptor.',
        'Possible action selectors come from a conservative source call graph.',
        'ROM pose data validity and metasprite execution have separate audits.',
    ],
}
OUTPUT.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('status',
      'literal_action_descriptors_checked', 'additional_to_generated_table')}))
