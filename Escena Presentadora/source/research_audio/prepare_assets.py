#!/usr/bin/env python3
"""Extract the WP02 prematch subset from user-provided WrestleFest ROMs.

No generated/substitute sound is used. PCM16BE preserves the full decoded
12-bit OKI signal shifted left four bits. Reducing it with arithmetic >> 8
must match the existing A16 PCM8 pack byte-for-byte.
"""
import argparse
import csv
import hashlib
import io
import json
import itertools
import struct
import re
from pathlib import Path
import zipfile

STEP = (16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,
        97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,
        408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552)
SHIFT = (-1,-1,-1,-1,2,4,6,8)
SELECTED = [(0,17),(0,18)] + [(1,s) for s in (*range(23,32),37,38,39)]
VOLUME_NUMERATORS = (32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0)

def decode_signals(data):
    signal, index = -2, 0
    out = []
    for byte in data:
        for x in (byte >> 4, byte & 15):
            step = STEP[index]
            diff = (step >> 3)
            if x & 1: diff += step >> 2
            if x & 2: diff += step >> 1
            if x & 4: diff += step
            signal = max(-2048, min(2047, signal + (-diff if x & 8 else diff)))
            index = max(0, min(48, index + SHIFT[x & 7]))
            out.append(signal)
    return out

