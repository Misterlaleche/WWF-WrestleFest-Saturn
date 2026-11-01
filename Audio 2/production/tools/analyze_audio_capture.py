#!/usr/bin/env python3
"""Inspect an actual native capture without interpreting audio fidelity."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import struct
import wave

def read_state(path):
    """Parse the native container as written by Mednafen 1.32.1 state.cpp."""
    packed = Path(path).read_bytes()
    data = gzip.decompress(packed) if packed[:2] == b'\x1f\x8b' else packed
    if data[:8] != b'MDFNSVST':
        raise ValueError('Not a supported native Mednafen save state')
    version, size_flags, width, height = struct.unpack_from('<4I', data, 16)
    size = size_flags & 0x7fffffff
    if len(data) != size:
        raise ValueError(f'State size mismatch: {len(data)} != {size}')
    offset = 32 + width * height * 3
    sections = {}
    while offset < size:
        name = data[offset:offset+32].split(b'\0', 1)[0].decode()
        length = struct.unpack_from('<I', data, offset+32)[0]
        offset += 36
        end = offset + length
        fields = {}
        while offset < end:
            n = data[offset]
            field = data[offset+1:offset+1+n].decode()
            field_length = struct.unpack_from('<I', data, offset+1+n)[0]
            offset += 1 + n + 4
            if offset + field_length > end:
                raise ValueError('State field runs past section boundary')
            fields[field] = data[offset:offset+field_length]
            offset += field_length
        if offset != end or name in sections:
            raise ValueError('State section boundary or duplicate error')
        sections[name] = fields
    metadata = {'version_numeric': version, 'bytes': size,
                'data_byte_order': 'big' if size_flags & 0x80000000 else 'little',
                'preview': {'width': width, 'height': height},
                'file_sha256': hashlib.sha256(packed).hexdigest()}
    return metadata, sections


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', required=True, type=Path)
    parser.add_argument('--firmware', required=True, type=Path)
    parser.add_argument('--symbols', required=True, type=Path)
    parser.add_argument('--layout', required=True, type=Path,
                        help='11 big-endian uint32 constants compiled from sfx-layout.c using m68k GCC')
    args = parser.parse_args()
    states = list((args.capture / 'mednafen-home/mcs').glob('*.mc0'))
    if len(states) != 1:
        raise ValueError('Require exactly one captured native state')
    metadata, sections = read_state(states[0])
    ram = sections['SCSP']['RAM']
    if metadata['data_byte_order'] == 'little':
        ram = b''.join(ram[i:i+2][::-1] for i in range(0, len(ram), 2))
    layout = dict(zip(['sfx_size', 'underruns', 'frames', 'ticks', 'bound', 'fault',
                       'pcm', 'pcm_size', 'pcm_playing', 'pcm_write_at', 'pcm_played'],
                      struct.unpack('>11I', args.layout.read_bytes())))
    symbols = json.loads(args.symbols.read_text())
    base = symbols['wf_sfx_engine']
    firmware = args.firmware.read_bytes()
    phoff = struct.unpack_from('>I', firmware, 28)[0]
    phsize, phnum = struct.unpack_from('>HH', firmware, 42)
    segments = [struct.unpack_from('>8I', firmware, phoff+i*phsize) for i in range(phnum)]
    text = next(segment for segment in segments if segment[0] == 1 and segment[2] == 0)
    expected = firmware[text[1]:text[1]+512]
    report = {'native_state_sha256': digest(states[0]),
              'firmware_sha256': digest(args.firmware),
              'symbols_sha256': digest(args.symbols), 'layout_sha256': digest(args.layout),
              'firmware_first512_bytes_match_SCSP_RAM': ram[:512] == expected,
              'layout': layout, 'sfx_engine_address': base,
              'historical_probe01_to03_word9_note': 'C05A was reserved, not the SFX underrun counter; later harnesses expose the actual 32-bit counter'}
    if not report['firmware_first512_bytes_match_SCSP_RAM']:
        raise ValueError('SCSP RAM does not match the specified firmware; refusing to label counters')
    for name in ('underruns', 'frames', 'ticks'):
        at = base + layout[name]
        report[name + '_32bit'] = int.from_bytes(ram[at:at+4], 'big')
    for name in ('bound', 'fault'):
        report[name] = ram[base+layout[name]]
    report['pcm_voices'] = []
    for voice in range(4):
        at = base + layout['pcm'] + voice * layout['pcm_size']
        report['pcm_voices'].append({
            'voice': voice, 'playing': ram[at+layout['pcm_playing']],
            'write_at': int.from_bytes(ram[at+layout['pcm_write_at']:at+layout['pcm_write_at']+4], 'big'),
            'played': int.from_bytes(ram[at+layout['pcm_played']:at+layout['pcm_played']+4], 'big')})
    for at in (0xc05a, 0xc05c):
        report[f'mail_reserved_{at:04X}'] = int.from_bytes(ram[at:at+2], 'big')
    with wave.open(str(args.capture / 'native.wav')) as audio:
        if audio.getsampwidth() != 2:
            raise ValueError('Expected 16-bit PCM capture')
        frames, channels, rate = audio.getnframes(), audio.getnchannels(), audio.getframerate()
        report['audio'] = {'channels': channels, 'sample_rate': rate,
                           'frames': frames, 'duration_seconds': frames / rate,
                           'wav_sha256': digest(args.capture / 'native.wav'), 'windows': []}
        for start in range(0, frames, rate * 5):
            audio.setpos(start)
            data = audio.readframes(min(rate * 5, frames - start))
            samples = struct.unpack('<%dh' % (len(data) // 2), data)
            report['audio']['windows'].append({
                'start_seconds': start / rate, 'end_seconds': (start + len(samples) / channels) / rate,
                'peak': max(map(abs, samples), default=0),
                'rms': (sum(x*x for x in samples) / len(samples))**0.5,
                'nonzero_samples': sum(x != 0 for x in samples)})
    (args.capture / 'sfx-state-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
