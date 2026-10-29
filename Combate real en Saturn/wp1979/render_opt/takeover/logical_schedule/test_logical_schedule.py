#!/usr/bin/env python3
"""Synthetic scheduler/parser regression. Never loads a core or writes Saturn RAM.

The actor fixtures are explicitly resampled copies of an existing trace, not
new emulator evidence. The production scheduler block and callback are executed
with Python mocks; the independent analyzer must reject a delayed-trigger bug.
"""
import ast
import copy
import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
PROJECT = next(p for p in HERE.parents if (p / 'native/src/runtime').is_dir())
BUILD = HERE / 'build'
BUILD.mkdir(exist_ok=True)
CAPTURE = HERE / 'capture_yabause-proposed.py'
spec = importlib.util.spec_from_file_location('logical_analyzer', HERE / 'analyze_takeover-proposed.py')
analyzer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analyzer)
analyzer.CAPTURE = CAPTURE
physical = json.loads((HERE.parent / 'controller-schedule.json').read_text())
logical_path = HERE / 'controller-logical-schedule.json'
logical = json.loads(logical_path.read_text())
actual_baseline = json.loads((PROJECT / 'measure/candidate-2-controller/report.json').read_text())


def scheduler(source, schedule, samples):
    """Execute the actual pre-retro_run AST block, with no ctypes/core import."""
    tree = ast.parse(source)
    selected = []
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name in ('prepare_schedule', 'schedule_due', 'input_state'):
            node.decorator_list = []
            selected.append(node)
        elif isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'buttons' for t in node.targets):
            selected.append(node)
    scope = {}
    exec(compile(ast.Module(body=selected, type_ignores=[]), '<actual-scheduler-functions>', 'exec'), scope)
    prepared = scope['prepare_schedule'](copy.deepcopy(schedule)) if 'prepare_schedule' in scope else sorted(schedule, key=lambda e: e['frame'])
    loop = next(n for n in ast.walk(tree) if isinstance(n, ast.For) and isinstance(n.target, ast.Name) and n.target.id == 'frame')
    end = next(i for i, n in enumerate(loop.body) if isinstance(n, ast.Expr) and isinstance(n.value, ast.Call)
               and isinstance(n.value.func, ast.Attribute) and n.value.func.attr == 'retro_run')
    begin = next(i for i, n in enumerate(loop.body) if isinstance(n, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == 'observed_game_frame' for t in n.targets)) if 'prepare_schedule' in scope else next(i for i, n in enumerate(loop.body) if isinstance(n, ast.While))
    code = compile(ast.Module(body=loop.body[begin:end + 1], type_ignores=[]), '<actual-pre-retro-run-block>', 'exec')
    read_count = 0
    callbacks = []
    before = {}

    def read_memory(address, size):
        nonlocal read_count
        read_count += 1
        assert size == 4
        return before.get('wp1979_started' if address == 0x100 else 'game_frame', 0).to_bytes(4, 'big')

    def retro_run():
        callback = scope['input_state']
        callbacks.append([[button for button in range(16) if callback(port, 1, 0, button)] for port in (0, 1)])
        assert not callback(2, 1, 0, 4) and not callback(0, 2, 0, 4)

    scope.update(schedule=prepared, event_at=0, events=[], pad=[set(), set()],
                 logical_schedule=any('observed_game_frame' in e for e in prepared),
                 logical_watch={'wp1979_started': (0x100, 4), 'game_frame': (0x104, 4)},
                 read_memory=read_memory, core=SimpleNamespace(retro_run=retro_run))
    for frame in range(len(samples)):
        before = samples[frame - 1] if frame else {'wp1979_started': 0, 'game_frame': 0xffffffff}
        scope['frame'] = frame
        exec(code, scope)
    return scope['events'], callbacks, read_count, scope


source = CAPTURE.read_text()
old_source = (HERE / 'capture_yabause-baseline.py').read_text()
old_events, old_callbacks, _, _ = scheduler(old_source, list(reversed(physical)), actual_baseline['samples'])
new_events, new_callbacks, reads, scope = scheduler(source, list(reversed(physical)), actual_baseline['samples'])
assert old_events == new_events == actual_baseline['events']
assert old_callbacks == new_callbacks and reads == 0, 'Frame-only path must not add memory reads'
events, callbacks, _, _ = scheduler(source, logical, actual_baseline['samples'])
assert [e['applied_at_frame'] for e in events] == [e['applied_at_frame'] for e in old_events]
assert callbacks == old_callbacks, 'Logical triggers must reproduce every baseline pad callback'
for event in events[1:]:
    assert event['trigger_game_frame'] == event['observed_game_frame']
for invalid in [{'frame': 1, 'observed_game_frame': 1}, {'frame': -1}, {'observed_game_frame': True}, {'frame': 1, 'buttons': ['bogus']}]:
    try:
        scope['prepare_schedule']([invalid])
    except ValueError:
        pass
    else:
        raise AssertionError(f'Invalid event accepted: {invalid}')

# Counter jumps are >= conditions; the requested threshold and actual reading
# must both survive in the log. Garbage before runtime initialization is ignored.
tiny = [dict(wp1979_started=0, game_frame=0xffffffff),
        dict(wp1979_started=1, game_frame=63),
        dict(wp1979_started=1, game_frame=66),
        dict(wp1979_started=1, game_frame=67)]
