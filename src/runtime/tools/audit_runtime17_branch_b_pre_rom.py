#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys
rom=Path(sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest_main68k.bin').read_bytes()
want='a726c46f350723ea73158bc78055d17d418b14e2'
sha=hashlib.sha1(rom).hexdigest(); print('SHA1',sha,'PASS' if sha==want else 'FAIL')
if sha!=want: raise SystemExit(1)
checks={
0x285DA:'48e780c061286100026c6100022241f9001c182a43f9001c185a303c002f', # TST.W $1C16D2
0x2867A:'00028696000286960002875a000287e0000287e2000286f80002875a',
0x26A42:'4a79001c16d2',
0x26B76:'48e7808041f900026b8e3039001c16d4e548207000004ed000026b9e00026bda00026bbc00026bf8', # direction dispatch preamble
0x7548:'4a39001c16c56600009c4a79001c007c67000092',
0x7566:'70000828000700006700006c',
0x75D4:'6100012261000014',
0x76F8:'48e780003028006e4a406a06',
0x7720:'700043f9001c05b0',
0x776C:'48e7f8c041f9001c05b07000',
}
for a,h in checks.items():
    exp=bytes.fromhex(h); got=rom[a:a+len(exp)]
    ok=got==exp; print(f'${a:06X}', 'PASS' if ok else f'FAIL got={got.hex()} exp={exp.hex()}')
    if not ok: raise SystemExit(1)
print('BRANCH-B PRE + $7548 ROM AUDIT PASS')