def pcm8(data):
    return bytes((signal >> 4) & 255 for signal in decode_signals(data))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sources', type=Path, default=Path('project_sources'))
    ap.add_argument('--out', type=Path,
                    default=Path('wp02/research_audio/integration'))
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out/'cd').mkdir(exist_ok=True)
    (args.out/'assets').mkdir(exist_ok=True)
    (args.out/'original').mkdir(exist_ok=True)
    with zipfile.ZipFile(next(args.sources.glob('01-*.zip'))) as z:
        rom = z.read('31j10.ic73')
    assert len(rom) == 0x80000
    with zipfile.ZipFile(next(args.sources.glob('02-*.zip'))) as z:
        prefix = 'WWF-Saturn-Audio-Complete-Integration/'
        oldpack = z.read(prefix+'audio-integration-files/cd/AUDPCM.PAK')
        rows = list(csv.DictReader(io.StringIO(z.read(
            prefix+'audio-docs/AUDPCM-INDEX-A15.csv').decode())))
        reference = {(int(r['bank']),int(r['sample_id'])): r for r in rows}
        original_hashes = {}
        for name in ('wf_audio_sfx.c','wf_audio_sfx.h',
                     'ponesound_min.c','ponesound_min.h'):
            data = z.read(prefix+'audio-integration-files/'+name)
            (args.out/name).write_bytes(data)
            original_hashes[name] = hashlib.sha256(data).hexdigest()
        driver = z.read(prefix+'audio-integration-files/assets/SDRV.BIN')
        (args.out/'assets/SDRV.BIN').write_bytes(driver)
        original_hashes['assets/SDRV.BIN'] = hashlib.sha256(driver).hexdigest()
    pack, metadata = bytearray(), []
    # Every supported cue uses a single constant original OKI volume for its
    # sample. Recover it from the unmodified sequencer instead of relying on
    # the incompatible A16 -> older poneSound volume translation.
    nibbles={}
    command_lines=(args.out/'wf_audio_sfx.c').read_text().splitlines()[2:100]
    for command in (0x31,0x32,*range(0x6D,0x76),0x7B,0x7C,0x7D):
        nums=[int(x) for x in re.findall(r'(\d+)u',command_lines[command-0x20])]
        _,policy,count,_=nums[:4]
        for i in range(count):
            sid,_,volume,_=nums[4+i*4:8+i*4]
            bank=policy if policy<2 else 0
            key=(bank,sid)
            assert key not in nibbles or nibbles[key]==volume
            nibbles[key]=volume
    for bank, sid in SELECTED:
        table = bank * 0x40000 + sid * 8
        start = int.from_bytes(rom[table:table+3], 'big')
        end = int.from_bytes(rom[table+3:table+6], 'big')
        assert start <= end < 0x40000
        adpcm = rom[bank*0x40000+start:bank*0x40000+end+1]
        signals = decode_signals(adpcm)
        reduced8 = bytes((signal >> 4) & 255 for signal in signals)
        decoded = b''.join(struct.pack('>h',signal*16) for signal in signals)
        r = reference[bank,sid]
        off, size = int(r['offset_bytes']), int(r['pcm8_bytes'])
        assert size == len(signals)
        assert reduced8 == oldpack[off:off+size], (bank,sid,'PCM8 reference mismatch')
        pack.extend(bytes((-len(pack)) % 2048))
        row = dict(bank=bank, sample_id=sid, sector=len(pack)//2048,
                   pcm16_bytes=len(decoded), samples=len(signals),
                   original_volume_nibble=nibbles[bank,sid],
                   original_gain_numerator=VOLUME_NUMERATORS[nibbles[bank,sid]],
                   original_gain_denominator=32,
                   rom_offset=bank*0x40000+start, adpcm_bytes=len(adpcm),
                   sample_rate=8000, duration_seconds=len(signals)/8000,
                   timer_a_ticks=(len(signals)*3579545 + 8000*64*253-1)//(8000*64*253),
                   adpcm_sha256=hashlib.sha256(adpcm).hexdigest(),
                   pcm16be_sha256=hashlib.sha256(decoded).hexdigest(),
                   a16_pcm8_sha256=hashlib.sha256(reduced8).hexdigest())
        pack.extend(decoded)
        metadata.append(row)
    pack.extend(bytes((-len(pack)) % 2048))
    (args.out/'cd/WP02PCM.PAK').write_bytes(pack)
    header = ['/* Generated ROM-derived PCM16BE; reduced PCM8 verified against A16. */',
              '#ifndef WP02_AUDIO_ASSETS_H', '#define WP02_AUDIO_ASSETS_H',
              '#include <stdint.h>',
              'typedef struct { uint8_t bank, sample, volume; uint16_t sector, samples, ticks; uint32_t bytes; } wp02_pcm_asset_t;',
              'static const wp02_pcm_asset_t wp02_pcm_assets[] = {']
    for r in metadata:
        header.append(' {%du,%du,%du,%du,%du,%du,%du},' % (
            r['bank'],r['sample_id'],r['original_volume_nibble'],r['sector'],r['samples'],
            r['timer_a_ticks'],r['pcm16_bytes']))
    header += ['};', '#define WP02_PCM_ASSET_COUNT 14u',
               '#define WP02_PCM_PACK_BYTES %du' % len(pack),
               '#endif', '']
    (args.out/'wp02_audio_assets.h').write_text('\n'.join(header))
    identity = [23,24,25,30,29,29,27,26,28,31,29,29]
    scenarios=[]
    for ids in itertools.combinations(range(12),4):
        keys=set([(0,17),(0,18),(1,37),(1,38),(1,39)]+[(1,identity[i]) for i in ids])
        scsp=0x14000
        for r in metadata:
            if (r['bank'],r['sample_id']) in keys:
                scsp=((scsp+31)&~31)+r['pcm16_bytes']
        scenarios.append((scsp,ids))
    worst_end,worst_ids=max(scenarios)
    assert worst_end==0x6C428 and worst_end<0x80000
    report = dict(rom_sha256=hashlib.sha256(rom).hexdigest(),
                  original_file_sha256=original_hashes,
                  pcm_pack_sha256=hashlib.sha256(pack).hexdigest(),
                  pack_bytes=len(pack), pcm_payload_bytes=sum(r['pcm16_bytes'] for r in metadata),
                  scsp_start=0x14000, worst_scsp_end_exclusive=worst_end,
                  worst_scsp_payload_and_alignment_bytes=worst_end-0x14000,
                  worst_ids=list(worst_ids), sound_ram_headroom=0x80000-worst_end,
                  distinct_roster_combinations_verified=len(scenarios),
                  all_reduced_pcm8_equal_to_a16=True,
                  pcm_format='Signed PCM16 big-endian; original OKI signal * 16; no quantization loss',
                  source_package_difference='A16 PCM8 truncates; Test255 PCM8 rounds; WP02 retains all 12 bits',
                  samples=metadata)
    (args.out/'audio_assets.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('samples','original_file_sha256')},indent=2))

if __name__ == '__main__':
    main()
