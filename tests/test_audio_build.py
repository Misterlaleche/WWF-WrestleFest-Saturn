#!/usr/bin/env python3
"""Private ROM resource identities and independently checked sound sequences."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import struct
import sys
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rom-zip', required=True, type=Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('build_audio', ROOT / 'tools/build_audio.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    with zipfile.ZipFile(args.rom_zip) as z:
        sound = z.read('31a11-2.ic42')
        samples = z.read('31j10.ic73')
    result = m.build_sfx(sound, samples)
    pack = result['pack']
    assert len(pack) == 440710
    assert hashlib.sha256(pack).hexdigest() == 'dbe749cf4ce13f333ff7318a74a64e30fa2b9af05d953688a0813311cbabd216'
    assert zlib.crc32(pack[32:]) == struct.unpack_from('>I', pack, 28)[0]
    starts = sum((e[3] & 3) != 1 for c in result['commands'] for e in c['events'])
    assert starts == 100, 'F1 continuations must not retrigger after physical sample completion'
    assert result['commands'][0x38-0x20]['events'] == [[24,255,3,3],[24,30,3,1]]
    assert result['commands'][0x53-0x20]['events'] == [[50,88,3,0],[51,164,3,0]]
    assert max(s['frames'] for s in result['samples']) == 36300
    assert len(result['pcm16be']) == 1755496
    assert hashlib.sha256(result['pcm16be']).hexdigest() == '45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee'
    try:
        m.build_sfx(sound[:-1], samples)
    except ValueError:
        pass
    else:
        raise AssertionError('wrong sound ROM accepted')
    tag=m.music_plan('tag',600)
    assert all(tag[c]==600 for c in (4,6,7,10,11,12,15))
    assert all(tag[c]==120 for c in (13,16,17))
    assert all(m.music_plan('arcade',600)[c]==600 for c in (2,3,14))
    assert m.music_plan('arcade',600)[5] == 12
    assert m.music_plan('arcade',600)[9] == 12
    assert m.music_plan('arcade',600)[1] == 600
    assert m.music_plan('arcade',600)[0x11] == 120
    assert m.music_plan('arcade',600)[0x12] == 120
    assert len(m.music_plan('arcade',600)) == 18
    assert len(m.music_plan('all',600))==18
    assert sum(m.music_plan('tag', 600).values()) * 88200 < 650_000_000
    assert sum(m.music_plan('all',600).values()) * 88200 + len(pack) < 650_000_000
    print('PASS: 95 sample identities, 98 commands, 100 physical starts, exact ROM digests, known-route disc budget')

if __name__ == '__main__':
    main()
