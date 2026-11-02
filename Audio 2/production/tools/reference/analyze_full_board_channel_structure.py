#!/usr/bin/env python3
"""Reproduce channel-cycle evidence from existing coupled captures, without replay.

Inputs: the frozen music-state capture directory and its full-board reference
directory. Output: a fresh private directory with versioned evidence and an
allowlisted public summary. No ROM, trace, PCM or original state array is
copied into the source package. The frozen observer/analyzer are reused.
"""
import argparse
import bisect
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys

HERE=Path(__file__).resolve().parent
PACKAGE=HERE.parents[1]
TOOL_VERSION='1.0.1'
SCHEMA_VERSION=1
spec=importlib.util.spec_from_file_location('frozen_music_structure',HERE/'analyze_full_board_music_structure.py')
observer=importlib.util.module_from_spec(spec);spec.loader.exec_module(observer)
CLOCK=observer.CLOCK
PCM_KINDS={'OKI':1,'PLAY':2,'STOP':3,'BANK':4,'F1SKIP':5}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare_output(out,package=PACKAGE):
    out=Path(out).resolve();package=Path(package).resolve()
    if out==package or package in out.parents:
        raise ValueError('Detailed state evidence requires an output outside the source package')
    out.mkdir(parents=True,exist_ok=False)
    return out


