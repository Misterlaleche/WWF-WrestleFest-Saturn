#!/usr/bin/env python3
"""Verify every actual emulator scene frame against exact native RGB hashes.

The capture is decoded losslessly, cropped from704x480 atx32 to640x480,
and reduced2:1 with nearest-neighbor sampling. No interpolation, palette
tolerance, approximate image matching, or emulator metrics are used.
"""
from __future__ import annotations
import argparse
import csv
from fractions import Fraction
import hashlib
import json
import statistics
import subprocess
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parent
OUT=ROOT/'generated'
ARCADE_HZ=7_000_000/(448*272)


def hash_runs(hashes):
    runs=[]
    for index,digest in enumerate(hashes):
        if runs and runs[-1]['sha256']==digest:
            runs[-1]['last']=index;runs[-1]['count']+=1
        else:runs.append({'first':index,'last':index,'count':1,'sha256':digest})
    return runs


def align_runs(reference,actual):
    """Longest common subsequence over visual *occurrences*, not unique hashes."""
    a=[r['sha256']for r in reference];b=[r['sha256']for r in actual]
    n,m=len(a),len(b)
    dp=[[0]*(m+1)for _ in range(n+1)]
    for i in range(n-1,-1,-1):
        for j in range(m-1,-1,-1):
            dp[i][j]=1+dp[i+1][j+1]if a[i]==b[j]else max(dp[i+1][j],dp[i][j+1])
    matches=[];i=j=0
    while i<n and j<m:
        if a[i]==b[j]:matches.append((i,j));i+=1;j+=1
        elif dp[i+1][j]>=dp[i][j+1]:i+=1
        else:j+=1
    ri={i for i,j in matches};ai={j for i,j in matches}
    missing=[i for i in range(n)if i not in ri]
    extra=[j for j in range(m)if j not in ai]
    known=set(a)
    # Unknown/corrupt images are reported separately from reverse progression.
    extra_known=[j for j in extra if b[j]in known]
    return {'exact_visual_sequence':a==b,'reference_visual_runs':n,'actual_visual_runs':m,
            'matched_visual_runs':len(matches),'monotonic_known_progression':not extra_known,
            'no_omitted_visual_states':not missing,'missing_reference_runs':missing,
            'extra_actual_runs':extra,'matches':matches}


def timing_analysis(reference,actual,pts,arcade_hz,refresh_period,timestamp_quantum):
    """Compare displayed transition times with original update counts.

    A transition first becomes observable at a display-frame boundary. Two
    transitions quantized to the same refresh grid may differ in duration
    from their ideal update interval by at most one refresh period. Two
    timestamp quanta cover container timestamp rounding at both boundaries.
    """
    assert len(reference)==len(actual)
    tolerance=refresh_period+2*timestamp_quantum
    boundaries=[];holds=[]
    for i,(r,a)in enumerate(zip(reference,actual)):
        seconds=pts[a['first']]
        boundaries.append({'native_update':r['first'],'actual_video_frame':a['first'],
                           'actual_seconds':seconds,'ideal_seconds':r['first']/arcade_hz,
                           'phase_seconds':seconds-r['first']/arcade_hz})
        if i:
            old=boundaries[i-1]
            observed=seconds-old['actual_seconds']
            ideal=(r['first']-old['native_update'])/arcade_hz
            holds.append({'first_native_update':old['native_update'],
                          'next_native_update':r['first'],
                          'native_updates':r['first']-old['native_update'],
                          'actual_seconds':observed,'expected_seconds':ideal,
                          'difference_seconds':observed-ideal,
                          'within_refresh_tolerance':abs(observed-ideal)<=tolerance})
    phase=[p['phase_seconds']for p in boundaries]
    # Long-period rate fit supplements individual holds and is not used to
    # excuse an omitted visual run or an out-of-tolerance hold.
    xs=[p['native_update']for p in boundaries];ys=[p['actual_seconds']for p in boundaries]
    mx,my=statistics.mean(xs),statistics.mean(ys)
    slope=sum((x-mx)*(y-my)for x,y in zip(xs,ys))/sum((x-mx)**2 for x in xs)
    return {'original_update_rate_hz':arcade_hz,'capture_refresh_period_seconds':refresh_period,
            'container_timestamp_quantum_seconds':timestamp_quantum,
            'duration_tolerance_seconds':tolerance,
            'tolerance_basis':'one output refresh period plus two container timestamp quanta',
            'transition_phase_range_seconds':max(phase)-min(phase),
            'fitted_native_update_rate_hz':1/slope,
            'all_visual_hold_durations_within_refresh_tolerance':all(p['within_refresh_tolerance']for p in holds),
            'maximum_absolute_hold_error_seconds':max(abs(p['difference_seconds'])for p in holds),
            'holds':holds,'boundaries':boundaries}