jump, jump_callbacks, _, _ = scheduler(source, [{'observed_game_frame': 64, 'buttons': ['left']}], tiny)
assert jump == [{'observed_game_frame': 66, 'buttons': ['left'], 'trigger_game_frame': 64, 'applied_at_frame': 3}]
assert jump_callbacks[:3] == [[[], []]] * 3 and jump_callbacks[3] == [[6], []]

# Deliberately warp only the observation clock. This is an analyzer fixture,
# never evidence that new hardware/emulator execution preserved the route.
by_game = {s['game_frame']: s for s in actual_baseline['samples'] if s.get('wp1979_started') == 1}
warped_samples = [dict(wp1979_started=0, game_frame=0xffffffff, wp1979_demo_active=0,
                       fatal_owner=0, fatal_dependency=0) for _ in range(517)]
for game, sample in sorted(by_game.items()):
    keys = ('wp1979_started', 'game_frame', 'wp1979_demo_active', 'fatal_owner', 'fatal_dependency', 'actor0', 'actor1', 'actor4', 'actor5')
    observed = {key: sample[key] for key in keys}
    warped_samples.append(observed.copy())
    if game % 19 == 0:
        warped_samples.append(observed.copy())
for frame, sample in enumerate(warped_samples):
    sample['emulator_frame'] = frame
warped_events, warped_callbacks, _, _ = scheduler(source, logical, warped_samples)
assert warped_events[1]['applied_at_frame'] != 540
assert [e.get('observed_game_frame') for e in warped_events] == [e.get('observed_game_frame') for e in events]

baseline_fixture = copy.deepcopy(actual_baseline)
baseline_fixture['synthetic'] = True
baseline_path = BUILD / 'synthetic-baseline-report.json'
baseline_path.write_text(json.dumps(baseline_fixture))
fixture = BUILD / 'synthetic-candidate'
fixture.mkdir(exist_ok=True)
(fixture / 'identity.json').write_text(json.dumps({'a_bin_matches': True, 'sha256': {'fixture': 'SYNTHETIC-NO-DISC'}}))
(fixture / 'command.json').write_text(json.dumps(['python', str(CAPTURE), '--schedule', str(logical_path)]))
warped = dict(status='complete', synthetic=True, frames_run=len(warped_samples),
              samples=warped_samples, events=warped_events,
              capture_script_sha256=analyzer.sha(CAPTURE), schedule_sha256=analyzer.sha(logical_path))


def analyze(report):
    (fixture / 'report.json').write_text(json.dumps(report))
    return analyzer.analyze(fixture, logical_path, allow_synthetic=True, baseline_report=baseline_path)


green = analyze(warped)
assert green['status'] == 'pass', green['failures']
assert green['actor_route_comparison']['compared_actor_records'] == 2052

# Actual scheduler mutation: strict > waits one additional logical counter.
mutated_source = source.replace("event['observed_game_frame'] <= observed_game_frame", "event['observed_game_frame'] < observed_game_frame")
assert mutated_source != source
late_events, _, _, _ = scheduler(mutated_source, logical, warped_samples)
late = copy.deepcopy(warped)
late['events'] = late_events
red_late = analyze(late)
assert red_late['status'] == 'fail' and any('first eligible' in s for s in red_late['failures'])
wrong_actor = copy.deepcopy(warped)
for sample in wrong_actor['samples']:
    if sample.get('wp1979_started') == 1 and sample['game_frame'] == 200:
        actor = bytearray.fromhex(sample['actor4'])
        actor[6] ^= 1
        sample['actor4'] = actor.hex()
red_actor = analyze(wrong_actor)
assert red_actor['status'] == 'fail' and red_actor['actor_route_comparison']['different_records'] == 1
without_opt_in = analyzer.analyze(fixture, logical_path)
assert without_opt_in['status'] == 'fail' and 'Synthetic fixture is not an actual capture' in without_opt_in['failures']
analyze(warped)  # Keep a clearly labeled valid fixture for inspection.
result = dict(kind='synthetic-scheduler-and-analyzer-regression', actual_capture_performed=False,
              frame_only_events_and_all_callbacks_unchanged=True, frame_only_extra_memory_reads=reads,
              baseline_trace_replayed_events=len(events), baseline_callback_frames=len(callbacks),
              original_first_press_emulator_frame=events[1]['applied_at_frame'],
              warped_first_press_emulator_frame=warped_events[1]['applied_at_frame'],
              warped_first_press_observed_game_frame=warped_events[1]['observed_game_frame'],
              boot_garbage_guard=True, jumped_counter_keeps_threshold_and_observation=True,
              green=green['status'], rejects_delayed_scheduler_mutation=red_late['status'] == 'fail',
              rejects_changed_actor_byte=red_actor['status'] == 'fail',
              refuses_synthetic_as_actual=without_opt_in['status'] == 'fail',
              exact_fixture_actor_records=green['actor_route_comparison']['compared_actor_records'],
              exact_fixture_actor_bytes=green['actor_route_comparison']['compared_bytes'],
              source_sha256={p.name: analyzer.sha(p) for p in (CAPTURE, HERE / 'analyze_takeover-proposed.py')})
(HERE / 'test-result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
