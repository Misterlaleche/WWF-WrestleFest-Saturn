#!/usr/bin/env python3
"""Original score end/recurrence observation, separate from audio-loop claims.

All original ROM state bytes stay in the explicit private output directory.
Strict control equality preserves CPU R/flags, RAM and the relative timer phase.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor,as_completed
import gzip
import hashlib
import json
from pathlib import Path
import struct
import subprocess

HERE=Path(__file__).resolve().parent
PACKAGE=HERE.parents[1]
CLOCK=3579545
ROM_SHA='f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7'
Z80_SHA='d82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698'
HEADER=struct.Struct('<8sIIQQQ')
PREFIX_BYTES=156

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def recurrence_key(state,level):
    fields={'pointers':('pointers',),'ram':('ram',),
            'visible_score':('ram','visible','io'),
            'exact_control':('ram','visible','cpu','io','timer')}[level]
    h=hashlib.sha256()
    for field in fields:
        value=state[field]
        h.update(struct.pack('<I',len(value)));h.update(value)
    return h.digest()

def original_end_observed(global_end_count,music_active):
    return global_end_count>0 and music_active==0

def decode_record(raw,cpu_size):
    cycle,tick,ym,oki,keyons,last,ends=struct.unpack_from('<6QI',raw)
    channel_ends=struct.unpack_from('<10I',raw,52)
    back_jumps=struct.unpack_from('<10I',raw,92)
    first_end,fade_sent,fade_clear=struct.unpack_from('<3Q',raw,132)
    at=PREFIX_BYTES
    cpu=raw[at:at+cpu_size];at+=cpu_size
    io=raw[at:at+6];at+=6
    timer=raw[at:at+8];at+=8
    ram=raw[at:at+2048];at+=2048
    visible=raw[at:at+264]
    return dict(cycle=cycle,tick=tick,ym=ym,oki=oki,keyons=keyons,last_audio_cycle=last,
        global_ends=ends,channel_ends=channel_ends,back_jumps=back_jumps,
        first_end_cycle=first_end,fade_sent_cycle=fade_sent,fade_clear_cycle=fade_clear,
        ram=ram,cpu=cpu,io=io,timer=timer,visible=visible,pointers=ram[0x152:0x166])

def describe_state(s):
    return dict(cycle=s['cycle'],seconds=s['cycle']/CLOCK,tick=s['tick'],
        music_active=s['ram'][6],music_command=s['ram'][7],
        original_channel_stop_flags=list(s['ram'][0x58:0x62]),
        visible_key_masks=list(s['visible'][-8:]),global_end_handler_hits=s['global_ends'],
        channel_end_handler_hits=list(s['channel_ends']),backward_jump_handler_hits=list(s['back_jumps']),
        positive_key_writes=s['keyons'],ym_writes_except_timer_control=s['ym'],oki_writes=s['oki'],
        first_global_end_cycle=s['first_end_cycle'] or None,
        fade_sent_cycle=s['fade_sent_cycle'] or None,fade_clear_cycle=s['fade_clear_cycle'] or None,
        strict_state_sha256=recurrence_key(s,'exact_control').hex())

def analyze_file(path):
    with Path(path).open('rb') as source:
        magic,cpu_size,command,origin,fade,tail=HEADER.unpack(source.read(HEADER.size))
        if magic!=b'WFMS0002' or not 64<=cpu_size<=256:raise ValueError('Unexpected original state probe format')
        record_size=PREFIX_BYTES+cpu_size+6+8+2048+264
        length=Path(path).stat().st_size-HEADER.size
        if length<=0 or length%record_size:raise ValueError('Truncated original state capture')
        count=length//record_size
        levels=('pointers','ram','visible_score','exact_control')
        seen={level:{} for level in levels};matches={level:None for level in levels}
        pool=[];first_end=None;first_all_stopped=None;before_fade=None;final=None
        for index in range(count):
            raw=source.read(record_size);s=decode_record(raw,cpu_size);final=s
            if s['cycle']<fade:
                before_fade=s
                if first_end is None and original_end_observed(s['global_ends'],s['ram'][6]):first_end=s
                if first_all_stopped is None and all(s['ram'][0x58:0x61]):first_all_stopped=s
            # Last row is an arbitrary final instruction boundary. Never use it
            # as an after-service recurrence checkpoint, nor any fade-tail row.
            if index==count-1 or s['cycle']>=fade or not s['ram'][6]:continue
            pool_index=None
            for level in levels:
                if matches[level] is not None:continue
                key=recurrence_key(s,level)
                previous_index=seen[level].get(key)
                if previous_index is not None:
                    old_raw,old_index=pool[previous_index];old=decode_record(old_raw,cpu_size)
                    if s['keyons']>old['keyons']:
                        # Verify complete selected byte components, not hashes alone.
                        names={'pointers':('pointers',),'ram':('ram',),
                          'visible_score':('ram','visible','io'),
                          'exact_control':('ram','visible','cpu','io','timer')}[level]
                        if not all(s[n]==old[n] for n in names):raise ValueError('State digest collision')
                        matches[level]=dict(first_record=old_index,repeat_record=index,
                            first=describe_state(old),repeat=describe_state(s),
                            period_cycles=s['cycle']-old['cycle'],period_seconds=(s['cycle']-old['cycle'])/CLOCK,
                            period_timer_ticks=s['tick']-old['tick'],positive_key_writes_between=s['keyons']-old['keyons'],
                            equal_components=list(names),equality='Exact bytes for every named component',
                            pcm_or_hidden_synth_loop_proven=False)
                else:
                    if pool_index is None:pool_index=len(pool);pool.append((raw,index))
                    seen[level][key]=pool_index
        classification=('ORIGINAL_GLOBAL_END_HANDLER_OBSERVED' if first_end is not None else
            'EXACT_SEQUENCER_CONTROL_STATE_RECURRENCE' if matches['exact_control'] else
            'VISIBLE_SCORE_STATE_RECURRENCE_ONLY' if matches['visible_score'] else
            'COMPLETE_RAM_RECURRENCE_ONLY' if matches['ram'] else
            'POINTER_RECURRENCE_ONLY' if matches['pointers'] else 'NO_CLOSURE_DEMONSTRATED_IN_WINDOW')
        return dict(command=command,classification=classification,origin_cycle=origin,
            score_seconds=fade/CLOCK,tail_seconds=tail/CLOCK,records=count,cpu_state_bytes=cpu_size,
            natural_global_end=describe_state(first_end) if first_end else None,
            all_nine_channel_stop_flags_observed=describe_state(first_all_stopped) if first_all_stopped else None,
            recurrences=matches,before_fade=describe_state(before_fade) if before_fade else None,
            final=describe_state(final),fade_clear_observed=bool(final['fade_clear_cycle']),
            fade_clear_after_command_seconds=(final['fade_clear_cycle']-final['fade_sent_cycle'])/CLOCK if final['fade_clear_cycle'] else None,
            pcm_loop_proven=False,snapshot_sha256=sha(path))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--package',type=Path,default=PACKAGE)
    p.add_argument('--seconds',type=int,default=600)
    p.add_argument('--tail-seconds',type=int,default=8)
    p.add_argument('--first',type=lambda v:int(v,0),default=1)
    p.add_argument('--last',type=lambda v:int(v,0),default=18)
    p.add_argument('--jobs',type=int,default=2)
    p.add_argument('--cc',default='cc')
    a=p.parse_args();a.out=a.out.resolve();a.package=a.package.resolve();a.rom=a.rom.resolve()
    if not 1<=a.first<=a.last<=18 or not 1<=a.seconds<=1800 or not 0<=a.tail_seconds<=60 or not 1<=a.jobs<=4:
        p.error('Invalid command, duration or worker range')
    if a.out==a.package or a.package in a.out.parents:raise ValueError('Private original states cannot be written into source package')
    if sha(a.rom)!=ROM_SHA:raise ValueError('Original SOUND.ROM identity mismatch')
    core=a.package/'src/reference_driver/z80.c'
    if sha(core)!=Z80_SHA:raise ValueError('Independent original Z80 core identity mismatch')
    a.out.mkdir(parents=True,exist_ok=False)
    probe=HERE/'probe_music_state.c';oracle=HERE/'oracle_a53/tools/capture_full_audio_a50.c'
    source_paths=(Path(__file__).resolve(),probe,oracle,core,core.with_suffix('.h'),a.rom)
    before={str(x):sha(x) for x in source_paths}
    exe=a.out/'probe_music_state'
    build=[a.cc,'-std=c99','-O3','-I',str(core.parent),str(probe),str(core),'-o',str(exe)]
    subprocess.run(build,check=True)
    report=dict(kind='ORIGINAL_ROM_SCORE_STRUCTURE_NOT_PCM_LOOP_PROOF',source_sha256=before,
        build_command=build,scope='Original Z80 control/MMIO model; no synthesis-state or physical hardware certification',tracks=[])
    def run(command):
        raw=a.out/f'command-{command:02X}.bin';log=a.out/f'command-{command:02X}.log'
        args=[str(exe),str(a.rom),str(command),str(a.seconds),str(a.tail_seconds),str(raw)]
        with log.open('w') as stream:subprocess.run(args,stdout=stream,stderr=subprocess.STDOUT,check=True)
        item=analyze_file(raw);item['probe_command']=args;item['log_sha256']=sha(log)
        compressed=raw.with_suffix('.bin.gz')
        with raw.open('rb') as source,gzip.open(compressed,'wb',compresslevel=1) as target:
            while block:=source.read(1024*1024):target.write(block)
        item['compressed_snapshot_sha256']=sha(compressed);item['compressed_snapshot']=str(compressed)
        raw.unlink();return item
    with ThreadPoolExecutor(max_workers=a.jobs) as pool:
        for task in as_completed([pool.submit(run,c) for c in range(a.first,a.last+1)]):
            item=task.result();report['tracks'].append(item);report['tracks'].sort(key=lambda x:x['command'])
            (a.out/'structure-progress.json').write_text(json.dumps(report,indent=2)+'\n')
            print(json.dumps({'command':f'0x{item["command"]:02X}','classification':item['classification'],
                 'fade_clear':item['fade_clear_observed']}),flush=True)
    report['source_stable']=before=={str(x):sha(x) for x in source_paths}
    report['complete']=len(report['tracks'])==a.last-a.first+1 and report['source_stable']
    report['limitations']=['A finite duration does not prove musical completeness.',
        'Pointer, RAM, visible score and complete control state recurrences are distinct evidence levels.',
        'Even exact control recurrence excludes hidden YM/OKI synthesis state and cannot define a PCM loop.',
        'Natural ROM end and original fade control are separate from the audible sample/envelope tail.']
    (a.out/'music-structure.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['complete'] else 1

if __name__=='__main__':raise SystemExit(main())