def probe(path):
    command=['ffprobe','-v','error','-select_streams','v:0','-show_entries',
             'stream=codec_name,width,height,r_frame_rate,avg_frame_rate,time_base,duration,nb_frames:frame=best_effort_timestamp_time,pkt_duration_time',
             '-of','json',str(path)]
    result=subprocess.run(command,capture_output=True,text=True,check=True)
    if result.stderr.strip():raise RuntimeError('ffprobe reported decode errors: '+result.stderr[:2000])
    info=json.loads(result.stdout)
    return info['streams'][0],info['frames']


def decode(path,tag,known):
    command=['ffmpeg','-v','error','-i',str(path),'-map','0:v:0','-an','-sn',
             '-vf','crop=640:480:32:0,scale=320:240:flags=neighbor',
             '-vsync','0','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
    errors=OUT/f'{tag}_decode.log'
    hashes=[];first_known=False;saved_unknown=False
    with errors.open('wb')as err:
        process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=err)
        try:
            while True:
                data=process.stdout.read(320*240*3)
                if not data:break
                if len(data)!=320*240*3:raise RuntimeError('Incomplete decoded RGB frame')
                digest=hashlib.sha256(data).hexdigest();hashes.append(digest)
                if digest in known:first_known=True
                elif first_known and not saved_unknown:
                    Image.frombytes('RGB',(320,240),data).save(OUT/f'{tag}_first_nonreference_after_scene.png')
                    saved_unknown=True
            code=process.wait()
        except BaseException:
            process.kill();process.wait();raise
    diagnostics=errors.read_text()
    if code or diagnostics.strip():raise RuntimeError(f'ffmpeg exit{code}: '+diagnostics[:2000])
    return hashes,command


