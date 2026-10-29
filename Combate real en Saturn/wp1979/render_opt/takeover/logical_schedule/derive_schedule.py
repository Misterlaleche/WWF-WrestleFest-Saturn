#!/usr/bin/env python3
"""Derive ordinary pad triggers from the previous observed baseline sample."""
import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
PROJECT = next(p for p in HERE.parents if (p / 'native/src/runtime').is_dir())


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def derive(report_path, schedule_path):
    report = json.loads(Path(report_path).read_text())
    schedule = json.loads(Path(schedule_path).read_text())
    assert report['status'] == 'complete' and not report.get('synthetic')
    schedule = sorted(schedule, key=lambda e: e['frame'])
    assert report['events'] == [dict(e, applied_at_frame=e['frame']) for e in schedule]
    samples = {s['emulator_frame']: s for s in report['samples']}
    result = []
    evidence = []
    for event in schedule:
        frame = event['frame']
        if frame == 0:
            assert not event.get('buttons'), 'Only initial neutral setup can lack a prior observation'
            result.append(dict(event))
            continue
        prior = samples[frame - 1]
        assert prior['wp1979_started'] == 1
        threshold = prior['game_frame']
        first_eligible = min(s['emulator_frame'] + 1 for s in samples.values()
                             if s.get('wp1979_started') == 1 and s['game_frame'] >= threshold)
        assert first_eligible == frame, 'Original event was not at the first eligible observation'
        derived = {k: v for k, v in event.items() if k != 'frame'}
        derived['observed_game_frame'] = threshold
        result.append(derived)
        evidence.append(dict(original_emulator_frame=frame,
                             preceding_sample_emulator_frame=frame - 1,
                             observed_game_frame=threshold,
                             first_eligible_emulator_frame=first_eligible,
                             buttons=event.get('buttons', [])))
    return result, dict(kind='derived-from-actual-baseline-read-only-observations',
                        baseline_report=str(Path(report_path).resolve()),
                        baseline_report_sha256=digest(report_path),
                        original_schedule_sha256=digest(schedule_path),
                        method='Threshold = report.samples[event.frame - 1].game_frame; no forced native counter',
                        event_derivation=evidence)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, default=PROJECT / 'measure/candidate-2-controller/report.json')
    p.add_argument('--schedule', type=Path, default=HERE.parent / 'controller-schedule.json')
    p.add_argument('--out', type=Path, default=HERE / 'controller-logical-schedule.json')
    a = p.parse_args()
    schedule, evidence = derive(a.report, a.schedule)
    a.out.write_text(json.dumps(schedule, indent=2) + '\n')
    evidence['derived_schedule_sha256'] = digest(a.out)
    a.out.with_suffix('.provenance.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
