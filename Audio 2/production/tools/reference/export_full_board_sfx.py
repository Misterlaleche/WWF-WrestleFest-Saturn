#!/usr/bin/env python3
"""Export validated original full-board SFX windows without altering level/time."""
import argparse
import json
from pathlib import Path
import numpy as np
from reproduce_full_board_references import PACKAGE,sha,write_wav

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--validation',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();a.validation=a.validation.resolve();a.out=a.out.resolve()
    if a.out==PACKAGE or PACKAGE in a.out.parents:p.error('Private audio must remain outside source package')
    source=a.validation/'full-board-sfx-validation.json';data=json.loads(source.read_text())
    if not data['complete'] or not data['native_original_start_gate_passed']:raise ValueError('Require complete original98 start gate')
    a.out.mkdir(parents=True,exist_ok=False);(a.out/'wav').mkdir()
    report={'kind':'FULL_ORIGINAL_BOARD_SFX_SOFTWARE_REFERENCE_NOT_HARDWARE_CAPTURE','profile':'full-board-sfx',
      'sample_rate':44100,'channels':1,'bits':16,'source_sample_rate':8000,'normalization':False,
      'timers':'Pinned coupled Nuked TimerA+B and YMbusy; original Z80 control',
      'oki':'Original ROM-addressed ADPCM, reset0, busy/status and global banking',
      'route_gain':'21627/32768 nominal OKI route; no measured analog filter',
      'origin_rule':'Original sound latch/NMI at time0 after two-second board boot; actual driver latency retained',
      'end_rule':'Complete fixed validation window, ceil(historical A15 reference duration)+2seconds; final exact silence checked separately',
      'validation_report_sha256':sha(source),'historical_A15_extra_start_commands':data['legacy_model_mismatched_commands'],
      'commands':[],'complete':False,'limitations':['Host instruction-level board model, not physical arcade recording',
        'Matching isolated starts does not close mixed-command priorities, bank changes or native waveform fidelity']}
    for item in data['commands']:
        c=item['command'];capture=item['capture'];raw=Path(capture['pcm'])
        if sha(raw)!=capture['output_sha256']['pcm']:raise ValueError('Raw PCM changed')
        samples=np.frombuffer(raw.read_bytes(),dtype='>i2');active=np.flatnonzero(samples)
        silence=len(samples)-int(active[-1])-1 if len(active) else len(samples)
        wav=a.out/'wav'/f'ARCADE-REFERENCIA-SFX-0x{c:02X}.wav';write_wav(raw,wav)
        report['commands'].append({'command':c,'frames':len(samples),'seconds':len(samples)/44100,
          'wav':str(wav),'wav_sha256':sha(wav),'pcm_sha256':sha(raw),
          'trace_sha256':capture['output_sha256']['trace'],'original_start_count':len(item['original_starts']),
          'final_silence_frames':silence,'final_second_exact_silence':silence>=44100,
          'peak':int(np.abs(samples.astype(np.int32)).max()),'clipped':capture['clipped_frames']})
    report['complete']=len(report['commands'])==98
    report['all_final_seconds_exact_silence']=all(r['final_second_exact_silence'] for r in report['commands'])
    (a.out/'reference-sfx-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'complete':report['complete'],'all_final_seconds_exact_silence':report['all_final_seconds_exact_silence']}))
    return 0 if report['complete'] and report['all_final_seconds_exact_silence'] else 1

if __name__=='__main__':raise SystemExit(main())