def exit_indices():
    actors=[]
    with (ROOT/'native_scene.csv').open()as f:
        for row in csv.DictReader(f):
            if row['actor']=='1':actors.append({k:int(v)for k,v in row.items()})
    state_start=next(a['index']for a in actors if a['state']>=21)
    movement_start=next(a['index']for a,p in zip(actors[1:],actors)
                        if a['state']>=21 and (a['x'],a['y'],a['z'])!=(p['x'],p['y'],p['z']))
    return state_start,movement_start


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    parser.add_argument('--reference',type=Path,default=OUT/'reference_frame_hashes.json')
    args=parser.parse_args()
    if args.capture.name=='reference-v3.mov':raise SystemExit('Refusing known-corrupt reference-v3.mov')
    reference=json.loads(args.reference.read_text())
    pairs=sorted((int(k),v)for k,v in reference['sha256_by_native_update'].items())
    assert [i for i,h in pairs]==list(range(len(pairs)))
    native=[h for i,h in pairs];native_runs=hash_runs(native)
    stream,frame_info=probe(args.capture)
    assert (stream['width'],stream['height'])==(704,480),stream
    tag=args.capture.stem.replace('-','_')
    actual,command=decode(args.capture,tag,set(native))
    assert len(actual)==len(frame_info),(len(actual),len(frame_info))
    pts=[float(p['best_effort_timestamp_time'])for p in frame_info]
    assert all(b>a for a,b in zip(pts,pts[1:])),'Nonmonotonic video timestamps'
    periods=[b-a for a,b in zip(pts,pts[1:])]
    period=statistics.median(periods)
    quantum=float(Fraction(stream['time_base']))
    starts=[i for i,h in enumerate(actual)if h==native[0]]
    start=starts[0]if starts else None
    ends=[i for i,h in enumerate(actual)if h==native[-1]and start is not None and i>=start]
    end=ends[0]if ends else None
    report={'capture':str(args.capture),'reference':str(args.reference),'stream':stream,
            'decode_command':command,'decoded_frames':len(actual),
            'crop':'704x480 -> crop x32,y0,w640,h480 -> nearest-neighbor320x240',
            'exact_hash_comparison':'SHA256 of RGB888 row-major pixels; zero color/geometry tolerance',
            'scene_first_video_frame':start,'terminal_clear_first_video_frame':end,
            'status':'FAIL'}
    if start is None or end is None:
        report['error']='Complete reference start and terminal clear were not both found'
        report['matching_frames']=sum(h in set(native)for h in actual)
    else:
        segment=actual[start:end+1]
        observed=hash_runs(segment)
        # Convert local run locations back to actual capture frame indices.
        for run in observed:run['first']+=start;run['last']+=start
        alignment=align_runs(native_runs,observed)
        reference_set=set(native);actual_set=set(segment)
        unknown=[i for i in range(start,end+1)if actual[i]not in reference_set]
        report.update({'scene_first_seconds':pts[start],'terminal_clear_first_seconds':pts[end],
                       'scene_video_frames_through_clear':len(segment),
                       'all_scene_images_exact_native_images':not unknown,
                       'unknown_scene_frame_count':len(unknown),
                       'unknown_scene_frames':unknown,
                       'reference_unique_images':len(reference_set),'actual_unique_scene_images':len(actual_set),
                       'missing_unique_reference_images':sorted(reference_set-actual_set),
                       'alignment':alignment,'actual_visual_runs':observed})
        if alignment['exact_visual_sequence']:
            timing=timing_analysis(native_runs,observed,pts,ARCADE_HZ,period,quantum)
            state_exit,visible_exit=exit_indices()
            exit_run=next(i for i,r in enumerate(native_runs)if r['first']==visible_exit)
            exit_video=observed[exit_run]['first']
            segments=[]
            for label,first_update,first_frame in [('complete_scene',0,start),('visible_exit',visible_exit,exit_video)]:
                expected=(len(native)-1-first_update)/ARCADE_HZ
                measured=pts[end]-pts[first_frame]
                segments.append({'segment':label,'first_native_update':first_update,
                                 'terminal_clear_native_update':len(native)-1,
                                 'first_video_frame':first_frame,'terminal_clear_video_frame':end,
                                 'original_update_intervals':len(native)-1-first_update,
                                 'expected_seconds':expected,'actual_seconds':measured,
                                 'difference_seconds':measured-expected,
                                 'within_refresh_tolerance':abs(measured-expected)<=timing['duration_tolerance_seconds']})
            timing['segments']=segments
            timing['exit_state_first_update']=state_exit
            timing['exit_visible_movement_first_update']=visible_exit
            timing['exit_boundary_note']='State21 begins one unchanged-image update before visible movement; video timing uses first visible movement.'
            timing['all_segment_durations_within_refresh_tolerance']=all(r['within_refresh_tolerance']for r in segments)
            report['timing']=timing
            if not unknown and timing['all_visual_hold_durations_within_refresh_tolerance'] and timing['all_segment_durations_within_refresh_tolerance']:
                report['status']='PASS'
    destination=OUT/f'{tag}_capture_audit.json'
    destination.write_text(json.dumps(report,indent=2)+'\n')
    (OUT/f'{tag}_decoded_hashes.json').write_text(json.dumps({'pts_seconds':pts,'sha256':actual},separators=(',',':'))+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in {'actual_visual_runs','timing','alignment','unknown_scene_frames','decode_command'}},indent=2))
    if 'alignment'in report:print('alignment',json.dumps({k:v for k,v in report['alignment'].items()if k!='matches'}))
    if 'timing'in report:print('timing',json.dumps({k:v for k,v in report['timing'].items()if k not in {'holds','boundaries'}},indent=2))
    print('report',destination)
    return 0 if report['status']=='PASS'else 1


if __name__=='__main__':raise SystemExit(main())
