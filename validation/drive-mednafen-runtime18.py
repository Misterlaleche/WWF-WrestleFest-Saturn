#!/usr/bin/env python3
"""Drive the real emulated Saturn pad using X keyboard events; observe read-only.

No emulator/game memory is written. The exact executable signature and SH-2
layout identify WRAM. Event timing and observed inputs are retained, so this
is an automated live-input test, never a claim of physical controller play.
"""
import argparse,contextlib,io,json,os,re,struct,subprocess,sys,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('pid',type=int);p.add_argument('binary',type=Path);p.add_argument('linker_map',type=Path);p.add_argument('actors_layout',type=Path);p.add_argument('--config',type=Path);p.add_argument('--events',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--seconds',type=float,default=600);p.add_argument('--stop-file',type=Path)
a=p.parse_args();events=json.loads(a.events.read_text());assert events==sorted(events,key=lambda e:e['frame']);assert all(e['frame']>=0 and e['action'] in ('keydown','keyup') for e in events)
layout=struct.unpack('>5I',a.actors_layout.read_bytes());assert layout[1]==0x10c
observer=Path(__file__).with_name('observe-mednafen-runtime18.py')
source=observer.read_text().replace('candidates.append(values)',"values['_host_ram_base']=base; candidates.append(values)")
started=time.monotonic();ns=None
while time.monotonic()-started<a.seconds:
 try:
  sys.argv=[str(observer),str(a.pid),str(a.binary),str(a.linker_map)]
  trial={'__name__':'__main__'}
  with contextlib.redirect_stdout(io.StringIO()):exec(compile(source,str(observer),'exec'),trial)
  if len(trial['candidates'])==1:ns=trial;break
 except (AssertionError,SystemExit):pass
 time.sleep(.05)
if ns is None:raise SystemExit('Executable WRAM signature was not found')
host=ns['candidates'][0]['_host_ram_base'];symbols=ns['symbols'];rt=symbols['rt']
match=re.search(r'\.bss\.gameplay(?:\.lto_priv\.\d+)?\s+0x([0-9a-f]+)\s',ns['mapping']);assert match
gameplay=int(match[1],16)
window=subprocess.check_output(['xdotool','search','--pid',str(a.pid),'--name','Wrestle'],text=True).splitlines()[0]
subprocess.run(['xdotool','windowfocus',window],check=True)
held=set();sent=[];samples=[];last_frame=-1;event_index=0;result={'method':'X keyboard -> Mednafen Saturn pad -> Yaul SMPC; memory read-only','binary_sha256':ns['hashlib'].sha256(a.binary.read_bytes()).hexdigest(),'emulator_pid':a.pid,'window_id':window,'config_sha256':ns['hashlib'].sha256(a.config.read_bytes()).hexdigest() if a.config else None,'events':sent,'samples':samples,'sampling':'asynchronous read-only observations; frame checked before/after each sample, no emulator pause'}
with open(f'/proc/{a.pid}/mem','rb',buffering=0) as memory:
 def read(address,n):
  assert address%2==0 and n%2==0
  memory.seek(host+address-0x06000000);raw=memory.read(n);assert len(raw)==n
  return b''.join(raw[i:i+2][::-1] for i in range(0,n,2))
 def u32(address):return int.from_bytes(read(address,4),'big')
 try:
  while time.monotonic()-started<a.seconds:
   frame=u32(rt+24)
   fatal=u32(symbols['fatal_owner']);finished=u32(symbols['finished_frame'])
   if frame!=last_frame:
    actor=read(rt+layout[0],layout[1]);word=lambda o:int.from_bytes(actor[o:o+2],'big')
    input_bits=read(gameplay+layout[4],10)[0]
    if u32(rt+24)!=frame:continue
    samples.append({'seconds':round(time.monotonic()-started,6),'frame':frame,'input':input_bits,'position':[word(6),word(10)],'state':word(0x20),'selector':word(0x60),'stamina':word(0x66),'held':word(0xa6),'fresh':word(0xa2),'direction':word(0xa8)})
    last_frame=frame
   while event_index<len(events) and frame>=events[event_index]['frame']:
    e=events[event_index];subprocess.run(['xdotool',e['action'],'--delay','0',e['key']],check=True)
    if e['action']=='keydown':held.add(e['key'])
    else:held.discard(e['key'])
    sent.append({**e,'observed_frame':frame,'frame_after_command':u32(rt+24),'seconds':round(time.monotonic()-started,6)});event_index+=1
   if fatal:
    result.update(status='blocked',fatal_owner=fatal,fatal_dependency=u32(symbols['fatal_dependency']),fatal_frame=u32(symbols['fatal_frame']));break
   if finished and u32(symbols['finished_presentation']):
    result.update(status='complete',finished_frame=finished,finished_result=u32(symbols['finished_result']),finished_count=u32(symbols['finished_count']),finished_timer=u32(symbols['finished_timer']),finished_vblanks=u32(symbols['finished_vblanks']),finished_presentation_vblanks=u32(symbols['finished_presentation_vblanks']));break
   time.sleep(.001)
  else:result.update(status='timeout',last_frame=last_frame)
 finally:
  cleanup_errors=[]
  for key in sorted(held):
   try:
    release=subprocess.run(['xdotool','keyup','--delay','0',key],check=False)
    if release.returncode:cleanup_errors.append({'key':key,'exit':release.returncode})
   except OSError as error:cleanup_errors.append({'key':key,'error':str(error)})
  result['cleanup_errors']=cleanup_errors
  result['sampled_non_ff_frames']=sum(s['frame']>0 and s['input']!=255 for s in samples)
  result['decoded_action_frames']=sum(bool(s['held'] or s['fresh']) for s in samples)
  result['decoded_direction_frames']=sum(bool(s['direction']) for s in samples)
  result['late_events']=[e for e in sent if max(e['observed_frame'],e['frame_after_command'])>e['frame']+1]
  downs={};zero_holds=[]
  for event in sent:
   if event['action']=='keydown':downs[event['key']]=event
   elif event['key'] in downs and event['observed_frame']<=downs.pop(event['key'])['observed_frame']:zero_holds.append(event)
  result['zero_frame_holds']=zero_holds
  result['elapsed_seconds']=time.monotonic()-started;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
  if a.stop_file and result.get('status')=='complete':a.stop_file.touch()
print(json.dumps({k:v for k,v in result.items() if k not in ('samples',)},indent=2))
raise SystemExit(0 if result.get('status')=='complete' else 1)
