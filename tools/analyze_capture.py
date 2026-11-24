#!/usr/bin/env python3
"""Read stock-Mednafen captures without changing emulator or game state.

Speeds use Saturn fields, never host elapsed time. Window endpoints are actual
observations, with no interpolated timestamps. A report can demonstrate a fault
or slowdown; an average alone does not establish whole-game acceptance.
"""
import argparse
from bisect import bisect_left
import hashlib
import json
from pathlib import Path
import statistics
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SATURN_HZ = 59.82612341538
ARCADE_NUM, ARCADE_DEN = 37609, 39168
TARGET_HZ = SATURN_HZ * ARCADE_NUM / ARCADE_DEN
DIAGNOSTIC_FIELDS = ('magic stage fields ticks draws max_gap debt max_debt '
    'fault_owner fault_dependency rng demo cartridge start_field finish_field '
    'audio_ok frame_tag g0166_before g0166_ready g0166_committed g0166_ack '
    'handoff_debt_total handoff_debt_max handoff_count').split()
HANDOFF_FIELDS = 'handoff_debt_total handoff_debt_max handoff_count'.split()
FRONT_FIELDS = 'phase pc arcade_ticks physical_fields raster_irqs vblank_irqs commits debt'.split()
LAYOUT_FIELDS = ('magic pointer_bytes runtime_bytes globals_bytes actors_offset actors_bytes '
    'actor_stride actor_count frame_counter_offset mode_offset frame_stage_offset frame_ready_offset').split()
PHASES = dict(enumerate(('reset', 'boot', 'title', 'mode_plate', 'mode', 'roster', 'prematch')))
AUDIO_ISSUES = ('fault pcm_underruns queue_overflows music_underruns music_io_errors '
    'music_errors music_missing_mask music_exhaustions unsupported_fades invalid_bank_starts music_fades music_fade_completions music_fade_volume').split()
TERMINAL_FIELDS = ('magic version actor_bytes combat_ticks terminal_field royal owner rng '
    'combat_start_field front_ticks_before pending remainder owner_start_field '
    'owner_end_field front_ticks_after complete').split()
