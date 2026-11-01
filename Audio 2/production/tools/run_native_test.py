#!/usr/bin/env python3
"""Run the exact isolated build in standalone Mednafen and record real counters."""
import argparse,json,subprocess,sys
from pathlib import Path

FIELDS=['magic','state','videoFrames','error','commandStartFrame','musicState',
        'musicEvents','batchB03A','lateB028','sfxUnderruns32','lastCommand',
        'DSPclock16','diagAddress','mailState','FMdiscardEvents','discardBatches',
        'rejectedFM','ringRejected','SCSPelapsed','APIstate','APIerror','commandDSPclock',
        'replayAccepted','replayExpected']

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('build','environment-root','bios','out'):p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--display',type=int,default=86)
    p.add_argument('--wall-seconds',type=float,default=90)
    p.add_argument('--audio-only',action='store_true')
    a=p.parse_args();b=json.loads((a.build/'saturn-build.json').read_text())
    command=[sys.executable,str(Path(__file__).with_name('capture_mednafen.py')),
        '--environment-root',str(a.environment_root),'--cue',str(a.build/'AUDIO2.cue'),
        '--bios',str(a.bios),'--out',str(a.out),'--seconds',str(a.wall_seconds),
        '--display',str(a.display),'--dump-wram','--ready-watch','magic',
        '--ready-value','0x57464132','--stop-watch','state','--stop-value','4']
    if a.audio_only:command+=['--audio-only']
    for i,name in enumerate(FIELDS):command+=['--watch',f'{name}:{b["diag_address"]+i*4}:4']
    subprocess.run(command,check=True)
    report=json.loads((a.out/'report.json').read_text())
    d=report.get('final_watch_values',{})
    gates={
        'real_module_entered':d.get('magic')==0x57464132,
        'requested_window_completed':d.get('state')==4,
        'no_reported_worker_error':d.get('error')==0,
        'no_reported_api_error':d.get('APIerror')==0,
        'no_reported_fm_discard':d.get('FMdiscardEvents')==0 and d.get('discardBatches')==0,
        'no_reported_rejected_events':d.get('rejectedFM')==0 and d.get('ringRejected')==0,
        'no_reported_pcm_underrun':d.get('sfxUnderruns32')==0,
        'no_reported_transport_lateness':d.get('lateB028')==0,
        'worker_still_running':d.get('musicState')==2,
        'native_still_ready':d.get('mailState')==2,
    }
    if b.get('replay'):
        gates['all_replay_words_accepted']=d.get('replayAccepted')==b['replay']['events'] and d.get('replayExpected')==b['replay']['events']
    result={'build':b,'gates':gates,'diagnostics':d,'passed':all(gates.values()),
            'scope':'One isolated native capture; not gameplay, waveform fidelity or hardware certification.',
            'hardware_status':'HARDWARE PENDING'}
    (a.out/'native-gates.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2));return 0 if result['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
