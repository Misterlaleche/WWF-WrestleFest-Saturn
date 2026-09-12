#!/usr/bin/env python3
from pathlib import Path
import sys,re
root=Path(sys.argv[1])
source=list((root/'src').rglob('*.[ch]'))
text='\n'.join(p.read_text(errors='replace') for p in source)
assert not any(p.name=='main.c' for p in source), 'master main.c present'
for bad in ['rand(', 'srand(', 'wp28', 'collision_step(', 'cpu_ai', 'scsp_', 'cdfs_']:
    assert bad.lower() not in text.lower(), f'forbidden ownership token: {bad}'
# test harness main is permitted only under tests/ and is not scanned above.
assert 'WP12_OP_HANDOFF_F8C' in text
assert 'WP12_NEED_RNG_21B4' in text
print('WP12 R02 ownership/static audit: PASS')