def sequence_evidence(events,boundaries,timeline):
    """Compare full sequences as bytes; hashes identify, not replace, equality.

    Events are (external_cycle, kind, a, b). For YM, a/b are register/value;
    PCM kinds preserve the existing capture's OKI/PLAY/STOP/BANK/F1SKIP fields.
    A write belongs to the next canonical post-service checkpoint. External
    inter-write deltas are also compared without the first anchor-relative
    delta, preventing a checkpoint offset from masquerading as timing drift.
    """
    times=[e[0] for e in events]
    checkpoint_cycles=[p[0] for p in timeline]
    checkpoint_ticks=[p[1] for p in timeline]
    spans=[];values=[];services=[];external=[];internal=[];first_mismatches=[]
    for start,end in zip(boundaries,boundaries[1:]):
        if start['cycle']>=end['cycle'] or start['tick']>=end['tick']:
            raise ValueError('Non-increasing channel boundary')
        rows=events[bisect.bisect_left(times,start['cycle']):bisect.bisect_left(times,end['cycle'])]
        value_bytes=bytearray();service_bytes=bytearray();external_bytes=bytearray();deltas=[]
        previous=start['cycle']
        for cycle,kind,a,b in rows:
            at=bisect.bisect_left(checkpoint_cycles,cycle)
            if at==len(checkpoint_cycles):raise ValueError('Event has no following service checkpoint')
            tick=checkpoint_ticks[at]-start['tick']
            if tick<0 or tick>end['tick']-start['tick']:raise ValueError('Event outside channel service interval')
            value_bytes.extend(struct.pack('<BHH',kind,a,b))
            service_bytes.extend(struct.pack('<QBHH',tick,kind,a,b))
            delta=cycle-previous;previous=cycle
            external_bytes.extend(struct.pack('<QBHH',delta,kind,a,b));deltas.append((delta,kind,a,b))
        v=bytes(value_bytes);s=bytes(service_bytes);e=bytes(external_bytes)
        inside=e[13:] if rows else b'' # <QBHH is 13 bytes; omit first event's anchor-relative delta.
        if internal:
            old=internal[-1]
            first=next((i for i in range(min(len(old),len(inside))//13)
                        if old[13*i:13*i+13]!=inside[13*i:13*i+13]),None)
            if first is not None:
                left=struct.unpack_from('<QBHH',old,13*first)
                right=struct.unpack_from('<QBHH',inside,13*first)
                first_mismatches.append(dict(first_interval=len(spans)-1,second_interval=len(spans),
                    event_index=first+1,first=list(left),second=list(right)))
        spans.append(dict(start_cycle=start['cycle'],end_cycle=end['cycle'],start_service=start['tick'],
            end_service=end['tick'],service_count=end['tick']-start['tick'],event_count=len(rows),
            value_sha256=digest(v),service_value_sha256=digest(s),external_delta_sha256=digest(e),
            internal_external_delta_sha256=digest(inside)))
        values.append(v);services.append(s);external.append(e);internal.append(inside)
    def all_equal(items):
        return bool(items) and all(item==items[0] for item in items[1:])
    def triples(items):
        return [i for i in range(len(items)-2) if items[i]==items[i+1]==items[i+2]]
    periods={span['service_count'] for span in spans}
    positive=bool(spans) and all(span['event_count'] for span in spans)
    value_equal=all_equal(values);service_equal=all_equal(services)
    verified=len(spans)>=3 and positive and len(periods)==1 and value_equal and service_equal
    counts={span['event_count'] for span in spans}
    return dict(score_cycle_observed=verified,cycle_timer_b_services=next(iter(periods)) if len(periods)==1 else None,
        complete_cycles=len(spans),events_per_cycle=next(iter(counts)) if len(counts)==1 else None,
        all_value_sequences_equal=value_equal,all_service_sequences_equal=service_equal,
        exact_complete_bytes_compared=True,
        three_internal_external_cycle_sequences_equal=bool(triples(internal)),
        three_anchor_external_cycle_sequences_equal=bool(triples(external)),
        matching_three_value_sequences=triples(values),matching_three_service_sequences=triples(services),
        matching_three_internal_external_sequences=triples(internal),
        intervals=spans,first_internal_external_mismatches=first_mismatches)


def classify_channels(channels,global_observation):
    active=[c for c in channels if c.get('state')!='ORIGINAL_CHANNEL_END_OBSERVED']
    verified=bool(active) and all(c.get('score_cycle_observed',False) for c in active)
    return dict(classification='PER_CHANNEL_SCORE_CYCLES_OBSERVED' if verified else 'CHANNEL_CYCLE_EVIDENCE_INCOMPLETE',
        global_observation=global_observation,global_period_proven=False,pcm_period_proven=False)


def public_summary(private,all_music):
    """Explicit allowlist: never propagate arbitrary private evidence fields."""
    supplements={t['command']:t for t in private['tracks']}
    result={key:private[key] for key in ('schema_version','tool_version','passed','source_sha256','input_sha256') if key in private}
    result.update(kind='PUBLIC_METADATA_ONLY_COMPLETE_MUSIC_STRUCTURE',
        protected_state_trace_ROM_PCM_included=False,tracks=[],
        scope='Bounded original-ROM score evidence in the coupled host model; no universal or sample-exact PCM period claim')
    channel_fields=('channel','kind','state','score_cycle_observed','cycle_timer_b_services',
        'complete_cycles','events_per_cycle','all_value_sequences_equal','all_service_sequences_equal',
        'exact_complete_bytes_compared','three_internal_external_cycle_sequences_equal','FD_guard_verified')
    for track in all_music['tracks']:
        command=track['command'];supplement=supplements.get(command)
        row=dict(command=command,hex=f'0x{command:02X}',global_observation=track['classification'],
                 classification=supplement['classification'] if supplement else track['classification'])
        end=track.get('natural_global_end');visible=track.get('recurrences',{}).get('visible_score')
        if end:
            row['natural_FB_entry_cycle']=end['first_global_end_cycle']
            row['natural_FB_entry_seconds']=end['first_global_end_cycle']/CLOCK
        if visible:
            row['visible_state_interval_cycles']=visible['period_cycles']
            row['visible_state_interval_seconds']=visible['period_seconds']
        if 'fade_clear_observed' in track:row['original_fade_control_clear_observed']=track['fade_clear_observed']
        if supplement:
            for key in ('timer_b_register','timer_b_period_external_cycles','max_pointer_pattern_cycle_groups',
                        'global_external_cycle_sequences_equal','state_difference_causes'):
                if key in supplement:row[key]=supplement[key]
            row['channels']=[{key:c[key] for key in channel_fields if key in c} for c in supplement['channels']]
        row.update(global_period_proven=False,pcm_period_proven=False)
        result['tracks'].append(row)
    return result


def build_layout(out,reference,cc):
    """Derive ABI offsets from the pinned headers instead of hard-coding them."""
    if sys.byteorder!='little':raise ValueError('The frozen raw-struct capture requires a little-endian host')
    source=out/'capture_layout.c';exe=out/'capture_layout'
    source.write_text('#include <stdio.h>\n#include <stddef.h>\n#include "z80.h"\n#include "opm.h"\n'
        'int main(void){printf("{\\"cpu_size\\":%zu,\\"cpu_sp\\":%zu,\\"opm_size\\":%zu,'
        '\\"opm_opp\\":%zu,\\"opm_test\\":%zu,\\"opm_tb\\":%zu}\\n",'
        'sizeof(z80),offsetof(z80,sp),sizeof(opm_t),offsetof(opm_t,opp),'
        'offsetof(opm_t,mode_test),offsetof(opm_t,timer_b_reg));}\n')
    command=[cc,'-std=c99','-Wall','-Wextra','-Werror','-I',str(PACKAGE/'src/reference_driver'),
             '-I',str(reference/'deps'),str(source),'-o',str(exe)]
    run=subprocess.run(command,capture_output=True,text=True)
    (out/'layout-build.log').write_text(run.stdout+run.stderr)
    if run.returncode:raise RuntimeError('Capture layout build failed; see layout-build.log')
    result=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
    layout=json.loads(result.stdout);(out/'capture-layout.json').write_text(json.dumps(layout,indent=2)+'\n')
    return layout,command


def capture_header(path,layout):
    with gzip.open(path,'rb') as stream:raw=stream.read(observer.HEADER.size)
    if len(raw)!=observer.HEADER.size:raise ValueError('Truncated capture header')
    magic,cpu,opm,oki,command,origin,fade,duration=observer.HEADER.unpack(raw)
    if magic!=b'WFMB0001' or cpu!=layout['cpu_size'] or opm!=layout['opm_size'] or not 64<=oki<=1024:
        raise ValueError('Unsupported capture ABI/format')
    return dict(raw=raw,cpu=cpu,opm=opm,oki=oki,command=command,origin=origin,fade_sample=fade,
        duration=duration,size=observer.PREFIX_BYTES+cpu+8+24+2048+264+opm+oki+8)


def states_from_capture(path,header,expected_raw_sha=None):
    digestor=hashlib.sha256();final=False;last_cycle=-1
    with gzip.open(path,'rb') as stream:
        raw=stream.read(observer.HEADER.size)
        if raw!=header['raw']:raise ValueError('Capture header changed')
        digestor.update(raw);index=0
        while raw:=stream.read(header['size']):
            if len(raw)!=header['size'] or final:raise ValueError('Truncated or misplaced final checkpoint')
            digestor.update(raw);state=observer.decode_record(raw,header['cpu'],header['opm'],header['oki'])
            if state['cycle']<last_cycle:raise ValueError('Non-monotonic capture')
            last_cycle=state['cycle'];final=not state['canonical'];state['record']=index
            yield state;index+=1
    if not final or last_cycle<header['duration']:raise ValueError('Missing completed final checkpoint')
    if expected_raw_sha and digestor.hexdigest()!=expected_raw_sha:raise ValueError('Uncompressed snapshot SHA mismatch')


def scan_capture(path,header,layout,track):
    patterns={};branches=[[] for _ in range(9)];last=[0]*9;timeline=[];previous=None;timer_modes=set()
    fade_cycle=(header['fade_sample']*CLOCK+44100-1)//44100
    for state in states_from_capture(path,header,track['snapshot_sha256']):
        if state['canonical']:timeline.append((state['cycle'],state['tick']))
        eligible=(state['canonical'] and state['cycle']<fade_cycle and not state['fade_sent_cycle'] and state['ram'][6])
        if eligible:
            group=tuple(state['back_jumps'][:8]);patterns.setdefault(state['pointers'],{}).setdefault(group,state['record'])
            chip=state['opm'];timer_modes.add((chip[layout['opm_opp']],chip[layout['opm_test']+2],chip[layout['opm_tb']]))
            for channel,count in enumerate(state['back_jumps'][:9]):
                if count>last[channel]:
                    if previous is None or count!=last[channel]+1:
                        raise ValueError('Multiple/unbounded FD entries between canonical checkpoints')
                    branches[channel].append(dict(record=state['record'],cycle=state['cycle'],tick=state['tick'],
                        count=count,pointer_before=struct.unpack_from('<H',previous['pointers'],2*channel)[0],
                        pointer_after=struct.unpack_from('<H',state['pointers'],2*channel)[0]))
                    last[channel]=count
        previous=state
    if not patterns:raise ValueError('No active canonical score states')
    pointer,groups=min(patterns.items(),key=lambda item:(-len(item[1]),min(item[1].values())))
    best=list(groups.values())
    anchor_indices=best if len(best)>=4 else [row['record'] for row in branches[0]]
    if len(anchor_indices)<4:raise ValueError('Insufficient channel/global anchors for three intervals')
    recurrence=track['recurrences']['pointers']
    wanted=set(anchor_indices+best[:4]+[recurrence['first_record'],recurrence['repeat_record']])
    selected={s['record']:s for s in states_from_capture(path,header) if s['record'] in wanted}
    if not all(selected[i]['pointers']==pointer for i in best[:4]):raise ValueError('Pointer candidate equality failed')
    return dict(branches=branches,timeline=timeline,timer_modes=timer_modes,selected=selected,best=best,
        anchors=anchor_indices,max_groups=len(groups),anchor_kind='equal_global_pointers' if len(best)>=4 else 'channel0_FD_returns')


def state_differences(a,b,layout):
    def describe(address):
        offset=address-0xc000
        if 0x66<=offset<=0x6e:return f'{"FM" if offset<0x6e else "PCM"} note countdown channel {offset-0x66}'
        if address>=0xc400:
            sp=min(struct.unpack_from('<H',s['cpu'],layout['cpu_sp'])[0] for s in (a,b))
            return 'residual stack below idle SP' if address<sp else 'stack at/above idle SP'
        if 0x9a<=offset<0xaa:return 'FM pitch write cache'
        if 0xd2<=offset<0x152:return 'pitch modulator state'
        if 0xba<=offset<0xca:return 'two-byte note storage'
        return 'other driver RAM'
    return dict(first_record=a['record'],second_record=b['record'],first_cycle=a['cycle'],second_cycle=b['cycle'],
        equal_pointers=a['pointers']==b['pointers'],equal_visible=a['visible']==b['visible'],equal_io=a['io']==b['io'],
        changed_RAM=[dict(address=0xc000+i,before=x,after=y,meaning=describe(0xc000+i))
                     for i,(x,y) in enumerate(zip(a['ram'],b['ram'])) if x!=y])


def read_trace(path,origin):
    ym=[];pcm=[];previous=-1
    with path.open() as stream:
        if next(stream).strip()!='cycle,kind,a,b':raise ValueError('Unexpected trace header')
        for line in stream:
            cycle,kind,a,b=line.rstrip().split(',');cycle=int(cycle)
            if cycle<previous:raise ValueError('Non-monotonic trace')
            previous=cycle
            if kind=='YM_D':ym.append((cycle-origin,0,int(a),int(b)))
            elif kind in PCM_KINDS:pcm.append((cycle-origin,PCM_KINDS[kind],int(a),int(b)))
    return ym,pcm


def verify_controller(rom):
    if digest(rom)!=observer.PINNED['SOUND.ROM']:raise ValueError('Original ROM identity mismatch')
    # Verify semantic operands without copying any original instruction array.
    word=lambda at:struct.unpack_from('<H',rom,at)[0]
    valid=(rom[0x818]==0x21 and word(0x819)==0xc066 and rom[0x81c]==0x35
        and rom[0x81d]==0xc2 and word(0x81e)==0x08a7
        and rom[0x6f3]==0x21 and word(0x6f4)==0xc066 and rom[0x6f7]==0x35 and rom[0x6f8]==0xc0
        and word(0xc5b)==0x0d84 and rom[0xd95]==0xdd and rom[0xd96]==0x23
        and rom[0xd97]==0xc3 and word(0xd98)==0x0849)
    if not valid:raise ValueError('Original countdown/F2 controller guard mismatch')


def analyze_track(command,structure,reference,layout,track,rom):
    work=structure/'cases'/f'music-{command:02X}'
    snapshot=work/'states.bin.gz';trace=reference/'cases'/f'music-{command:02X}'/'trace.csv'
    if observer.sha(snapshot)!=track['compressed_snapshot_sha256']:raise ValueError('Compressed snapshot SHA mismatch')
    trace_sha=observer.sha(trace)
    if trace_sha!=track['baseline_output_sha256']['trace.csv']['baseline']:raise ValueError('Preserved trace SHA mismatch')
    header=capture_header(snapshot,layout)
    if header['command']!=command:raise ValueError('Snapshot command mismatch')
    scan=scan_capture(snapshot,header,layout,track);selected=scan['selected']
    ym,pcm=read_trace(trace,header['origin'])
    fade_cycle=(header['fade_sample']*CLOCK+44100-1)//44100
    tempos=[event[3] for event in ym if 0<=event[0]<fade_cycle and event[2]==0x12]
    if not tempos or len(set(tempos))!=1:raise ValueError('Variable or missing Timer B tempo')
    timer=tempos[0]
    if scan['timer_modes']!={(0,0,timer)}:raise ValueError('Snapshot timer mode/value disagrees with normal OPM trace')
    channels=[]
    for channel,boundaries in enumerate(scan['branches']):
        if not boundaries:
            stopped=(track['before_fade']['original_channel_stop_flags'][channel]
                     and track['before_fade']['channel_end_handler_hits'][channel])
            channels.append(dict(channel=channel,kind='FM' if channel<8 else 'PCM',
                state='ORIGINAL_CHANNEL_END_OBSERVED' if stopped else 'NO_CHANNEL_CYCLE_DEMONSTRATED',
                score_cycle_observed=False));continue
        first=track['coupled_capture']['backward_branch_first'][channel]
        source=first['from'];target=first['target']
        before={row['pointer_before'] for row in boundaries};after={row['pointer_after'] for row in boundaries}
        guarded=(rom[source]==0xfd and struct.unpack_from('<H',rom,source+1)[0]==target
            and len(before)==len(after)==1 and next(iter(after))==target+2
            and (next(iter(before))==source or (next(iter(before))==source-1 and rom[source-1]==0xf2)))
        if channel<8:
            own=[e for e in ym if (e[2]==8 and (e[3]&7)==channel) or (e[2]>=0x20 and (e[2]&7)==channel)]
        else:own=pcm
        evidence=sequence_evidence(own,boundaries,scan['timeline'])
        evidence.update(channel=channel,kind='FM' if channel<8 else 'PCM',FD_guard_verified=guarded,
            FD_source=source,FD_target=target,FD_checkpoints=boundaries)
        evidence['score_cycle_observed'] &= guarded
        channels.append(evidence)
    anchors=[selected[i] for i in scan['anchors']]
    global_musical=[e for e in ym if e[2]!=0x14]
    global_evidence=sequence_evidence(global_musical,anchors,scan['timeline'])
    unassigned=[e for e in ym if anchors[0]['cycle']<=e[0]<anchors[-1]['cycle'] and e[2]<0x20 and e[2] not in (8,0x14)]
    best_states=[selected[i] for i in scan['best'][:4]]
    differences=[state_differences(a,b,layout) for a,b in zip(best_states,best_states[1:])]
    witness=differences[1] if len(differences)>1 else differences[0]
    result=dict(command=command,**classify_channels(channels,track['classification']),channels=channels,
        timer_b_register=timer,timer_b_period_external_cycles=1024*(256-timer),standard_timer_mode_verified=True,
        max_pointer_pattern_cycle_groups=scan['max_groups'],global_anchor_kind=scan['anchor_kind'],
        global_external_cycle_sequences_equal=global_evidence['three_internal_external_cycle_sequences_equal'],
        global_sequence_evidence=global_evidence,unassigned_global_musical_writes=len(unassigned),
        state_difference_causes=sorted({row['meaning'] for row in witness['changed_RAM']}),
        equal_pointer_state_witnesses=differences,
        input_sha256={'snapshot':track['compressed_snapshot_sha256'],'uncompressed_snapshot':track['snapshot_sha256'],'trace':trace_sha})
    if unassigned:
        result['classification']='CHANNEL_CYCLE_EVIDENCE_INCOMPLETE'
        result['limitation']='Global musical writes require separate analysis before channel-only closure can be claimed'
    if observer.sha(snapshot)!=track['compressed_snapshot_sha256'] or observer.sha(trace)!=trace_sha:
        raise ValueError('Snapshot/trace changed during channel analysis')
    return result


def compare_prior(report,prior):
    mismatches=[];old_tracks={t['command']:t for t in prior['tracks']}
    for track in report['tracks']:
        old={c['channel']:c for c in old_tracks[track['command']]['channels']}
        for channel in track['channels']:
            if channel.get('state')=='ORIGINAL_CHANNEL_END_OBSERVED':continue
            before=old[channel['channel']]
            fields={'cycle_timer_b_services':'score_cycle_timer_b_services',
                    'complete_cycles':'complete_observed_cycles','events_per_cycle':'events_per_cycle'}
            for new_key,old_key in fields.items():
                if channel[new_key]!=before[old_key]:mismatches.append([track['command'],channel['channel'],new_key])
            if not channel['all_value_sequences_equal'] or not channel['all_service_sequences_equal']:
                mismatches.append([track['command'],channel['channel'],'complete_sequence_equality'])
            if channel['three_internal_external_cycle_sequences_equal']:
                mismatches.append([track['command'],channel['channel'],'external_cycle_equality'])
    return dict(matched=not mismatches,compared_fields=['per-channel service length','complete cycle count',
        'event count','complete value/service equality','external-cycle inequality'],mismatches=mismatches)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--structure-dir',type=Path,required=True)
    parser.add_argument('--reference-root',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--commands',nargs='+',type=lambda value:int(value,0),default=[0x0b,0x0c,0x11])
    parser.add_argument('--prior-summary',type=Path)
    parser.add_argument('--cc',default='cc')
    args=parser.parse_args();structure=args.structure_dir.resolve();reference=args.reference_root.resolve()
    if not args.commands or len(set(args.commands))!=len(args.commands) or any(not 1<=c<=18 for c in args.commands):
        parser.error('Choose distinct original music commands1..18')
    capture_path=structure/'music-structure.json';capture=json.loads(capture_path.read_text())
    if not capture['complete'] or not capture['source_stable']:raise ValueError('Incomplete or unstable base capture')
    tracks={t['command']:t for t in capture['tracks']}
    if any(c not in tracks for c in args.commands):raise ValueError('Selected command absent from capture')
    frozen=('tools/reference/probe_full_board_music_state.c','tools/reference/analyze_full_board_music_structure.py',
            'tests/test_full_board_music_state.c','tests/test_full_board_music_state.py')
    for name in (*frozen,'src/reference_driver/z80.h'):
        recorded=[value for path,value in capture['source_sha256'].items() if path.endswith('/'+name)]
        if len(recorded)!=1 or observer.sha(PACKAGE/name)!=recorded[0]:raise ValueError('Frozen observer source mismatch: '+name)
    rom_path=reference/'inputs/SOUND.ROM';rom=rom_path.read_bytes();verify_controller(rom)
    if observer.sha(reference/'deps/opm.h')!=observer.PINNED['opm.h']:raise ValueError('Pinned OPM header mismatch')
    out=prepare_output(args.out)
    layout,build_command=build_layout(out,reference,args.cc)
    source_files={str(Path(__file__).relative_to(PACKAGE)):Path(__file__),**{name:PACKAGE/name for name in frozen},
                  'tests/test_full_board_channel_structure.py':PACKAGE/'tests/test_full_board_channel_structure.py',
                  'src/reference_driver/z80.h':PACKAGE/'src/reference_driver/z80.h',
                  'dependency/opm.h':reference/'deps/opm.h'}
    source_sha={name:observer.sha(path) for name,path in source_files.items()}
    report=dict(schema_version=SCHEMA_VERSION,tool_version=TOOL_VERSION,
        kind='BOUNDED_PER_CHANNEL_SCORE_CYCLES_FROM_EXISTING_CAPTURES',source_sha256=source_sha,
        input_sha256={'all18_capture_report':observer.sha(capture_path),'sound_rom':digest(rom)},
        arguments={'structure_dir':str(structure),'reference_root':str(reference),'out':str(out),'commands':args.commands},
        layout=layout,layout_build_command=build_command,external_clock_hz=CLOCK,
        timer_unit='One acknowledged original Timer B service at a canonical post-handler checkpoint',
        controller_guards_verified=True,tracks=[],passed=False,
        limitations=['Only the measured original-ROM control sequences are established.',
            'Independent channel cycles need not share a short global period.',
            'External-cycle delta comparisons retain every difference with zero tolerance.',
            'Neither sample-exact PCM periodicity nor universal future behavior or physical hardware fidelity is certified.'])
    for command in args.commands:
        item=analyze_track(command,structure,reference,layout,tracks[command],rom)
        report['tracks'].append(item)
        for name,value in item['input_sha256'].items():report['input_sha256'][f'music_{command:02X}_{name}']=value
        (out/f'command-{command:02X}-channel-structure.json').write_text(json.dumps(item,indent=2)+'\n')
        print(json.dumps({'command':f'0x{command:02X}','classification':item['classification']}),flush=True)
    report['source_stable']=source_sha=={name:observer.sha(path) for name,path in source_files.items()}
    report['capture_report_stable']=observer.sha(capture_path)==report['input_sha256']['all18_capture_report']
    report['ROM_stable']=observer.sha(rom_path)==report['input_sha256']['sound_rom']
    report['active_FM_channels_verified']=sum(c['kind']=='FM' and c.get('score_cycle_observed',False) for t in report['tracks'] for c in t['channels'])
    report['PCM_score_channels_verified']=sum(c['kind']=='PCM' and c.get('score_cycle_observed',False) for t in report['tracks'] for c in t['channels'])
    report['passed']=report['source_stable'] and report['capture_report_stable'] and report['ROM_stable'] and all(t['classification']=='PER_CHANNEL_SCORE_CYCLES_OBSERVED' for t in report['tracks'])
    if args.prior_summary:
        prior=args.prior_summary.resolve();report['input_sha256']['previous_supplement']=observer.sha(prior)
        report['previous_result_comparison']=compare_prior(report,json.loads(prior.read_text()))
        report['passed'] &= report['previous_result_comparison']['matched']
    detailed=out/'channel-structure.json';detailed.write_text(json.dumps(report,indent=2)+'\n')
    public=public_summary(report,capture)
    public.update(detailed_report_sha256=observer.sha(detailed),source_stable=report['source_stable'],
        active_FM_channels_verified=report['active_FM_channels_verified'],PCM_score_channels_verified=report['PCM_score_channels_verified'])
    (out/'public-summary.json').write_text(json.dumps(public,indent=2)+'\n')
    print(json.dumps({'passed':report['passed'],'FM_channels':report['active_FM_channels_verified'],
        'PCM_channels':report['PCM_score_channels_verified'],'report_sha256':observer.sha(detailed)}),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
