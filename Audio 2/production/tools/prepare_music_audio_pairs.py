#!/usr/bin/env python3
"""Extract real native WAVs and replay the original board at observed admissions.

Default: all 18 music commands and transitions from a completed, passing
>=600-second score matrix with an >=8-second requested fade tail.
--diagnostic-only --case command-01 permits one
short or failed-runtime case, without granting collection/fidelity approval.
The native WAV remains stereo and includes its original music lead. Reference
WAVs are mono host-model output, never described as physical arcade or Saturn
recordings. No gain change, normalization, resampling or waveform fitting.

--reference-root reuses an already built reproduce_full_board_references.py
directory. The executable, source and original ROM identities are checked;
there is no extra build or audio renderer. New private output is required.
Only public-manifest.json is intended for public metadata distribution.
"""
import argparse
import array
from collections import deque
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
import csv
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import wave

HERE = Path(__file__).resolve().parent
PACKAGE = HERE.parent
VERSION = '1.2.0'
RATE = 44100
CLOCK = 3579545
SOUND_SHA = 'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7'
SAMPLES_SHA = 'e4cbe71f079e2cb95b3463115557fa1d99e18ea975f2d3354827b01994fc4615'
AUDIT_HELPERS = ('run_native_test.py', 'analyze_audio_capture.py', 'capture_mednafen.py')
NATIVE_GATES = frozenset((
    'capture_subprocess_succeeded', 'emulator_exited_successfully', 'native_capture_method',
    'normal_window_stop', 'real_module_entered', 'requested_window_completed',
    'no_reported_worker_error', 'no_reported_api_error', 'no_reported_fm_discard',
    'no_reported_rejected_events', 'no_reported_pcm_underrun', 'no_reported_transport_lateness',
    'worker_still_running', 'native_still_ready', 'api_still_running',
    'requested_scsp_duration_reached', 'build_and_capture_identities_match',
    'native_wav_finalized_and_unchanged', 'native_wav_covers_scsp_window',
    'observed_history_has_no_errors', 'observed_clock_did_not_reset',
    'native_f5_and_linked_elf_evidence_complete', 'embedded_firmware_matches_elf',
    'all_loaded_firmware_bytes_match', 'all_linked_sh2_function_bytes_match',
    'diagnostic_mirror_matches_f5_and_elf', 'no_actual_native_counter_errors',
    'all_replay_words_accepted'))
ADDITIONAL_GATES = frozenset(('firmware_first512_matches', 'native_wav_finalized',
    'selected_emulator_identity', 'no_expansion_cartridge', 'replay_words_results_and_admission_times'))
RUNTIME_FAILURE_GATES = frozenset((
    'no_reported_worker_error', 'no_reported_api_error', 'no_reported_fm_discard',
    'no_reported_rejected_events', 'no_reported_pcm_underrun', 'no_reported_transport_lateness',
    'worker_still_running', 'native_still_ready', 'api_still_running',
    'observed_history_has_no_errors', 'no_actual_native_counter_errors'))


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def dump(path, value):
    with Path(path).open('x') as stream:
        stream.write(json.dumps(value, indent=2, allow_nan=False) + '\n')


