#!/usr/bin/env python3
"""Intact-ROM scope of the clear owners used by the frontend visibility gate."""
import argparse
import hashlib
import json
from pathlib import Path
from test_credits import ROOT, Oracle, seed

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    oracle=Oracle(rom);cases=[]
    try:
        for pattern in (0x39,0xa7):
            initial,registers=seed(rom)
            for at,length in ((0x80000,0x10000),(0xc0000,0x4000)):
                initial[at:at+length]=bytes([pattern])*length
            for owner in (0x1f6c,0x1fc0,0x1fde):
                actual=bytearray(initial)
                oracle.execute(actual,registers,owner)
                map_same=actual[0x80000:0x90000]==initial[0x80000:0x90000]
                hud_same=actual[0xc0000:0xc2000]==initial[0xc0000:0xc2000]
                sprite_same=actual[0xc2000:0xc4000]==initial[0xc2000:0xc4000]
                if owner==0x1fde:
                    assert map_same and hud_same and not sprite_same
                    assert actual[0xc2000:0xc3000]==bytes(0x1000)
                    assert actual[0xc3000:0xc4000]==initial[0xc3000:0xc4000]
                elif owner==0x1f6c:
                    assert not map_same and hud_same and sprite_same
                else:
                    assert map_same and not hud_same and not sprite_same
                cases.append(dict(owner=hex(owner),pattern=pattern,map_unchanged=map_same,
                    hud_unchanged=hud_same,sprites_unchanged=sprite_same))
        assert oracle.mem.r_block(0,len(rom))==rom
    finally:oracle.machine.cleanup()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(status='PASS',rom_sha1=hashlib.sha1(rom).hexdigest(),
        cases=cases,rom_modifications=[],scope='Original clear writes only; no Saturn timing/scanout claim'),indent=2)+'\n')
    print('PASS: six intact-ROM clear cases; $1FDE preserves all background/HUD bytes')

if __name__=='__main__':main()
