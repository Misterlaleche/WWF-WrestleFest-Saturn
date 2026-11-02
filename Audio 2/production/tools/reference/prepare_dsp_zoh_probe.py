#!/usr/bin/env python3
"""Prepare one private DSP-only ZOH counterfactual from a frozen source tree.

No production DSP/engine is modified. This keeps phase/rate, PCM, volume, FM,
clock, program length and all other source bytes. Native A/B remains required.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from source_snapshot import snapshot_package, source_identity


def sign(value, bits):
    return ((value & ((1 << bits) - 1)) ^ (1 << (bits - 1))) - (1 << (bits - 1))


def saturate24(value):
    return max(-0x800000, min(0x7fffff, value))


def evaluate(program, first, second, fraction, gain):
    """Only the nine arithmetic steps; selected Mednafen RunDSP integer rules."""
    mem = {0: 0, 2: first << 8, 3: second << 8}
    temp = [0] * 128
    coef = {0: -4096, 5: gain, 63: 0}
    sft = result = 0
    for words in program[29:38]:
        instruction = sum(word << shift for word, shift in zip(words, (48, 32, 16, 0)))
        shft0, shft1 = (instruction >> 20) & 1, (instruction >> 21) & 1
        shifted = sign(sft, 26) << (shft0 ^ shft1)
        if not shft1:
            shifted = saturate24(shifted)
        shifted &= 0xffffff
        address = (instruction >> 56) & 127
        x = mem[(instruction >> 38) & 63] if (instruction >> 47) & 1 else sign(temp[address], 24)
        ysel = (instruction >> 45) & 3
        y = fraction if ysel == 0 else coef[(instruction >> 9) & 63]
        if ysel > 1:
            raise ValueError('Unexpected arithmetic Y selector')
        b = sft if (instruction >> 16) & 1 else temp[address]
        if (instruction >> 18) & 1:
            b = -b
        if (instruction >> 17) & 1:
            b = 0
        sft = ((sign(y, 13) * sign(x, 24) >> 12) + b) & 0x3ffffff
        if (instruction >> 55) & 1:
            temp[(instruction >> 48) & 127] = shifted
        if (instruction >> 28) & 1:
            result = sign(shifted >> 8, 16)
    return result


def verify_arithmetic(linear, zoh):
    pairs = [(-32768, 32767), (32767, -32768), (0, 32767), (32767, 0),
             (-32768, 0), (0, -32768), (-1, 1), (1, -1)]
    levels = (32, 22, 16, 11, 8, 6, 4, 3, 2, 0)
    cases = mismatches = 0
    for level in levels:
        raw_gain = -((21627 * level + 16) // 32)
        gain = sign((raw_gain & 65535) >> 3, 13)
        for a, b in pairs:
            for fraction in range(4096):
                expected_linear = saturate24(-((a << 8) - ((a << 8) * fraction >> 12) + ((b << 8) * fraction >> 12)))
                expected_zoh = saturate24(-(a << 8))
                expected_linear = saturate24(expected_linear * gain >> 12) >> 8
                expected_zoh = saturate24(expected_zoh * gain >> 12) >> 8
                actual_linear = evaluate(linear, a, b, fraction, gain)
                actual_zoh = evaluate(zoh, a, b, fraction, gain)
                if actual_linear != expected_linear or actual_zoh != expected_zoh:
                    raise ValueError('DSP arithmetic differs from the declared integer equations')
                cases += 1
                mismatches += actual_linear != actual_zoh
    if not mismatches:
        raise ValueError('Counterfactual did not alter interpolation')
    return dict(cases=cases, all_4096_fractions=True, distinct_nonzero_volume_levels=9,
                silent_volume_level=True, linear_integer_equation_matches=True,
                zoh_integer_equation_matches=True, baseline_fails_zoh_equality_cases=mismatches,
                scope='All interpolation fractions and original volume coefficients for eight signed boundary pairs; exact integer equations include coefficient truncation and saturation. Native capture is a separate gate.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out = args.out.resolve()
    package = Path(__file__).resolve().parents[2]
    if args.out == package or package in args.out.parents:
        parser.error('The experiment must stay outside production')
    args.out.mkdir(parents=True, exist_ok=False)
    snapshot = args.out / 'source'
    before = snapshot_package(args.source, snapshot)
    source = snapshot / 'tools/build_dsp_mixer.py'
    original = source.read_text()
    marker = '    return p+[[0]*4 for _ in range(128-len(p))]'
    if original.count(marker) != 1:
        raise ValueError('Generator shape differs')
    changed = original.replace(marker, '''    # PRIVATE ZOH EXPERIMENT: retain every rate/phase/clock/volume instruction.
    # Replace the next-sample read and its unused latch with pipeline NOPs.
    # At the original interpolation point latch -current PCM instead.
    for v in range(4):
        start=16+v*22
        p[start+9]=instruction()
        p[start+12]=instruction()
        p[start+13]=instruction(xsel=1,ira=2+v*2,coef=0,zero=1,bsel=0)
        for relative in (14,15,16,17):p[start+relative]=instruction()
    assert sum(bool(w[2]&0x6000) for w in p)==8
    return p+[[0]*4 for _ in range(128-len(p))]''')
    changed = changed.replace('120 active steps, 12 RAM accesses', 'PRIVATE ZOH: 120 active steps, 8 RAM accesses')
    changed = changed.replace('dict(steps=120,ram_accesses=12,', 'dict(steps=120,ram_accesses=8,')
    source.write_text(changed)
    def load(path, name):
        spec = importlib.util.spec_from_file_location(name, path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module.build_program()
    linear = load(args.source / 'tools/build_dsp_mixer.py', 'original_dsp')
    zoh = load(source, 'private_zoh_dsp')
    allowed = {16 + voice * 22 + step for voice in range(4) for step in (9, 12, 13, 14, 15, 16, 17)}
    changed_steps = [step for step, (a, b) in enumerate(zip(linear, zoh)) if a != b]
    if set(changed_steps) != allowed:
        raise ValueError('Unexpected DSP program mutation')
    after = source_identity(snapshot)
    changed_files = [name for name in before if before[name] != after.get(name)]
    if changed_files != ['tools/build_dsp_mixer.py']:
        raise ValueError('Unexpected source mutation')
    report = dict(kind='PRIVATE_DSP_ZOH_COUNTERFACTUAL', source_origin=str(args.source.resolve()),
        source_snapshot=str(snapshot), source_sha256_before=before, source_sha256_after=after,
        changed_source_files=changed_files, changed_dsp_steps=changed_steps,
        unchanged_phase_pcm_volume_clock_fm_instructions=True, linear_ram_accesses=12,
        zoh_ram_accesses=8, active_program_steps=120, production_changed=False,
        arithmetic=verify_arithmetic(linear, zoh),
        native_sample_phase_increment=743, native_sample_phase_denominator=4096,
        nominal_oki_hz=8000, native_quantized_oki_hz=44100 * 743 / 4096,
        scsp_source_sha256='0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906',
        native_capture_status='NOT RUN', performance_or_fidelity_acceptance=False)
    (args.out / 'zoh-probe-preparation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'out': str(args.out), 'changed_files': changed_files, 'arithmetic': report['arithmetic']}))


if __name__ == '__main__':
    main()