def canonical_sha(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def accepted_events(observation):
    if observation.get('passed') is not True or not all(observation.get('gates', {}).values()):
        raise ValueError('Missing, unaccepted or mismatched native command admissions')
    rows = observation.get('observations', [])
    if not rows or any(r.get('result') != 0 or r.get('recorded') is not True for r in rows):
        raise ValueError('Native admission rows are incomplete')
    events = [{'sample': r['sample'], 'word': r['word']} for r in rows]
    if events[0]['sample'] != 0 or any(a['sample'] >= b['sample'] for a, b in zip(events, events[1:])):
        raise ValueError('No invented epoch or spacing for simultaneous command admissions')
    if not any(e['word'] == 0x1f for e in events):
        raise ValueError('Original 1F admission is required for a complete fade window')
    return events


def native_window(total_frames, details, clock16, frames):
    diagnostics = details['final_watch_values']
    if diagnostics.get('magic') != 0x57464132 or diagnostics.get('state') != 4:
        raise ValueError('Native capture did not finish its requested window')
    elapsed = diagnostics.get('SCSPelapsed')
    command_clock, diagnostic_clock = diagnostics.get('commandDSPclock'), diagnostics.get('DSPclock16')
    if (any(type(v) is not int or not 0 <= v <= 0xffff for v in (clock16, command_clock, diagnostic_clock))
            or type(elapsed) is not int or not 0 <= elapsed <= 0xffffffff
            or type(frames) is not int or type(total_frames) is not int or elapsed < frames):
        raise ValueError('Missing or invalid coherent native epoch/elapsed clocks')
    # SCSPelapsed is accumulated from the same pre-call clock as commandDSPclock.
    # DSPclock16 is read later in that iteration; combining it with elapsed would
    # move the origin by the time between those two diagnostic reads.
    elapsed_clock = (command_clock + elapsed) & 0xffff
    residual = (clock16 - elapsed_clock) & 0xffff
    diagnostic_skew = (diagnostic_clock - elapsed_clock) & 0xffff
    if residual > 1475 or diagnostic_skew > residual:
        raise ValueError('Native elapsed clock is stale or inconsistent with the final diagnostic')
    start = total_frames - elapsed - residual
    if frames <= 0 or not 0 <= start < start + frames <= total_frames:
        raise ValueError('Native soundrecord does not cover the complete requested window')
    return {'start_frame': start, 'end_frame_exclusive': start + frames,
            'diagnostic_staleness_frames': residual,
            'elapsed_clock16': elapsed_clock, 'diagnostic_clock_skew_frames': diagnostic_skew,
            'method': 'Final SCSP clock minus coherent elapsed from the observed pre-call API clock mark',
            'admission_clock_resolution': 'SCSP clock sampled before each accepted API call by the video-frame harness; not an exact call-instant measurement',
            'native_music_lead_removed_frames': 0,
            'documented_native_music_lead_preserved_frames': 4096,
            'waveform_fitted_alignment': False, 'residual_DAC_alignment': 'Not independently calibrated'}


def wav_metadata(path):
    with wave.open(str(path)) as stream:
        if stream.getsampwidth() != 2 or stream.getframerate() != RATE or stream.getcomptype() != 'NONE':
            raise ValueError('Expected uncompressed PCM16/44100 WAV')
        digest = hashlib.sha256()
        frames, channels = stream.getnframes(), stream.getnchannels()
        expected = frames * channels * 2
        read = 0
        while chunk := stream.readframes(65536):
            digest.update(chunk)
            read += len(chunk)
        if read != expected:
            raise ValueError('Truncated WAV PCM data')
    return {'file': Path(path).name, 'sha256': sha(path), 'pcm_sha256': digest.hexdigest(),
            'frames': frames, 'sample_rate': RATE, 'bits': 16, 'channels': channels,
            'seconds': frames / RATE, 'bytes': Path(path).stat().st_size}


def copy_wav_window(source, target, start, frames):
    """Copy samples, including silence and channel layout; never synthesize native audio."""
    with wave.open(str(source)) as stream:
        if stream.getframerate() != RATE or stream.getsampwidth() != 2 or stream.getcomptype() != 'NONE':
            raise ValueError('Expected uncompressed native PCM16/44100 WAV')
        if not 0 <= start < start + frames <= stream.getnframes():
            raise ValueError('Missing requested raw WAV window')
        channels = stream.getnchannels()
        stream.setpos(start)
        digest = hashlib.sha256()
        with Path(target).open('xb') as output, wave.open(output, 'wb') as sink:
            sink.setparams((channels, 2, RATE, frames, 'NONE', 'not compressed'))
            remaining = frames
            while remaining:
                count = min(remaining, 65536)
                chunk = stream.readframes(count)
                if len(chunk) != count * channels * 2:
                    raise ValueError('Truncated native PCM')
                digest.update(chunk)
                sink.writeframesraw(chunk)
                remaining -= count
    result = wav_metadata(target)
    result['source_pcm_bytes_match'] = result['pcm_sha256'] == digest.hexdigest()
    if not result['source_pcm_bytes_match']:
        raise ValueError('Extracted WAV differs from the original native PCM window')
    return result


def pcm16be_to_wav(source, target, frames):
    """Container/endian conversion of real full-board output, with complete byte check."""
    if Path(source).stat().st_size != frames * 2:
        raise ValueError('Original-board PCM length does not match the full window')
    digest = hashlib.sha256()
    with Path(source).open('rb') as stream, Path(target).open('xb') as output, wave.open(output, 'wb') as sink:
        sink.setparams((1, 2, RATE, frames, 'NONE', 'not compressed'))
        while chunk := stream.read(131072):
            swapped = bytearray(len(chunk))
            swapped[0::2], swapped[1::2] = chunk[1::2], chunk[0::2]
            digest.update(swapped)
            sink.writeframesraw(swapped)
    result = wav_metadata(target)
    result['source_pcm_bytes_match'] = result['pcm_sha256'] == digest.hexdigest()
    if not result['source_pcm_bytes_match']:
        raise ValueError('Reference WAV differs from the captured original-board PCM')
    return result


def runtime_inventory(report):
    result = []
    for item in report.get('commands', []):
        gates = item.get('native_gates', {})
        failed = [k for k, v in gates.get('gates', {}).items() if v is not True]
        failed += [k for k, v in item.get('additional_gates', {}).items() if v is not True]
        if not native_gate_schema(gates):
            failed.append('native_gate_schema_complete')
        if not gate_map(item.get('additional_gates'), ADDITIONAL_GATES):
            failed.append('additional_gate_schema_complete')
        if gates.get('evidence_errors') != []:
            failed.append('native_evidence_errors')
        passed = (item.get('status') == 'complete' and item.get('passed_runtime_gates') is True
                  and gates.get('passed') is True and not failed)
        result.append({'id': item['id'], 'command': item['command'], 'kind': item['kind'],
                       'runtime_status': 'PASS' if passed else 'FAIL', 'failed_gates': failed,
                       'audio_availability_does_not_override_runtime': True})
    return result


def gate_map(value, expected):
    return (isinstance(value, dict) and set(value) == expected
            and all(type(v) is bool for v in value.values()))


def native_gate_schema(value):
    return (isinstance(value, dict) and type(value.get('gate_version')) is int
            and value['gate_version'] == 2 and type(value.get('passed')) is bool
            and isinstance(value.get('evidence_errors'), list)
            and gate_map(value.get('gates'), NATIVE_GATES))


def case_identity(case):
    command, kind = case.get('command'), case.get('kind')
    if type(command) is not int or not 1 <= command <= 18 or kind not in ('music', 'transitions'):
        raise ValueError('Unknown music/transition case identity')
    expected = f'command-{command:02X}' if kind == 'music' else 'transitions'
    if case.get('id') != expected or (kind == 'transitions' and command != 1):
        raise ValueError('Case IDs must be canonical names for their exact commands')
    return expected


def case_directory(parent, case):
    identity = case_identity(case)
    parent = Path(parent).resolve()
    path = (parent / identity).resolve()
    if path.parent != parent:
        raise ValueError('Case directory escapes its designated parent')
    return path


def select_cases(report, case_id, diagnostic):
    if report.get('kind') != 'LONG_NATIVE_MUSIC_FADE_MATRIX':
        raise ValueError('Need a native matrix with explicit original 1F schedules')
    if not all(report.get(k) is True for k in ('complete', 'source_stable', 'inputs_stable', 'emulator_stable')):
        raise ValueError('Need a complete immutable native matrix, including failed-case records')
    cases = report.get('cases', [])
    rows = report.get('commands', [])
    if not isinstance(cases, list) or not cases or not isinstance(rows, list):
        raise ValueError('Need complete declared cases and runtime rows')
    ids = [case_identity(c) for c in cases]
    runtime_ids = [case_identity(row) for row in rows]
    if (len(set(ids)) != len(ids) or len(set(runtime_ids)) != len(runtime_ids)
            or set(ids) != set(runtime_ids)):
        raise ValueError('Runtime inventory must cover each exact declared case once')
    by_id = {row['id']: row for row in rows}
    score, tail = report.get('score_seconds_before_1F'), report.get('tail_seconds')
    if (type(score) is not int or type(tail) is not int or score < 1 or not 1 <= tail <= 60
            or score + tail > 1800 or report.get('native_window_seconds') != score + tail):
        raise ValueError('Need exact integer score and requested fade-tail durations')
    for case in cases:
        if (set(case) != {'id', 'command', 'kind', 'seconds', 'events'}
                or any(by_id[case['id']].get(k) != case[k] for k in ('command', 'kind'))):
            raise ValueError('Runtime case mapping differs from its declared case')
        expected = ([{'sample': 0, 'word': case['command']}, {'sample': score * RATE, 'word': 0x1f}]
                    if case['kind'] == 'music' else
                    [{'sample': i * 3 * RATE, 'word': word}
                     for i, word in enumerate((1, 6, 0x12, 0x1f, 3, 0x1f, 2, 1, 0x1f))])
        seconds = score + tail if case['kind'] == 'music' else 24 + tail
        events = case['events']
        if (type(case['seconds']) is not int or case['seconds'] != seconds
                or not isinstance(events, list)
                or any(not isinstance(e, dict) or set(e) != {'sample', 'word'}
                       or type(e['sample']) is not int or type(e['word']) is not int for e in events)
                or events != expected):
            raise ValueError('Every case must contain its exact declared original schedule and fade-tail duration')
    if diagnostic:
        selected = [c for c in cases if c['id'] == case_id]
        if len(selected) != 1:
            raise ValueError('Diagnostic mode requires exactly one explicit --case')
        return selected
    inventory = runtime_inventory(report)
    if (case_id is not None or score < 600 or tail < 8 or len(cases) != 19
            or report.get('all_runtime_gates_passed') is not True
            or any(c['runtime_status'] != 'PASS' for c in inventory)
            or len(inventory) != len(cases)
            or {c['command'] for c in cases if c['kind'] == 'music'} != set(range(1, 19))
            or sum(c['kind'] == 'music' for c in cases) != 18
            or sum(c['kind'] == 'transitions' for c in cases) != 1):
        raise ValueError('Full export requires exactly 18 passing >=600-second music cases and original transitions, with >=8-second requested tails')
    return sorted(cases, key=lambda c: c['id'])


def pick(value, fields):
    return {key: value[key] for key in fields if key in value}


def hash_metadata(value):
    result = {}
    for key, digest in value.items():
        if (Path(key).is_absolute() or '..' in Path(key).parts or not isinstance(digest, str)
                or len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest)):
            raise ValueError('Public identities must be relative labels and SHA-256 values')
        result[key] = digest
    return result


