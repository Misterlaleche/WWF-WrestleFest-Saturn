#!/usr/bin/env python3
"""Read-only analysis of asynchronous native controller observations.

Input: a standalone capture directory containing samples.jsonl.gz,
actions.json, input-events.json and report.json. A failed/incomplete capture
can be inspected, but cannot receive an overall PASS. JSON is printed to stdout
unless --output is provided; original capture files are never changed.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import gzip
import hashlib
import json
from pathlib import Path
import statistics


BUTTONS = {
    'left': dict(pad=0x4000, active=0x02, actor=0x02, axis='x', sign=-1),
    'right': dict(pad=0x8000, active=0x01, actor=0x01, axis='x', sign=1),
    'up': dict(pad=0x1000, active=0x08, actor=0x08, axis='y', sign=-1),
    'down': dict(pad=0x2000, active=0x04, actor=0x04, axis='y', sign=1),
    'a': dict(pad=0x0400, active=0x10, actor=0x01),
    'b': dict(pad=0x0100, active=0x20, actor=0x02),
    'c': dict(pad=0x0200, active=0x40, actor=0x04),
    'start': dict(pad=0x0800, active=0x80, actor=0x100),
}
MAPPED_MASK = sum(value['pad'] for value in BUTTONS.values())
RESET_BUTTONS = frozenset(('a', 'b', 'c', 'start'))
RESET_MASK = 0x0f00
MASK32 = 0xffffffff


def difference(value, origin):
    """Signed difference of nearby uint32 native clocks, including wrap."""
    return ((value - origin + 0x80000000) & MASK32) - 0x80000000


def stable(row):
    before, after = row.get('boundary_before', {}), row.get('boundary_after', {})
    return (row.get('boundary_stable') is True and 'video_clock' in before and before == after
            and all(row.get(name) == value for name, value in before.items()))


def actor(raw):
    try:
        data = bytes.fromhex(raw)
    except (TypeError, ValueError):
        return None
    if len(data) != 268:
        return None
    u16 = lambda offset: int.from_bytes(data[offset:offset + 2], 'big')
    i32 = lambda offset: int.from_bytes(data[offset:offset + 4], 'big', signed=True)
    return dict(character=u16(2), active=bool(data[0] & 0x80), state=u16(0x20) & 0x7f,
                x_raw=i32(6), y_raw=i32(0x0a), direction=u16(0xa8),
                fresh=u16(0xa2), accumulated=u16(0xa4), held=u16(0xa6))


def reference(row):
    result = {key: row.get(key) for key in ('sample_index', 'video_clock', 'relative_field',
               'game_frame', 'wp1980_draws', 'start_clock', 'royal_stage',
               'wp1980_demo_active', 'boundary_stable', 'frame_ready', 'frame_stage',
               'fatal_owner', 'fatal_dependency', 'fatal_frame')}
    result['pad_level'] = f"0x{row['pad_level']:04x}" if isinstance(row.get('pad_level'), int) else None
    result['input0'] = f"0x{row['input0']:02x}" if isinstance(row.get('input0'), int) else None
    value = actor(row.get('actor0'))
    if value:
        result['actor0'] = {key: value[key] for key in ('character', 'state', 'direction', 'fresh', 'accumulated', 'held')}
        result['actor0'].update(x_pixels=value['x_raw'] / 65536, y_pixels=value['y_raw'] / 65536)
    return result


def window(rows):
    return dict(samples=len(rows), native_clocks=len({row.get('video_clock') for row in rows}),
                first=reference(rows[0]) if rows else None,
                last=reference(rows[-1]) if rows else None)


def histogram(rows, field, width=0):
    counts = Counter(row.get(field) for row in rows)
    return {('null' if key is None else f'0x{key:0{width}x}' if width and isinstance(key, int) else str(key)): count
            for key, count in sorted(counts.items(), key=lambda item: str(item[0]))}


def planned_pulses(actions):
    groups = defaultdict(set)
    for item in actions.get('ranges', []):
        if item.get('button') not in BUTTONS:
            raise ValueError('Unknown planned button: ' + str(item.get('button')))
        first, last = int(item['first']), int(item['last'])
        if first < 0 or last < first:
            raise ValueError('Invalid planned button range')
        groups[first, last].add(item['button'])
    return [dict(first=first, last=last, buttons=sorted(buttons))
            for (first, last), buttons in sorted(groups.items())]


def actual_pulses(events, last_field):
    held = set();current = None;result = []
    for event in events:
        field = event.get('relative_field')
        if not isinstance(field, int):
            continue
        next_held = (held - set(event.get('release', []))) | set(event.get('press', []))
        if next_held == held:
            continue
        if current:
            current['last'] = field - 1
            current['release_event'] = event
            result.append(current)
        held = next_held
        current = dict(first=field, buttons=sorted(held), press_event=event) if held else None
    if current:
        current.update(last=last_field, release_event=None)
        result.append(current)
    return result


def wait_phase(row):
    before, after = row.get('boundary_before', {}), row.get('boundary_after', {})
    return stable(row) and all(before.get(key) == after.get(key) == row.get(key) == 1
                              for key in ('frame_ready', 'frame_stage'))


def motion_analysis(rows, button):
    """Only consecutive observed free-walking updates; never interpolate gaps."""
    specification = BUTTONS[button]
    grouped = defaultdict(list)
    for row in rows:
        value = actor(row.get('actor0'))
        if not stable(row) or not value or row.get('royal_stage', 3) != 3:
            continue
        if row.get('wp1980_started') != 1 or not isinstance(row.get('game_frame'), int):
            continue
        grouped[row.get('start_clock'), row['game_frame']].append((row, value))
    selected = {};rejected = Counter()
    for key, values in grouped.items():
        waiting = [item for item in values if wait_phase(item[0])]
        if waiting:
            selected[key] = (*waiting[-1], 'bracketed_native_wait_phase')
            continue
        # Without phase guards, require an equal tail observed twice. This is
        # conservative corroboration, not an atomic-snapshot claim.
        if len(values) >= 2:
            a, b = values[-2], values[-1]
            fields = ('pad_level', 'input0', 'wp1980_demo_active')
            if a[1] == b[1] and all(a[0].get(field) == b[0].get(field) for field in fields):
                selected[key] = (*b, 'repeated_equal_projection')
                continue
        rejected['unsettled_or_single_observation'] += 1
    pairs = []
    for (epoch, frame), first in sorted(selected.items(), key=lambda item: (str(item[0][0]), item[0][1])):
        second = selected.get((epoch, frame + 1))
        if second is None:
            continue
        r0, a0, m0 = first;r1, a1, m1 = second
        expected_input = 0xff ^ specification['active']
        if not all(a['active'] and a['state'] == 1 for a in (a0, a1)):
            rejected['not_free_walking_state1'] += 1;continue
        if a0['character'] != a1['character']:
            rejected['character_changed'] += 1;continue
        if not all(r.get('pad_connected') == 1 and r.get('pad_level') == specification['pad'] and
                   r.get('input0') == expected_input and r.get('wp1980_demo_active') == 0
                   for r in (r0, r1)):
            rejected['input_not_stably_matching'] += 1;continue
        if not all((a['direction'] & 0xf) == specification['actor'] for a in (a0, a1)):
            rejected['actor_direction_not_matching'] += 1;continue
        dx = (a1['x_raw'] - a0['x_raw']) / 65536
        dy = (a1['y_raw'] - a0['y_raw']) / 65536
        pairs.append(dict(start_clock=epoch, from_frame=frame, to_frame=frame + 1,
                          from_clock=r0['video_clock'], to_clock=r1['video_clock'],
                          dx_pixels=dx, dy_pixels=dy, selection=[m0, m1]))
    axis = specification['axis'];sign = specification['sign']
    deltas = [pair['d' + axis + '_pixels'] for pair in pairs]
    intended = sum(value * sign > 0 for value in deltas)
    opposed = sum(value * sign < 0 for value in deltas)
    status = 'PASS' if intended >= 2 and not opposed else 'INCONCLUSIVE'
    return dict(status=status, scope='Observed displacement during qualified consecutive state1 updates; no skipped-frame interpolation',
                qualifying_pairs=len(pairs), intended_direction_pairs=intended, opposed_direction_pairs=opposed,
                stationary_pairs=sum(value == 0 for value in deltas), rejected=dict(rejected),
                observed_states=dict(Counter(str(actor(row.get('actor0'))['state']) for row in rows if actor(row.get('actor0')))),
                total_dx_pixels=sum(pair['dx_pixels'] for pair in pairs),
                total_dy_pixels=sum(pair['dy_pixels'] for pair in pairs),
                axis_pixels_per_native_update=(dict(mean=statistics.mean(deltas), median=statistics.median(deltas),
                    minimum=min(deltas), maximum=max(deltas)) if deltas else None),
                first_pairs=pairs[:6], last_pairs=pairs[-2:] if len(pairs) > 6 else [],
                reason=('At least two qualified updates move in the requested direction.' if status == 'PASS' else
                        'No consistent measured walking interval; combat state, sampling phase or gaps can make this inconclusive.'))


def analyze_data(samples, actions, events, capture_report, *, complete=True, lag_fields=8):
    origin = capture_report.get('controller_start_clock')
    origin_source = 'capture_report'
    if not isinstance(origin, int):
        origins = [(event['native_clock'] - event['relative_field']) & MASK32 for event in events
                   if isinstance(event.get('native_clock'), int) and isinstance(event.get('relative_field'), int)]
        if origins:
            origin = Counter(origins).most_common(1)[0][0];origin_source = 'input_events'
        else:
            initial = next((row for row in samples if row.get('wp1980_started') == 1 and row.get('game_frame', 0) >= 1), None)
            origin = initial.get('video_clock') if initial else None;origin_source = 'first_observed_started_game_frame'
    rows = []
    for index, original in enumerate(samples):
        row = dict(original);row['sample_index'] = index
        row['relative_field'] = difference(row['video_clock'], origin) if isinstance(origin, int) and isinstance(row.get('video_clock'), int) else None
        rows.append(row)
    live = [row for row in rows if row.get('wp1980_started') == 1]
    guarded = [row for row in live if stable(row)]
    planned = planned_pulses(actions)
    last_field = max((row['relative_field'] for row in rows if isinstance(row['relative_field'], int)), default=-1)
    sent = actual_pulses(events, last_field)
    pulse_results = []
    for pulse in planned:
        matches = [item for item in sent if item['buttons'] == pulse['buttons'] and
                   abs(item['first'] - pulse['first']) <= lag_fields]
        actual = min(matches, key=lambda item: abs(item['first'] - pulse['first'])) if matches else None
        first = actual['first'] if actual else pulse['first']
        last = actual['last'] if actual else pulse['last']
        observed = [row for row in guarded if row['relative_field'] is not None and first <= row['relative_field'] <= last + lag_fields]
        mask = sum(BUTTONS[name]['pad'] for name in pulse['buttons'])
        physical = [row for row in observed if row.get('pad_connected') == 1 and row.get('pad_level') == mask]
        result = dict(planned=pulse, action_sent=actual is not None, actual=actual,
                      expected_pad_mask=f'0x{mask:04x}', window=window(observed),
                      observed_pad_masks=histogram(observed, 'pad_level', 4), observed_input_bytes=histogram(observed, 'input0', 2),
                      physical_status='PASS' if actual and physical else 'INCONCLUSIVE',
                      physical_evidence=reference(physical[0]) if physical else None)
        if len(pulse['buttons']) == 1:
            button = pulse['buttons'][0];spec = BUTTONS[button]
            expected_input = 0xff ^ spec['active']
            mapped = [row for row in physical if row.get('input0') == expected_input and row.get('wp1980_demo_active') == 0]
            decoded = []
            for row in mapped:
                value = actor(row.get('actor0'))
                if value and ((value['direction'] & 0xf) == spec['actor'] if 'axis' in spec else
                              bool((value['fresh'] | value['accumulated'] | value['held']) & spec['actor'])):
                    decoded.append(row)
            result.update(button=button, expected_input0=f'0x{expected_input:02x}',
                          expected_actor_mask=f"0x{spec['actor']:04x}",
                          gameplay_status='PASS' if actual and mapped else 'INCONCLUSIVE',
                          actor_input_status='PASS' if actual and decoded else 'INCONCLUSIVE',
                          gameplay_evidence=reference(mapped[0]) if mapped else None,
                          actor_input_evidence=reference(decoded[0]) if decoded else None)
            result['status'] = 'PASS' if actual and physical and mapped and decoded else 'INCONCLUSIVE'
            if 'axis' in spec:
                result['movement'] = motion_analysis(observed, button)
        else:
            result['status'] = result['physical_status']
            result['scope'] = 'Combined physical input; individual active-low bytes and actor actions are not asserted.'
        pulse_results.append(result)

    # Manual-mode persistence is assessed only after physical inputs begin.
    earliest_sent = min((item['first'] for item in sent), default=None)
    physical_rows = [row for row in guarded if earliest_sent is not None and
                     row['relative_field'] is not None and row['relative_field'] >= earliest_sent and
                     row.get('pad_connected') == 1 and isinstance(row.get('pad_level'), int) and row['pad_level'] & MAPPED_MASK]
    first_physical = physical_rows[0] if physical_rows else None
    zeros = [row for row in guarded if first_physical and row['sample_index'] >= first_physical['sample_index'] and row.get('wp1980_demo_active') == 0]
    first_manual = zeros[0] if zeros else None
    subsequent = [row for row in guarded if first_manual and row['sample_index'] >= first_manual['sample_index']]
    reverted = [row for row in subsequent if row.get('wp1980_demo_active') == 1]
    takeover_delay = difference(first_manual['video_clock'], first_physical['video_clock']) if first_manual else None
    persistence_span = difference(subsequent[-1]['video_clock'], first_manual['video_clock']) if subsequent else 0
    takeover_status = ('FAIL' if reverted else 'PASS' if first_manual and takeover_delay <= lag_fields and persistence_span >= 1 else 'INCONCLUSIVE')
    takeover = dict(status=takeover_status, first_physical=reference(first_physical) if first_physical else None,
                    first_manual=reference(first_manual) if first_manual else None, native_fields_to_manual=takeover_delay,
                    observed_post_takeover_fields=persistence_span, observed_manual_samples=len(zeros),
                    reverted_samples=len(reverted), first_reversion=reference(reverted[0]) if reverted else None,
                    scope='Manual flag remains 0 in recorded guarded observations; unobserved intervals are not claimed atomic.')

    resets = []
    for pulse_result in pulse_results:
        pulse = pulse_result['planned']
        if not RESET_BUTTONS.issubset(pulse['buttons']):
            continue
        start_field = pulse_result['actual']['first'] if pulse_result['actual'] else pulse['first']
        before = [row for row in guarded if row['relative_field'] is not None and row['relative_field'] < start_field and row.get('start_clock', 0) > 0]
        baseline = before[-1] if before else None
        after = [row for row in guarded if row['relative_field'] is not None and row['relative_field'] >= start_field]
        old_clock = baseline.get('start_clock') if baseline else None
        changed = [row for row in after if isinstance(row.get('start_clock'), int) and row['start_clock'] > 0 and
                   old_clock is not None and difference(row['start_clock'], old_clock) > 0]
        epochs = list(dict.fromkeys(row['start_clock'] for row in changed))
        first_restart = changed[0] if changed else None
        reload_rows = [row for row in after if not first_restart or row['sample_index'] <= first_restart['sample_index']]
        stage_rows = [row for row in reload_rows if row.get('royal_stage') in (1, 2)]
        stage_complete = [row for row in after if stage_rows and row['sample_index'] > stage_rows[0]['sample_index'] and row.get('royal_stage') == 3]
        dropped = [row for row in reload_rows if baseline and isinstance(row.get('game_frame'), int) and row['game_frame'] < baseline.get('game_frame', 0)]
        resumed = [row for row in changed if row.get('royal_stage') == 3 and row.get('game_frame', 0) > 0 and
                   baseline and row.get('wp1980_draws', 0) > baseline.get('wp1980_draws', 0)]
        resumed_frames = sorted({row['game_frame'] for row in resumed})
        manual_after = bool(changed) and all(row.get('wp1980_demo_active') == 0 for row in changed)
        reset_status = ('FAIL' if len(epochs) > 1 or (changed and not manual_after) else
                        'PASS' if pulse_result['physical_status'] == 'PASS' and len(epochs) == 1 and
                        dropped and stage_rows and stage_complete and len(resumed_frames) >= 2 and manual_after else 'INCONCLUSIVE')
        first_input_status = ('NOT_REQUESTED' if actions.get('scenario') != 'reset-first' else
                              'PASS' if first_physical and first_physical.get('pad_level') == RESET_MASK and
                              sent and set(sent[0]['buttons']) == RESET_BUTTONS else 'INCONCLUSIVE')
        resets.append(dict(status=reset_status, planned=pulse, physical_status=pulse_result['physical_status'],
                           old_start_clock=old_clock, observed_new_start_clocks=epochs,
                           game_frame_decrease_observed=bool(dropped), reload_stages=sorted({row.get('royal_stage') for row in reload_rows}),
                           reload_marker_sequence_observed=bool(stage_rows and stage_complete),
                           first_restarted_sample=reference(first_restart) if first_restart else None,
                           first_resumed_sample=reference(resumed[0]) if resumed else None,
                           resumed_frame_range=[resumed_frames[0], resumed_frames[-1]] if resumed_frames else None,
                           manual_after_restart=manual_after, reset_armed_observed=histogram(after, 'reset_armed'),
                           first_native_input_is_full_chord=first_input_status,
                           reload_scope='Successful original-load markers plus restarted counters; not a bytewise atomic snapshot comparison.'))

    buttons = {}
    for name, spec in BUTTONS.items():
        singles = [result for result in pulse_results if result.get('button') == name]
        chords = [result for result in pulse_results if name in result['planned']['buttons'] and len(result['planned']['buttons']) > 1]
        status = ('PASS' if singles and all(result['status'] == 'PASS' for result in singles) else
                  'INCONCLUSIVE' if singles else 'PASS' if chords and all(result['physical_status'] == 'PASS' for result in chords) else
                  'INCONCLUSIVE' if chords else 'NOT_REQUESTED')
        buttons[name] = dict(status=status, expected_pad_mask=f"0x{spec['pad']:04x}",
                             expected_single_input0=f"0x{0xff ^ spec['active']:02x}",
                             single_pulses=len(singles), chord_pulses=len(chords),
                             scope='Individual physical/gameplay/actor mapping' if singles else 'Physical bit inside requested chord only' if chords else 'Not requested')
    fatal = [row for row in live if any(isinstance(row.get(name), int) and row[name] != 0 for name in ('fatal_owner', 'fatal_dependency'))]
    fatal_complete = bool(live) and all(all(row.get(name) == 0 for name in ('fatal_owner', 'fatal_dependency')) for row in live)
    health = dict(status='FAIL' if fatal else 'PASS' if fatal_complete else 'INCONCLUSIVE',
                  first_fatal=reference(fatal[0]) if fatal else None, live_samples=len(live))
    malformed = Counter()
    for row in live:
        for slot in range(9):
            if actor(row.get('actor' + str(slot))) is None:malformed['actor' + str(slot)] += 1
    required = [item['status'] for item in buttons.values() if item['status'] != 'NOT_REQUESTED']
    required += [takeover_status, health['status']]
    required += [item['status'] for item in resets]
    required += [item['first_native_input_is_full_chord'] for item in resets if item['first_native_input_is_full_chord'] != 'NOT_REQUESTED']
    overall = ('FAIL' if 'FAIL' in required else 'PASS' if complete and planned and not malformed and all(status == 'PASS' for status in required) else 'INCONCLUSIVE')
    return dict(status=overall, scope='Read-only ordinary-controller observations; no memory writes, no every-field or atomic-state claim',
                capture_complete=complete, capture_status=capture_report.get('status'), scenario=actions.get('scenario'),
                controller_origin=origin, origin_source=origin_source, ordinary_input_lag_allowance_fields=lag_fields,
                sample_count=len(rows), live_guarded_samples=len(guarded), live_unstable_samples=len(live) - len(guarded),
                observer_sampling_statistics=capture_report.get('observer_stats'),
                actor_records=dict(slots=9, bytes_each=268, malformed_or_missing_samples=dict(malformed)),
                buttons=buttons, pulses=pulse_results, manual_takeover=takeover, resets=resets, fatal=health,
                limitations=['Guarded samples remain asynchronous; actors are not asserted to be atomic snapshots.',
                             'Movement rates exclude gaps, non-walking states, changed inputs/characters and unsettled observations.',
                             'A/B/C register evidence does not assert a named attack outcome.',
                             'PASS describes observed requested controls; disc finality is determined by the capture identity, not this analyzer.'])


def read_json(path, fallback, errors):
    try:return json.loads(path.read_text())
    except FileNotFoundError:return fallback
    except (OSError, json.JSONDecodeError, UnicodeDecodeError) as exc:
        errors[str(path)] = repr(exc)
        return fallback


def analyze_capture(directory, lag_fields=8):
    directory = Path(directory).resolve()
    paths = [directory / name for name in ('samples.jsonl.gz', 'input-events.json', 'actions.json', 'report.json', 'failure.json')]
    before = {str(path): (path.stat().st_size, path.stat().st_mtime_ns) for path in paths if path.exists()}
    samples = [];read_error = None
    try:
        with gzip.open(paths[0], 'rt') as source:
            for line in source:
                if line.strip():samples.append(json.loads(line))
    except (OSError, EOFError, json.JSONDecodeError, UnicodeDecodeError) as exc:
        read_error = repr(exc)
    json_errors = {}
    report = read_json(directory / 'report.json', {}, json_errors)
    actions = read_json(directory / 'actions.json', {}, json_errors)
    events = read_json(directory / 'input-events.json', report.get('input_events', []), json_errors)
    failure = read_json(directory / 'failure.json', None, json_errors)
    after = {str(path): (path.stat().st_size, path.stat().st_mtime_ns) for path in paths if path.exists()}
    inputs_unchanged = before == after
    complete = report.get('status') == 'PASS' and not failure and not read_error and not json_errors and inputs_unchanged
    result = analyze_data(samples, actions, events, report, complete=complete, lag_fields=lag_fields)
    result.update(capture_directory=str(directory), sample_read_error=read_error, json_read_errors=json_errors,
                  capture_failure=failure.get('exception') if failure else None,
                  input_files_unchanged_during_read=inputs_unchanged,
                  input_hashes={str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths if path.exists()})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--lag-fields', type=int, default=8)
    args = parser.parse_args()
    if not 0 <= args.lag_fields <= 120:parser.error('--lag-fields must be 0..120')
    result = analyze_capture(args.capture, args.lag_fields)
    text = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
        print(json.dumps({key: result[key] for key in ('status', 'capture_complete', 'scenario', 'buttons', 'fatal')}, indent=2))
    else:print(text, end='')


if __name__ == '__main__':main()
