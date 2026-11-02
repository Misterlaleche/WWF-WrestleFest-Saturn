#!/usr/bin/env python3
"""Full original-board model replay at observed native API admission times.

This isolates sound after API admission. It does not reconstruct PCB gameplay
timing, assert cycle-exact DAC alignment, or remove native runtime failures.
"""
import argparse
import json
from pathlib import Path
import struct
import wave

from reproduce_full_board_references import PACKAGE,prepare_and_build,run_case,sha,write_wav
from run_music_matrix import replay_observation

def excerpt(source,target,start,frames):
    with wave.open(str(source)) as f:
        if not 0<=start<start+frames<=f.getnframes():raise ValueError('Missing requested raw WAV window')
        params=f.getparams();f.setpos(start);data=f.readframes(frames)
    with wave.open(str(target),'wb') as f:
        f.setnchannels(params.nchannels);f.setsampwidth(params.sampwidth);f.setframerate(params.framerate);f.writeframes(data)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('rom-zip','out','replay','native-build','native-capture'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--opm-dir',type=Path);p.add_argument('--cc',default='cc');p.add_argument('--seconds',type=int,default=138)
    p.add_argument('--excerpt-start',type=int,default=50);p.add_argument('--excerpt-seconds',type=int,default=30)
    a=p.parse_args()
    for name,value in vars(a).items():
        if isinstance(value,Path):setattr(a,name,value.resolve())
    if a.out==PACKAGE or PACKAGE in a.out.parents:p.error('Private output must remain outside source package')
    plan=json.loads(a.replay.read_text());observed=replay_observation(a.native_build,a.native_capture,plan['events'])
    events=[{'sample':r['sample'],'word':r['word']} for r in observed['observations']]
    if not all(observed['gates'][k] for k in ['all_rows_present','exact_original_words','all_results_accepted','first_admission_is_epoch','no_early_admission','monotonic_admission']):
        raise ValueError('Cannot reconstruct missing or mismatched native admissions')
    if any(left['sample']>=right['sample'] for left,right in zip(events,events[1:])):
        raise ValueError('Simultaneous observed admissions; no invented NMI spacing')
    if not 0<=a.excerpt_start<a.excerpt_start+a.excerpt_seconds<=a.seconds:p.error('Excerpt outside replay window')
    baseline=prepare_and_build(a.out,a.rom_zip,a.opm_dir,a.cc)
    (a.out/'requested-replay.json').write_text(json.dumps(plan,indent=2)+'\n')
    (a.out/'observed-replay.json').write_text(json.dumps(observed,indent=2)+'\n')
    item=run_case(a.out,'observed-mix',events,a.seconds)
    destination=a.out/'wav';destination.mkdir()
    ref=destination/'ARCADE-REFERENCIA-MEZCLA.wav';native=destination/'SATURN-AUDIO2-MEZCLA.wav';write_wav(item['pcm'],ref)
    capture=json.loads((a.native_capture/'report.json').read_text());d=capture['final_watch_values']
    ram=(a.native_capture/'SCSP-RAM-saturn-order.bin').read_bytes();clock=struct.unpack_from('>H',ram,0xb010)[0]
    residual=(clock-d['DSPclock16'])&65535
    if residual>1475:raise ValueError('Native final clock is more than two video frames stale')
    with wave.open(str(a.native_capture/'native.wav')) as f:
        if f.getframerate()!=44100 or f.getsampwidth()!=2:raise ValueError('Expected raw nativePCM16/44100')
        origin=f.getnframes()-d['SCSPelapsed']-residual
    excerpt(a.native_capture/'native.wav',native,origin,a.seconds*44100)
    windows=[]
    for wav in (ref,native):
        target=wav.with_name(wav.stem+f'-{a.excerpt_start}-{a.excerpt_start+a.excerpt_seconds}.wav')
        excerpt(wav,target,a.excerpt_start*44100,a.excerpt_seconds*44100)
        windows.append({'wav':str(target),'sha256':sha(target)})
    native_gates=json.loads((a.native_capture/'native-gates.json').read_text())
    report={**baseline,'kind':'OBSERVED_NATIVE_ADMISSION_FULL_BOARD_MIX_COMPARISON','profile':'full-board',
      'seconds':a.seconds,'reference_capture':item,'original_requested_replay_sha256':sha(a.replay),
      'observed_admissions':observed,'native_runtime_gates_preserved':native_gates,
      'native_capture_report_sha256':sha(a.native_capture/'report.json'),'native_firmware_sha256':sha(a.native_build/'SCSP.ELF'),
      'raw_native_wav_sha256':sha(a.native_capture/'native.wav'),
      'raw_native_source_begin_frame':origin,'raw_native_source_end_frame_exclusive':origin+a.seconds*44100,
      'final_diagnostic_staleness_samples':residual,'native_music_lead_not_removed_samples':4096,
      'normalization':False,'resampling':False,'time_stretch':False,'waveform_fitted_alignment':False,
      'reference_wav':str(ref),'reference_wav_sha256':sha(ref),'native_wav':str(native),'native_wav_sha256':sha(native),
      'excerpt_start_seconds':a.excerpt_start,'excerpt_seconds':a.excerpt_seconds,'excerpts':windows,
      'source_stable':all(sha(Path(path))==value for path,value in baseline['source_sha256'].items()),
      'scope':'Isolated observed API-admission replay. Native music lead and runtime failures remain audible; no gameplay/video or PCB timing equivalence.',
      'hardware_status':'HARDWARE PENDING','fidelity_approval':'PENDING','complete':True}
    (a.out/'mix-reference-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'complete':True,'native_runtime_passed':native_gates['passed'],'out':str(a.out)}))
    return 0 if report['source_stable'] else 1

if __name__=='__main__':raise SystemExit(main())
