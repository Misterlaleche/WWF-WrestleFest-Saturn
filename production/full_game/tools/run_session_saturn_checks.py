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
p.add_argument('--attract',action='store_true');p.add_argument('--combat',action='store_true');p.add_argument('--through-match',action='store_true');p.add_argument('--royal',action='store_true');p.add_argument('--sdk',default='/sdk');p.add_argument('--bios',default='/bios/mpr-17933.bin')
p.add_argument('--expect-video-error',type=int,choices=[4],help='Negative FRONT.TEX CRC test only')
a=p.parse_args();a.through_match=a.through_match or a.combat or a.attract;a.out.mkdir(parents=True,exist_ok=False)
shutil.copy2(__file__,a.out/'harness.py')
os.environ['WP1980_HOST_TOOLS_ROOT']='/';os.environ['WP1980_MEDNAFEN_RUNTIME_ROOT']='/usr'
sys.path.insert(0,str(a.reference_tools.resolve()))
import run_mednafen as ref
from observer import ReadOnlyObserver,NotReady,load_symbols,build_watches
import observer as observer_module
observer_module.GUARD_NAMES += ("wf_logic_sequence","wf_snapshot_sequence")
version=subprocess.run(['/usr/games/mednafen','--help'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True).stdout
assert '1.32.1' in version
program_path=a.out/'program.bin'
subprocess.run([a.sdk+'/bin/sh2eb-elf-objcopy','-O','binary',str(a.elf),str(program_path)],check=True)
program=program_path.read_bytes();assert ref.disc_program(a.cue)==program
shutil.copy2(a.elf,a.out/'program.elf')
symbols=load_symbols(a.elf,a.sdk)
optional=[n for n in ('wf_load_fields','wf_load_depth','wf_clock_origin','wf_video_atlas','wf_combat_stage','wf_combat_frame','wf_combat_ack','wf_combat_error','wf_combat_dependency','wf_combat_sprites','wf_combat_rng','wf_combat_audio_count','wf_combat_audio_last','wf_combat_work','wf_combat_wait','wf_handoff_rng','wf_handoff_frame') if n in symbols]
handoff=['wf_handoff_id%d:wf_handoff_ids+%d'%(i,i*4) for i in range(9)] if 'wf_handoff_ids' in symbols else []
watches=build_watches(symbols,defaults=False,extra_specs=[*optional,*handoff,
 'video_clock:wf_fields','game_frame:wf_logic_ticks',
 *['wf_'+n for n in ('session_phase','session_pc','session_owner','session_generation','credits','paid','state','slot','partner','royal','scroll','inputs','irq2_count','irq3_count','board_line','snapshot_sequence','cold_rng','error','cart_id','cart_bytes','cart_valid','logic_fields','logic_ticks','logic_pending','logic_sequence','cd_stage','cd_error','cd_windows','cd_sectors','video_error','video_commits','video_sprites','video_tile','render_work','render_wait')]])
settings={'ss.cart':a.cart,'ss.region_autodetect':'0','ss.region_default':'na','ss.dbg_cem':'full','ss.bios_na_eu':a.bios,
 'video.driver':'softfb','video.frameskip':'0','video.blit_timesync':'0','nothrottle':'0','ffspeed':'1','fftoggle':'0',
 'ss.correct_aspect':'0','ss.h_overscan':'1','ss.h_blend':'0','ss.slstart':'0','ss.slend':'239','ss.xscale':'2','ss.yscale':'2',
 'ss.input.port1':'gamepad','ss.input.port2':'gamepad','ss.input.sport1.multitap':'0','ss.input.sport2.multitap':'0',
 'sound':'1','sound.driver':'dummy','sound.rate':'44100','srwautoenable':'0','input.ckdelay':'0',
 'qtrecord.w_double_threshold':'0','qtrecord.h_double_threshold':'0','qtrecord.vcodec':'cscd',
 'filesys.path_snap':str(a.out/'snapshots'),'command.take_snapshot':'keyboard 0x0 19'}
settings.update({'ss.input.port1.gamepad.'+k:'keyboard 0x0 '+str(v) for k,v in ref.SCANCODES.items()})
command=['/usr/games/mednafen']
for k,v in settings.items():command+=['-'+k,v]
command+=['-qtrecord',str(a.out/'native.mov'),str(a.cue)]
report={'status':'RUNNING','cartridge':a.cart,'royal':a.royal,'attract':a.attract,'settings':settings,'command':command,
 'runner_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 'program_sha256':hashlib.sha256(program).hexdigest(),'bios_sha256':ref.sha(a.bios),'emulator_sha256':ref.sha('/usr/games/mednafen'),
 'source_tools':{str(x.name):ref.sha(x) for x in (a.reference_tools/'observer.py',a.reference_tools/'run_mednafen.py',a.reference_tools/'launch_identity.py')},
 'milestones':[],'claims':'Native session with original scene owners and first combat frames when requested; no complete-game or hardware claim'}
ref.write_json(a.out/'report.json',report)
xs=xl=process=observer=None
started=time.monotonic();last=None;cd_baseline=None;clock_samples={}
try:
 xs,xl,display,_=ref.start_xvfb(a.out)
 env=os.environ.copy();env.update(DISPLAY=display,MEDNAFEN_HOME=str(a.out/'home'),SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_RUNTIME_DIR=str(a.out/'xdg'))
 (a.out/'home').mkdir();(a.out/'xdg').mkdir(mode=0o700);(a.out/'snapshots').mkdir()
 log=(a.out/'emulator.log').open('wb');samples=gzip.open(a.out/'samples.jsonl.gz','wt')
 process=subprocess.Popen([sys.executable,str(a.reference_tools/'launch_identity.py'),str(a.out/'identity.json'),*command],env=env,stdout=log,stderr=subprocess.STDOUT)
 def sample():
  global observer,last,cd_baseline
  if time.monotonic()-started>300:raise TimeoutError('platform route timeout300s')
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
   if s['wf_error']:raise RuntimeError('native error: '+json.dumps(s))
   if s['boundary_stable'] and not s['wf_logic_sequence']&1 and s['wf_logic_ticks']:
    f,t,d=(s['wf_logic_fields'],s['wf_logic_ticks'],s['wf_logic_pending'])
    if t+d!=f*37609//39168:raise RuntimeError('native clock lost or added debt')
    clock_samples[s['wf_logic_sequence']]={k:s[k] for k in ('wf_state','wf_logic_fields','wf_logic_ticks','wf_logic_pending','wf_render_work','wf_render_wait')}
    if cd_baseline is None:cd_baseline=s['wf_cd_sectors']
    if s['wf_cd_sectors']!=cd_baseline and not a.through_match:raise RuntimeError('CD read during live session')
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
  s=until(lambda s:s['boundary_stable']);(a.out/name).mkdir();observer.dump_hwram(a.out/name)
  snaps=a.out/'snapshots';before=set(snaps.glob('*.png'))
  ref.keyboard_command(display,'keydown','--delay','0','p');fields(8)
  ref.keyboard_command(display,'keyup','--delay','0','p');fields(8)
  fresh=set(snaps.glob('*.png'))-before
  if len(fresh)!=1:raise RuntimeError('expected one native screenshot at '+name)
  shutil.copy2(fresh.pop(),a.out/name/'native.png')
  report['milestones'].append({'name':name,'sample':s});print(name,s['wf_state'],s['wf_slot'],flush=True);return s
 def press(key):
  ref.keyboard_command(display,'keydown','--delay','0',ref.KEYS[key]);fields(8)
  ref.keyboard_command(display,'keyup','--delay','0',ref.KEYS[key]);fields(8)
 until(lambda s:s['wf_irq2_count']>30)
 if a.expect_video_error:raise RuntimeError('expected asset rejection, but session started')
 ref.focus_window(display);fields(8);mark('boot')
 if a.attract:
  until(lambda s:s['wf_session_phase']==2);s=mark('attract-f8c')
 else:
  press('c');until(lambda s:s['wf_credits']>0);mark('credit')
  if a.through_match:
   ref.keyboard_command(display,'keydown','--delay','0',ref.KEYS['start']);until(lambda s:s['wf_state']==4)
   ref.keyboard_command(display,'keyup','--delay','0',ref.KEYS['start']);fields(15);mark('mode')
  else:
   until(lambda s:s['wf_state']==2);press('start');until(lambda s:s['wf_state']==4);fields(15);mark('mode')
  if a.royal:press('left');assert mark('royal')['wf_royal']==1
  press('a');until(lambda s:s['wf_state']==5);fields(20);mark('roster')
  if a.royal:
   for expected in (1,2,3,4):press('right');until(lambda s:s['wf_slot']==expected);fields(35)
   press('down');until(lambda s:s['wf_slot']==9);fields(20);mark('crush')
   press('a')
  else:
   press('a');fields(20);mark('partner')
   press('right');until(lambda s:s['wf_partner']==1);fields(10);press('a')
  until(lambda s:s['wf_session_phase']==1 and s['wf_paid']==0x8000);fields(10);s=mark('selected-ac0')
  if a.through_match:
   until(lambda s:s.get('wf_video_atlas')==1 and s['wf_video_sprites']>0);mark('entry')
   if not a.royal:
    until(lambda s:s.get('wf_video_atlas')==2 and s['wf_video_sprites']>0);mark('presenter')
   until(lambda s:s['wf_session_phase']==2);s=mark('handoff-f8c')
 if a.combat:
  until(lambda s:s.get('wf_combat_frame',0)>=10);mark('combat-first')
  ref.keyboard_command(display,'keydown','--delay','0',ref.KEYS['right']);fields(30)
  ref.keyboard_command(display,'keyup','--delay','0',ref.KEYS['right']);press('a')
  until(lambda s:s.get('wf_combat_frame',0)>=180);s=mark('combat-180')
  assert s['wf_combat_ack']==s['wf_combat_frame'] and s['wf_combat_sprites']>0
 assert s['wf_video_error']==0
 want={'none':(0,0,0),'extram1':(0x5a,0x100000,0),'extram4':(0x5c,0x400000,1)}[a.cart]
 assert (s['wf_cart_id'],s['wf_cart_bytes'],s['wf_cart_valid'])==want
 observer.verify_signature();observer.dump_hwram(a.out)
 report.update(status='PASS',final=s)
except BaseException as e:
 if a.expect_video_error and isinstance(e,RuntimeError) and str(e).startswith('native error: ') and last and last['wf_error']==30 and last['wf_video_error']==a.expect_video_error and last['wf_logic_ticks']==0 and last['wf_video_commits']==0:
  observer.verify_signature();observer.dump_hwram(a.out)
  report.update(status='PASS',expected_failure=True,error=repr(e),final=last)
 else:
  report.update(status='FAIL',error=repr(e),final=last);raise
finally:
 if observer is not None:observer.close()
 ref.stop_process(process);ref.stop_process(xs)
 if xl:xl.close()
 if 'samples' in globals():samples.close()
 if 'log' in globals():log.close()
 report['clock_publications']=list(clock_samples.values());report['cd_sectors_during_session']=cd_baseline
 ref.write_json(a.out/'report.json',report)
