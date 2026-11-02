#!/usr/bin/env python3
"""Run or audit one native capture, binding its counters to the captured ELF/RAM."""
import argparse
import hashlib
import json
import shlex
import struct
import subprocess
import sys
import wave
from pathlib import Path

from analyze_audio_capture import read_state
from capture_mednafen import finalized_wav

FIELDS=['magic','state','videoFrames','error','commandStartFrame','musicState',
        'musicEvents','batchB03A','lateB028','sfxUnderruns32','lastCommand',
        'DSPclock16','diagAddress','mailState','FMdiscardEvents','discardBatches',
        'rejectedFM','ringRejected','SCSPelapsed','APIstate','APIerror','commandDSPclock',
        'replayAccepted','replayExpected']

COUNTERS = {
    'FMdiscardEvents': '_wf_music_fm_discarded_events',
    'discardBatches': '_wf_music_fm_discarded_batches',
    'rejectedFM': '_wf_music_fm_rejected_events',
    'ringRejected': '_wf_music_ring_rejected_events',
}
# The native harness has a matching _Static_assert. Never infer this address
# from a detached symbols.json or from a reserved mailbox word.
SFX_UNDERRUNS_OFFSET = 32


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def span(data, offset, size):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError('Binary span is outside the captured file or RAM')
    return data[offset:offset + size]


def read_elf(path, machine):
    """Read symbols and file spans from this ELF itself, without external nm."""
    data = Path(path).read_bytes()
    header = struct.unpack('>16sHHIIIIIHHHHHH', span(data, 0, 52))
    ident, kind, arch, version, _, phoff, shoff, _, ehsize, phsize, phnum, shsize, shnum, _ = header
    if (ident[:7] != b'\x7fELF\x01\x02\x01' or kind != 2 or arch != machine
            or version != 1 or ehsize != 52 or phsize != 32 or shsize != 40
            or not phnum or not shnum):
        raise ValueError('Expected a linked ELF32 MSB executable with native symbol tables')
    loads = []
    for i in range(phnum):
        row = struct.unpack('>8I', span(data, phoff + i * phsize, phsize))
        if row[0] == 1:
            if row[4] > row[5]:
                raise ValueError('ELF file span exceeds its load memory span')
            span(data, row[1], row[4])
            loads.append(row)
    sections = [struct.unpack('>10I', span(data, shoff + i * shsize, shsize)) for i in range(shnum)]
    symbols, functions = {}, []
    for section in sections:
        if section[1] != 2:
            continue
        if section[9] != 16 or section[5] % 16 or section[6] >= shnum:
            raise ValueError('Invalid ELF symbol table')
        strings_section = sections[section[6]]
        if strings_section[1] != 3:
            raise ValueError('ELF symbol table has no string table')
        strings = span(data, strings_section[4], strings_section[5])
        for at in range(section[4], section[4] + section[5], 16):
            no, value, size, info, _, index = struct.unpack('>IIIBBH', span(data, at, 16))
            end = strings.find(b'\0', no)
            if end < no or no >= len(strings):
                raise ValueError('Unterminated ELF symbol name')
            name = strings[no:end].decode('ascii')
            symbol = {'value': value, 'size': size, 'section': index, 'kind': info & 15}
            if name and index and info >> 4 in (1, 2):
                if name in symbols and symbols[name] != symbol:
                    raise ValueError('Conflicting defined ELF symbol: ' + name)
                symbols[name] = symbol
            if info & 15 == 2 and size and 0 < index < shnum:
                code = sections[index]
                if code[1] != 1 or not code[2] & 2:
                    raise ValueError('Function symbol is not in allocated file data')
                expected = span(span(data, code[4], code[5]), value - code[3], size)
                functions.append((name, value, expected))
    if not loads or not symbols:
        raise ValueError('ELF has no usable native loads/symbols')
    return {'data': data, 'loads': loads, 'symbols': symbols, 'functions': functions}


def elf_bytes(elf, address, size):
    matches = [p for p in elf['loads'] if p[2] <= address and address + size <= p[2] + p[4]]
    if size <= 0 or len(matches) != 1:
        raise ValueError('Embedded asset does not occupy one unambiguous ELF file span')
    p = matches[0]
    return span(elf['data'], p[1] + address - p[2], size)


def saturn_order(data, byte_order):
    if len(data) % 2 or byte_order not in ('little', 'big'):
        raise ValueError('Invalid native uint16 RAM representation')
    if byte_order == 'big':
        return data
    result = bytearray(data)
    result[::2], result[1::2] = data[1::2], data[::2]
    return bytes(result)


