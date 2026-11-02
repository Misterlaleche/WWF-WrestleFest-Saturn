#!/usr/bin/env python3
"""Reproduce or inspect one private native linear/ZOH SFX A/B pair.

Inputs are locally generated original resources/full-board references, the
reproduced toolchain, and the user's BIOS. No ROM, BIOS, WAV or emulator is
embedded. Production runtime sources and the main native matrix are untouched.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
PACKAGE = HERE.parents[1]
sys.path.insert(0, str(PACKAGE / 'tools'))
from analyze_audio_capture import read_state
from source_snapshot import snapshot_package

SOUND_SHA = '2e83769d98a737a9e277aab7a42c6b14686aa0eb5df84e16490ed4bc195f0bbb'
SCSP_SHA = '0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def program_range(image, symbols):
    base = symbols['wf_dsp_program']
    at, size, count = struct.unpack_from('>I', image, 28)[0], *struct.unpack_from('>HH', image, 42)
    for index in range(count):
        typ, off, _, phys, length, _, _, _ = struct.unpack_from('>8I', image, at + size * index)
        if typ == 1 and phys <= base < phys + length:
            start = off + base - phys
            return start, start + 1024
    raise ValueError('DSP program is not in a loaded firmware segment')


def analyze(root, environment, command):
    root, environment = root.resolve(), environment.resolve()
    sound = environment / 'src/mednafen/src/ss/sound.cpp'
    scsp = environment / 'src/mednafen/src/ss/scsp.inc'
    if sha(sound) != SOUND_SHA or sha(scsp) != SCSP_SHA:
        raise ValueError('Selected unmodified emulator source identity differs')
    if 'bp[0] = (bp[0] * 27 + 16) >> 5;' not in sound.read_text():
        raise ValueError('Expected upstream post-SCSP output factor is absent')
    runs, images, symbols, states, ram, comparisons, wavs = {}, {}, {}, {}, {}, {}, {}
    for name in ('linear', 'zoh'):
        directory = root / (name + '-run')
        report = json.loads((directory / 'native-sfx-matrix.json').read_text())
        if report['range'] != [command, command] or report['reference_profile'] != 'full-board':
            raise ValueError('Require the same isolated original command and full-board reference')
        runs[name] = report
        firmware = directory / 'inputs/firmware/SCSP.ELF'
        images[name] = firmware.read_bytes()
        symbols[name] = json.loads((firmware.parent / 'symbols.json').read_text())
        capture = directory / f'command-{command:02X}' / 'capture'
        paths = list((capture / 'mednafen-home/mcs').glob('*.mc0'))
        if len(paths) != 1:
            raise ValueError('Expected one native F5 state')
        metadata, sections = read_state(paths[0])
        states[name] = sections['SCSP']
        endian = '<' if metadata['data_byte_order'] == 'little' else '>'
        actual_program = struct.unpack(endian + '128Q', states[name]['DSP.MPROG'])
        start, end = program_range(images[name], symbols[name])
        expected_program = struct.unpack('>128Q', images[name][start:end])
        if actual_program != expected_program:
            raise ValueError('Native DSP program differs from its specified firmware')
        if (int.from_bytes(states[name]['MasterVolume'], metadata['data_byte_order']) != 256
                or states[name]['MVOL'] != b'\x0f'):
            raise ValueError('Unexpected native master volume')
        ram[name] = (capture / 'SCSP-RAM-saturn-order.bin').read_bytes()
        comparison = directory / 'pairs' / f'COMPARACION-{command:02X}.json'
        comparisons[name] = json.loads(comparison.read_text())
        wavs[name] = next((directory / 'pairs').glob('SATURN*.wav'))
    if symbols['linear'] != symbols['zoh'] or len(images['linear']) != len(images['zoh']):
        raise ValueError('Firmware layout changed')
    start, end = program_range(images['linear'], symbols['linear'])
    changes = [at for at, (a, b) in enumerate(zip(images['linear'], images['zoh'])) if a != b]
    gates = {
        'all_firmware_changes_inside_dsp_program': bool(changes) and all(start <= at < end for at in changes),
        'native_programs_match_their_elf': True,
        'same_original_reference': comparisons['linear']['identities']['reference_wav'] == comparisons['zoh']['identities']['reference_wav'],
        'same_emulator': runs['linear']['emulator_sha256'] == runs['zoh']['emulator_sha256'],
        'same_64_native_coefficients': states['linear']['DSP.COEF'] == states['zoh']['DSP.COEF'],
        'same_native_slot_registers': states['linear']['SlotRegs'] == states['zoh']['SlotRegs'],
        'same_native_address_registers': states['linear']['DSP.MADRS'] == states['zoh']['DSP.MADRS'],
        'same_all_four_final_pcm_ring_bytes': ram['linear'][0x3000:0xb008] == ram['zoh'][0x3000:0xb008],
        'same_native_phase_clock_constants': ram['linear'][0xb012:0xb018] == ram['zoh'][0xb012:0xb018] == bytes.fromhex('e700e5198000'),
        'both_runtime_pass': all(runs[n]['all_runtime_gates_passed'] for n in runs),
        'sources_and_inputs_stable': all(runs[n]['source_stable'] and runs[n]['private_inputs_stable'] and runs[n]['emulator_stable'] for n in runs),
    }
    metrics = {name: comparisons[name]['metrics'] for name in comparisons}
    backend_db = 20 * math.log10(27 / 32)
    listening = root / 'listening'
    listening.mkdir(exist_ok=True)
    copied = {}
    for name in ('linear', 'zoh'):
        filename = f'SATURN-EXPERIMENTO-{name.upper()}-0x{command:02X}.wav'
        shutil.copyfile(wavs[name], listening / filename)
        copied[filename] = sha(listening / filename)
    reference = root / 'linear-run/inputs/reference' / f'reference-{command:02X}.wav'
    filename = f'ARCADE-REFERENCIA-SFX-0x{command:02X}.wav'
    shutil.copyfile(reference, listening / filename)
    copied[filename] = sha(listening / filename)
    result = dict(kind='ONE_ORIGINAL_SFX_NATIVE_INTERPOLATION_CAUSAL_EXPERIMENT', command=command,
        linear_firmware_sha256=hashlib.sha256(images['linear']).hexdigest(),
        zoh_firmware_sha256=hashlib.sha256(images['zoh']).hexdigest(),
        different_elf_bytes=len(changes), dsp_program_file_range=[start, end],
        gates=gates, isolated_experiment_valid=all(gates.values()), metrics=metrics,
        measured_zoh_minus_linear_rms_db=metrics['zoh']['saturn_minus_reference_rms_db'] - metrics['linear']['saturn_minus_reference_rms_db'],
        upstream_post_scsp_gain=dict(numerator=27, denominator=32, decibels=backend_db,
            location='selected Mednafen1.32.1 src/ss/sound.cpp RunSCSP, after SCSP.RunSample',
            source_sha256=SOUND_SHA, scsp_source_sha256=SCSP_SHA,
            scope='Emulator digital output factor; not an assertion about original PCB or physical Saturn analog gain'),
        zoh_rms_minus_analytic_backend_prediction_db=metrics['zoh']['saturn_minus_reference_rms_db'] - backend_db,
        normalization=False, gain_adjustment_applied=False, output_pcm_modified=False,
        native_quantized_oki_hz=44100 * 743 / 4096, nominal_oki_hz=8000,
        native_vs_reference_phase_independently_calibrated=False,
        listening_files_sha256=copied, runtime_scope='One original cold-boot command only; no load/combat throughput improvement is claimed',
        production_installed=False, full_audio_fidelity_accepted=False)
    (root / 'dsp-zoh-causal.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--analyze-existing', type=Path)
    p.add_argument('--source', type=Path, default=PACKAGE)
    p.add_argument('--out', type=Path)
    for field in ('environment-root', 'resources', 'reference-report', 'sdk', 'bios', 'runtime-source'):
        p.add_argument('--' + field, type=Path, required=field == 'environment-root')
    p.add_argument('--command', type=lambda value: int(value, 0), default=0x21)
    p.add_argument('--display', type=int, default=87)
    p.add_argument('--expected-linear-elf-sha256')
    a = p.parse_args()
    if not 0x20 <= a.command <= 0x81:
        p.error('Use one original SFX command')
    if a.analyze_existing:
        report = analyze(a.analyze_existing, a.environment_root, a.command)
    else:
        if any(getattr(a, name) is None for name in ('out', 'resources', 'reference_report', 'sdk', 'bios', 'runtime_source')):
            p.error('A new run requires output, resources, full-board reference report, SDK, BIOS and pinned runtime source')
        a.out = a.out.resolve()
        if a.out == PACKAGE or PACKAGE in a.out.parents:
            p.error('Private outputs must stay outside production')
        a.out.mkdir(parents=True, exist_ok=False)
        original = a.out / 'source-linear'
        snapshot_package(a.source, original)
        commands = []
        def run(command):
            with (a.out / f'stage-{len(commands):02d}.log').open('w') as log:
                result = subprocess.run(list(map(str, command)), stdout=log, stderr=subprocess.STDOUT)
            commands.append(dict(command=list(map(str, command)), exit_code=result.returncode))
            (a.out / 'reproduction-commands.json').write_text(json.dumps(commands, indent=2) + '\n')
            if result.returncode:
                raise RuntimeError('Stage failed; original evidence and logs are retained')
        run([sys.executable, HERE / 'prepare_dsp_zoh_probe.py', '--source', original, '--out', a.out / 'variant'])
        for name, source in [('linear', original), ('zoh', a.out / 'variant/source')]:
            firmware = a.out / ('firmware-' + name)
            run([sys.executable, source / 'tools/build_firmware.py', '--package', source,
                 '--out', firmware, '--runtime-source', a.runtime_source.resolve(), '--no-timing'])
            if name == 'linear' and a.expected_linear_elf_sha256 and sha(firmware / 'SCSP.ELF') != a.expected_linear_elf_sha256:
                raise ValueError('Requested baseline firmware identity differs')
            run([sys.executable, source / 'tools/run_sfx_matrix.py', '--package', source,
                 '--resources', a.resources.resolve(), '--firmware', firmware / 'SCSP.ELF',
                 '--sdk', a.sdk.resolve(), '--environment-root', a.environment_root.resolve(), '--bios', a.bios.resolve(),
                 '--reference-report', a.reference_report.resolve(), '--reference-profile', 'full-board',
                 '--out', a.out / (name + '-run'), '--first', hex(a.command), '--last', hex(a.command),
                 '--jobs', 1, '--display', a.display, '--presentation-speed', 4])
        report = analyze(a.out, a.environment_root, a.command)
    print(json.dumps({'valid': report['isolated_experiment_valid'], 'metrics': report['metrics'],
                      'upstream_post_scsp_gain': report['upstream_post_scsp_gain']}, indent=2))
    return 0 if report['isolated_experiment_valid'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
