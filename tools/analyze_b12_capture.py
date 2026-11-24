#!/usr/bin/env python3
"""Compare B12 Saturn captures to the independently run native application.

This release wrapper supplies B12's changed TAG outcome to the existing
capture analyzer without overwriting its historical B11 fixture constants.
It validates compiled target layout and records the native reference hashes.
Host results are logic references, never Saturn timing measurements.
"""
from pathlib import Path
import hashlib
import json
import analyze_capture as capture

ROOT=Path(__file__).resolve().parents[1]


def main():
    evidence=ROOT/'evidence/boot12'
    native_path=evidence/'main-oracle/summary.json'
    native=json.loads(native_path.read_text())
    assert not native['sources_changed_during_run']
    references=evidence/'analysis-references';references.mkdir(exist_ok=True)
    contexts={}
    for mode in ('tag','royal'):
        run=native['runs']['frontend-'+mode]
        assert run['returncode']==0
        terminal=run['terminal']
        actor_path=evidence/'main-oracle'/('frontend-'+mode)/'terminal-actors.bin'
        if not actor_path.is_file():actor_path=references/('frontend-'+mode+'-terminal-actors.bin')
        actors=actor_path.read_bytes()
        assert len(actors)==2412
        digest=hashlib.sha256(actors).hexdigest()
        assert digest==terminal['actor_sha256']
        contexts[mode]={'tick':terminal['frame'],'owner':terminal['terminal_owner'],
                        'rng':terminal['rng'],'front_ticks':terminal['frontend_ticks'],
                        'sha256':digest}
        (references/('frontend-'+mode+'-terminal-actors.bin')).write_bytes(actors)
    capture.FRONTEND_REFERENCES=contexts
    layout=json.loads((evidence/'capture-sh2-layout.json').read_text())
    assert all(capture.sha(ROOT/path)==sha for path,sha in layout['sources_sha256'].items())
    report=evidence/'tag-complete/report.json'
    result=capture.analyze(report,layout,references,[120,600],evidence/'tag-emulator-terminal-actors.bin')
    terminal=result['terminal_actors']
    assert result['capture_status']=='CAPTURED'
    assert result['stop_reason']=='frontend result owner completed'
    assert terminal['status']=='MATCH' and terminal['actors_equal'] and terminal['rng_equal']
    assert terminal['header']['complete']==1 and not result['faults']
    assert result['audio']['last']['fault']==0
    output={'format':'wf-b12-native-sh2-comparison','reference_scope':
            'B12 production native main executed with deterministic ordinary input; '
            'ROM parity is established separately for the changed owners, not every frame.',
            'native_summary_sha256':capture.sha(native_path),'native_reference_contexts':contexts,
            'analyses':[result]}
    (evidence/'tag-capture-analysis.json').write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps({'terminal':terminal['status'],'combat':result['combat_from_terminal_snapshot'],
                      'post_combat':result['frontend_post_transition'],
                      'audio':{k:result['audio']['last'][k] for k in
                               ('pcm_underruns','music_underruns','fault','queue_overflows','music_io_errors')}},indent=2))


if __name__=='__main__':main()