def audit_capture(build, capture, capture_exit_code=None):
    """Read-only audit. Missing/mismatched evidence fails; it never becomes zero."""
    build, capture = Path(build), Path(capture)
    b = json.loads((build / 'saturn-build.json').read_text())
    errors = []
    try:
        report = json.loads((capture / 'report.json').read_text())
    except (OSError, ValueError) as error:
        report = {}
        errors.append({'stage': 'capture_report', 'error': str(error)})
    d = report.get('final_watch_values', {})
    gates = {
        'capture_subprocess_succeeded': capture_exit_code in (None, 0),
        'emulator_exited_successfully': report.get('exit_code') == 0,
        'native_capture_method': report.get('emulator') == 'standalone Mednafen 1.32.1'
            and report.get('frame_skip') is False and report.get('emulated_hardware_clock_change') is False
            and report.get('memory_access') == 'read only',
        'normal_window_stop': report.get('stop_reason') == 'watch_threshold_reached',
        'real_module_entered':d.get('magic')==0x57464132,
        'requested_window_completed':d.get('state')==4,
        'no_reported_worker_error':d.get('error')==0,
        'no_reported_api_error':d.get('APIerror')==0,
        'no_reported_fm_discard':d.get('FMdiscardEvents')==0 and d.get('discardBatches')==0,
        'no_reported_rejected_events':d.get('rejectedFM')==0 and d.get('ringRejected')==0,
        'no_reported_pcm_underrun':d.get('sfxUnderruns32')==0,
        'no_reported_transport_lateness':d.get('lateB028')==0,
        'worker_still_running':d.get('musicState')==2,
        'native_still_ready':d.get('mailState')==2,
        'api_still_running': d.get('APIstate') == 2,
    }
    evidence = {}
    seconds = b.get('seconds')
    requested = seconds * 44100 if type(seconds) is int and 1 <= seconds <= 0xffffffff // 44100 else None
    elapsed = d.get('SCSPelapsed')
    gates['requested_scsp_duration_reached'] = (requested is not None and type(elapsed) is int
                                               and requested <= elapsed <= 0xffffffff)
    evidence['requested_scsp_samples'] = requested
    evidence['observed_scsp_samples'] = elapsed

    gates['build_and_capture_identities_match'] = False
    try:
        files = b['files']
        if not all(name in files for name in ('AUDIO2.ELF', 'AUDIO2.iso', 'AUDIO2.cue')):
            raise ValueError('Build does not identify the ELF and exact boot disc')
        checked = {}
        for name, identity in files.items():
            path = (build / name).resolve()
            if path.parent != build.resolve():
                raise ValueError('Build file identity must name a direct build artifact')
            checked[name] = {'sha256': sha(path), 'bytes': path.stat().st_size}
            if checked[name] != {k: identity[k] for k in ('sha256', 'bytes')}:
                raise ValueError('Build artifact differs from its manifest: ' + name)
        if report.get('cue_sha256') != checked['AUDIO2.cue']['sha256']:
            raise ValueError('Capture did not use this CUE')
        disc_images = []
        for line in (build / 'AUDIO2.cue').read_text().splitlines():
            parts = shlex.split(line)
            if parts and parts[0].upper() == 'FILE':
                path = (build / parts[1]).resolve()
                if path.parent != build.resolve():
                    raise ValueError('CUE image must be inside this isolated build')
                disc_images.append((path.name, path.stat().st_size, sha(path)))
        captured_images = [(Path(item['path']).name, item['bytes'], item['sha256'])
                           for item in report.get('disc_images', [])]
        if not disc_images or disc_images != captured_images:
            raise ValueError('Capture boot images differ from this build')
        evidence['build_artifacts'] = checked
        gates['build_and_capture_identities_match'] = True
    except (OSError, ValueError, KeyError, TypeError) as error:
        errors.append({'stage': 'build_identity', 'error': str(error)})

    gates['native_wav_finalized_and_unchanged'] = False
    gates['native_wav_covers_scsp_window'] = False
    try:
        wav = finalized_wav(capture / 'native.wav')
        evidence['wav_validation'] = wav
        previous = report.get('wav_validation')
        if previous is not None and previous != wav:
            raise ValueError('WAV differs from the capture-time validation record')
        gates['native_wav_finalized_and_unchanged'] = True
        gates['native_wav_covers_scsp_window'] = type(elapsed) is int and 0 < elapsed <= wav['frames']
    except (OSError, ValueError, EOFError, wave.Error) as error:
        errors.append({'stage': 'native_wav', 'error': str(error)})

    # Final zeros alone can hide a reset/wrap. Keep every observed error after
    # startup; uninitialized RAM before the harness magic is not evidence.
    rows = [row for row in report.get('trace', []) if row.get('magic') == 0x57464132
            and row.get('state') in (2, 3, 4) and row.get('commandStartFrame', 0) > 0]
    history_fields = ('error', 'APIerror', 'FMdiscardEvents', 'discardBatches', 'rejectedFM',
                      'ringRejected', 'sfxUnderruns32', 'lateB028', 'batchB03A')
    bad = [{'wall_seconds': row.get('wall_seconds'), 'field': field, 'value': row.get(field)}
           for row in rows for field in history_fields if row.get(field) != 0]
    gates['observed_history_has_no_errors'] = bool(rows) and not bad
    history_elapsed = [row.get('SCSPelapsed') for row in rows] + [elapsed]
    gates['observed_clock_did_not_reset'] = (bool(rows) and all(type(n) is int for n in history_elapsed)
                                           and all(a <= z for a, z in zip(history_elapsed, history_elapsed[1:])))
    evidence['history'] = {'active_samples': len(rows), 'error_observations': len(bad),
                           'first_errors': bad[:16], 'scope': 'Observed samples; no claim about unobserved intervals.'}

    gates['native_f5_and_linked_elf_evidence_complete'] = False
    try:
        states = list((capture / 'mednafen-home/mcs').glob('*.mc0'))
        if len(states) != 1:
            raise ValueError('Require exactly one actual native F5 state')
        metadata, sections = read_state(states[0])
        if metadata['version_numeric'] != 1061377:
            raise ValueError('Unsupported native state version')
        claimed_state = report.get('native_state', {})
        if claimed_state.get('file_sha256') != metadata['file_sha256']:
            raise ValueError('Native F5 state differs from the captured identity')
        byte_order = metadata['data_byte_order']
        scsp = saturn_order(sections['SCSP']['RAM'], byte_order)
        high = saturn_order(sections['MAIN']['WorkRAMH'], byte_order)
        low = saturn_order(sections['MAIN']['WorkRAML'], byte_order)
        if len(scsp) != 512 * 1024 or len(high) != 1024 * 1024 or len(low) != 1024 * 1024:
            raise ValueError('Unexpected native RAM span')
        for name, data in (('SCSP-RAM', scsp), ('WorkRAMH', high), ('WorkRAML', low)):
            if (capture / (name + '-saturn-order.bin')).read_bytes() != data:
                raise ValueError('RAM dump differs from native F5: ' + name)

        def memory(address, size):
            physical = address & 0x1fffffff
            if 0x06000000 <= physical and physical + size <= 0x06100000:
                return span(high, physical - 0x06000000, size)
            if 0x00200000 <= physical and physical + size <= 0x00300000:
                return span(low, physical - 0x00200000, size)
            raise ValueError('Linked diagnostic/function is outside captured Work RAM')

        firmware = read_elf(build / 'SCSP.ELF', 4)
        sh2 = read_elf(build / 'AUDIO2.ELF', 42)
        firmware_start = sh2['symbols']['_audio2_firmware']['value']
        firmware_end = sh2['symbols']['_audio2_firmware_end']['value']
        gates['embedded_firmware_matches_elf'] = (elf_bytes(sh2, firmware_start, firmware_end - firmware_start)
                                                 == firmware['data'])
        segments = [{'address': p[2], 'bytes': p[4], 'matches': span(scsp, p[2], p[4])
                     == span(firmware['data'], p[1], p[4])} for p in firmware['loads']]
        gates['all_loaded_firmware_bytes_match'] = bool(segments) and all(p['matches'] for p in segments)
        evidence['firmware'] = {'sha256': sha(build / 'SCSP.ELF'), 'loaded_segments': segments}
        # Output sections also hold mutable data. Compare actual function spans,
        # not whole .wf_hotcode/.uncached sections or just the first 512 bytes.
        functions = sh2['functions']
        mismatches = [name for name, address, expected in functions if memory(address, len(expected)) != expected]
        gates['all_linked_sh2_function_bytes_match'] = bool(functions) and not mismatches
        evidence['sh2_functions'] = {'count': len(functions), 'bytes': sum(len(f[2]) for f in functions),
                                     'mismatches': mismatches}
        diag_symbol = sh2['symbols']['_audio2_diag']
        diag_address = diag_symbol['value']
        if diag_symbol['size'] < len(FIELDS) * 4:
            raise ValueError('Linked diagnostic mirror is too small')
        actual_diag = dict(zip(FIELDS, struct.unpack('>' + 'I' * len(FIELDS), memory(diag_address, len(FIELDS) * 4))))
        gates['diagnostic_mirror_matches_f5_and_elf'] = (diag_address == b['diag_address']
            and actual_diag == d and actual_diag['diagAddress'] == diag_address)
        engine = firmware['symbols']['wf_sfx_engine']
        if engine['kind'] != 1 or engine['size'] < SFX_UNDERRUNS_OFFSET + 4:
            raise ValueError('No defined native SFX engine object with the asserted counter layout')
        direct = {'sfxUnderruns32': int.from_bytes(span(scsp, engine['value'] + SFX_UNDERRUNS_OFFSET, 4), 'big'),
                  'lateB028': int.from_bytes(span(scsp, 0xb028, 2), 'big'),
                  'batchB03A': int.from_bytes(span(scsp, 0xb03a, 2), 'big'),
                  'mailState': int.from_bytes(span(scsp, 0xc046, 2), 'big')}
        for name, symbol_name in COUNTERS.items():
            symbol = sh2['symbols'][symbol_name]
            if symbol['size'] != 4:
                raise ValueError('Unexpected native transport counter layout')
            direct[name] = int.from_bytes(memory(symbol['value'], 4), 'big')
        gates['no_actual_native_counter_errors'] = all(value == (2 if name == 'mailState' else 0)
                                                      for name, value in direct.items())
        evidence['native_counters'] = direct
        evidence['sfx_engine_address_from_elf'] = engine['value']
        evidence['sfx_underruns_offset_asserted_by_harness'] = SFX_UNDERRUNS_OFFSET
        evidence['native_state'] = metadata
        gates['native_f5_and_linked_elf_evidence_complete'] = True
    except (OSError, ValueError, KeyError, TypeError, struct.error) as error:
        errors.append({'stage': 'native_memory_and_elf', 'error': str(error)})

    if b.get('replay'):
        gates['all_replay_words_accepted']=d.get('replayAccepted')==b['replay']['events'] and d.get('replayExpected')==b['replay']['events']
    return {'gate_version': 2, 'audit_tool_sha256': sha(Path(__file__)),
            'audit_dependencies': {name: sha(Path(__file__).with_name(name))
                                   for name in ('analyze_audio_capture.py', 'capture_mednafen.py')}, 'build': b,
            'gates': gates, 'diagnostics': d, 'evidence': evidence, 'evidence_errors': errors,
            'capture_subprocess_exit_code': capture_exit_code, 'passed': not errors and all(gates.values()),
            'input_report_sha256': sha(capture / 'report.json') if (capture / 'report.json').is_file() else None,
            'scope': 'One isolated native capture; not gameplay, waveform fidelity, complete replay timing or hardware certification.',
            'hardware_status': 'HARDWARE PENDING'}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for key in ('build', 'out'):
        p.add_argument('--' + key, type=Path, required=True)
    for key in ('environment-root', 'bios'):
        p.add_argument('--' + key, type=Path)
    p.add_argument('--display', type=int, default=86)
    p.add_argument('--wall-seconds', type=float, default=90)
    p.add_argument('--audio-only', action='store_true')
    p.add_argument('--presentation-speed', type=int, choices=(1, 4), default=1)
    p.add_argument('--audit-only', action='store_true', help='Read an existing capture; never run or modify it')
    p.add_argument('--audit-output', type=Path, help='New audit JSON outside the build/capture directories')
    a = p.parse_args()
    if a.audit_output and not a.audit_only:
        p.error('--audit-output requires --audit-only')
    if not a.audit_only and (a.environment_root is None or a.bios is None):
        p.error('--environment-root and --bios are required for a new capture')
    if a.audit_output:
        for protected in (a.build.resolve(), a.out.resolve()):
            if a.audit_output.resolve().is_relative_to(protected):
                p.error('--audit-output must be outside the existing capture and build')
        if a.audit_output.exists():
            p.error('--audit-output must name a new file; existing evidence is never overwritten')
    capture_exit = None
    if not a.audit_only:
        b = json.loads((a.build / 'saturn-build.json').read_text())
        command = [sys.executable, str(Path(__file__).with_name('capture_mednafen.py')),
            '--environment-root', str(a.environment_root), '--cue', str(a.build / 'AUDIO2.cue'),
            '--bios', str(a.bios), '--out', str(a.out), '--seconds', str(a.wall_seconds),
            '--display', str(a.display), '--dump-wram', '--ready-watch', 'magic',
            '--ready-value', '0x57464132', '--stop-watch', 'state', '--stop-value', '4',
            '--presentation-speed', str(a.presentation_speed)]
        if a.audio_only:
            command += ['--audio-only']
        for i, name in enumerate(FIELDS):
            command += ['--watch', f'{name}:{b["diag_address"] + i * 4}:4']
        capture_exit = subprocess.run(command, check=False).returncode
    result = audit_capture(a.build, a.out, capture_exit)
    output = a.audit_output if a.audit_only else a.out / 'native-gates.json'
    if output is not None:
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open('x') as destination:
            destination.write(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    return 0 if result['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
