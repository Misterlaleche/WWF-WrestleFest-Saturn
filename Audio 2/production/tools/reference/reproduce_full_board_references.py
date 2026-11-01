#!/usr/bin/env python3
"""Original full Z80 coupled to pinned OPM TimerA+B/busy and ROM-addressed OKI.

Host-only software reference. Bus timestamps retain the original CPU core's
instruction granularity; no analog filtering or physical-board claim is made.
Historical A53 and board-pcm16 outputs are never overwritten.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import wave

from prepare_reference_inputs import prepare
from reproduce_all_references import dependencies, OPM_COMMIT

HERE=Path(__file__).resolve().parent
PACKAGE=HERE.parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def prepare_and_build(out,rom_zip,opm_dir,cc='cc'):
    out=Path(out);out.mkdir(parents=True,exist_ok=False)
    inputs=out/'inputs';deps=out/'deps';prepare(Path(rom_zip).resolve(),inputs);dependencies(deps,opm_dir)
    audit=json.loads((HERE/'oracle_sfx/WF_OKI_AUDIT.json').read_text())
    if sha(inputs/'PCM16BE.PAK')!=audit['pak_sha256']:raise ValueError('Independent complete PCM audit mismatch')
    for sample in audit['samples']:
        data=(inputs/'PCM16BE.PAK').read_bytes()[sample['offset']:sample['offset']+sample['bytes']]
        if hashlib.sha256(data).hexdigest()!=sample['sha256_pcm16be']:raise ValueError('Independent PCM sample mismatch')
    fixture='typedef struct {unsigned bank,id,offset,frames;} fixture_t;\nstatic const fixture_t fixtures[114]={\n'
    fixture+=''.join('{%d,%d,%d,%d},\n'%(s['bank'],s['sample_id'],s['offset'],s['frames']) for s in audit['samples'])+'};\n'
    (inputs/'sfx_fixture.h').write_text(fixture)
    core=PACKAGE/'src/reference_driver';source=HERE/'capture_board_replay.c';obj=out/'capture_board_replay.o'
    builds=[
      [cc,'-O3','-std=c99','-Wall','-Wextra','-Werror','-I',str(core),'-I',str(deps),'-c',str(source),'-o',str(obj)],
      [cc,'-O3','-std=c99',str(obj),str(core/'z80.c'),str(deps/'opm.c'),'-lm','-o',str(out/'capture_board_replay')],
      [cc,'-O2','-std=c99','-I',str(HERE),'-I',str(inputs),str(PACKAGE/'tests/test_reference_oki.c'),'-o',str(out/'test_reference_oki')],
      [str(out/'test_reference_oki'),str(inputs/'SAMPLES.ROM'),str(inputs/'PCM16BE.PAK')]]
    for index,command in enumerate(builds):
        result=subprocess.run(command,capture_output=True,text=True)
        (out/f'build-{index}.log').write_text(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError('Host board build/114PCM gate failed: '+str(command))
    paths=[Path(__file__).resolve(),source,HERE/'reference_oki.h',HERE/'oracle_a53/tools/capture_full_audio_a50.c',
           core/'z80.c',core/'z80.h',deps/'opm.c',deps/'opm.h',PACKAGE/'tests/test_reference_oki.c']
    return {'source_sha256':{str(p):sha(p) for p in paths},'build_commands':builds,
            'all_114_complete_pcm_busy_tail_and_bank_tests_passed':True,
            'input_sha256':{str(p):sha(p) for p in [Path(rom_zip),inputs/'SOUND.ROM',inputs/'SAMPLES.ROM',inputs/'PCM16BE.PAK']},
            'executable_sha256':sha(out/'capture_board_replay')}

def run_case(out,stem,events,seconds,profile='full'):
    work=out/'cases'/stem;work.mkdir(parents=True,exist_ok=False)
    (work/'schedule.csv').write_text(''.join(f'{e["sample"]},{e["word"]}\n' for e in events))
    paths={key:work/name for key,name in [('trace','trace.csv'),('pcm','audio.pcm'),('report','capture.json'),('deliveries','deliveries.csv')]}
    command=[str(out/'capture_board_replay'),str(out/'inputs/SOUND.ROM'),str(out/'inputs/SAMPLES.ROM'),
             str(work/'schedule.csv'),str(seconds),str(paths['trace']),str(paths['pcm']),str(paths['report']),str(paths['deliveries']),profile]
    started=time.monotonic();result=subprocess.run(command,capture_output=True,text=True)
    (work/'capture.log').write_text(result.stdout+result.stderr)
    if result.returncode:raise RuntimeError('Full original replay failed: '+str(work))
    report=json.loads(paths['report'].read_text())
    if not report['complete'] or report['frames']!=seconds*44100:raise ValueError('Incomplete full-board reference')
    return {**report,**{k:str(v) for k,v in paths.items()},'commands':[command],
            'output_sha256':{k:sha(v) for k,v in paths.items()},'wall_seconds':time.monotonic()-started}

def write_wav(raw,destination):
    import array
    data=array.array('h',Path(raw).read_bytes())
    if sys.byteorder=='little':data.byteswap()
    if sys.byteorder=='big':data.byteswap()
    with wave.open(str(destination),'wb') as sink:
        sink.setnchannels(1);sink.setsampwidth(2);sink.setframerate(44100);sink.writeframes(data.tobytes())

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom-zip',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--opm-dir',type=Path);p.add_argument('--seconds',type=int,default=30)
    p.add_argument('--first',type=int,default=1);p.add_argument('--last',type=int,default=18)
    p.add_argument('--jobs',type=int,default=2);p.add_argument('--cc',default='cc')
    a=p.parse_args();a.out=a.out.resolve();a.rom_zip=a.rom_zip.resolve()
    if not 1<=a.first<=a.last<=18 or not 1<=a.seconds<=1800 or not 1<=a.jobs<=4:p.error('Invalid range/duration/workers')
    if a.out==PACKAGE or PACKAGE in a.out.parents:p.error('Private ROM/audio output must stay outside source package')
    baseline=prepare_and_build(a.out,a.rom_zip,a.opm_dir.resolve() if a.opm_dir else None,a.cc)
    (a.out/'wav').mkdir()
    report={**baseline,'kind':'FULL_BOARD_HOST_MODEL_NOT_PHYSICAL_ARCADE_RECORDING','profile':'full-board',
       'opm_commit':OPM_COMMIT,'external_ym_clock_hz':3579545,'opm_pipeline_divider':2,
       'oki_hz':8000,'sample_rate':44100,'channels':1,'bits':16,'normalization':False,'tracks':[],
       'timers':'Both A and B plus busy/status/IRQ are supplied by the coupled pinned OPM core',
       'oki_model':'Original ADPCM ROM addressing, busy/status, global active bank changes, decoder reset0',
       'route_gains':{'YM_left_Q15':10813,'YM_right_Q15':10813,'OKI_Q15':21627},
       'limitations':['Instruction-level CPU bus/IRQ boundaries, not a pin-cycle hardware capture',
                      'Digital sample/hold; no measured analog reconstruction filter',
                      'Software reference and listening comparison do not certify SCSP fidelity or physical hardware'],
       'complete':False}
    def job(command):
        item=run_case(a.out,f'music-{command:02X}',[{'sample':0,'word':command}],a.seconds)
        wav=a.out/'wav'/f'ARCADE-REFERENCIA-{command:02d}.wav';write_wav(item['pcm'],wav)
        item.update(command=command,hex=f'0x{command:02X}',seconds=a.seconds,wav=str(wav),wav_sha256=sha(wav))
        return item
    with ThreadPoolExecutor(max_workers=a.jobs) as pool:
        for task in as_completed([pool.submit(job,c) for c in range(a.first,a.last+1)]):
            item=task.result();report['tracks'].append(item);report['tracks'].sort(key=lambda t:t['command'])
            (a.out/'reference-progress.json').write_text(json.dumps(report,indent=2)+'\n')
            print(json.dumps({'command':item['hex'],'frames':item['frames'],'full-board':True}),flush=True)
    report['source_stable']=all(sha(Path(path))==value for path,value in report['source_sha256'].items())
    report['complete']=len(report['tracks'])==a.last-a.first+1 and report['source_stable']
    (a.out/'reference-report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['complete'] else 1

if __name__=='__main__':raise SystemExit(main())