def public_pair(pair):
    result = pick(pair, ('id', 'command', 'hex', 'kind', 'seconds', 'frames', 'runtime_status',
                         'raw_native_wav_sha256', 'native_capture_report_sha256', 'reference_schedule_sha256',
                         'native_gate_report_sha256', 'native_f5_sha256', 'native_audit_revalidated',
                         'transition_schedule_replayed', 'native_loop_verified', 'fidelity_approved',
                         'complete_recorded_window'))
    for field in ('requested_events', 'observed_events'):
        if field in pair:
            result[field] = [pick(event, ('sample', 'word')) for event in pair[field]]
    if 'reference_command_deliveries' in pair:
        result['reference_command_deliveries'] = [pick(event, ('word', 'admitted_native_sample',
            'reference_target_cycle', 'reference_delivery_cycle', 'reference_latch_read_cycle',
            'reference_instruction_boundary_delay_cycles')) for event in pair['reference_command_deliveries']]
    if 'native_window' in pair:
        result['native_window'] = pick(pair['native_window'], ('start_frame', 'end_frame_exclusive',
            'diagnostic_staleness_frames', 'elapsed_clock16', 'diagnostic_clock_skew_frames',
            'method', 'admission_clock_resolution', 'native_music_lead_removed_frames',
            'documented_native_music_lead_preserved_frames', 'waveform_fitted_alignment', 'residual_DAC_alignment'))
    for field in ('native_wav', 'original_host_wav'):
        if field in pair:
            result[field] = pick(pair[field], ('file', 'sha256', 'pcm_sha256', 'frames', 'sample_rate',
                'bits', 'channels', 'seconds', 'bytes', 'source_pcm_bytes_match'))
    for field in ('native_build_sha256', 'reference_capture_sha256'):
        if field in pair:
            result[field] = hash_metadata(pair[field])
    if 'fade' in pair:
        result['fade'] = {kind: pick(pair['fade'][kind], ('last_fade_admission_sample',
            'captured_after_fade_frames', 'last_nonzero_after_fade_frame', 'trailing_exact_zero_frames',
            'final_window_frames', 'final_window_peak', 'final_window_all_channels_exact_zero',
            'natural_end_or_loop_proven_by_silence')) for kind in ('native', 'original_host') if kind in pair['fade']}
    if 'original_structure' in pair:
        result['original_structure'] = (pick(pair['original_structure'], ('classification', 'global_observation',
            'natural_FB_entry_seconds', 'visible_state_interval_seconds', 'original_fade_control_clear_observed',
            'evidence_applies_to', 'native_loop_verified', 'exact_PCM_period_proven'))
            if pair['original_structure'] is not None else None)
    return result


