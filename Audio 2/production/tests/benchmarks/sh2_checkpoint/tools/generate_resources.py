#!/usr/bin/env python3
"""Regenerate Audio 2 private assets directly from the pinned user ROM.

No PCM, sound ROM, music recording, or recovered command table is embedded here.
Table addresses/bytecode are the audited A15 contract, checked against a digest.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import zipfile
import zlib

PACKAGE = Path(__file__).resolve().parents[1]


def private_output(path):
    """Protect source/history and prior evidence before generating any files."""
    original = Path(path)
    out = original.resolve()
    protected = {PACKAGE.resolve()}
    if PACKAGE.name == 'production' and PACKAGE.parent.name == 'Audio 2':
        protected.add(PACKAGE.parent.parent.resolve())
    for parent in PACKAGE.resolve().parents:
        if (parent / '.git').exists():
            protected.add(parent)
            break
    if original.is_symlink() or out.exists() or any(
            out == root or root in out.parents for root in protected):
        raise ValueError('Use a fresh private output outside the source package and repository')
    return out


SOUND_SHA256 = 'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7'
SAMPLES_SHA256 = 'e4cbe71f079e2cb95b3463115557fa1d99e18ea975f2d3354827b01994fc4615'
COMMANDS_SHA256 = '962114bf6693c4b32f59eac0139d7206161a4a67780e459cdffdb3f0dfd4e624'
PCM_SHA256 = '45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee'
STEPS = (16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552)
SHIFTS = (-1,-1,-1,-1,2,4,6,8)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def decode(encoded):
    signal = index = 0
    out = bytearray()
    for byte in encoded:
        for nibble in (byte >> 4, byte & 15):
            step = STEPS[index]
            delta = (step >> 3) + ((step >> 2) if nibble & 1 else 0) + ((step >> 1) if nibble & 2 else 0) + (step if nibble & 4 else 0)
            signal = max(-2048, min(2047, signal + (-delta if nibble & 8 else delta)))
            index = max(0, min(48, index + SHIFTS[nibble & 7]))
            out += struct.pack('>h', signal * 16)
    return bytes(out)

def commands(sound):
    result = []
    normalized = bytearray()
    for i in range(98):
        priority, bank = sound[0x20df+i], sound[0x2141+i]
        pos = struct.unpack_from('<H', sound, 0x21a3+i*2)[0]
        volume = 0; suppress = False; cleanup = 0; events = []
        for _ in range(128):
            op = sound[pos]; pos += 1
            if op == 0xf0:
                break
            if op == 0xef:
                volume = sound[pos]; pos += 1
                if volume > 15: raise ValueError('Invalid volume')
            elif op == 0xf1:
                if not events: raise ValueError('Suppression without sample')
                events[-1][3] |= 3
                suppress = True
            elif op == 0xf2:
                suppress = False; cleanup = 1
            elif 1 <= op <= 73:
                delay = sound[pos]; pos += 1
                events.append([op, delay, volume, int(suppress)]); cleanup = 0
            else:
                raise ValueError('Unrecognized original SFX opcode')
        else:
            raise ValueError('Unterminated original SFX program')
        if len(events) > 9 or bank > 2: raise ValueError('Invalid command')
        row = dict(command=i+0x20, priority=priority, bank=bank,
                   cleanup_stop=cleanup, events=events)
        result.append(row)
        normalized += bytes((priority, bank, len(events), cleanup))
        for event in events: normalized += bytes(event)
    if sha(normalized) != COMMANDS_SHA256:
        raise ValueError('Recovered command identity differs from audited A15')
    return result

def generate(sound, samples):
    if len(sound) != 65536 or sha(sound) != SOUND_SHA256:
        raise ValueError('Sound ROM identity mismatch')
    if len(samples) != 524288 or sha(samples) != SAMPLES_SHA256:
        raise ValueError('Sample ROM identity mismatch')
    cmds = commands(sound)
    cmd_bytes = bytearray(); event_bytes = bytearray()
    for c in cmds:
        cmd_bytes += struct.pack('>4BH', c['priority'], c['bank'], len(c['events']), c['cleanup_stop'], len(event_bytes))
        for e in c['events']: event_bytes += bytes(e)
    raw_offset = 32 + 116*8 + len(cmd_bytes) + len(event_bytes)
    table = bytearray(116*8); raw = bytearray(); pcm = bytearray()
    seen = {}; rows = []
    for bank, count in ((0,73), (1,41)):
        for sample in range(1,count+1):
            entry = bank*0x40000 + sample*8
            start = int.from_bytes(samples[entry:entry+3], 'big')
            end = int.from_bytes(samples[entry+3:entry+6], 'big')
            if not 0 <= start <= end < 0x40000: raise ValueError('Invalid sample range')
            encoded = samples[bank*0x40000+start:bank*0x40000+end+1]
            identity = hashlib.sha1(encoded).hexdigest()
            if identity not in seen:
                decoded = decode(encoded)
                pcm += b'\0' * (-len(pcm) % 32)
                seen[identity] = (raw_offset+len(raw), len(pcm), sha(decoded))
                raw += encoded; pcm += decoded
            address, pcm_offset, pcm_hash = seen[identity]
            frames = len(encoded)*2
            ticks = (frames*3579545 + 8000*64*253-1)//(8000*64*253)
            struct.pack_into('>IHH', table, (bank*74+sample)*8, address, len(encoded), ticks)
            rows.append(dict(bank=bank, sample_id=sample, adpcm_start=start,
                             adpcm_bytes=len(encoded), offset=pcm_offset, frames=frames,
                             sha1_adpcm=identity, sha256_pcm16be=pcm_hash))
    payload = table + cmd_bytes + event_bytes + raw
    payload += b'\0' * (-len(payload) % 2)
    pack = struct.pack('>4sHH6I', b'WFSX',1,116,32+len(payload),32,960,1548,raw_offset,zlib.crc32(payload)) + payload
    if len(seen)!=95 or sha(pcm)!=PCM_SHA256: raise ValueError('PCM identity differs from audited decoder')
    if len(pack)>0x6c000: raise ValueError('Sound RAM pack overflow')
    return dict(pack=bytes(pack), pcm16be=bytes(pcm), sound=sound,
                commands=cmds, samples=rows, distinct_samples=len(seen))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom-zip',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True,help='New private directory outside the source package/repository')
    a=p.parse_args()
    a.out=private_output(a.out)
    with zipfile.ZipFile(a.rom_zip) as archive:
        data=generate(archive.read('31a11-2.ic42'),archive.read('31j10.ic73'))
    a.out.mkdir(parents=True,exist_ok=False)
    identities={}
    for name,key in [('SFX.DAT','pack'),('SOUND.ROM','sound'),('PCM16BE.PAK','pcm16be')]:
        content=data.pop(key);(a.out/name).write_bytes(content)
        identities[name]=dict(bytes=len(content),sha256=sha(content))
    data.update(files=identities,source_sound_sha256=SOUND_SHA256,source_samples_sha256=SAMPLES_SHA256,
                commands_sha256=COMMANDS_SHA256,generated_locally=True)
    (a.out/'resources.json').write_text(json.dumps(data,indent=2)+'\n')
    print(json.dumps(identities,indent=2))

if __name__=='__main__': main()
