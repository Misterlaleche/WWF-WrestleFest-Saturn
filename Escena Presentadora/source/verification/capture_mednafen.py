#!/usr/bin/env python3
"""Capture the actual independent Saturn disc and extract its in-RAM counters."""
from pathlib import Path
import argparse, gzip, hashlib, json, os, re, shutil, signal, struct, subprocess, time

wp=Path(__file__).resolve().parents[1]
sysroot=wp/'deps/sysroot'
bundled_mednafen=wp/'deps/mednafen132/usr/games/mednafen'
bundled_xdotool=sysroot/'usr/bin/xdotool'
p=argparse.ArgumentParser()
p.add_argument('cue',type=Path)
p.add_argument('prefix',type=Path)
p.add_argument('--mednafen',default=str(bundled_mednafen) if bundled_mednafen.is_file() else shutil.which('mednafen') or 'mednafen',help='Mednafen 1.32.1 executable')
p.add_argument('--xdotool',default=str(bundled_xdotool) if bundled_xdotool.is_file() else shutil.which('xdotool') or 'xdotool')
p.add_argument('--home',type=Path,default=wp/'verification/mednafen_home',help='Private Mednafen home containing your own Saturn BIOS')
p.add_argument('--display',default=os.environ.get('DISPLAY',':82'))
p.add_argument('--cache',choices=['full','data','auto'],default='full')
p.add_argument('--seconds',type=float,default=55)
p.add_argument('--save-at',type=float,default=49)
p.add_argument('--restart-at',type=float)
p.add_argument('--save-after-restart',type=float,default=88)
a=p.parse_args()
cue=a.cue.resolve()
cue_text=cue.read_text()
match=re.search(r'^FILE "([^"]+)" BINARY$',cue_text,re.M)
if not match:raise RuntimeError('Expected one local BINARY track')
disc=cue.parent/match.group(1)
disc_bytes=disc.read_bytes()
disc_hash=hashlib.sha256(disc_bytes).hexdigest()
if not disc_bytes or len(disc_bytes)%2352:raise RuntimeError('Incomplete raw disc sector')
manifest_path=cue.parent/'disc-validation.json'
if manifest_path.is_file():
 manifest=json.loads(manifest_path.read_text())
 if len(disc_bytes)!=manifest['sectors']*2352 or disc_hash!=manifest['bin_sha256']:
  raise RuntimeError('Disc differs from validated complete image; refuse capture')
 if hashlib.sha256(cue_text.encode()).hexdigest()!=manifest['cue_sha256']:
  raise RuntimeError('CUE differs from validated image')
del disc_bytes
prefix=a.prefix.resolve()
prefix.parent.mkdir(parents=True,exist_ok=True)
home=a.home.resolve()
home.mkdir(parents=True,exist_ok=True)
env=os.environ.copy()
env.update(DISPLAY=a.display,SDL_AUDIODRIVER='dummy',MEDNAFEN_HOME=str(home))
if sysroot.is_dir():
 env['LD_LIBRARY_PATH']=f'{sysroot}/lib/x86_64-linux-gnu:{sysroot}/usr/lib/x86_64-linux-gnu'+(':'+env['LD_LIBRARY_PATH'] if env.get('LD_LIBRARY_PATH') else '')
cmd=[a.mednafen,'-ss.cart','none','-ss.dbg_cem',a.cache,'-filesys.path_firmware',str(home),
 '-video.driver','softfb','-video.blit_timesync','0','-ss.correct_aspect','0',
 '-ss.h_overscan','1','-ss.stretch','0','-ss.xscale','2','-ss.yscale','2',
 '-ss.slstart','0','-ss.slend','239','-sound.driver','dummy','-sound.rate','44100',
 '-qtrecord.w_double_threshold','384','-qtrecord.h_double_threshold','256',
 '-qtrecord',str(prefix.with_suffix('.mov')),str(a.cue.resolve())]
