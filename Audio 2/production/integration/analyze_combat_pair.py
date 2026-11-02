#!/usr/bin/env python3
"""Analyze immutable native TAG or Royal captures; never writes emulator memory.

Requires the build's ELF symbol manifests, cross-compiled layout constants,
native states/RAM dumps, and unmodified native MOV/WAV captures. This proves
only the recorded integration path, not music fidelity or real hardware.
"""
import argparse
from collections import Counter
from fractions import Fraction
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import subprocess


def digest(data):
    return hashlib.sha256(data).hexdigest()


def file_digest(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def fields(data, names):
    return dict(zip(names, struct.unpack('>' + 'I' * len(names), data)))


def elf_loads(path):
    data = Path(path).read_bytes()
    if data[:6] != b'\x7fELF\x01\x02':
        raise ValueError('Expected ELF32 big endian')
    at = struct.unpack_from('>I', data, 28)[0]
    stride, count = struct.unpack_from('>HH', data, 42)
    for i in range(count):
        p = struct.unpack_from('>8I', data, at + i * stride)
        if p[0] == 1:
            yield p, data[p[1]:p[1] + p[4]]


def elf_sections(path, machine):
    """Read the linked ELF itself; detached symbol JSON is never authority."""
    data = Path(path).read_bytes()
    if (len(data) < 52 or data[:7] != b'\x7fELF\x01\x02\x01' or
            struct.unpack_from('>HHI', data, 16) != (2, machine, 1)):
        raise ValueError('Expected a linked native ELF32 big-endian executable')
    table = struct.unpack_from('>I', data, 32)[0]
    stride, count = struct.unpack_from('>HH', data, 46)
    if stride != 40 or not count or table + stride * count > len(data):
        raise ValueError('Missing or truncated native ELF sections')
    sections = [struct.unpack_from('>10I', data, table + i * stride) for i in range(count)]
    for section in sections:
        if section[1] != 8 and section[4] + section[5] > len(data):
            raise ValueError('Native ELF section exceeds file bounds')
    return data, sections


def verify_symbols(path, machine, sidecar, required):
    data, sections = elf_sections(path, machine)
    found = {name: set() for name in required}
    for section in sections:
        if section[1] != 2:
            continue
        if section[9] != 16 or section[5] % 16 or section[6] >= len(sections):
            raise ValueError('Invalid native ELF symbol table')
        strings = sections[section[6]]
        if strings[1] != 3:
            raise ValueError('ELF symbol names are not in a string table')
        names = data[strings[4]:strings[4] + strings[5]]
        for at in range(section[4], section[4] + section[5], 16):
            name_at, value, _, info, _, index = struct.unpack_from('>IIIBBH', data, at)
            end = names.find(b'\0', name_at)
            if name_at >= len(names) or end < name_at:
                raise ValueError('Invalid ELF symbol name')
            name = names[name_at:end].decode('ascii')
            # Include local/LTO symbols when explicitly requested. Reject an
            # ambiguous same-name definition rather than choosing one by order.
            if name in found and index and info & 15 not in (3, 4):
                found[name].add(value)
    verified = {}
    for name, values in found.items():
        if len(values) != 1:
            raise ValueError('Missing or ambiguous native ELF symbol: ' + name)
        value = next(iter(values))
        if type(sidecar.get(name)) is not int or sidecar[name] != value:
            raise ValueError('Symbol sidecar disagrees with actual ELF: ' + name)
        verified[name] = value
    return verified, {'elf': str(path), 'elf_sha256': digest(data),
                      'verified_values': verified, 'verified': True}


def immutable_master_sections(path):
    data, sections = elf_sections(path, 42)
    # Writable executable sections can also hold live shared data. Only
    # allocated, nonwritable executable bytes are immutable at section scope.
    return [(s[3], data[s[4]:s[4] + s[5]]) for s in sections
            if s[1] == 1 and s[5] and s[2] & 6 == 6 and not s[2] & 1]


def comparable_audio_work(audio):
    """Keep worker survival separate from delivered work and native integrity."""
    diag = audio.get('diagnostics', {})
    native = audio.get('native_audio_counters', {})
    operational = (diag.get('enabled') == 1 and diag.get('boot_success') == 1 and
                   diag.get('api_state') == 2 and diag.get('worker_state') == 2 and
                   diag.get('api_error') == 0 and diag.get('worker_error') == 0 and
                   diag.get('first_audio_fault_vblank') == 0)
    produced = diag.get('produced')
    requested = (type(produced) is int and produced >= 0 and
                 produced == len(audio.get('producer_events', [])) and
                 diag.get('accepted') == produced and diag.get('queue_completed') == produced and
                 all(diag.get(name) == 0 for name in
                     ('queue_used', 'queue_overflows', 'unsupported', 'skipped')))
    zero = all(native.get(name) == 0 for name in
               ('fm_discarded_events', 'fm_discarded_batches', 'fm_rejected_events',
                'ring_rejected_events', 'sfx_underruns_32bit', 'sfx_fault',
                'transport_late_16bit', 'invalid_fm_batch_16bit', 'music_shared_error'))
    zero = (zero and native.get('sfx_bound') == 1 and native.get('mail_state') == 2 and
            native.get('music_shared_state') == 2)
    immutable = all(isinstance(audio.get(name), list) and bool(audio[name]) and
                    all(item.get('loaded_bytes_match') is True for item in audio[name])
                    for name in ('loaded_master_executable_sections',
                                 'loaded_module_segments', 'loaded_firmware_segments'))
    return {'audio_remained_operational': operational,
            'requested_work_integrity': requested,
            'zero_loss_and_deadlines': zero,
            'immutable_code_verified': immutable}


def percentile(histogram, p):
    target = math.ceil(sum(histogram) * p / 100)
    seen = 0
    for gap, count in enumerate(histogram):
        seen += count
        if seen >= target:
            return gap
    raise ValueError('Empty cadence histogram')


def recording_clock(capture):
    # Native quicktime video packet durations are measured in recorded audio
    # samples. The rounded metadata r_frame_rate is deliberately not used.
    cache = capture / 'video-packet-timestamps.json'
    if cache.exists():
        data = load(cache)
    else:
        command = ['ffprobe', '-v', 'error', '-select_streams', 'v:0',
                   '-show_entries', 'stream=time_base,avg_frame_rate,nb_frames',
                   '-show_entries', 'packet=pts,duration', '-of', 'json',
                   str(capture / 'native.mov')]
        data = json.loads(subprocess.check_output(command))
        cache.write_text(json.dumps(data, indent=2) + '\n')
    packets = data['packets']
    if len(packets) < 1001:
        raise ValueError('Need 1000 stable game video intervals')
    # Exclude the final packet, which may be truncated on capture shutdown.
    tail = packets[-1001:-1]
    ticks = sum(int(p['duration']) for p in tail)
    timebase = Fraction(data['streams'][0]['time_base'])
    hz = float(len(tail) / (ticks * timebase))
    return {'method': 'last 1000 complete native video packet durations',
            'time_base': str(timebase), 'duration_ticks': ticks,
            'packet_count': len(tail), 'video_hz': hz,
            'duration_histogram_ticks': dict(Counter(int(p['duration']) for p in tail)),
            'timestamp_file_sha256': file_digest(cache)}


def inspect(profile, a, read_state, master, module, build, layout, sfx_layout):
    capture = a.build / (profile + '-fullmatch')
    capture_report = load(capture / 'report.json')
    if capture_report['stop_reason'] != 'watch_threshold_reached':
        raise ValueError('Capture did not reach the requested logical endpoint')
    if capture_report['emulator'] != 'standalone Mednafen 1.32.1':
        raise ValueError('Different emulator/version')
    if capture_report['frame_skip']:
        raise ValueError('Frame skipping is not a valid comparison')
    wram = (capture / 'WorkRAMH-saturn-order.bin').read_bytes()
    artifact = build.get('artifact_basename', 'AUDIO2TAG')
    profile_elf = a.build / 'profiles' / profile / (artifact + '.ELF')
    if file_digest(profile_elf) != build['pairs'][profile][artifact + '.ELF']:
        raise ValueError('Actual master ELF differs from build identity')
    if capture_report.get('cue_sha256') != build['pairs'][profile][artifact + '.cue']:
        raise ValueError('Capture CUE differs from build identity')
    captured_discs = capture_report.get('disc_images', [])
    if (len(captured_discs) != 1 or
            Path(captured_discs[0]['path']).name != artifact + '.iso' or
            captured_discs[0]['sha256'] != build['pairs'][profile][artifact + '.iso']):
        raise ValueError('Captured disc does not match the built profile')

    def memory(symbol, length):
        offset = (master[symbol] & 0x1fffffff) - 0x06000000
        assert 0 <= offset <= len(wram) - length
        return wram[offset:offset + length]

    runtime = memory('_rt', layout['runtime_bytes'])
    frame = int.from_bytes(runtime[layout['frame_offset']:layout['frame_offset'] + 4], 'big')
    actors = runtime[layout['actors_offset']:layout['actors_offset'] + layout['actor_bytes'] * layout['actor_slots']]
    cadence_words = struct.unpack('>43I', memory('_wf_audio2_cadence', 43 * 4))
    cadence = dict(zip(('initialized', 'first_frame', 'first_vblank', 'last_frame',
                       'last_vblank', 'logical_steps', 'elapsed_vblanks', 'updates',
                       'unobserved_steps', 'max_gap', 'regressions'), cadence_words[:11]))
    histogram = cadence_words[11:]
    if cadence['unobserved_steps'] or cadence['regressions'] or histogram[-1]:
        raise ValueError('Cannot report precise per-logical-step percentiles')
    if (sum(histogram) != cadence['logical_steps'] or
            sum(i * n for i, n in enumerate(histogram)) != cadence['elapsed_vblanks'] or
            cadence['last_frame'] != frame):
        raise ValueError('Cadence counters are inconsistent')
    cadence['gap_histogram_vblanks'] = {i: n for i, n in enumerate(histogram) if n}
    cadence['gap_percentile_vblanks'] = {str(p): percentile(histogram, p) for p in (50, 95, 99, 100)}
    clock = recording_clock(capture)
    cadence['elapsed_seconds'] = cadence['elapsed_vblanks'] / clock['video_hz']
    cadence['logical_steps_per_second'] = cadence['logical_steps'] / cadence['elapsed_seconds']
    cadence['gap_percentile_ms'] = {p: n * 1000 / clock['video_hz']
                                    for p, n in cadence['gap_percentile_vblanks'].items()}
    diag_names = ('enabled', 'boot_stage', 'api_state', 'api_error', 'worker_state',
                  'worker_error', 'accepted', 'queue_used', 'queue_completed',
                  'queue_overflows', 'unsupported', 'skipped', 'produced',
                  'command_hash', 'first_audio_fault_vblank', 'max_mail_latency_nominal_us',
                  'reserved16', 'reserved17', 'queue_peak', 'last_raw', 'boot_success',
                  'pack_bytes', 'module_bytes', 'last_vblank')
    diag = fields(memory('_wf_audio2_integration_diag', 96), diag_names)
    if diag['produced'] > 512:
        raise ValueError('Audio trace capacity exceeded; hash alone is insufficient')
    events = [dict(zip(('frame', 'raw_word', 'reserved'), struct.unpack_from('>IHH',
              memory('_wf_audio2_game_trace', 4096), i * 8))) for i in range(diag['produced'])]
    actors_decoded = []
    # wf_actor_raw is the immutable ROM field layout in wf_actor.h.
    for i in range(layout['actor_slots']):
        raw = actors[i * layout['actor_bytes']:(i + 1) * layout['actor_bytes']]
        actors_decoded.append({'slot': i, 'active': raw[0], 'character': raw[2],
                              'role': raw[0x56], 'state': int.from_bytes(raw[0x20:0x22], 'big')})
    result = {'profile': profile, 'game_frame': frame, 'cadence': cadence,
              'recording_clock': clock, 'runtime_sha256': digest(runtime),
              'actors_sha256': digest(actors), 'actors': actors_decoded,
              'diagnostics': diag, 'producer_events': events,
              'capture_report_sha256': file_digest(capture / 'report.json'),
              'raw_wav_sha256': file_digest(capture / 'native.wav'),
              'capture_final_watches': capture_report['final_watch_values'],
              'emulator_sha256': capture_report['emulator_sha256'],
              'bios_sha256': capture_report['bios_sha256']}
    master_checks = []
    for address, expected in immutable_master_sections(profile_elf):
        offset = (address & 0x1fffffff) - 0x06000000
        if not 0 <= offset <= len(wram) - len(expected):
            raise ValueError('Immutable master executable section is outside captured HWRAM')
        actual = wram[offset:offset + len(expected)]
        master_checks.append({'address': address, 'bytes': len(expected),
                              'sha256': digest(expected), 'loaded_bytes_match': actual == expected})
    result['loaded_master_executable_sections'] = master_checks
    if profile == 'audio2':
        metadata, sections = read_state(capture_report['native_state']['path'])
        vram, vdp2, scsp = sections['VDP1']['VRAM'], sections['VDP2']['VRAM'], sections['SCSP']['RAM']
        if metadata['data_byte_order'] == 'little':
            vram = b''.join(vram[i:i+2][::-1] for i in range(0, len(vram), 2))
            vdp2 = b''.join(vdp2[i:i+2][::-1] for i in range(0, len(vdp2), 2))
            scsp = b''.join(scsp[i:i+2][::-1] for i in range(0, len(scsp), 2))
        def native_memory(address, length):
            address &= 0x1fffffff
            for base, data in ((0x05c00000, vram), (0x05e00000, vdp2), (0x06000000, wram)):
                if base <= address and address + length <= base + len(data):
                    return data[address-base:address-base+length]
            raise ValueError(f'Unmapped native audio address {address:#x}, length {length}')
        checks = []
        for segment, expected in elf_loads(a.build / 'cd/MUSIC4.ELF'):
            if not expected:
                continue
            address = segment[2] & 0x1fffffff
            actual = native_memory(address, len(expected))
            checks.append({'address': address, 'bytes': len(expected),
                           'sha256': digest(expected), 'loaded_bytes_match': actual == expected})
        result['loaded_module_segments'] = checks
        firmware_loads = list(elf_loads(a.firmware))
        first = next(payload for segment, payload in firmware_loads if segment[2] == 0)
        if scsp[:512] != first[:512]:
            raise ValueError('Firmware identity mismatch in native SCSP RAM')
        result['loaded_firmware_segments'] = [
            {'address': segment[2], 'bytes': len(payload), 'sha256': digest(payload),
             'loaded_bytes_match': scsp[segment[2]:segment[2]+len(payload)] == payload}
            for segment, payload in firmware_loads if payload]
        base = a.verified_firmware_symbols['wf_sfx_engine']
        counters = {}
        for symbol in ('_wf_music_fm_discarded_events', '_wf_music_fm_discarded_batches',
                       '_wf_music_fm_rejected_events', '_wf_music_ring_rejected_events'):
            counters[symbol.removeprefix('_wf_music_')] = int.from_bytes(native_memory(module[symbol], 4), 'big')
        for name in ('underruns', 'frames', 'ticks'):
            at = base + sfx_layout[name]
            counters['sfx_' + name + '_32bit'] = int.from_bytes(scsp[at:at+4], 'big')
        for name in ('bound', 'fault'):
            counters['sfx_' + name] = scsp[base + sfx_layout[name]]
        counters['transport_late_16bit'] = int.from_bytes(scsp[0xb028:0xb02a], 'big')
        counters['invalid_fm_batch_16bit'] = int.from_bytes(scsp[0xb03a:0xb03c], 'big')
        counters['last_poll_gap_over_512_fs'] = int.from_bytes(scsp[0xb034:0xb036], 'big')
        counters['playing_mask_at_last_long_poll'] = int.from_bytes(scsp[0xb036:0xb038], 'big')
        counters['mail_state'] = int.from_bytes(scsp[0xc046:0xc048], 'big')
        shared = build.get('music_memory_profile', {}).get('shared', 0x05c7f000)
        counters['music_shared_state'] = int.from_bytes(native_memory(shared+132, 2), 'big')
        counters['music_shared_error'] = int.from_bytes(native_memory(shared+136, 4), 'big')
        result['native_audio_counters'] = counters
        if build.get('scenario') == 'royal':
            result['royal_memory_observation'] = {
                'vdp2_gap_after_module_sha256': digest(native_memory(0x05e3f000, 4094)),
                'vdp2_back_color_word': int.from_bytes(native_memory(0x05e3fffe, 2), 'big'),
                'vdp1_unused_tail_after_hot_sha256': digest(native_memory(0x05c7c820, 0x37e0)),
                'note': 'End-state observation; no fabricated canary or before/after overwrite proof.'}
        low = build['slave_stack_bottom'] - 0x06000000
        high = build['slave_stack_top'] - 0x06000000
        canary = wram[low:high]
        first_used = next((i for i, byte in enumerate(canary) if byte != 0xa5), len(canary))
        result['slave_stack_canary'] = {'reserved_bytes': len(canary),
                                        'lowest_untouched_prefix_bytes': first_used,
                                        'observed_modified_span_from_top': len(canary) - first_used,
                                        'note': 'Observed high-water evidence only; canary is not a complete overflow proof.'}
    return result, runtime, actors, events


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('build', 'game-layout', 'sfx-layout', 'firmware', 'firmware-symbols',
                 'capture-analyzer', 'out'):
        p.add_argument('--' + name, required=True, type=Path)
    inputs=p.add_mutually_exclusive_group(required=True)
    inputs.add_argument('--input-tape', type=Path, help='Actual TAG logical-frame input fixture')
    inputs.add_argument('--input-policy', type=Path, help='Unchanged Royal HW1 ordinary-input policy source; not a recorded input tape')
    a = p.parse_args()
    spec = importlib.util.spec_from_file_location('audio_capture_reader', a.capture_analyzer)
    capture_reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture_reader)
    build = load(a.build / 'integration-build.json')
    royal = build.get('scenario') == 'royal'
    if royal != bool(a.input_policy):
        raise ValueError('Royal requires its actual input policy; TAG requires its recorded input tape')
    if file_digest(a.firmware) != build['firmware_sha256']:
        raise ValueError('Requested firmware is not the firmware recorded in this build')
    artifact = build.get('artifact_basename', 'AUDIO2TAG')
    master_names = ('_rt', '_wf_audio2_cadence', '_wf_audio2_integration_diag',
                    '_wf_audio2_game_trace', '___audio2_hot_start', '___audio2_hot_end',
                    '___audio2_slave_stack_bottom', '___audio2_slave_stack_top', '___end')
    module_names = ('_wf_music_fm_discarded_events', '_wf_music_fm_discarded_batches',
                    '_wf_music_fm_rejected_events', '_wf_music_ring_rejected_events')
    symbol_proof = {}
    master_sidecar = load(a.build / 'master-symbols.json')
    for profile in ('control', 'audio2'):
        master, symbol_proof[profile] = verify_symbols(
            a.build / 'profiles' / profile / (artifact + '.ELF'), 42, master_sidecar, master_names)
    for key, symbol in (('hot_start', '___audio2_hot_start'), ('hot_end', '___audio2_hot_end'),
                        ('slave_stack_bottom', '___audio2_slave_stack_bottom'),
                        ('slave_stack_top', '___audio2_slave_stack_top'), ('user_heap_start', '___end')):
        if build[key] != master[symbol]:
            raise ValueError('Memory ownership manifest disagrees with actual master ELF: ' + key)
    module_debug = a.build / 'module/MUSIC4.DEBUG.ELF'
    module, symbol_proof['module'] = verify_symbols(
        module_debug, 42, load(a.build / 'module-symbols.json'), module_names)
    # Packing changes file offsets and p_align, never destination, size, flags
    # or any payload byte. Bind counter symbols from DEBUG to the loaded file.
    def load_signature(path):
        return [(segment[2:7], digest(payload)) for segment, payload in elf_loads(path)]
    if load_signature(module_debug) != load_signature(a.build / 'cd/MUSIC4.ELF'):
        raise ValueError('Symbol-bearing module differs from packed load payloads')
    a.verified_firmware_symbols, symbol_proof['firmware'] = verify_symbols(
        a.firmware, 4, load(a.firmware_symbols), ('wf_sfx_engine',))
    layout = fields(a.game_layout.read_bytes(), ('actors_offset', 'actor_bytes', 'actor_slots',
                    'frame_offset', 'mode_offset', 'progression_offset', 'runtime_bytes'))
    sfx_layout = fields(a.sfx_layout.read_bytes(), ('sfx_size', 'underruns', 'frames', 'ticks',
                        'bound', 'fault', 'pcm', 'pcm_size', 'pcm_playing', 'pcm_write_at', 'pcm_played'))
    profiles = {name: inspect(name, a, capture_reader.read_state, master, module, build, layout, sfx_layout)
                for name in ('control', 'audio2')}
    control, audio = profiles['control'][0], profiles['audio2'][0]
    cv, av = control['cadence']['elapsed_vblanks'], audio['cadence']['elapsed_vblanks']
    integrity = comparable_audio_work(audio)
    integrity['immutable_code_verified'] = (integrity['immutable_code_verified'] and
        bool(control['loaded_master_executable_sections']) and
        all(x['loaded_bytes_match'] for x in control['loaded_master_executable_sections']))
    endpoint = build.get('expected_gameplay_endpoint', 8474 if royal else 7372)
    full_endpoint = all(p['game_frame'] == endpoint and p['cadence']['logical_steps'] == endpoint
                        for p in (control, audio))
    same_work = all(profiles['control'][i] == profiles['audio2'][i] for i in (1, 2, 3))
    same_machine = all(control[k] == audio[k] for k in ('emulator_sha256', 'bios_sha256'))
    comparable = all(integrity.values()) and full_endpoint and same_work and same_machine
    result = {'analysis_schema_version': 2,
              'analyzer_sha256': file_digest(__file__),
              'scope': 'frozen performance/integration evidence; no fidelity acceptance',
              'input_tape_sha256': file_digest(a.input_tape) if a.input_tape else None,
              'input_policy_sha256': file_digest(a.input_policy) if a.input_policy else None,
              'input_evidence': ('Same original state-dependent Royal policy and released physical controls. Complete per-call input bytes were not recorded; endpoint/event equality is checked separately.'
                                 if royal else 'Same immutable logical-frame TAG input tape.'),
              'game_layout': layout,
              'symbol_verification': symbol_proof,
              'immutable_code_scope': 'Nonwritable allocated master executable sections, complete module file payloads, and complete firmware file payloads; mutable shared state is excluded.',
              'firmware_sha256': file_digest(a.firmware), 'build': build,
              'profiles': {name: data[0] for name, data in profiles.items()},
              'pair': {'runtime_byte_identical': profiles['control'][1] == profiles['audio2'][1],
                       **integrity,
                       'requested_logical_endpoint_reached': full_endpoint,
                       'full_match_audio_cost_comparable': comparable,
                       'actors_byte_identical': profiles['control'][2] == profiles['audio2'][2],
                       'producer_events_identical': profiles['control'][3] == profiles['audio2'][3],
                       'emulator_identical': control['emulator_sha256'] == audio['emulator_sha256'],
                       'bios_identical': control['bios_sha256'] == audio['bios_sha256'],
                       'logical_endpoint_identical': control['game_frame'] == audio['game_frame'],
                       'additional_combat_vblanks': av - cv,
                       'additional_combat_time_percent': 100 * (av - cv) / cv,
                       'logical_throughput_change_percent': 100 * (cv / av - 1)},
              'timing_note': 'Combat window uses cadence first/last VBlank, excluding boot. Rates remain observed facts when full_match_audio_cost_comparable is false, but cannot certify the cost of complete, zero-loss, on-time audio work. API worker survival alone is insufficient. If audio_remained_operational is false, the mean also includes time after the worker failed. API mail latency uses16667 nominal microseconds per VBlank and is quantized, not precise FM/DAC jitter.'}
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'pair': result['pair'], 'profiles': {name: {'cadence': data['cadence'],
          'diagnostics': data['diagnostics'], 'audio_counters': data.get('native_audio_counters')}
          for name, data in result['profiles'].items()}}, indent=2))


if __name__ == '__main__':
    main()
