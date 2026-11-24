#!/usr/bin/env python3
"""Analyze a cold Saturn capture against a freshly executed native reference.

The native run supplies logic and the original-wait clock schedule. It does
not measure the original PCB or physical Saturn. Guest VBlank counters, not
host execution time, supply the measured duration.
"""
import argparse
import hashlib
import json
from pathlib import Path
import analyze_capture as capture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--require-terminal', action='store_true')
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    native = json.loads((args.native/'summary.json').read_text())
    assert not native['sources_changed_during_run'], 'Native source changed during reference execution'
    reference = args.output.parent/'references'
    reference.mkdir(exist_ok=True)
    contexts, schedules = {}, {}
    for mode in ('royal', 'tag'):
        run = native['runs']['frontend-'+mode]
        assert run['returncode'] == 0
        terminal = run['terminal']
        actors = (args.native/('frontend-'+mode)/'terminal-actors.bin').read_bytes()
        digest = hashlib.sha256(actors).hexdigest()
        assert len(actors) == 2412 and digest == terminal['actor_sha256']
        (reference/('frontend-'+mode+'-terminal-actors.bin')).write_bytes(actors)
        contexts[mode] = dict(tick=terminal['frame'], owner=terminal['terminal_owner'],
                              rng=terminal['rng'], front_ticks=terminal['frontend_ticks'], sha256=digest)
        schedules[mode] = terminal['host_fields']-run['prepared']['host_fields']
    capture.FRONTEND_REFERENCES = contexts
    layout = capture.prove_layout(args.output.parent/'capture-sh2-layout.json')
    source = args.capture/'report.json' if args.capture.is_dir() else args.capture
    result = capture.analyze(source, layout, reference, [120, 600], args.output.parent/'terminal-actors.bin')
    terminal = result['terminal_actors']
    measured = result['combat_from_terminal_snapshot']
    if measured:
        target = schedules[terminal['mode']]
        result['native_clock_schedule'] = {
            'fields': target, 'seconds': target/capture.SATURN_HZ,
            'measured_fields': measured['fields'], 'extra_fields': measured['fields']-target,
            'schedule_speed_percent': target*100/measured['fields'],
            'scope': 'Actual B12 main on zero-cost host hardware with the unchanged rational clock and original waits; not PCB timing.'}
    good = (result['capture_status'] == 'CAPTURED' and
            result['stop_reason'] == 'frontend result owner completed' and
            terminal.get('status') == 'MATCH' and terminal.get('actors_equal') and terminal.get('rng_equal') and
            terminal.get('header', {}).get('complete') == 1 and not result['faults'] and
            not result['audio'].get('nonzero_issue_counters'))
    result['terminal_logic_audio_gate'] = bool(good)
    result['native_summary_sha256'] = capture.sha(args.native/'summary.json')
    result['native_reference_contexts'] = contexts
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({'terminal': terminal.get('status'), 'terminal_logic_audio_gate': bool(good),
                      'combat': measured, 'schedule': result.get('native_clock_schedule'),
                      'faults': result['faults'], 'audio_issues': result['audio'].get('nonzero_issue_counters')}, indent=2))
    return 0 if good or not args.require_terminal else 1


if __name__ == '__main__':
    raise SystemExit(main())
