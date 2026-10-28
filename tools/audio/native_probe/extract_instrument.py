"""Extract an observed patch and generate an explicitly uncalibrated SCSP probe."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import struct


def modulation_selector(source, target, latest):
    if not 0 <= source < 32 or not 0 <= target < 32:
        raise ValueError('Invalid SCSP slot')
    delta = source - target
    selector = delta & 31
    current_high = not (delta < 0 or delta >= 28)
    return selector | (32 if current_high == latest else 0)


def pitch_word(hz):
    if not math.isfinite(hz) or not 44100 / 1024 / 256 <= hz <= (
            44100 / 1024 * 128 * (1 + 1023 / 1024)):
        raise ValueError('Frequency outside representable SCSP range')
    candidates = []
    for octave in range(-8, 8):
        fraction = round((hz / (44100 / 1024 * 2 ** octave) - 1) * 1024)
        if 0 <= fraction <= 1023:
            actual = 44100 / 1024 * 2 ** octave * (1 + fraction / 1024)
            candidates.append((abs(math.log2(actual / hz)),
                               ((octave & 15) << 11) | fraction, actual))
    _, word, actual = min(candidates)
    return word, actual


def sine_three_cycles():
    cycle = [round(32767 * math.sin(2 * math.pi * i / 1024)) for i in range(1024)]
    return struct.pack('>3072h', *(cycle * 3))


def extract_snapshot(trace, channel, ordinal):
    if not 0 <= channel <= 7 or ordinal < 1:
        raise ValueError('Invalid channel/ordinal')
    shadow = [None] * 256
    seen = 0
    origin = None
    selected = None
    amd = pmd = 0
    with trace.open(newline='') as stream:
        for row in csv.DictReader(stream):
            kind, register, data = row['kind'], int(row['reg']), int(row['data'])
            if kind == 'command' and origin is None:
                origin = int(row['cycle'])
            if kind == 'ym_write':
                shadow[register] = data
                if register == 0x19:
                    if data & 128:
                        pmd = data & 127
                    else:
                        amd = data
                if row['phase'] == 'run' and register == 8 and data == (0x78 | channel):
                    seen += 1
                    if seen == ordinal:
                        required = [0x20 + channel, 0x28 + channel,
                                    0x30 + channel, 0x38 + channel]
                        required += [base + 8 * op + channel for op in range(4)
                                     for base in (0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0)]
                        if any(shadow[r] is None for r in required) or origin is None:
                            raise ValueError('Incomplete register provenance')
                        selected = {
                            'channel': channel, 'keyon_write_ordinal': ordinal,
                            'key': data, 'cycle': int(row['cycle']), 'origin_cycle': origin,
                            'seconds_after_command': (int(row['cycle']) - origin) / 3579545,
                            'pc_after_write_instruction': int(row['pc']),
                            'algorithm': shadow[0x20 + channel] & 7,
                            'feedback': (shadow[0x20 + channel] >> 3) & 7,
                            'rl': shadow[0x20 + channel] >> 6,
                            'kc': shadow[0x28 + channel], 'kf': shadow[0x30 + channel],
                            'pms_ams': shadow[0x38 + channel], 'noise': shadow[0x0f] or 0,
                            'lfo_amd': amd, 'lfo_pmd': pmd,
                            'operators': [
                                {'name': name, 'raw': [shadow[base + 8 * op + channel]
                                 for base in (0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0)]}
                                for op, name in enumerate(('M1', 'M2', 'C1', 'C2'))],
                        }
            elif selected is not None and kind == 'pg_increment' and (
                    int(row['cycle']) == selected['cycle']):
                if register % 8 == channel and 0 <= register // 8 < 4:
                    selected['operators'][register // 8]['pg_increment'] = data
                if all('pg_increment' in op for op in selected['operators']):
                    return selected
    raise ValueError('Requested complete key-on snapshot not found')


def build_plan(source):
    if source['algorithm'] != 4 or source['key'] != (0x78 | source['channel']):
        raise ValueError('This first probe supports full-key algorithm 4 only')
    if source['pms_ams'] & 0x73 or (
            source['channel'] == 7 and source['noise'] & 128):
        raise ValueError('LFO sensitivity/noise requires a separate calibrated mapping')
    words, pitch = [], []
    # Register order is M1,M2,C1,C2; signal order here is M1,C1,M2,C2.
    for slot, original in enumerate((0, 2, 1, 3)):
        operator = source['operators'][original]
        mul_dt, level, attack, decay1, decay2, sustain_release = operator['raw']
        if decay1 & 128:
            raise ValueError('AM-enabled operators are not supported by this probe')
        # OPM phase is 20 bits and advances once per 32 master clocks.
        target_hz = operator['pg_increment'] * 3579545 / (32 * 2 ** 20)
        frequency, actual = pitch_word(target_hz)
        pitch.append({'slot': slot, 'operator': operator.get('name', original),
                      'target_hz_from_opm_state': target_hz, 'scsp_hz': actual,
                      'quantization_cents': 1200 * math.log2(actual / target_hz)})
        modulation = 0
        if slot == 0 and source['feedback']:
            modulation = ((source['feedback'] + 4) << 12) | (0x20 << 6)
        elif slot in (1, 3):
            selector = modulation_selector(slot - 1, slot, True)
            modulation = (12 << 12) | (selector << 6) | selector
        # Provisional EG rate/level mapping, deliberately NOT a fidelity claim.
        envelope1 = ((decay2 & 31) << 11) | ((decay1 & 31) << 6) | (attack & 31)
        decay_level = 31 if sustain_release >> 4 == 15 else (sustain_release >> 4) * 2
        envelope2 = (15 << 10) | (decay_level << 5) | ((sustain_release & 15) * 2 + 1)
        words.append([0x0027, 0x0800, 0, 1024, envelope1, envelope2,
                      (level & 127) * 2, modulation, frequency, 0, 0,
                      0xa000 if slot in (1, 3) else 0])
    return {
        'status': 'EXPERIMENTAL_REGISTER_PROBE_NOT_HARDWARE_VALIDATED',
        'wave_start': 0x70000, 'wave_bytes': 6144, 'wave_period_samples': 1024,
        'slot_order': ['M1', 'C1', 'M2', 'C2'], 'slot_words': words, 'pitch': pitch,
        'uncalibrated': ['SCSP envelope rates and key scaling', 'feedback/modulation gain',
                         'sine/log-sine differences and internal phase delay',
                         'direct mixer gain and full timbre', 'hardware bus timing'],
        'policy': 'Four slots for one isolated patch; not a solution for 36 voices/32 slots',
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trace', required=True, type=Path)
    parser.add_argument('--channel', type=int, default=1)
    parser.add_argument('--ordinal', type=int, default=2)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    source = extract_snapshot(args.trace, args.channel, args.ordinal)
    plan = build_plan(source)
    plan['source_trace_sha256'] = hashlib.sha256(args.trace.read_bytes()).hexdigest()
    plan['observed_ym2151'] = source
    args.out.mkdir(parents=True, exist_ok=True)
    wave = sine_three_cycles()
    plan['wave_sha256'] = hashlib.sha256(wave).hexdigest()
    (args.out / 'first-instrument.json').write_text(json.dumps(plan, indent=2) + '\n')
    (args.out / 'sine-three-cycles.pcm').write_bytes(wave)
    rows = ',\n'.join('    {' + ','.join(f'0x{word:04x}u' for word in row) + '}'
                     for row in plan['slot_words'])
    header = ('/* Generated experimental patch; see first-instrument.json for limits. */\n'
              '#include "wf_scsp_fm_probe.h"\n'
              'static const wf_scsp_fm_probe_patch_t wf_first_fm_patch = {\n'
              '  0x70000u, {\n' + rows + '\n  }\n};\n')
    (args.out / 'first-instrument.h').write_text(header, encoding='ascii')
    with (args.out / 'scsp-register-plan.csv').open('w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(('slot', 'register_offset_hex', 'value_hex'))
        writer.writerows((slot, f'0x{2 * word:02x}', f'0x{value:04x}')
                         for slot, row in enumerate(plan['slot_words'])
                         for word, value in enumerate(row))
    print(json.dumps({'cycle': source['cycle'], 'channel': source['channel'],
                      'algorithm': source['algorithm'], 'pitch': plan['pitch']}, indent=2))