FRONTEND_REFERENCES = {
    'tag': {'tick': 9666, 'owner': 0x11B6, 'rng': 0x6E0F91A0, 'front_ticks': 4510,
            'sha256': '53960c39e24bac5b44e1228b54def3963e03a20d82b08cf87cfe1802df70b755'},
    'royal': {'tick': 14409, 'owner': 0x1AFC, 'rng': 0x9F7D9DAE, 'front_ticks': 1771,
              'sha256': '2e545e2dae74e212386d23277479d7a68291e0429a0f57ac1f3f4ea783506443'},
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def integer(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def physical(point):
    # Main's fields freezes during blocking result/scene owners; live_field
    # is the independently sampled Jo VBlank counter and remains meaningful.
    value = point.get('live_field', point.get('fields'))
    return integer(value) if value is not None else None


def delta(start, end):
    value = (end - start) & 0xFFFFFFFF
    return value if value < 0x80000000 else None


def rate(ticks, fields):
    if fields is None or fields <= 0:
        return None
    return {'ticks': ticks, 'fields': fields, 'emulated_seconds': fields / SATURN_HZ,
            'ticks_per_second': ticks * SATURN_HZ / fields,
            'arcade_speed_percent': ticks * ARCADE_DEN * 100 / (fields * ARCADE_NUM),
            'expected_ticks_from_zero_remainder': fields * ARCADE_NUM // ARCADE_DEN,
            'tick_shortfall_from_zero_remainder': fields * ARCADE_NUM // ARCADE_DEN - ticks}


def mark(point):
    return {'sample': point.get('_index'), 'source': point.get('origin', 'trace'),
            'field': physical(point), 'ticks': point.get('ticks'), 'stage': point.get('stage')}


def windows(points, span):
    # Keep the first observation of a tick. Replacing it with a later repeated
    # observation would hide a stall at the start of a window.
    unique = []
    for p in points:
        if not unique or p['ticks'] > unique[-1]['ticks']:
            unique.append(p)
    ticks = [p['ticks'] for p in unique]
    values = []
    for i, start in enumerate(unique):
        j = bisect_left(ticks, start['ticks'] + span, i + 1)
        if j == len(unique):
            break
        end = unique[j]
        value = rate(end['ticks'] - start['ticks'], delta(physical(start), physical(end)))
        if value:
            values.append(dict(value, start=mark(start), end=mark(end)))
    if not values:
        return {'requested_ticks': span, 'observed_windows': 0, 'status': 'INSUFFICIENT_COVERAGE'}
    speeds = sorted(v['arcade_speed_percent'] for v in values)
    return {'requested_ticks': span, 'observed_windows': len(values),
            'minimum_percent': speeds[0], 'median_percent': statistics.median(speeds),
            'maximum_percent': speeds[-1], 'worst': min(values, key=lambda v: v['arcade_speed_percent']),
            'first': values[0], 'last': values[-1],
            'endpoint_policy': 'First sampled tick at or beyond requested span; actual ticks and fields shown.'}


def combat_segments(trace, spans):
    groups, active = [], None
    for p in trace:
        if physical(p) is None or 'ticks' not in p:
            continue
        if p.get('stage') == 10:
            if active is None or p.get('start_field') != active[-1].get('start_field') or p['ticks'] < active[-1]['ticks']:
                active = []
                groups.append(active)
            active.append(p)
        elif active is not None:
            if p.get('stage') in (20, 21, 22) and p.get('finish_field'):
                # finish_field is recorded on combat completion, before the
                # blocking result waits. Later capture time is not combat time.
                active.append(dict(p, live_field=p['finish_field'], origin='recorded_combat_finish'))
            active = None
    result = []
    for group in groups:
        first, last = group[0], group[-1]
        start_field = first.get('start_field', physical(first))
        from_start = rate(last['ticks'], delta(start_field, physical(last)))
        observed = rate(last['ticks'] - first['ticks'], delta(physical(first), physical(last)))
        gaps = [delta(physical(a), physical(b)) for a, b in zip(group, group[1:])]
        drawings = [p for p in group if 'draws' in p]
        result.append({'first': mark(first), 'last': mark(last), 'recorded_start_field': start_field,
            'from_recorded_start': from_start, 'between_first_and_last_observation': observed,
            'max_observation_gap_fields': max((g for g in gaps if g is not None), default=0),
            'debt': {'last': last.get('debt'),
                     'observed_peak': max(p.get('debt', 0) for p in group),
                     'recorded_peak': max(p.get('max_debt', 0) for p in group)},
            'presentation': {'last_draws': drawings[-1]['draws'] if drawings else None,
                             'draws_between_observations': drawings[-1]['draws'] - drawings[0]['draws'] if drawings else None,
                             'recorded_max_gap_fields': max(p.get('max_gap', 0) for p in group)},
            'windows': {str(span): windows(group, span) for span in spans},
            'completed_combat_boundary_observed': last.get('origin') == 'recorded_combat_finish'})
    return result


def result_transitions(trace):
    result, current = [], None
    for p in trace:
        if physical(p) is None:
            continue
        finish = p.get('finish_field', 0)
        if p.get('stage') in (20, 21) and finish:
            if current is None or current['finish_field'] != finish:
                current = {'finish_field': finish, 'stage20_first_observation': None,
                           'completion_first_observation': None, 'status': 'INCOMPLETE'}
                result.append(current)
            if p['stage'] == 20:
                if current['stage20_first_observation'] is None:
                    current['stage20_first_observation'] = mark(p)
                current['_last20'] = p
            elif current['completion_first_observation'] is None:
                previous = current.pop('_last20', None)
                lower = delta(finish, physical(previous)) if previous else 0
                upper = delta(finish, physical(p))
                current.update(status='OBSERVED', completion_first_observation=mark(p),
                    completion_source=p.get('origin', 'trace'),
                    duration_fields_bounds=[lower, upper],
                    duration_seconds_bounds=[v / SATURN_HZ if v is not None else None for v in (lower, upper)],
                    timing='Completion occurred after the last stage20 sample and by the first stage21 sample; no exact completion timestamp is invented.')
        elif current and current.get('completion_first_observation') is None and p.get('stage') != 20:
            current['next_observed_stage'] = p.get('stage')
            current = None
    for item in result:
        item.pop('_last20', None)
    return result


def frontend_summary(trace):
    observed = []
    for p in trace:
        f = p.get('frontend')
        if f and any(f.get(k, 0) for k in ('pc', 'arcade_ticks', 'commits')):
            observed.append(dict(f, sample=p.get('_index'), source=p.get('origin', 'trace'),
                                 live_field=physical(p)))
    result = {'observations': len(observed), 'phase_changes': [],
              'title_hold_first_observation': None, 'title_hold_entry_bounds': None}
    previous = None
    for f in observed:
        if previous is None or f['phase'] != previous['phase']:
            result['phase_changes'].append({'phase': f['phase'], 'name': PHASES.get(f['phase'], 'unknown'),
                                           'first_observation': f, 'previous_observation': previous})
        # These are original title-hold loop seams, after all 12 portraits.
        # Seeing a later mode selection does not prove the intro was completed.
        if f['phase'] == 2 and f['pc'] in (0x837E, 0x83A4, 0x8482) and result['title_hold_first_observation'] is None:
            result['title_hold_first_observation'] = f
            if previous and previous['phase'] == 2 and previous['pc'] in (0x821A, 0x823A, 0x8272, 0x8296, 0x8302, 0x8378):
                result['title_hold_entry_bounds'] = {
                    'arcade_ticks': [previous['arcade_ticks'], f['arcade_ticks']],
                    'physical_fields': [previous['physical_fields'], f['physical_fields']]}
        previous = f
    if observed:
        result['first'], result['last'] = observed[0], observed[-1]
        result['max_debt_observed'] = max(f.get('debt', 0) for f in observed)
    result['timing'] = 'Phase/PC timestamps are observations, not inferred scene-entry times; absent boundaries remain null.'
    return result


def frontend_active(point):
    # Direct builds can retain the adapter's all-zero diagnostic symbol.
    # Merely finding that symbol or a nonempty dictionary is not execution.
    front = point.get('frontend', {})
    return any(front.get(name, 0) for name in ('phase', 'pc', 'arcade_ticks', 'commits'))


def prove_layout(destination, compiler=None):
    compiler = Path(compiler or ROOT / 'sdk/Compiler/LINUX/bin/sh-none-elf-gcc-8.2.0').resolve()
    source = ROOT / 'tests/capture_layout.c'
    args = [str(compiler), '-m2', '-std=gnu99', '-I' + str(ROOT / 'core/include'),
            '-I' + str(ROOT / 'sdk/Compiler/WINDOWS/sh-elf/include'), str(source)]
    destination.parent.mkdir(parents=True, exist_ok=True)
    assembly = destination.with_suffix('.s')
    with tempfile.TemporaryDirectory(prefix='wf-capture-abi-') as temp:
        obj = Path(temp) / 'layout.o'
        subprocess.run([*args, '-c', '-o', str(obj)], check=True)
        subprocess.run([*args, '-S', '-o', str(assembly)], check=True)
        data = obj.read_bytes()
        if data[:6] != b'\x7fELF\x01\x02':
            raise ValueError('ABI proof must be big-endian ELF32, not a host object')
        header = struct.unpack_from('>HHIIIIIHHHHHH', data, 16)
        if header[1] != 42:
            raise ValueError('ABI proof must be compiled for SuperH')
        shoff, shentsize, shnum, shstrndx = header[5], header[10], header[11], header[12]
        sections = [struct.unpack_from('>10I', data, shoff + i * shentsize) for i in range(shnum)]
        strings = sections[shstrndx]
        names = data[strings[4]:strings[4] + strings[5]]
        selected = [s for s in sections if names[s[0]:].split(b'\0', 1)[0] == b'.wf_capture_abi']
        if len(selected) != 1 or selected[0][5] != 4 * len(LAYOUT_FIELDS):
            raise ValueError('Missing SH-2 layout constants')
        values = dict(zip(LAYOUT_FIELDS, struct.unpack_from('>' + 'I' * len(LAYOUT_FIELDS), data, selected[0][4])))
        if values['magic'] != 0x57464C31 or values['pointer_bytes'] != 4:
            raise ValueError('Wrong target ABI')
        sources = [source, *(ROOT / 'core/include' / n for n in ('wf_runtime.h', 'wf_actor.h', 'wf_r17_m68k_shadow.h'))]
        proof = {'architecture': 'SH-2; big-endian ELF32; e_machine=42', 'compiler': str(compiler),
                 'command': args, 'object_sha256': hashlib.sha256(data).hexdigest(),
                 'assembly': str(assembly.relative_to(ROOT)) if assembly.is_relative_to(ROOT) else str(assembly),
                 'sources_sha256': {str(p.relative_to(ROOT)): sha(p) for p in sources}, 'values': values}
    destination.write_text(json.dumps(proof, indent=2) + '\n')
    return proof


def inspect_state(report, folder, layout, references, actor_output=None, allow_reference=True):
    path = folder / 'WorkRAMH-state.bin'
    if not path.is_file():
        return {'status': 'UNAVAILABLE', 'reason': 'No final saved WorkRAMH dump'}, None
    memory, syms = path.read_bytes(), report.get('symbols', {})
    def read_symbol(name, size):
        address = integer(syms[name]) - 0x06000000
        if address < 0 or address + size > len(memory):
            raise ValueError('Symbol outside WorkRAMH: ' + name)
        return memory[address:address + size]
    try:
        # WJ01 began with21 words. The capture itself records whether its
        # executable has the appended3 handoff counters; never read adjacent
        # globals as invented diagnostics for an older image.
        fields = DIAGNOSTIC_FIELDS if any(all(k in p for k in HANDOFF_FIELDS) for p in report.get('trace', [])) else DIAGNOSTIC_FIELDS[:21]
        words = struct.unpack('>' + 'I' * len(fields), read_symbol('wf_jo_diagnostic', len(fields) * 4))
        snapshot = dict(zip(fields, words), origin='save_state', diagnostic_words=len(fields))
        if snapshot['magic'] != 0x574A3031:
            raise ValueError('Saved diagnostic signature does not match the report symbols')
        snapshot['live_field'] = struct.unpack('>I', read_symbol('jo_core_vblank_count', 4))[0]
        if 'wf_jo_front_diagnostic' in syms:
            snapshot['frontend'] = dict(zip(FRONT_FIELDS, struct.unpack('>8I', read_symbol('wf_jo_front_diagnostic', 32))))
    except (KeyError, ValueError, struct.error) as error:
        return {'status': 'UNAVAILABLE', 'reason': str(error)}, None
    if not layout:
        return {'status': 'UNVERIFIED_LAYOUT', 'saved_diagnostic': snapshot}, snapshot
    try:
        values = layout['values']
        runtime = read_symbol('rt', values['runtime_bytes'])
        actor_offset, length = values['actors_offset'], values['actors_bytes']
        actors = runtime[actor_offset:actor_offset + length]
        if len(actors) != length:
            raise ValueError('Actor range exceeds runtime')
        frame = struct.unpack_from('>I', runtime, values['frame_counter_offset'])[0]
        mode = 'royal' if runtime[values['mode_offset']] & 1 else 'tag'
        expected_tick = {'tag': 8537, 'royal': 8474}[mode]
        result = {'status': 'NOT_TERMINAL', 'mode': mode, 'expected_endpoint_tick': expected_tick,
            'saved_stage': snapshot['stage'], 'saved_diagnostic_ticks': snapshot['ticks'],
            'saved_runtime_ticks': frame, 'saved_runtime_frame_stage': runtime[values['frame_stage_offset']],
            'rt_address': hex(integer(syms['rt'])), 'actors_offset': actor_offset,
            'actors_address': hex(integer(syms['rt']) + actor_offset), 'actor_bytes': length,
            'actor_sha256': hashlib.sha256(actors).hexdigest(), 'state_sha256': hashlib.sha256(memory).hexdigest()}
        if actor_output:
            actor_output.parent.mkdir(parents=True, exist_ok=True)
            actor_output.write_bytes(actors)
            result['extracted_actors'] = str(actor_output)
        if not allow_reference:
            result['reason'] = 'Frontend current rt may already belong to a later owner. Only the published WorkRAML WFT1 copy can prove its terminal.'
            return result, snapshot
        if snapshot['stage'] != 21 or snapshot['ticks'] != expected_tick or frame != expected_tick or snapshot['fault_owner'] or snapshot['fault_dependency']:
            result['reason'] = 'Comparison requires saved stage21, exact fixture endpoint in both counters, and no diagnostic fault.'
            return result, snapshot
        reference = references / (mode + '-terminal-actors.bin')
        if not reference.is_file():
            result.update(status='REFERENCE_MISSING', reference=str(reference))
            return result, snapshot
        expected = reference.read_bytes()
        if len(expected) != length:
            raise ValueError('Reference actor size differs from target ABI')
        differences = [{'slot': i // values['actor_stride'], 'actor_offset': i % values['actor_stride'],
                        'actual': a, 'expected': b} for i, (a, b) in enumerate(zip(actors, expected)) if a != b]
        result.update(status='MISMATCH' if differences else 'MATCH', equal=not differences,
                      reference=str(reference), reference_sha256=hashlib.sha256(expected).hexdigest(),
                      difference_bytes=len(differences), differences=differences[:32])
        result['scope'] = 'Terminal actor bytes for the supplied deterministic fixture; this does not verify all intermediate frames or other inputs.'
        return result, snapshot
    except (KeyError, ValueError, struct.error) as error:
        return {'status': 'UNAVAILABLE', 'reason': str(error)}, snapshot


def inspect_frontend_terminal(folder, layout, references, actor_output=None):
    """Read the profiler's immutable post-ACK actor copy, before result owners.

    The magic publishes header+actors. complete is a separate marker for the
    subsequent post owner and is deliberately not required for actor equality.
    No current WorkRAMH actors or guessed direct endpoint are used here.
    """
    path = folder / 'WorkRAML-state.bin'
    if not path.is_file():
        return {'status': 'NOT_RECORDED', 'source': 'WorkRAML/WFT1',
                'reason': 'No saved WorkRAML terminal snapshot'}
    memory = path.read_bytes()
    result = {'source': 'WorkRAML/WFT1', 'address': '0x00200000',
              'state_sha256': hashlib.sha256(memory).hexdigest()}
    if len(memory) < 4:
        return dict(result, status='INVALID_HEADER', reason='Truncated WFT1 publication word')
    if memory[:4] != b'WFT1':
        return dict(result, status='NOT_RECORDED', reason='No published WFT1 terminal image')
    result['published_signature'] = True
    if len(memory) < 64:
        return dict(result, status='INVALID_HEADER', reason='Truncated WFT1 header')
    header = dict(zip(TERMINAL_FIELDS, struct.unpack_from('>16I', memory)))
    result['header'] = header
    size = header['actor_bytes']
    if (header['version'] != 1 or header['royal'] not in (0, 1) or header['complete'] not in (0, 1)
            or size != 2412 or 64 + size > min(len(memory), 4096)
            or header['remainder'] >= ARCADE_DEN):
        return dict(result, status='INVALID_HEADER', reason='Unsupported version/mode, invalid actor range, completion marker, or clock remainder')
    mode = 'royal' if header['royal'] else 'tag'
    actors = memory[64:64 + size]
    result.update(mode=mode, actor_bytes=size, actor_sha256=hashlib.sha256(actors).hexdigest(),
                  combat=rate(header['combat_ticks'], delta(header['combat_start_field'], header['terminal_field'])))
    transition = {'status': 'INCOMPLETE', 'owner': header['owner'],
                  'owner_start_field': header['owner_start_field'],
                  'terminal_to_owner_fields': delta(header['terminal_field'], header['owner_start_field'])
                    if header['owner_start_field'] else None,
                  'timing': 'Instrumented Saturn field-counter samples; no host seconds or interpolated boundaries. The diagnostic actor copy precedes the owner interval and has its own overhead.'}
    if header['complete']:
        fields = delta(header['owner_start_field'], header['owner_end_field'])
        ticks = delta(header['front_ticks_before'], header['front_ticks_after'])
        if fields is None or ticks is None or transition['terminal_to_owner_fields'] is None:
            transition.update(status='INVALID_TIMING', reason='Counter order exceeds the supported unsigned half-range')
        else:
            transition.update(status='COMPLETE', fields=fields, seconds=fields / SATURN_HZ,
                              arcade_ticks=ticks, owner_end_field=header['owner_end_field'],
                              observed_tick_rate=rate(ticks, fields))
            if ticks:
                # The stored remainder precedes entry/transport work, so its
                # exact phase at owner_start is not assumed. Bound all legal
                # remainders instead of inventing sub-field precision.
                nominal_min = ((ticks - 1) * ARCADE_DEN) // ARCADE_NUM + 1
                nominal_max = (ticks * ARCADE_DEN + ARCADE_NUM - 1) // ARCADE_NUM
                transition.update(nominal_clock_fields_bounds=[nominal_min, nominal_max],
                                  fields_beyond_nominal_upper_bound=max(0, fields - nominal_max),
                                  nominal_scope='Clock-admission budget for the observed owner ticks, allowing every fractional phase. This is not a measured original-arcade scene trace.')
    result['transition'] = transition
    if actor_output:
        actor_output.parent.mkdir(parents=True, exist_ok=True)
        actor_output.write_bytes(actors)
        result['extracted_actors'] = str(actor_output)
    if not layout:
        return dict(result, status='UNVERIFIED_LAYOUT', reason='No current SH-2 actor ABI proof')
    values = layout['values']
    if size != values['actors_bytes'] or values['actor_stride'] * values['actor_count'] != size:
        return dict(result, status='INVALID_HEADER', reason='WFT1 actor size disagrees with the compiled SH-2 layout')
    expected_context = FRONTEND_REFERENCES[mode]
    result['expected_reference'] = expected_context
    if (header['combat_ticks'] != expected_context['tick'] or header['owner'] != expected_context['owner']
            or header['front_ticks_before'] != expected_context['front_ticks']):
        return dict(result, status='NOT_REFERENCE_ENDPOINT',
                    reason='This is not the first automated frontend fixture at its recorded native endpoint; no direct-route or guessed endpoint is substituted.')
    reference = references / ('frontend-' + mode + '-terminal-actors.bin')
    result['reference'] = str(reference)
    if not reference.is_file():
        return dict(result, status='REFERENCE_MISSING')
    expected = reference.read_bytes()
    result['reference_sha256'] = hashlib.sha256(expected).hexdigest()
    if len(expected) != size or result['reference_sha256'] != expected_context['sha256']:
        return dict(result, status='REFERENCE_INVALID', reason='Reference is not the verified fullfrontend native fixture')
    differences = [{'slot': i // values['actor_stride'], 'actor_offset': i % values['actor_stride'],
                    'actual': a, 'expected': b} for i, (a, b) in enumerate(zip(actors, expected)) if a != b]
    rng_equal = header['rng'] == expected_context['rng']
    result.update(status='MATCH' if not differences and rng_equal else 'MISMATCH',
                  equal=not differences and rng_equal, actors_equal=not differences, rng_equal=rng_equal,
                  difference_bytes=len(differences), differences=differences[:32],
                  scope='Post-ACK actor bytes and RNG for the first deterministic native fullfrontend fixture, before the result owner. This does not verify intermediate frames, physical speed, sound, other inputs or full-game acceptance.')
    return result


def with_frontend_boundary(points, terminal):
    """Insert only the independently recorded finish into its observed match."""
    header = terminal.get('header')
    if not header or terminal['status'] in ('INVALID_HEADER', 'NOT_RECORDED'):
        return points
    start, finish = header['combat_start_field'], header['terminal_field']
    candidates = [i for i, p in enumerate(points) if p.get('stage') == 10
                  and p.get('start_field') == start and p.get('ticks', 0) <= header['combat_ticks']
                  and physical(p) is not None and delta(physical(p), finish) is not None]
    if not candidates:
        return points
    i = candidates[-1]
    boundary = {'stage': 22, 'ticks': header['combat_ticks'], 'fields': finish,
                'live_field': finish, 'start_field': start, 'finish_field': finish,
                'debt': header['pending'], 'origin': 'recorded_combat_finish',
                'boundary_source': 'WorkRAML/WFT1'}
    return [*points[:i+1], boundary, *points[i+1:]]


def counters(trace, key):
    values = [p[key] for p in trace if p.get(key)]
    if not values:
        return {'status': 'NOT_OBSERVED'}
    first, last = values[0], values[-1]
    result = {'last': last, 'observed_max': {k: max(v.get(k, 0) for v in values) for k in last},
              'first_to_last_delta': {k: last[k] - first.get(k, 0) for k in last if last[k] >= first.get(k, 0)}}
    if key == 'audio':
        result['nonzero_issue_counters'] = {k: result['observed_max'].get(k, 0) for k in AUDIO_ISSUES if result['observed_max'].get(k, 0)}
    return result


def coin_profile_summary(trace):
    result = counters(trace, 'coin_profile')
    if 'last' not in result:
        return result
    last = result['last']
    calls, irqs = last.get('calls', 0), last.get('irq2s', 0)
    periods = irqs / 17
    result.update(recorded_irq_periods=periods,
        mean_ms_per_call=last.get('total_ms', 0) / calls if calls else None,
        mean_ms_per_irq_period=last.get('total_ms', 0) / periods if periods else None,
        mean_frt_per_call=last.get('total_frt', 0) / calls if calls else None,
        mean_frt_per_irq_period=last.get('total_frt', 0) / periods if periods else None,
        scope='Entire combat coin bridge, including clock, input snapshot, IRQ dispatch and credit synchronization; not an isolated owner434 measurement.',
        precision='Millisecond intervals are individually quantized. FRT intervals use CPU/32 and wrap at65536 ticks; max_ms must be checked before interpreting FRT sums. No host time is used.')
    return result


def analyze(path, layout, references, spans, actor_output=None):
    report = json.loads(path.read_text())
    trace = [dict(p, _index=i) for i, p in enumerate(report.get('trace', []))]
    syms = report.get('symbols', {})
    frontend_capture = 'wf_jo_terminal_completed' in syms or any(frontend_active(p) for p in trace)
    state, snapshot = inspect_state(report, path.parent, layout, references,
                                     None if frontend_capture else actor_output,
                                     allow_reference=not frontend_capture)
    if snapshot and frontend_active(snapshot):
        frontend_capture = True
    frontend_terminal = inspect_frontend_terminal(path.parent, layout, references,
                                                 actor_output if frontend_capture else None)
    if frontend_terminal.get('published_signature'):
        if not frontend_capture and actor_output:
            frontend_terminal = inspect_frontend_terminal(path.parent, layout, references, actor_output)
        frontend_capture = True
    runtime_observation = state if frontend_capture else None
    if frontend_capture:
        state = frontend_terminal
    observations = [*trace]
    if snapshot:
        snapshot['_index'] = len(trace)
        observations.append(snapshot)
    # Exit/autosave can take extra fields after the last requested live
    # sample. Active-state snapshots do not extend the speed measurement.
    # A saved terminal still supplies the recorded combat finish boundary.
    combat_observations = observations if snapshot and snapshot['stage'] in (20, 21, 22) else trace
    if frontend_capture:
        combat_observations = with_frontend_boundary(combat_observations, frontend_terminal)
    faults = []
    for p in observations:
        value = (p.get('fault_owner', 0), p.get('fault_dependency', 0))
        if any(value) and (not faults or value != (faults[-1]['owner'], faults[-1]['dependency'])):
            faults.append(dict(mark(p), owner=value[0], dependency=value[1]))
    warnings = []
    if any('live_field' not in p for p in trace):
        warnings.append('Some trace samples lack live_field; fields can freeze inside a blocking owner.')
    if not layout:
        warnings.append('No current SH-2 ABI proof: actors were not compared.')
    handoff_samples = [p for p in observations if all(k in p for k in HANDOFF_FIELDS)]
    handoff = {'status': 'NOT_RECORDED'}
    if handoff_samples:
        handoff = {'status': 'RECORDED', 'last': {k: handoff_samples[-1][k] for k in HANDOFF_FIELDS},
                   'observed_max': {k: max(p[k] for p in handoff_samples) for k in HANDOFF_FIELDS}}
    return {'report': str(path), 'report_sha256': sha(path), 'capture_status': report.get('status'),
        'stop_reason': report.get('stop_reason'), 'trace_samples': len(trace),
        'identity': {k: report.get(k) for k in ('emulator_sha256', 'elf_sha256', 'raw_sha256', 'cue_sha256', 'bios_sha256', 'cold_boot', 'guest_writes', 'state_loaded', 'hardware_verified')},
        'reference_clock': {'saturn_fields_per_second': SATURN_HZ, 'arcade_ticks_per_field': [ARCADE_NUM, ARCADE_DEN], 'arcade_ticks_per_second': TARGET_HZ},
        'combat': combat_segments(combat_observations, spans), 'result_transitions': result_transitions(observations),
        'frontend': frontend_summary(observations), 'video': counters(trace, 'video'),
        'audio': counters(trace, 'audio'), 'faults': faults, 'terminal_actors': state,
        'frontend_capture': frontend_capture,
        'current_runtime_actor_observation': runtime_observation,
        'combat_from_terminal_snapshot': state.get('combat') if frontend_capture else None,
        'frontend_post_transition': state.get('transition') if frontend_capture else None,
        'coin_profile': coin_profile_summary(trace), 'asset_profile': counters(trace, 'asset_profile'),
        'handoff_debt': handoff,
        'actor_abi_proof': {'values': layout['values'], 'object_sha256': layout.get('object_sha256'),
                            'sources_sha256': layout.get('sources_sha256')} if layout else None,
        'whole_game_accepted': False,
        'acceptance_scope': 'Measurements only. Averages, terminal equality, or one mode cannot establish whole-game speed, transition, visual, audio, or hardware acceptance.',
        'warnings': warnings,
        'sampling_limits': 'Reads are asynchronous observations. Windows use actual sampled endpoints and may exceed the requested tick span. No timestamps are interpolated; no host wall-clock value is used as Saturn time.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reports', nargs='*', type=Path, help='report.json or capture directory')
    parser.add_argument('--output', type=Path, help='Write the selected analysis; default stdout')
    parser.add_argument('--layout', type=Path, default=ROOT / 'evidence/capture-sh2-layout.json')
    parser.add_argument('--prove-layout', action='store_true', help='Compile target constants and save the ABI evidence')
    parser.add_argument('--compiler', type=Path)
    parser.add_argument('--references', type=Path, default=ROOT / 'evidence')
    parser.add_argument('--windows', type=int, nargs='+', default=[120, 600])
    parser.add_argument('--actors-dir', type=Path, help='Optionally save extracted actor bytes as new files')
    args = parser.parse_args()
    if any(v <= 0 for v in args.windows):
        parser.error('Window sizes must be positive')
    if args.prove_layout:
        prove_layout(args.layout, args.compiler)
    layout = json.loads(args.layout.read_text()) if args.layout.is_file() else None
    if layout and any(not (ROOT / p).is_file() or sha(ROOT / p) != value for p, value in layout['sources_sha256'].items()):
        parser.error('SH-2 ABI proof is stale; rerun with --prove-layout')
    if not args.reports:
        if args.prove_layout:
            print(json.dumps(layout, indent=2))
            return
        parser.error('At least one capture is required')
    results = []
    for item in args.reports:
        if item.is_dir():
            candidates = [p for p in (item / 'report.json', item / 'report-progress.json') if p.is_file()]
            if not candidates:
                parser.error('No report in ' + str(item))
            # The capture writes progress while running and replaces the final
            # report on exit. Analyze the newest actual observation file.
            path = max(candidates, key=lambda p: p.stat().st_mtime).resolve()
        else:
            path = item.resolve()
        actor_output = args.actors_dir / (path.parent.name + '-actors.bin') if args.actors_dir else None
        results.append(analyze(path, layout, args.references, args.windows, actor_output))
    payload = json.dumps({'analyses': results}, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload)
        for result in results:
            combat = result['combat']
            speed = combat[-1]['from_recorded_start'] if combat else None
            print(f"{Path(result['report']).parent.name}: {speed['ticks_per_second']:.4f} ticks/s, {speed['arcade_speed_percent']:.3f}% arcade" if speed else f"{Path(result['report']).parent.name}: no observed combat interval")
            print('  actors:', result['terminal_actors']['status'], '| audio issues:', result['audio'].get('nonzero_issue_counters', {}), '| faults:', len(result['faults']))
    else:
        print(payload, end='')


if __name__ == '__main__':
    main()
