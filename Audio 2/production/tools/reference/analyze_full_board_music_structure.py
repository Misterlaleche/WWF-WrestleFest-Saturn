#!/usr/bin/env python3
"""Observe original score control in the coupled board without inventing loops.

Replay existing private full-board schedules and require identical MMIO trace,
PCM and latch deliveries. All ROM state, branch addresses and audio stay in
the explicit private output. CPU refresh equality is a separate digital-state
diagnostic, never a requirement for reporting visible score recurrence.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor,as_completed
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time

HERE=Path(__file__).resolve().parent
PACKAGE=HERE.parents[1]
CLOCK=3579545
HEADER=struct.Struct('<8sIIIIQQQ')
PREFIX_BYTES=160
PINNED={
    'SOUND.ROM':'f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7',
    'SAMPLES.ROM':'e4cbe71f079e2cb95b3463115557fa1d99e18ea975f2d3354827b01994fc4615',
    'z80.c':'d82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698',
    'opm.c':'983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188',
    'opm.h':'186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39',
}
COMPONENTS={
    'pointers':('pointers',),
    'ram':('ram',),
    'visible_score':('ram','visible','io'),
    'full_digital':('ram','visible','io','cpu','phases','opm','oki','output'),
}


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        while block:=stream.read(1024*1024):h.update(block)
    return h.hexdigest()


def natural_end_observed(state):
    return (state['global_ends']>0 and state['ram'][6]==0 and state['first_end_cycle']>0
            and (not state['fade_sent_cycle'] or state['first_end_cycle']<state['fade_sent_cycle']))


def recurrence_key(state,level):
    digest=hashlib.sha256()
    for name in COMPONENTS[level]:
        value=state[name]
        digest.update(struct.pack('<I',len(value)));digest.update(value)
    return digest.digest()


def describe_state(s):
    return dict(cycle=s['cycle'],seconds=s['cycle']/CLOCK,timer_b_services=s['tick'],
        music_active=s['ram'][6],music_command=s['ram'][7],
        original_channel_stop_flags=list(s['ram'][0x58:0x62]),
        visible_key_masks=list(s['visible'][-8:]),global_end_handler_hits=s['global_ends'],
        channel_end_handler_hits=list(s['channel_ends']),backward_jump_handler_hits=list(s['back_jumps']),
        positive_key_writes=s['keyons'],ym_writes_except_timer_control=s['ym'],oki_writes=s['oki_events'],
        first_global_end_cycle=s['first_end_cycle'] or None,
        fade_sent_cycle=s['fade_sent_cycle'] or None,fade_clear_cycle=s['fade_clear_cycle'] or None,
        canonical_after_timer_b_service=bool(s['canonical']))


def analyze_states(states,fade_cycle):
    seen={level:{} for level in COMPONENTS};matches={level:None for level in COMPONENTS}
    first_end=None;first_stopped=None;before_fade=None;final=None;count=0
    for index,s in enumerate(states):
        count+=1;final=s
        if s['cycle']<fade_cycle and not s['fade_sent_cycle']:
            before_fade=s
            if first_end is None and natural_end_observed(s):first_end=s
            if first_stopped is None and all(s['ram'][0x58:0x61]):first_stopped=s
        if (not s['canonical'] or s['cycle']>=fade_cycle or s['fade_sent_cycle'] or not s['ram'][6]):
            continue
        for level,names in COMPONENTS.items():
            if matches[level] is not None:continue
            key=recurrence_key(s,level);previous=seen[level].get(key)
            if previous is None:
                seen[level][key]=(s,index)
            elif s['keyons']>previous[0]['keyons']:
                old,old_index=previous
                if not all(s[name]==old[name] for name in names):raise ValueError('State digest collision')
                matches[level]=dict(first_record=old_index,repeat_record=index,
                    first=describe_state(old),repeat=describe_state(s),
                    period_cycles=s['cycle']-old['cycle'],period_seconds=(s['cycle']-old['cycle'])/CLOCK,
                    timer_b_services_between=s['tick']-old['tick'],
                    positive_key_writes_between=s['keyons']-old['keyons'],
                    equal_components=list(names),equality='Exact bytes of every named component',
                    complete_composition_loop_proven=False,pcm_loop_proven=False)
    if final is None:raise ValueError('Empty coupled state capture')
    score=before_fade or final
    classification=('ORIGINAL_GLOBAL_END_HANDLER_OBSERVED' if first_end is not None else
        'FULL_DIGITAL_CHECKPOINT_RECURRENCE' if matches['full_digital'] else
        'VISIBLE_SCORE_STATE_RECURRENCE' if matches['visible_score'] else
        'COMPLETE_RAM_RECURRENCE' if matches['ram'] else
        'POINTER_RECURRENCE_ONLY' if matches['pointers'] else
        'BACKWARD_FD_BRANCH_OBSERVED' if any(score['back_jumps']) else 'NO_CLOSURE_DEMONSTRATED_IN_WINDOW')
    return dict(classification=classification,records=count,
        natural_global_end=describe_state(first_end) if first_end else None,
        all_nine_channel_stop_flags_observed=describe_state(first_stopped) if first_stopped else None,
        recurrences=matches,before_fade=describe_state(before_fade) if before_fade else None,
        final=describe_state(final),fade_clear_observed=bool(final['fade_clear_cycle']),
        fade_clear_after_command_seconds=(final['fade_clear_cycle']-final['fade_sent_cycle'])/CLOCK
            if final['fade_clear_cycle'] else None,
        complete_composition_loop_proven=False,pcm_loop_proven=False)


def decode_record(raw,cpu_size,opm_size,oki_size):
    canonical,cycle,tick,ym,oki_events,keyons,last,ends=struct.unpack_from('<I6QI',raw)
    channel_ends=struct.unpack_from('<10I',raw,56)
    back_jumps=struct.unpack_from('<10I',raw,96)
    first_end,fade_sent,fade_clear=struct.unpack_from('<3Q',raw,136)
    at=PREFIX_BYTES;parts={}
    for name,size in (('cpu',cpu_size),('io',8),('phases',24),('ram',2048),('visible',264),
                      ('opm',opm_size),('oki',oki_size),('output',8)):
        parts[name]=raw[at:at+size];at+=size
    if canonical not in (0,1) or parts['io'][7]!=0:raise ValueError('Invalid coupled checkpoint/profile')
    return dict(canonical=canonical,cycle=cycle,tick=tick,ym=ym,oki_events=oki_events,keyons=keyons,
        last_audio_cycle=last,global_ends=ends,channel_ends=channel_ends,back_jumps=back_jumps,
        first_end_cycle=first_end,fade_sent_cycle=fade_sent,fade_clear_cycle=fade_clear,
        pointers=parts['ram'][0x152:0x166],**parts)


def analyze_file(path):
    path=Path(path)
    with path.open('rb') as source:
        header=source.read(HEADER.size)
        if len(header)!=HEADER.size:raise ValueError('Truncated coupled state header')
        magic,cpu_size,opm_size,oki_size,command,origin,fade_sample,duration=HEADER.unpack(header)
        if (magic!=b'WFMB0001' or not 64<=cpu_size<=256 or not 512<=opm_size<=16384
            or not 64<=oki_size<=1024 or not 1<=command<=18):raise ValueError('Unexpected coupled state format')
        record_size=PREFIX_BYTES+cpu_size+8+24+2048+264+opm_size+oki_size+8
        length=path.stat().st_size-HEADER.size
        if length<=0 or length%record_size:raise ValueError('Truncated coupled state capture')
        count=length//record_size
        fade_cycle=(fade_sample*CLOCK+44100-1)//44100 if fade_sample!=2**64-1 else duration+1
        result=analyze_states((decode_record(source.read(record_size),cpu_size,opm_size,oki_size)
                              for _ in range(count)),fade_cycle)
    result.update(command=command,origin_cycle=origin,duration_seconds=duration/CLOCK,
        fade_target_sample=fade_sample if fade_sample!=2**64-1 else None,
        cpu_state_bytes=cpu_size,opm_state_bytes=opm_size,oki_state_bytes=oki_size,snapshot_sha256=sha(path))
    return result


def build(out,reference_root,cc):
    core=PACKAGE/'src/reference_driver';deps=reference_root/'deps';inputs=reference_root/'inputs'
    paths=[inputs/'SOUND.ROM',inputs/'SAMPLES.ROM',core/'z80.c',deps/'opm.c',deps/'opm.h']
    for path in paths:
        if sha(path)!=PINNED[path.name]:raise ValueError('Pinned input mismatch: '+str(path))
    capture=HERE/'capture_board_replay.c';data=capture.read_bytes()
    marker=b'\nint main(int argc,char **argv)\n'
    if data.count(marker)!=1:raise ValueError('Ambiguous coupled context/main boundary')
    context=out/'capture_board_context.h';context.write_bytes(data.split(marker)[0]+b'\n')
    source_paths=paths+[Path(__file__).resolve(),HERE/'probe_full_board_music_state.c',capture,
        HERE/'reference_oki.h',HERE/'oracle_a53/tools/capture_full_audio_a50.c',core/'z80.h',
        PACKAGE/'tests/test_full_board_music_state.c',PACKAGE/'tests/test_full_board_music_state.py']
    identities={str(path):sha(path) for path in source_paths}
    previous=json.loads((reference_root/'reference-report.json').read_text())['source_sha256']
    for path in (capture,HERE/'reference_oki.h',HERE/'oracle_a53/tools/capture_full_audio_a50.c',core/'z80.c',core/'z80.h'):
        recorded=[value for name,value in previous.items() if name.endswith('/'+str(path.relative_to(PACKAGE)))]
        if len(recorded)!=1 or recorded[0]!=sha(path):raise ValueError('Baseline source mismatch: '+str(path))
    commands=[]
    for source,stem in ((HERE/'probe_full_board_music_state.c','probe_full_board_music_state'),
                        (PACKAGE/'tests/test_full_board_music_state.c','test_full_board_music_state')):
        obj=out/(stem+'.o');exe=out/stem
        commands.extend([
            [cc,'-O3','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-I',str(core),
             '-I',str(deps),'-I',str(HERE),'-I',str(out),'-c',str(source),'-o',str(obj)],
            [cc,'-O3','-std=c99',str(obj),str(core/'z80.c'),str(deps/'opm.c'),'-lm','-o',str(exe)]])
    commands.append([str(out/'test_full_board_music_state'),str(inputs/'SOUND.ROM'),str(inputs/'SAMPLES.ROM')])
    for i,command in enumerate(commands):
        result=subprocess.run(command,capture_output=True,text=True)
        (out/f'build-{i}.log').write_text(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError('Build/equivalence gate failed: '+str(command))
    return dict(source_sha256=identities,build_commands=commands,derived_context_sha256=sha(context),
                observer_equivalence_passed=True,executable_sha256=sha(out/'probe_full_board_music_state'))


def run_case(out,reference_root,command):
    baseline=reference_root/'cases'/f'music-{command:02X}'
    base_report=json.loads((baseline/'capture.json').read_text())
    if not base_report['complete'] or base_report['profile']!='full':raise ValueError('Incomplete baseline')
    frames=base_report['frames'];seconds=frames//44100
    if frames!=seconds*44100:raise ValueError('Nonintegral baseline duration')
    work=out/'cases'/f'music-{command:02X}';work.mkdir(parents=True,exist_ok=False)
    schedule=baseline/'schedule.csv';shutil.copyfile(schedule,work/'schedule.csv')
    rows=[tuple(map(int,line.split(','))) for line in schedule.read_text().splitlines()]
    if not rows or rows[0]!=(0,command) or any(word!=0x1f for _,word in rows[1:]):
        raise ValueError('Expected isolated music followed only by original1F')
    raw=work/'states.bin'
    args=[str(out/'probe_full_board_music_state'),str(reference_root/'inputs/SOUND.ROM'),
          str(reference_root/'inputs/SAMPLES.ROM'),str(work/'schedule.csv'),str(seconds),
          str(work/'trace.csv'),str(work/'audio.pcm'),str(work/'capture.json'),str(work/'deliveries.csv'),'full',str(raw)]
    started=time.monotonic()
    with (work/'capture.log').open('w') as log:subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,check=True)
    actual=json.loads((work/'capture.json').read_text())
    differences={key:{'expected':value,'actual':actual.get(key)} for key,value in base_report.items() if actual.get(key)!=value}
    hashes={name:{'baseline':sha(baseline/name),'observed':sha(work/name)} for name in ('trace.csv','audio.pcm','deliveries.csv')}
    identical=not differences and all(item['baseline']==item['observed'] for item in hashes.values())
    if not identical:
        (work/'equivalence-failure.json').write_text(json.dumps(dict(report_differences=differences,hashes=hashes),indent=2)+'\n')
        raise ValueError('Observer changed baseline outputs: '+str(work))
    result=analyze_file(raw)
    compressed=work/'states.bin.gz'
    with raw.open('rb') as source,gzip.open(compressed,'wb',compresslevel=1) as target:
        shutil.copyfileobj(source,target,1024*1024)
    result.update(probe_command=args,wall_seconds=time.monotonic()-started,
        baseline_output_sha256=hashes,baseline_report_fields_identical=True,
        coupled_capture=actual,compressed_snapshot=str(compressed),compressed_snapshot_sha256=sha(compressed))
    (work/'structure.json').write_text(json.dumps(result,indent=2)+'\n')
    # Retain compressed private state and identities; the identical raw trace
    # and audio already exist in the explicitly supplied reference directory.
    raw.unlink()
    for name in hashes:(work/name).unlink()
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-root',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--first',type=lambda v:int(v,0),default=1)
    parser.add_argument('--last',type=lambda v:int(v,0),default=18)
    parser.add_argument('--jobs',type=int,default=2)
    parser.add_argument('--cc',default='cc')
    args=parser.parse_args();args.out=args.out.resolve();args.reference_root=args.reference_root.resolve()
    if not 1<=args.first<=args.last<=18 or not 1<=args.jobs<=2:parser.error('Invalid command range or worker count')
    if args.out==PACKAGE or PACKAGE in args.out.parents:parser.error('Private states must stay outside the source package')
    args.out.mkdir(parents=True,exist_ok=False)
    metadata=build(args.out,args.reference_root,args.cc)
    report=dict(**metadata,kind='COUPLED_FULL_BOARD_SCORE_OBSERVATIONS_NOT_COMPOSITION_OR_PCM_LOOP_PROOF',
        reference_root=str(args.reference_root),profile='TimerA+B, busy/status/IRQ, original-ROM OKI banks',
        complete=False,tracks=[],limitations=[
            'A finite observation window cannot establish completeness of a composition.',
            'A guarded backward FD handler entry proves local score control flow, not whole-composition closure.',
            'Visible score recurrence compares complete RAM, visible YM writes/keys and current MMIO state; CPU refresh is not a musical criterion.',
            'Full digital checkpoint equality is a distinct diagnostic including CPU, synthesis and sample-clock phases.',
            'Neither branch evidence nor visible recurrence establishes a sample-exact PCM period.',
            'Natural FB completion and1F control clear do not themselves identify the last audible sample.',
            'This is the instruction-granular coupled host model, not a physical arcade or Saturn recording.'])
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for task in as_completed([pool.submit(run_case,args.out,args.reference_root,c) for c in range(args.first,args.last+1)]):
            item=task.result();report['tracks'].append(item);report['tracks'].sort(key=lambda x:x['command'])
            (args.out/'structure-progress.json').write_text(json.dumps(report,indent=2)+'\n')
            print(json.dumps(dict(command=f'0x{item["command"]:02X}',classification=item['classification'],
                baseline_pcm_trace_deliveries_identical=True,fade_clear=item['fade_clear_observed'])),flush=True)
    report['source_stable']=all(sha(path)==digest for path,digest in report['source_sha256'].items())
    report['complete']=len(report['tracks'])==args.last-args.first+1 and report['source_stable']
    (args.out/'music-structure.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['complete'] else 1


if __name__=='__main__':raise SystemExit(main())
