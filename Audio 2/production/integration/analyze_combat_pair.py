#!/usr/bin/env python3
"""Analyze two immutable native TAG captures; never writes emulator memory.

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
    if profile == 'audio2':
        metadata, sections = read_state(capture_report['native_state']['path'])
        vram, scsp = sections['VDP1']['VRAM'], sections['SCSP']['RAM']
        if metadata['data_byte_order'] == 'little':
            vram = b''.join(vram[i:i+2][::-1] for i in range(0, len(vram), 2))
            scsp = b''.join(scsp[i:i+2][::-1] for i in range(0, len(scsp), 2))
        checks = []
        for segment, expected in elf_loads(a.build / 'cd/MUSIC4.ELF'):
            if not expected:
                continue
            address = segment[2] & 0x1fffffff
            if 0x05c00000 <= address < 0x05c80000:
                actual = vram[address - 0x05c00000:address - 0x05c00000 + len(expected)]
            elif 0x06000000 <= address < 0x06100000:
                actual = wram[address - 0x06000000:address - 0x06000000 + len(expected)]
            else:
                raise ValueError('Unexpected module segment')
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
        fsymbols = load(a.firmware_symbols)
        base = fsymbols['wf_sfx_engine']
        counters = {}
        for symbol in ('_wf_music_fm_discarded_events', '_wf_music_fm_discarded_batches',
                       '_wf_music_fm_rejected_events', '_wf_music_ring_rejected_events'):
            at = module[symbol] - 0x05c00000
            counters[symbol.removeprefix('_wf_music_')] = int.from_bytes(vram[at:at+4], 'big')
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
        counters['music_shared_state'] = int.from_bytes(vram[0x7f084:0x7f086], 'big')
        counters['music_shared_error'] = int.from_bytes(vram[0x7f088:0x7f08c], 'big')
        result['native_audio_counters'] = counters
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
                 'capture-analyzer', 'input-tape', 'out'):
        p.add_argument('--' + name, required=True, type=Path)
    a = p.parse_args()
    spec = importlib.util.spec_from_file_location('audio_capture_reader', a.capture_analyzer)
    capture_reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture_reader)
    master, module = load(a.build / 'master-symbols.json'), load(a.build / 'module-symbols.json')
    build = load(a.build / 'integration-build.json')
    if file_digest(a.firmware) != build['firmware_sha256']:
        raise ValueError('Requested firmware is not the firmware recorded in this build')
    layout = fields(a.game_layout.read_bytes(), ('actors_offset', 'actor_bytes', 'actor_slots',
                    'frame_offset', 'mode_offset', 'progression_offset', 'runtime_bytes'))
    sfx_layout = fields(a.sfx_layout.read_bytes(), ('sfx_size', 'underruns', 'frames', 'ticks',
                        'bound', 'fault', 'pcm', 'pcm_size', 'pcm_playing', 'pcm_write_at', 'pcm_played'))
    profiles = {name: inspect(name, a, capture_reader.read_state, master, module, build, layout, sfx_layout)
                for name in ('control', 'audio2')}
    control, audio = profiles['control'][0], profiles['audio2'][0]
    cv, av = control['cadence']['elapsed_vblanks'], audio['cadence']['elapsed_vblanks']
    result = {'scope': 'frozen performance/integration evidence; no fidelity acceptance',
              'input_tape_sha256': file_digest(a.input_tape), 'game_layout': layout,
              'firmware_sha256': file_digest(a.firmware), 'build': build,
              'profiles': {name: data[0] for name, data in profiles.items()},
              'pair': {'runtime_byte_identical': profiles['control'][1] == profiles['audio2'][1],
                       'actors_byte_identical': profiles['control'][2] == profiles['audio2'][2],
                       'producer_events_identical': profiles['control'][3] == profiles['audio2'][3],
                       'emulator_identical': control['emulator_sha256'] == audio['emulator_sha256'],
                       'bios_identical': control['bios_sha256'] == audio['bios_sha256'],
                       'logical_endpoint_identical': control['game_frame'] == audio['game_frame'],
                       'additional_combat_vblanks': av - cv,
                       'additional_combat_time_percent': 100 * (av - cv) / cv,
                       'logical_throughput_change_percent': 100 * (cv / av - 1)},
              'timing_note': 'Combat window uses cadence first/last VBlank, excluding boot. API mail latency uses 16667 nominal microseconds per VBlank and is quantized, not precise jitter.'}
    a.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'pair': result['pair'], 'profiles': {name: {'cadence': data['cadence'],
          'diagnostics': data['diagnostics'], 'audio_counters': data.get('native_audio_counters')}
          for name, data in result['profiles'].items()}}, indent=2))


if __name__ == '__main__':
    main()
