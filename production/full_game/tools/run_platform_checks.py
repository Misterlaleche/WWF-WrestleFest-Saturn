#!/usr/bin/env python3
"""Real controller path + read-only native counters on a frozen universal disc.
Requires the supplied HW1 observer/tools and Mednafen1.32.1 Linux environment.
"""
import argparse,gzip,hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-tools',type=Path,required=True)
p.add_argument('--cue',type=Path,required=True);p.add_argument('--elf',type=Path,required=True)
p.add_argument('--out',type=Path,required=True);p.add_argument('--cart',choices=['none','extram1','extram4'],required=True)
p.add_argument('--expect-asset-error',action='store_true');p.add_argument('--confirm-during-scroll',action='store_true');p.add_argument('--royal',action='store_true');p.add_argument('--sdk',default='/sdk');p.add_argument('--bios',default='/bios/mpr-17933.bin')
a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False)
os.environ['WP1980_HOST_TOOLS_ROOT']='/';os.environ['WP1980_MEDNAFEN_RUNTIME_ROOT']='/usr'
sys.path.insert(0,str(a.reference_tools.resolve()))
import run_mednafen as ref
from observer import ReadOnlyObserver,NotReady,load_symbols,build_watches
import observer as observer_module
from platform_evidence import ClockEvidence
observer_module.GUARD_NAMES += ("wf_logic_sequence","wf_state")
version=subprocess.run(['/usr/games/mednafen','--help'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True).stdout
assert '1.32.1' in version
program_path=a.out/'program.bin'
subprocess.run([a.sdk+'/bin/sh2eb-elf-objcopy','-O','binary',str(a.elf),str(program_path)],check=True)
program=program_path.read_bytes();assert ref.disc_program(a.cue)==program
shutil.copy2(a.elf,a.out/'program.elf')
watches=build_watches(load_symbols(a.elf,a.sdk),defaults=False,extra_specs=[
 'video_clock:wf_fields','game_frame:wf_logic_ticks','resource_pointers:wf_frontend_resources:28',
 *['wf_'+s for s in ('state','slot','page','royal','scroll','error','cart_id','cart_bytes','cart_valid','logic_fields','logic_ticks','logic_pending','logic_sequence','pad_bits','cd_stage','cd_error','cd_windows','cd_sectors','assets_stage','assets_error','assets_index','assets_used','selected_identity')]])
settings={'ss.cart':a.cart,'ss.region_autodetect':'0','ss.region_default':'na','ss.dbg_cem':'full','ss.bios_na_eu':a.bios,
 'video.driver':'softfb','video.frameskip':'0','video.blit_timesync':'0','nothrottle':'0','ffspeed':'1','fftoggle':'0',
 'ss.correct_aspect':'0','ss.h_overscan':'1','ss.h_blend':'0','ss.slstart':'0','ss.slend':'239','ss.xscale':'2','ss.yscale':'2',
 'ss.input.port1':'gamepad','ss.input.port2':'gamepad','ss.input.sport1.multitap':'0','ss.input.sport2.multitap':'0',
 'sound':'1','sound.driver':'dummy','sound.rate':'44100','srwautoenable':'0','input.ckdelay':'0',
 'qtrecord.w_double_threshold':'0','qtrecord.h_double_threshold':'0','qtrecord.vcodec':'cscd'}
settings.update({'ss.input.port1.gamepad.'+k:'keyboard 0x0 '+str(v) for k,v in ref.SCANCODES.items()})
command=['/usr/games/mednafen']
for k,v in settings.items():command+=['-'+k,v]
command+=['-qtrecord',str(a.out/'native.mov'),str(a.cue)]
report={'status':'RUNNING','cartridge':a.cart,'royal':a.royal,'confirm_during_scroll':a.confirm_during_scroll,'expect_asset_error':a.expect_asset_error,'settings':settings,'command':command,
 'program_sha256':hashlib.sha256(program).hexdigest(),'bios_sha256':ref.sha(a.bios),'emulator_sha256':ref.sha('/usr/games/mednafen'),
 'source_tools':{str(x.name):ref.sha(x) for x in (a.reference_tools/'observer.py',a.reference_tools/'run_mednafen.py',a.reference_tools/'launch_identity.py')},
 'milestones':[],'claims':'Platform/frontend integration only; no combat/full game/hardware claim'}
ref.write_json(a.out/'report.json',report)
xs=xl=process=observer=None
started=time.monotonic();last=None;clock_evidence=ClockEvidence();cd_baseline=None
try:
 xs,xl,display,_=ref.start_xvfb(a.out)
 env=os.environ.copy();env.update(DISPLAY=display,MEDNAFEN_HOME=str(a.out/'home'),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_RUNTIME_DIR=str(a.out/'xdg'))
 (a.out/'home').mkdir();(a.out/'xdg').mkdir(mode=0o700)
 log=(a.out/'emulator.log').open('wb');samples=gzip.open(a.out/'samples.jsonl.gz','wt')
 process=subprocess.Popen([sys.executable,str(a.reference_tools/'launch_identity.py'),str(a.out/'identity.json'),*command],env=env,stdout=log,stderr=subprocess.STDOUT)
 def sample():
  global observer,last,cd_baseline
  if time.monotonic()-started>120:raise TimeoutError('platform route timeout120s')
  if process.poll() is not None:raise RuntimeError('emulator exited'+str(process.returncode))
  if observer is None:
   try:
    identity=json.loads((a.out/'identity.json').read_text())
    if ref.sha('/proc/'+str(identity['proc_pid'])+'/exe')==report['emulator_sha256']:
     observer=ReadOnlyObserver(process.pid,program,proc_pid=identity['proc_pid'])
   except FileNotFoundError:pass
  if observer is not None:
   if not observer.located:
    try:observer.locate();ref.write_json(a.out/'memory-identity.json',observer.identity())
    except NotReady:return None
   s=observer.sample(watches);samples.write(json.dumps(s)+'\n');last=s
   if s['wf_error'] and not a.expect_asset_error:raise RuntimeError('native error'+str(s['wf_error']))
   if s['wf_state'] in (1,2,3,4,5):
    if cd_baseline is None:cd_baseline=s['wf_cd_sectors']
    if s['wf_cd_sectors']!=cd_baseline:raise RuntimeError('CD read during frontend')
   clock_evidence.add(s)
   return s
  return None
 def until(predicate,timeout=120):
  limit=time.monotonic()+timeout
  while True:
   if time.monotonic()>limit:raise TimeoutError('expected native transition did not occur')
   s=sample()
   if s is not None and predicate(s):return s
   time.sleep(.01)
 def fields(n):
  s=until(lambda s:True);end=s['video_clock']+n
  return until(lambda s:s['video_clock']>=end)
 def mark(name):
  s=until(lambda s:s['boundary_stable']);report['milestones'].append({'name':name,'sample':s});print(name,s['wf_state'],s['wf_slot'],flush=True);return s
 def press(key):
  ref.keyboard_command(display,'keydown','--delay','0',ref.KEYS[key]);fields(8)
  ref.keyboard_command(display,'keyup','--delay','0',ref.KEYS[key]);fields(8)
 if a.expect_asset_error:
  until(lambda s:s['wf_assets_stage']==3);fields(10);s=mark('asset-rejected')
  assert s['wf_error']==21 and s['wf_assets_error']==5 and s['wf_assets_index']==6
  assert s['wf_state']==0 and s['resource_pointers']=='0'*56
 else:
  until(lambda s:s['wf_state']==1);ref.focus_window(display);fields(140);s=mark('mode')
  want={'none':(0,0,0),'extram1':(0x5a,0x100000,0),'extram4':(0x5c,0x400000,1)}[a.cart]
  assert (s['wf_cart_id'],s['wf_cart_bytes'],s['wf_cart_valid'])==want
  if a.royal:press('left');assert mark('royal')['wf_royal']==1
  press('a');until(lambda s:s['wf_state']==3);mark('roster')
  for expected in (1,2,3):
   press('right')
   if expected==3 and a.confirm_during_scroll:
    assert last['wf_state']==4
    ref.keyboard_command(display,'keydown','--delay','0',ref.KEYS['a'])
    until(lambda s:s['wf_state']==5,timeout=5)
    ref.keyboard_command(display,'keyup','--delay','0',ref.KEYS['a'])
   else:until(lambda s:s['wf_state']==3)
   s=mark('slot'+str(expected));assert s['wf_slot']==expected
  assert s['wf_page']==1 and s['wf_scroll']==528
  if a.confirm_during_scroll:
   fields(60);s=mark('confirmed-from-scroll');assert s['wf_slot']==3 and s['wf_state']==5
  else:
   for expected in (2,1):
    press('left');until(lambda s:s['wf_state']==3);s=mark('return'+str(expected));assert s['wf_slot']==expected
   assert s['wf_page']==0 and s['wf_scroll']==320
   press('a');until(lambda s:s['wf_state']==5);fields(60);s=mark('confirmed')
   assert s['wf_slot']==1 and s['wf_royal']==int(a.royal)
  assert clock_evidence.count>100
  assert s['wf_assets_stage']==2 and s['wf_assets_error']==0 and s['wf_assets_used']<=131072
  assert s['wf_selected_identity']==(8 if a.confirm_during_scroll else 7)
 observer.verify_signature();observer.dump_hwram(a.out)
 report.update(status='PASS',clock_distinct_active_publications=clock_evidence.count,max_observed_pending=clock_evidence.max_pending,cd_sectors_before_selection=cd_baseline,final=s)
except BaseException as e:
 report.update(status='FAIL',error=repr(e),final=last);raise
finally:
 if observer is not None:observer.close()
 ref.stop_process(process);ref.stop_process(xs)
 if xl:xl.close()
 if 'samples' in globals():samples.close()
 if 'log' in globals():log.close()
 ref.write_json(a.out/'report.json',report)
