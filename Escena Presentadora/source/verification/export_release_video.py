#!/usr/bin/env python3
"""Export two complete native presentations; cut only inter-scene idle time."""
from pathlib import Path
import hashlib
import json
import subprocess

wp=Path(__file__).resolve().parents[1]
source=wp/'verification/release-r02-full.mov'
output=wp/'deliverables/WP02-Saturn-TAG-R02-Mednafen.mp4'
audit=wp/'verification/release-media-r02.json'

def probe(path):
    command=['ffprobe','-v','error','-select_streams','v:0','-show_entries',
             'stream=codec_name,width,height,time_base,nb_frames:frame=best_effort_timestamp',
             '-of','json',str(path)]
    result=subprocess.run(command,capture_output=True,text=True,check=True)
    if result.stderr.strip():raise RuntimeError(result.stderr)
    data=json.loads(result.stdout)
    return data['streams'][0],[int(f['best_effort_timestamp'])for f in data['frames']]

stream,pts=probe(source)
assert stream['time_base']=='1/44100'
# First presentation through one second after exit; native START black frames
# and second presentation through one second after exit. End indices exclusive.
ranges=[(1871,3217),(4931,6563)]
expected=[];offset=0;intervals=[]
filters=['[0:v:0]split=2[vi0][vi1]','[0:a:0]asplit=2[ai0][ai1]']
for i,(first,end) in enumerate(ranges):
    begin,finish=pts[first],pts[end]
    expected.extend(offset+t-begin for t in pts[first:end])
    intervals.append({'source_first_frame':first,'source_end_frame_exclusive':end,
                      'source_first_pts':begin,'source_end_pts_exclusive':finish,
                      'source_first_seconds':begin/44100,'source_end_seconds':finish/44100,
                      'output_first_pts':offset,'frames':end-first})
    filters.append(f'[vi{i}]trim=start_frame={first}:end_frame={end},setpts=PTS-STARTPTS,crop=640:480:32:0[v{i}]')
    filters.append(f'[ai{i}]atrim=start_sample={begin}:end_sample={finish},asetpts=PTS-STARTPTS[a{i}]')
    offset+=finish-begin
filters.append('[v0][a0][v1][a1]concat=n=2:v=1:a=1[v][a]')
command=['ffmpeg','-v','error','-i',str(source),'-filter_complex',';'.join(filters),
         '-map','[v]','-map','[a]','-c:v','libx264','-crf','16','-preset','slow',
         '-pix_fmt','yuv420p','-fps_mode','passthrough','-enc_time_base:v','1:44100',
         '-video_track_timescale','44100','-movie_timescale','44100',
         '-c:a','aac','-b:a','192k','-movflags','+faststart',str(output)]
if output.exists():raise RuntimeError('Refuse to overwrite an existing final video')
subprocess.run(command,check=True)
actual_stream,actual=probe(output)
assert actual_stream['time_base']=='1/44100'
assert (actual_stream['width'],actual_stream['height'])==(640,480)
assert len(actual)==len(expected),(len(actual),len(expected))
errors=[a-e for a,e in zip(actual,expected)]
assert max(map(abs,errors))<=1,{'maximum_timestamp_error_samples':max(map(abs,errors))}
assert all(a<b for a,b in zip(actual,actual[1:]))
# Decode every exported video and audio packet, rejecting codec errors.
decoded=subprocess.run(['ffmpeg','-v','error','-i',str(output),'-f','null','-'],
                       capture_output=True,text=True,check=True)
if decoded.stderr.strip():raise RuntimeError(decoded.stderr)
report={'status':'PASS','source_capture':str(source),'video':str(output),
        'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
        'video_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
        'source_bin_sha256':'c491c330f5e2016bf2c48aa2046dc539fd760d7b20e672e4ae9ec2217bd1f552',
        'command':command,'intervals':intervals,'time_base':'1/44100',
        'expected_video_frames':len(expected),'actual_video_frames':len(actual),
        'max_abs_timestamp_error_samples':max(map(abs,errors)),
        'output_duration_samples':offset,'output_duration_seconds':offset/44100,
        'all_exported_frames_and_audio_decoded_without_errors':True,
        'edits':'Trim initial boot, inter-scene completed-ring idle, and final idle; crop32 pixels of horizontal overscan each side. All scene frames and native timing retained.',
        'encoding':'H.264/AAC viewing copy; exact RGB/PCM reference MOV is included in the ZIP.'}
audit.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='command'},indent=2))