def public_manifest(report):
    """Only preconstructed metadata fields; never copy paths, RAM, traces or PCM."""
    fields = ('schema_version', 'tool_version', 'kind', 'matrix_sha256', 'structure_evidence_sha256',
              'matrix_profile', 'complete_capture_export', 'source_stable',
              'native_runtime_passed', 'diagnostic_only', 'complete_18_music_collection', 'reference_mode',
              'reference_jobs', 'maximum_reference_cases_in_flight', 'reference_result_order',
              'reference_traces_retained',
              'normalization', 'gain', 'resampling', 'time_stretch', 'waveform_fitted_alignment',
              'native_kind', 'reference_kind', 'emulator_sha256', 'firmware_sha256')
    result = pick(report, fields)
    for key in ('source_sha256', 'matrix_source_sha256', 'matrix_input_sha256'):
        if key in report:
            result[key] = hash_metadata(report[key])
    if 'reference_identity' in report:
        reference = report['reference_identity']
        result['reference_identity'] = pick(reference, ('report_sha256', 'executable_sha256', 'opm_commit',
                                                       'external_clock_hz', 'timers', 'OKI'))
        for key in ('input_sha256', 'source_sha256'):
            result['reference_identity'][key] = hash_metadata(reference[key])
    if 'runtime_inventory' in report:
        result['runtime_inventory'] = [pick(item, ('id', 'command', 'kind', 'runtime_status',
            'failed_gates', 'audio_availability_does_not_override_runtime')) for item in report['runtime_inventory']]
    if 'pairs' in report:
        result['pairs'] = [public_pair(pair) for pair in report['pairs']]
    result.update(fidelity_approved=False, native_loop_verified=False, sample_exact_PCM_period_proven=False,
                  owner_listening_approval='PENDING', hardware_status='HARDWARE PENDING',
                  scope='Complete recorded windows at observed API admissions; original structure evidence is separate from native waveform/loop acceptance')
    return result


