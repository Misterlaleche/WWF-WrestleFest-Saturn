#!/usr/bin/env python3
"""Read actual actors/input from exact-build Mednafen WRAM; never writes it.

Usage: PID A.BIN LINKER_MAP ACTORS_LAYOUT_BIN
Compile runtime18-actors-layout.c with the executable's SH-2 compiler and
extract its .rodata. Capture a stopped test frame for a consistent snapshot.
"""
import contextlib
import io
import json
from pathlib import Path
import re
import struct
import sys

layout = struct.unpack('>5I', Path(sys.argv[4]).read_bytes())
assert layout[1] == 0x10c
sys.argv = sys.argv[:4]
observer = Path(__file__).with_name('observe-mednafen-runtime18.py')
source = observer.read_text().replace(
    'candidates.append(values)', "values['_host_ram_base']=base; candidates.append(values)")
ns = {'__name__': '__main__'}
stdout = io.StringIO()
with contextlib.redirect_stdout(stdout):
    exec(compile(source, str(observer), 'exec'), ns)
state = json.loads(stdout.getvalue())
host = state.pop('_host_ram_base')
rt = ns['symbols']['rt']
with open(f'/proc/{sys.argv[1]}/mem', 'rb', buffering=0) as memory:
    def read(address, size):
        assert address % 2 == 0 and size % 2 == 0
        memory.seek(host + address - 0x06000000)
        raw = memory.read(size)
        result = bytearray(raw)
        result[0::2], result[1::2] = raw[1::2], raw[0::2]
        return bytes(result)

    state['rng'] = int.from_bytes(read(rt + layout[2], 4), 'big')
    state['D0_D7'] = list(struct.unpack('>8I', read(rt + layout[3], 32)))
    match = re.search(r'\.bss\.gameplay\s+0x([0-9a-f]+)\s', ns['mapping'])
    assert match
    state['input_hw_low'] = list(read(int(match[1], 16) + layout[4], 10)[:9])
    state['actors'] = []
    for slot in (0, 1, 4, 5):
        raw = read(rt + layout[0] + slot * layout[1], layout[1])
        word = lambda at: int.from_bytes(raw[at:at + 2], 'big')
        long = lambda at: int.from_bytes(raw[at:at + 4], 'big')
        state['actors'].append({
            'slot': slot, 'character': word(2), 'role': hex(word(0x56)),
            'state': hex(word(0x20)), 'selector': hex(word(0x60)),
            'position': [word(6), word(10)],
            'live_stamina': word(0x66), 'pending_damage': word(0x68),
            'HUD_queue': int.from_bytes(raw[0x6a:0x6c], 'big', signed=True),
            'HUD_accumulator': int.from_bytes(raw[0x6c:0x6e], 'big', signed=True),
            'HUD_stamina': word(0x6e), 'maximum_stamina': word(0x72),
            'tier': word(0x70), 'target': hex(long(0x7a)),
            'team_link': hex(long(0x86)), 'distance': [word(0xb0), word(0xb2)],
            'distance_class': raw[0xb4], 'flags_B5_B7': list(raw[0xb5:0xb8])
        })
print(json.dumps(state, indent=2))
