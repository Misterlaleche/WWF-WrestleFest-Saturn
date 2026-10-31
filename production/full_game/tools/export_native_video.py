#!/usr/bin/env python3
"""Export a Mednafen MOV without changing, dropping or duplicating video PTS."""
import argparse,hashlib,json,subprocess
from fractions import Fraction
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path)
p.add_argument('--ffmpeg',default='ffmpeg');p.add_argument('--ffprobe',default='ffprobe');a=p.parse_args()
source=a.directory/'native.mov';target=a.directory/'native.mp4'
def probe(path):
 return json.loads(subprocess.check_output([a.ffprobe,'-v','error','-select_streams','v:0','-show_streams','-show_frames','-show_entries','stream=time_base,width,height:frame=pts','-of','json',str(path)],text=True))
before=probe(source);tb=before['streams'][0]['time_base'];den=Fraction(tb).denominator
command=[a.ffmpeg,'-hide_banner','-nostdin','-n','-i',str(source),'-map','0:v:0','-map','0:a?',
 '-fps_mode:v','passthrough','-enc_time_base:v',tb,'-c:v','libx264','-preset','fast','-crf','18',
 '-pix_fmt','yuv420p','-threads:v','2','-video_track_timescale',str(den),'-c:a','aac','-b:a','128k','-movflags','+faststart',str(target)]
with (a.directory/'video-export.log').open('wb') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
after=probe(target);ta=after['streams'][0]['time_base']
x=[Fraction(tb)*v['pts'] for v in before['frames']];y=[Fraction(ta)*v['pts'] for v in after['frames']]
report={'status':'PASS' if x==y and len(x)>1 else 'FAIL','scope':'Recorded frame timestamps, not game update rate',
 'command':command,'source_frames':len(x),'mp4_frames':len(y),'all_video_pts_identical':x==y,
 'source_time_base':tb,'mp4_time_base':ta,'source_geometry':before['streams'][0],
 'mean_recorded_field_rate':float(Fraction(len(x)-1)/(x[-1]-x[0])),
 'files':{v.name:hashlib.sha256(v.read_bytes()).hexdigest() for v in (source,target)}}
(a.directory/'video-pts-check.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2));raise SystemExit(report['status']!='PASS')
