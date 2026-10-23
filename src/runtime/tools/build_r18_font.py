#!/usr/bin/env python3
"""Original WP25 text chip -> all4096 native Saturn8x8 4bpp glyphs."""
import argparse,hashlib
from pathlib import Path
from wf_r18_rom_graphics import Rom,pack4
p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
r=Rom(a.rom);data=b''.join(pack4(r.text_tile(code),8,8) for code in range(4096))
assert len(data)==131072
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
print('FONT8.BIN',len(data),hashlib.sha256(data).hexdigest())