def tail_observation(wav, fade_sample):
    with wave.open(str(wav)) as stream:
        frames, channels = stream.getnframes(), stream.getnchannels()
        if not 0 <= fade_sample < frames:
            raise ValueError('Fade admission outside captured WAV')
        stream.setpos(fade_sample)
        data = array.array('h', stream.readframes(frames - fade_sample))
        if sys.byteorder != 'little':
            data.byteswap()
    last = next((i // channels for i in range(len(data)-1, -1, -1) if data[i]), None)
    final_frames = min(frames - fade_sample, 2 * RATE)
    final = data[-final_frames * channels:]
    return {'last_fade_admission_sample': fade_sample,
            'captured_after_fade_frames': frames - fade_sample,
            'last_nonzero_after_fade_frame': last,
            'trailing_exact_zero_frames': frames - fade_sample - (last + 1 if last is not None else 0),
            'final_window_frames': final_frames, 'final_window_peak': max(map(abs, final), default=0),
            'final_window_all_channels_exact_zero': not any(final),
            'natural_end_or_loop_proven_by_silence': False}


def validate_snapshot(report, matrix_root):
    # Match the frozen snapshot, not the developer's concurrently edited source.
    sys.path.insert(0, str(HERE))
    from source_snapshot import source_identity
    for prefix, directory in (('source', 'source'), ('input', 'inputs')):
        expected = report[prefix + '_sha256_before']
        if expected != report[prefix + '_sha256_after'] or source_identity(matrix_root / directory) != expected:
            raise ValueError('Frozen matrix ' + prefix + ' identity no longer matches')


def validate_reference(root):
    report_path = root / 'reference-report.json'
    report = load(report_path)
    if (report.get('profile') != 'full-board' or not report.get('source_stable')
            or not (report.get('complete') or report.get('complete_capture_collection'))
            or report.get('all_114_complete_pcm_busy_tail_and_bank_tests_passed') is not True):
        raise ValueError('Need a completed verified full-board reference build')
    binary = root / 'capture_board_replay'
    if sha(binary) != report['executable_sha256']:
        raise ValueError('Full-board executable identity changed')
    paths = {str(report_path): sha(report_path), str(binary): sha(binary)}
    identities = {}
    for name, expected in (('SOUND.ROM', SOUND_SHA), ('SAMPLES.ROM', SAMPLES_SHA)):
        path = root / 'inputs' / name
        if sha(path) != expected:
            raise ValueError('Wrong original input identity: ' + name)
        paths[str(path)] = expected
        identities[name] = expected
    source_identities = {}
    for source, expected in report['source_sha256'].items():
        path = Path(source)
        if '/deps/' in source:
            path = root / 'deps' / path.name
            label = 'deps/' + path.name
        else:
            suffix = next((source[source.index('/' + part) + 1:] for part in ('tools/', 'src/', 'tests/')
                           if '/' + part in source), None)
            if suffix is None:
                raise ValueError('Unknown reference source identity: ' + source)
            path, label = PACKAGE / suffix, suffix
        if sha(path) != expected:
            raise ValueError('Reference source changed: ' + label)
        paths[str(path)] = expected
        source_identities[label] = expected
    return paths, {'report_sha256': sha(report_path), 'executable_sha256': sha(binary),
                   'input_sha256': identities, 'source_sha256': source_identities,
                   'opm_commit': report['opm_commit'], 'external_clock_hz': CLOCK,
                   'timers': 'Coupled Timer A+B, busy/status/IRQ', 'OKI': 'Original ROM-addressed decoder'}


def inspect_native(matrix_root, matrix, case, item):
    sys.path.insert(0, str(HERE))
    from run_music_matrix import replay_observation
    from run_native_test import audit_capture
    work = case_directory(matrix_root, case)
    build, capture = work / 'build', work / 'capture'
    details, gates = load(capture / 'report.json'), load(capture / 'native-gates.json')
    build_report = load(build / 'saturn-build.json')
    if gates != item['native_gates'] or build_report != item['build']:
        raise ValueError('Native gate/build report differs from its matrix row')
    states = list((capture / 'mednafen-home/mcs').glob('*.mc0'))
    if len(states) != 1:
        raise ValueError('Require exactly one actual native F5 state')
    if any((build / name).resolve().parent != build.resolve() for name in build_report['files']):
        raise ValueError('Native build identity must name a direct build artifact')
    wav = capture / 'native.wav'
    files = [wav, capture / 'report.json', capture / 'native-gates.json', states[0],
             capture / 'SCSP-RAM-saturn-order.bin', capture / 'WorkRAMH-saturn-order.bin',
             capture / 'WorkRAML-saturn-order.bin', build / 'SCSP.ELF',
             build / 'symbols.json', build / 'saturn-build.json',
             *(build / name for name in build_report['files'])]
    identities = {str(p): sha(p) for p in files}
    for name in AUDIT_HELPERS:
        if sha(HERE / name) != matrix.get('source_sha256_before', {}).get('tools/' + name):
            raise ValueError('Native audit helper differs from the frozen matrix: ' + name)
    fresh = audit_capture(build, capture, gates.get('capture_subprocess_exit_code'))
    if not native_gate_schema(fresh) or fresh['evidence_errors'] or fresh != gates:
        raise ValueError('Fresh native F5/RAM/ELF audit differs from saved evidence or has evidence errors')
    if any(fresh['gates'][key] is not True for key in NATIVE_GATES - RUNTIME_FAILURE_GATES):
        raise ValueError('Native capture has a structural/provenance failure; diagnostic audio cannot override it')
    if sha(capture / 'report.json') != item['capture_report_sha256']:
        raise ValueError('Native capture report changed')
    if (details.get('emulator_sha256') != matrix['emulator_sha256']
            or details.get('cartridge') != 'none' or details.get('emulated_hardware_clock_change') is not False):
        raise ValueError('Unexpected native emulator/profile identity')
    if sha(build / 'SCSP.ELF') != matrix['firmware_sha256']:
        raise ValueError('Native firmware identity changed')
    for name, identity in build_report['files'].items():
        if sha(build / name) != identity['sha256']:
            raise ValueError('Native build identity changed: ' + name)
    requested = load(matrix_root / 'inputs/replays' / (case['id'] + '.json'))
    if requested['events'] != case['events'] or build_report['replay']['sha256'] != sha(matrix_root / 'inputs/replays' / (case['id'] + '.json')):
        raise ValueError('Requested replay differs from the frozen matrix/build')
    observed = replay_observation(build, capture, case['events'])
    if observed != item.get('replay_observation'):
        raise ValueError('Actual native RAM admissions differ from the matrix observation')
    events = accepted_events(observed)
    frames = case['seconds'] * RATE
    if (events[-1]['sample'] >= frames or build_report['seconds'] != case['seconds']
            or build_report['command'] != case['command']):
        raise ValueError('Native duration or command bounds differ from the planned window')
    info = wav_metadata(wav)
    recorded = details.get('wav_validation', {})
    if (recorded.get('finalized') is not True or recorded.get('sha256') != info['sha256']
            or any(recorded.get(k) != info[k] for k in ('frames', 'channels', 'sample_rate', 'bits', 'bytes'))
            or info['channels'] != 2):
        raise ValueError('Raw native soundrecord identity/format mismatch')
    ram = (capture / 'SCSP-RAM-saturn-order.bin').read_bytes()
    clock = struct.unpack_from('>H', ram, 0xb010)[0]
    window = native_window(info['frames'], details, clock, frames)
    if (identities[str(states[0])] != fresh['evidence']['native_state']['file_sha256']
            or any(sha(p) != identity for p, identity in identities.items())):
        raise ValueError('Native evidence changed during its audit/window inspection')
    return {'wav': wav, 'events': events, 'observation': observed, 'window': window,
            'identities': identities, 'raw_wav': info, 'build_report': build_report,
            'native_gate_report_sha256': identities[str(capture / 'native-gates.json')],
            'native_f5_sha256': identities[str(states[0])]}


def capture_reference(reference_root, out, case, events, timeout):
    work = case_directory(out / 'replay', case)
    work.mkdir(parents=True)
    schedule = work / 'schedule.csv'
    schedule.write_text(''.join(f'{e["sample"]},{e["word"]}\n' for e in events))
    paths = {k: work / n for k, n in (('trace', 'trace.csv'), ('pcm', 'audio.pcm'),
                                     ('report', 'capture.json'), ('deliveries', 'deliveries.csv'))}
    command = [str(reference_root / 'capture_board_replay'), str(reference_root / 'inputs/SOUND.ROM'),
               str(reference_root / 'inputs/SAMPLES.ROM'), str(schedule), str(case['seconds']),
               *(str(paths[k]) for k in ('trace', 'pcm', 'report', 'deliveries')), 'full']
    with (work / 'capture.log').open('x') as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
    if result.returncode:
        raise ValueError('Original full-board replay failed for ' + case['id'])
    captured = load(paths['report'])
    if (captured.get('complete') is not True or captured.get('profile') != 'full'
            or captured.get('frames') != case['seconds'] * RATE or captured.get('commands') != len(events)):
        raise ValueError('Incomplete original full-board replay')
    with paths['deliveries'].open() as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != len(events):
        raise ValueError('Not all original commands reached the original latch')
    deliveries = []
    for index, (event, row) in enumerate(zip(events, rows)):
        target = (event['sample'] * CLOCK + RATE - 1) // RATE
        delivery, read = int(row['delivery_relative_cycle']), int(row['latch_read_relative_cycle'])
        if int(row['index']) != index or not target <= delivery <= read:
            raise ValueError('Invalid original command delivery ordering')
        deliveries.append({'word': event['word'], 'admitted_native_sample': event['sample'],
                           'reference_target_cycle': target, 'reference_delivery_cycle': delivery,
                           'reference_latch_read_cycle': read,
                           'reference_instruction_boundary_delay_cycles': delivery - target})
    return {'paths': paths, 'command': command, 'report': captured, 'deliveries': deliveries,
            'output_sha256': {k: sha(v) for k, v in paths.items()}, 'schedule_sha256': sha(schedule)}


@contextmanager
def reference_captures(cases, capture, jobs=1):
    """Bound independent reference subprocesses and consume them in case order.

    The caller must serialize/verify a result and remove its temporary PCM/trace
    before requesting the next one. Only then is one replacement submitted.
    On any producer or consumer failure, cancel pending work and wait for every
    already-running process wrapper; no background capture outlives this scope.
    """
    if type(jobs) is not int or not 1 <= jobs <= 4:
        raise ValueError('Reference jobs must be an integer from 1 through 4')
    cases = list(cases)
    pending = deque()
    executor = ThreadPoolExecutor(max_workers=jobs)

    def submit(index):
        pending.append(executor.submit(capture, cases[index]))

    def ordered():
        for index, case in enumerate(cases):
            result = pending[0].result()
            yield case, result
            pending.popleft()
            if index + jobs < len(cases):
                submit(index + jobs)

    captures = ordered()
    try:
        for index in range(min(jobs, len(cases))):
            submit(index)
        yield captures
    finally:
        captures.close()
        for future in pending:
            future.cancel()
        executor.shutdown(wait=True, cancel_futures=True)


def structure_metadata(structure, command):
    track = next(t for t in structure['tracks'] if t['command'] == command)
    fields = ('classification', 'global_observation', 'natural_FB_entry_seconds',
              'visible_state_interval_seconds', 'original_fade_control_clear_observed')
    result = {k: track[k] for k in fields if k in track}
    result.update(evidence_applies_to='Separate verified original score-control observations',
                  native_loop_verified=False, exact_PCM_period_proven=False)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    for name in ('matrix-report', 'reference-root', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--structure-summary', type=Path,
                        default=HERE / 'reference/evidence/full-board-complete-music-structure.json')
    parser.add_argument('--diagnostic-only', action='store_true', help='One bounded case; never a full delivery')
    parser.add_argument('--case', help='Exact matrix case ID, required only in diagnostic mode')
    parser.add_argument('--reference-timeout', type=int, default=1800)
    parser.add_argument('--reference-jobs', type=int, default=1,
                        help='Independent original-board reference subprocesses in flight, 1..4; results stay in case order')
    parser.add_argument('--keep-reference-traces', action='store_true', help='Private only; default removes redundant raw PCM/trace after hashing')
    args = parser.parse_args()
    for key, value in vars(args).items():
        if isinstance(value, Path):
            setattr(args, key, value.resolve())
    if args.out == PACKAGE or PACKAGE in args.out.parents:
        parser.error('Private audio must remain outside the source package')
    matrix = load(args.matrix_report)
    selected = select_cases(matrix, args.case, args.diagnostic_only)
    root = args.matrix_report.parent
    if any(args.out == source or source in args.out.parents for source in (root, args.reference_root)):
        parser.error('New output must remain outside both immutable input directories')
    if args.out.exists():
        parser.error('Output already exists; nothing will be overwritten')
    if args.reference_timeout < 1:
        parser.error('Reference timeout must be positive')
    if not 1 <= args.reference_jobs <= 4:
        parser.error('Reference jobs must be from 1 through 4')
    source_paths = [Path(__file__), PACKAGE / 'tests/test_prepare_music_audio_pairs.py',
                    HERE / 'run_music_matrix.py', HERE / 'source_snapshot.py',
                    *(HERE / name for name in AUDIT_HELPERS)]
    sources = {str(p.relative_to(PACKAGE)): sha(p) for p in source_paths}
    validate_snapshot(matrix, root)
    reference_files, reference_identity = validate_reference(args.reference_root)
    structure = load(args.structure_summary)
    if (structure.get('passed') is not True or structure.get('input_sha256', {}).get('sound_rom') != SOUND_SHA
            or {t['command'] for t in structure.get('tracks', [])} != set(range(1, 19))):
        raise ValueError('Need verified structural evidence for all 18 original commands')
    items = {i['id']: i for i in matrix['commands']}
    inventory = runtime_inventory(matrix)
    native = {case['id']: inspect_native(root, matrix, case, items[case['id']]) for case in selected}
    # One native stereo WAV + one original mono WAV per case, with at most N
    # temporary PCM/traces. Completed results are consumed/deleted in order.
    required = sum(c['seconds'] * RATE * 6 + 88 for c in selected)
    in_flight = min(args.reference_jobs, len(selected))
    temporary = (in_flight * max(c['seconds'] for c in selected) * RATE * 2
                 + (len(selected) if args.keep_reference_traces else in_flight) * 192 * 1024**2)
    if not args.out.parent.is_dir() or shutil.disk_usage(args.out.parent).free < required + temporary:
        raise ValueError('Insufficient free disk for the WAVs and bounded parallel reference outputs')
    args.out.mkdir(exist_ok=False)
    (args.out / 'wav').mkdir()
    input_files = {str(args.matrix_report): sha(args.matrix_report),
                   str(args.structure_summary): sha(args.structure_summary), **reference_files}
    for item in native.values():
        input_files.update(item['identities'])
    report = {'schema_version': 1, 'tool_version': VERSION,
              'kind': 'DIAGNOSTIC_NATIVE_ORIGINAL_AUDIO_PAIR' if args.diagnostic_only else 'FULL_NATIVE_ORIGINAL_MUSIC_PAIR_COLLECTION',
              'source_sha256': sources, 'matrix_sha256': sha(args.matrix_report),
              'matrix_source_sha256': matrix['source_sha256_before'],
              'matrix_input_sha256': matrix['input_sha256_before'],
              'matrix_profile': matrix['profile_label'], 'emulator_sha256': matrix['emulator_sha256'],
              'firmware_sha256': matrix['firmware_sha256'], 'reference_identity': reference_identity,
              'structure_evidence_sha256': sha(args.structure_summary),
              'runtime_inventory': inventory, 'diagnostic_only': args.diagnostic_only,
              'native_runtime_passed': all(i['runtime_status'] == 'PASS' for i in inventory),
              'complete_18_music_collection': False, 'complete_capture_export': False,
              'reference_mode': 'Existing verified full-board executable; independent original-ROM subprocess replays',
              'reference_jobs': args.reference_jobs, 'maximum_reference_cases_in_flight': in_flight,
              'reference_result_order': 'Declared case order; replacement after WAV verification, PCM deletion and requested trace cleanup',
              'reference_traces_retained': args.keep_reference_traces,
              'normalization': False, 'gain': 1, 'resampling': False, 'time_stretch': False,
              'waveform_fitted_alignment': False,
              'native_kind': 'Real soundrecord from standalone Mednafen Saturn execution; not physical Saturn hardware',
              'reference_kind': 'Original full Z80/OPM/OKI host model; not a physical arcade recording',
              'pairs': [], 'private_input_paths_sha256': input_files, 'private_replay_commands': []}
    def capture(case):
        return capture_reference(args.reference_root, args.out, case, native[case['id']]['events'], args.reference_timeout)

    with reference_captures(selected, capture, args.reference_jobs) as captures:
        for case, reference in captures:
            observed = native[case['id']]
            suffix = f'{case["command"]:02d}' if case['kind'] == 'music' else 'TRANSITIONS'
            original_wav = args.out / 'wav' / f'ORIGINAL-BOARD-HOST-{suffix}.wav'
            native_wav = args.out / 'wav' / f'SATURN-MEDNAFEN-{suffix}.wav'
            original = pcm16be_to_wav(reference['paths']['pcm'], original_wav, case['seconds'] * RATE)
            actual = copy_wav_window(observed['wav'], native_wav, observed['window']['start_frame'], case['seconds'] * RATE)
            fade = [e['sample'] for e in observed['events'] if e['word'] == 0x1f][-1]
            pair = {'id': case['id'], 'command': case['command'], 'hex': f'0x{case["command"]:02X}',
                    'kind': case['kind'], 'seconds': case['seconds'], 'frames': case['seconds'] * RATE,
                    'runtime_status': next(i['runtime_status'] for i in inventory if i['id'] == case['id']),
                    'requested_events': case['events'], 'observed_events': observed['events'],
                    'reference_command_deliveries': reference['deliveries'], 'native_window': observed['window'],
                    'raw_native_wav_sha256': observed['raw_wav']['sha256'],
                    'native_capture_report_sha256': items[case['id']]['capture_report_sha256'],
                    'native_gate_report_sha256': observed['native_gate_report_sha256'],
                    'native_f5_sha256': observed['native_f5_sha256'], 'native_audit_revalidated': True,
                    'native_build_sha256': {k: v['sha256'] for k, v in observed['build_report']['files'].items()},
                    'native_wav': actual, 'original_host_wav': original,
                    'reference_capture_sha256': reference['output_sha256'],
                    'reference_schedule_sha256': reference['schedule_sha256'],
                    'fade': {'native': tail_observation(native_wav, fade),
                             'original_host': tail_observation(original_wav, fade)},
                    'original_structure': structure_metadata(structure, case['command']) if case['kind'] == 'music' else None,
                    'transition_schedule_replayed': case['kind'] == 'transitions',
                    'native_loop_verified': False, 'fidelity_approved': False,
                    'complete_recorded_window': True}
            report['pairs'].append(pair)
            report['private_replay_commands'].append(reference['command'])
            dump(args.out / (case['id'] + '-pair.json'), pair)
            # Preserve capture/delivery reports and hashes, remove only this tool's
            # redundant intermediate files after WAV serialization was verified.
            reference['paths']['pcm'].unlink()
            if not args.keep_reference_traces:
                reference['paths']['trace'].unlink()
            print(json.dumps({'id': case['id'], 'seconds': case['seconds'], 'native_runtime': pair['runtime_status'],
                              'observed_last_1F_sample': fade, 'native_pcm_preserved': actual['source_pcm_bytes_match']}), flush=True)
    validate_snapshot(matrix, root)
    report['source_stable'] = (all(sha(PACKAGE / p) == identity for p, identity in sources.items())
                               and all(sha(p) == identity for p, identity in input_files.items()))
    if not report['source_stable']:
        raise ValueError('An input or tool changed during audio preparation')
    report['complete_capture_export'] = len(report['pairs']) == len(selected)
    report['complete_18_music_collection'] = (not args.diagnostic_only and report['complete_capture_export']
                                            and report['native_runtime_passed'])
    dump(args.out / 'private-manifest.json', report)
    public = public_manifest(report)
    dump(args.out / 'public-manifest.json', public)
    print(json.dumps({'complete_capture_export': report['complete_capture_export'],
                      'complete_18_music_collection': report['complete_18_music_collection'],
                      'native_runtime_passed': report['native_runtime_passed'],
                      'public_manifest_sha256': sha(args.out / 'public-manifest.json')}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
