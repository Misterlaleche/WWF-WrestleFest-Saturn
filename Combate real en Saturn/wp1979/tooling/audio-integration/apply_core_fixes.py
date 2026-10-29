#!/usr/bin/env python3
"""Apply three exact, generic changes to a fresh libretro/yabause source tree.

Argument: directory containing yabause/src. Preserves source line endings and
refuses an unexpected source. It does not alter emulated CPU clocks or the game.
"""
from pathlib import Path
import argparse

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('source_tree', type=Path)
a = p.parse_args()
changes = [
    ('yabause/src/libretro/libretro.c',
     b'yinit.use_new_scsp         = 0;',
     b'yinit.use_new_scsp         = 1;'),
    ('yabause/src/scspdsp.c',
     b'else if (instruction.part.shift == 2)\n      shift_temp = dsp->acc & 0xffffff;',
     b'else if (instruction.part.shift == 3)\n      shift_temp = dsp->acc & 0xffffff;'),
    ('yabause/src/sh2int.c',
     b'static u32 FASTCALL FetchVram(u32 addr)',
     b'static u32 FASTCALL FetchVram(UNUSED SH2_struct *sh, u32 addr)'),
]
staged = []
for relative, old, new in changes:
    path = a.source_tree / relative
    data = path.read_bytes()
    if b'\r\n' in data and b'\n' in old:
        old, new = old.replace(b'\n', b'\r\n'), new.replace(b'\n', b'\r\n')
    if data.count(old) != 1:
        raise SystemExit(f'Refusing unexpected source: {relative}')
    staged.append((path, data.replace(old, new)))
for path, data in staged:
    path.write_bytes(data)
    print(path)