started=time.time()
first_state=None
def window():
 r=subprocess.run([a.xdotool,'search','--onlyvisible','--pid',str(proc.pid)],env=env,capture_output=True,text=True)
 return r.stdout.split()[0] if r.stdout.split() else None
def key(name):
 w=window()
 if not w:return False
 subprocess.run([a.xdotool,'key','--delay','120','--window',w,name],env=env,check=True)
 return True
with prefix.with_suffix('.log').open('w') as log:
 proc=subprocess.Popen(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
 saved=False;restarted=False;saved_again=False
 while proc.poll() is None and time.time()-started<a.seconds:
  if not saved and time.time()-started>=a.save_at:
   saved=key('F5')
  if a.restart_at and not restarted and time.time()-started>=a.restart_at:
   previous=[f for f in (home/'mcs').glob('*.mc0') if f.stat().st_mtime>=started]
   if previous:
    first_state=max(previous,key=lambda f:f.stat().st_mtime).read_bytes()
    prefix.with_name(prefix.name+'-before-restart.mc0').write_bytes(first_state)
   restarted=key('Return')
  if restarted and not saved_again and time.time()-started>=a.save_after_restart:
   saved_again=key('F5')
  time.sleep(.1)
 if proc.poll() is None:proc.send_signal(signal.SIGINT)
 proc.wait(timeout=20)
if hashlib.sha256(disc.read_bytes()).hexdigest()!=disc_hash:
 raise RuntimeError('Disc changed during emulator capture')
result={'cue':str(cue),'bin_sha256':disc_hash,'disc_integrity':'verified before and after capture','command':cmd,'exit_code':proc.returncode,'state_key_sent':saved,'restart_sent':restarted,'state_after_restart_sent':saved_again}
states=[f for f in (home/'mcs').glob('*.mc0') if f.stat().st_mtime>=started]
if states:
 latest=max(states,key=lambda f:f.stat().st_mtime)
 compressed=latest.read_bytes()
 prefix.with_suffix('.mc0').write_bytes(compressed)
 def metrics(compressed):
  raw=gzip.decompress(compressed)
  size=68
  magic=bytes.fromhex('575030324d45545249435332feedc0de')
  swapped_magic=b''.join(magic[i:i+2][::-1] for i in range(0,16,2))
  offset=raw.find(swapped_magic)
  if offset<0:
   size=56;magic=bytes.fromhex('575030324d45545249435331feedc0de')
   swapped_magic=b''.join(magic[i:i+2][::-1] for i in range(0,16,2))
   offset=raw.find(swapped_magic)
  if offset<0:raise RuntimeError('Native WP02 metrics not found in HWRAM')
  data=raw[offset:offset+size]
  values=struct.unpack('>'+str(size//4)+'I',b''.join(data[i:i+2][::-1] for i in range(0,size,2)))
  names=['ticks','renders','missed','max_commands','elapsed_us','done','ids_packed','state','vblanks','end_us']
  if size==68:names+=['bgm_refills','bgm_failures','bgm_max_service_us']
  m=dict(zip(names,values[4:]));m['ids']=list(values[10].to_bytes(4,'big'))
  return m,offset
 result['metrics'],result['state_offset']=metrics(compressed)
 raw=gzip.decompress(compressed)
 fault_magic=bytes.fromhex('575030324641554c54523032feedc0de')
 fault_magic=b''.join(fault_magic[i:i+2][::-1] for i in range(0,16,2))
 fault_offset=raw.find(fault_magic)
 if fault_offset>=0:
  data=raw[fault_offset:fault_offset+68]
  fields=struct.unpack('>17I',b''.join(data[i:i+2][::-1] for i in range(0,68,2)))[4:]
  result['fault']=dict(zip(['code','reason','raw_monitor','fill_bytes','refills','gap_us','elapsed_us','renders','missed','max_service_us','target','source_samples','source_offset'],fields))
 if first_state:result['metrics_before_restart'],_=metrics(first_state)
prefix.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
if proc.returncode or not states:raise SystemExit(1)
