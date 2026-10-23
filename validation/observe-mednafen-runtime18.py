#!/usr/bin/env python3
"""Read-only Linux observer of this SH-2 executable inside Mednafen 1.32.1.

Run next to the emulator (for example inside its Docker container). The BIN
payload and linker map must be the exact build currently running. No process
memory is written; matching instruction bytes identify native uint16 WRAM.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('pid', type=int)
p.add_argument('binary', type=Path)
p.add_argument('linker_map', type=Path)
p.add_argument('--layout', type=Path, help='SH-2 .rodata from runtime18-observer-layout.c')
p.add_argument('--graphics-layout', type=Path, help='SH-2 .rodata from runtime18-graphics-layout.c')
p.add_argument('--profile-stages',type=int,default=16)
p.add_argument('--profile', action='store_true', help='Read test-only profile_entry.c counters')
a = p.parse_args()
program = a.binary.read_bytes()
mapping = a.linker_map.read_text()
signature = bytearray(program[:4096])
signature[0::2], signature[1::2] = program[1:4096:2], program[:4096:2]
names = ('rt', 'video_clock', 'fatal_owner', 'fatal_dependency',
         'fatal_frame', 'finished_frame', 'finished_result', 'finished_count', 'finished_timer')
symbols = {}
# Older checkpoint executables do not yet present the in-ring result text.
for optional in ('finished_presentation','finished_vblanks','finished_presentation_vblanks'):
    if re.search(r'\.bss\.'+optional+r'(?:\.lto_priv\.\d+)?\s+0x', mapping):
        names += (optional,)
if a.layout:
    names += ('gameplay',)
if a.graphics_layout:
    names += ('backend', 'display', 'video', 'wf_saturn_cram_writes')
if a.profile:
    names += ('profile_frames', 'profile_vblanks', 'profile_totals', 'profile_calls')
for name in names:
    match = re.search(r'\.bss\.' + name + r'(?:\.lto_priv\.\d+)?\s+0x([0-9a-f]+)\s', mapping)
    if not match:
        raise SystemExit('Missing exact-build symbol: ' + name)
    symbols[name] = int(match[1], 16)
candidates = []
with open(f'/proc/{a.pid}/mem', 'rb', buffering=0) as mem:
    for line in Path(f'/proc/{a.pid}/maps').read_text().splitlines():
        fields = line.split()
        if not fields[1].startswith('rw') or len(fields) > 5 and fields[5] not in ('[heap]',):
            continue
        start, end = (int(x, 16) for x in fields[0].split('-'))
        if end - start > 128 * 1024 * 1024:
            continue
        mem.seek(start)
        try:
            data = mem.read(end - start)
        except OSError:
            continue
        at = data.find(signature)
        while at >= 0:
            base = start + at - 0x4000
            def read32(addr):
                mem.seek(base + addr - 0x06000000)
                raw = mem.read(4)
                return int.from_bytes(raw[1::-1] + raw[3:1:-1], 'big')
            values = {name: read32(addr) for name, addr in symbols.items() if name not in ('rt', 'gameplay', 'backend', 'display', 'video')}
            # g is the first field; frame_counter is at +24 on SH-2.
            values['frame'] = read32(symbols['rt'] + 24)
            if a.profile:
                for name in ('profile_totals', 'profile_calls'):
                    values[name] = [read32(symbols[name] + i * 4) for i in range(a.profile_stages)]
            if a.layout:
                import struct
                offsets = struct.unpack('>10I', a.layout.read_bytes())
                assert offsets[0] == 24, 'Unexpected SH-2 frame ABI'
                def read16(addr):
                    mem.seek(base + addr - 0x06000000)
                    return int.from_bytes(mem.read(2), 'little')
                values['camera_ram'] = [read16(symbols['rt'] + x) for x in offsets[1:5]]
                values['camera_adapter'] = [read16(symbols['gameplay'] + x) for x in offsets[5:9]]
                mem.seek(base + ((symbols['rt'] + offsets[9]) ^ 1) - 0x06000000)
                values['frame_ready'] = mem.read(1)[0]
            if a.graphics_layout:
                import struct
                offsets = struct.unpack('>12I', a.graphics_layout.read_bytes())
                def read16g(addr):
                    mem.seek(base + addr - 0x06000000)
                    return int.from_bytes(mem.read(2), 'little')
                values['texture_bytes'] = read32(symbols['backend'] + offsets[0])
                values['texture_DMA_runs'] = read32(symbols['backend'] + offsets[1])
                values['texture_tiles_reused'] = read32(symbols['backend'] + offsets[2])
                values['display_scroll_registers'] = [read16g(symbols['display'] + offsets[3] + i*2) for i in range(4)]
                values['shake_countdowns'] = [read16g(symbols['rt'] + o) for o in offsets[4:6]]
                values['camera_RAM'] = [read16g(symbols['rt'] + o) for o in offsets[6:10]]
            if values['video_clock'] and values['frame'] <= values['video_clock']:
                candidates.append(values)
            at = data.find(signature, at + 2)
if len(candidates) != 1:
    raise SystemExit(f'Expected one matching live WRAM image, found {len(candidates)}')
print(json.dumps({'observer': 'read-only /proc memory, 4096 instruction bytes matched',
                  'binary_sha256': hashlib.sha256(program).hexdigest(),
                  **candidates[0]}, indent=2))
