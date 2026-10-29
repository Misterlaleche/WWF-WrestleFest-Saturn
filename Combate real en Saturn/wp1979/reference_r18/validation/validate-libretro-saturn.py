#!/usr/bin/env python3
"""Diagnostic libretro runner: real Saturn pad callback, read-only WRAM.

This is a Beetle Saturn test, not standalone Mednafen or physical hardware.
No cheats, save states, CPU register writes or emulated RAM writes are used.
Video retains wall-clock duration by repeating frames when emulation is slow.
"""
import argparse, ctypes as C, hashlib, json, re, struct, subprocess, time
from pathlib import Path
from PIL import Image

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--core',type=Path,required=True);p.add_argument('--cue',type=Path,required=True)
p.add_argument('--program',type=Path,required=True);p.add_argument('--map',type=Path,required=True)
p.add_argument('--layout',type=Path,required=True);p.add_argument('--bios-dir',type=Path,required=True)
p.add_argument('--schedule',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
p.add_argument('--seconds',type=float,default=600);p.add_argument('--lead',type=int,default=2)
p.add_argument('--ffmpeg',default='ffmpeg');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
class Variable(C.Structure):_fields_=[('key',C.c_char_p),('value',C.c_char_p)]
class Info(C.Structure):_fields_=[('name',C.c_char_p),('version',C.c_char_p),('extensions',C.c_char_p),('fullpath',C.c_bool),('extract',C.c_bool)]
class Game(C.Structure):_fields_=[('path',C.c_char_p),('data',C.c_void_p),('size',C.c_size_t),('meta',C.c_char_p)]
class Geometry(C.Structure):_fields_=[('bw',C.c_uint),('bh',C.c_uint),('mw',C.c_uint),('mh',C.c_uint),('aspect',C.c_float)]
class Timing(C.Structure):_fields_=[('fps',C.c_double),('sample_rate',C.c_double)]
class AV(C.Structure):_fields_=[('geometry',Geometry),('timing',Timing)]
ENV=C.CFUNCTYPE(C.c_bool,C.c_uint,C.c_void_p);VIDEO=C.CFUNCTYPE(None,C.c_void_p,C.c_uint,C.c_uint,C.c_size_t)
AUDIO=C.CFUNCTYPE(None,C.c_int16,C.c_int16);BATCH=C.CFUNCTYPE(C.c_size_t,C.c_void_p,C.c_size_t)
POLL=C.CFUNCTYPE(None);INPUT=C.CFUNCTYPE(C.c_int16,C.c_uint,C.c_uint,C.c_uint,C.c_uint)
system=str(a.bios_dir.resolve()).encode();save=str(a.out.resolve()).encode();pixel=1;latest=None;mask=255
options={b'beetle_saturn_region':b'North America',b'beetle_saturn_cart':b'None'}
env_calls=set()
@ENV
def environment(cmd,data):
    global pixel
    env_calls.add(cmd)
    if cmd in (9,31):C.cast(data,C.POINTER(C.c_char_p))[0]=system if cmd==9 else save;return True
    if cmd==3:C.cast(data,C.POINTER(C.c_bool))[0]=True;return True
    if cmd==10:
        pixel=C.cast(data,C.POINTER(C.c_uint))[0];return pixel in (1,2)
    if cmd==15:
        var=C.cast(data,C.POINTER(Variable)).contents
        if var.key in options:var.value=options[var.key];return True
        return False
    if cmd==17:C.cast(data,C.POINTER(C.c_bool))[0]=False;return True
    if cmd==52:C.cast(data,C.POINTER(C.c_uint))[0]=0;return True
    return False
@VIDEO
def video(data,w,h,pitch):
    global latest
    if data and data!=C.c_void_p(-1).value:
        latest=(C.string_at(data,pitch*h),w,h,pitch,pixel)
@AUDIO
def audio(l,r):pass
@BATCH
def batch(data,frames):return frames
@POLL
def poll():pass
@INPUT
def input_state(port,device,index,button):
    if port!=0 or device!=1:return 0
    bits={7:1,6:2,5:4,4:8,0:16,8:32,11:64,3:128}
    bit=bits.get(button);return int(bit is not None and not(mask&bit))
core=C.CDLL(str(a.core.resolve()))
for name,kind,callback in [('environment',ENV,environment),('video_refresh',VIDEO,video),('audio_sample',AUDIO,audio),('audio_sample_batch',BATCH,batch),('input_poll',POLL,poll),('input_state',INPUT,input_state)]:
    fn=getattr(core,'retro_set_'+name);fn.argtypes=[kind];fn(callback)
core.retro_get_system_info.argtypes=[C.POINTER(Info)];info=Info();core.retro_get_system_info(C.byref(info))
core.retro_init();core.retro_set_controller_port_device.argtypes=[C.c_uint,C.c_uint];core.retro_set_controller_port_device(0,1)
core.retro_load_game.argtypes=[C.POINTER(Game)];core.retro_load_game.restype=C.c_bool
path=str(a.cue.resolve()).encode();game=Game(path,None,0,None)
assert core.retro_load_game(C.byref(game)),'Core refused CUE; inspect core log'
core.retro_get_memory_data.argtypes=[C.c_uint];core.retro_get_memory_data.restype=C.c_void_p
core.retro_get_memory_size.argtypes=[C.c_uint];core.retro_get_memory_size.restype=C.c_size_t
ram=core.retro_get_memory_data(2);ram_size=core.retro_get_memory_size(2);assert ram and ram_size>=0x200000
core.retro_get_system_av_info.argtypes=[C.POINTER(AV)];av=AV();core.retro_get_system_av_info(C.byref(av));fps=av.timing.fps
program=a.program.read_bytes();signature=bytearray(program[:4096]);signature[::2],signature[1::2]=program[1:4096:2],program[:4096:2]
mapping=a.map.read_text();names=('rt','gameplay','fatal_owner','fatal_dependency','fatal_frame','finished_frame','finished_result','finished_presentation','finished_count','finished_timer','finished_vblanks','finished_presentation_vblanks')
symbols={}
for name in names:
    match=re.search(r'\.bss\.'+name+r'(?:\.lto_priv\.\d+)?\s+0x([0-9a-f]+)\s',mapping);assert match,name;symbols[name]=int(match[1],16)
offsets=struct.unpack('>5I',a.layout.read_bytes());assert offsets[1]==0x10c
schedule=[tuple(map(int,line.split(','))) for line in a.schedule.read_text().splitlines() if line and not line.startswith('#')]
base=None;event_index=0;samples=[];events=[];last_frame=-1;first_pos=None;last_report=0;encoder=None;origin=None;written=0;last_pixels=None
cue_text=a.cue.read_text();disc_name=re.search(r'^FILE "([^"]+)" BINARY',cue_text,re.M)[1]
result={'method':'libretro joypad callback -> Beetle Saturn digital pad -> Yaul SMPC; WRAM reads only','core':info.name.decode(),'core_version':info.version.decode(),'core_sha256':hashlib.sha256(a.core.read_bytes()).hexdigest(),'program_sha256':hashlib.sha256(program).hexdigest(),'cue_sha256':hashlib.sha256(a.cue.read_bytes()).hexdigest(),'bin_sha256':hashlib.sha256((a.cue.parent/disc_name).read_bytes()).hexdigest(),'schedule_sha256':hashlib.sha256(a.schedule.read_bytes()).hexdigest(),'hardware_test':False,'fps':fps,'lead_frames':a.lead,'emulated_ram_bytes':ram_size,'samples':samples,'events':events}
def read(address,n):
    raw=C.string_at(base+address-0x06000000,n);return b''.join(raw[i:i+2][::-1] for i in range(0,n,2))
def u32(address):return int.from_bytes(read(address,4),'big')
def image():
    raw,w,h,pitch,fmt=latest
    if fmt==1:return Image.frombytes('RGB',(w,h),raw,'raw','BGRX',pitch)
    return Image.frombytes('RGB',(w,h),raw,'raw','BGR;16',pitch)
start=time.monotonic();ticks=0
try:
    while time.monotonic()-start<a.seconds:
        if base is not None:
            frame=u32(symbols['rt']+24)
            while event_index<len(schedule) and frame>=max(0,schedule[event_index][0]-a.lead):
                target,mask=schedule[event_index];events.append({'target_frame':target,'observed_frame':frame,'mask':mask});event_index+=1
        core.retro_run();ticks+=1
        if base is None:
            raw=C.string_at(ram,ram_size);at=raw.find(signature)
            if at>=0:base=ram+at-0x4000;result['signature_offset']=at;print('Exact 4096-byte WRAM signature found',at,flush=True)
            else:continue
        frame=u32(symbols['rt']+24)
        if frame!=last_frame:
            actor=read(symbols['rt']+offsets[0],0x10c);word=lambda at:int.from_bytes(actor[at:at+2],'big')
            byte_addr=symbols['gameplay']+offsets[4];actual=C.string_at(base+((byte_addr^1)-0x06000000),1)[0]
            samples.append({'frame':frame,'input':actual,'position':[word(6),word(10)],'state':word(0x20),'selector':word(0x60),'stamina':word(0x66),'held':word(0xa6),'fresh':word(0xa2),'direction':word(0xa8)})
            last_frame=frame
        if time.monotonic()-last_report>5:
            print(json.dumps({'seconds':round(time.monotonic()-start,2),'frame':frame,'fatal':u32(symbols['fatal_owner'])}),flush=True);last_report=time.monotonic()
        if latest and frame>0:
            if encoder is None:
                origin=time.monotonic();encoder=subprocess.Popen([a.ffmpeg,'-y','-loglevel','error','-f','rawvideo','-pixel_format','rgb24','-video_size','640x480','-framerate',str(fps),'-i','pipe:0','-c:v','libx264','-preset','veryfast','-crf','20','-pix_fmt','yuv420p',str(a.out/'combat.mp4')],stdin=subprocess.PIPE)
            pixels=image().resize((640,480),Image.Resampling.NEAREST).tobytes()
            target=max(written+1,round((time.monotonic()-origin)*fps))
            for _ in range(max(0,target-written-1)):encoder.stdin.write(last_pixels or pixels)
            encoder.stdin.write(pixels);written=target;last_pixels=pixels
        fatal=u32(symbols['fatal_owner']);finished=u32(symbols['finished_frame'])
        if fatal:
            result.update(status='blocked',fatal_owner=fatal,fatal_dependency=u32(symbols['fatal_dependency']),fatal_frame=u32(symbols['fatal_frame']))
            (a.out/'workram-at-block.bin').write_bytes(C.string_at(ram,ram_size))
            result['actors_at_block']=[read(symbols['rt']+offsets[0]+i*offsets[1],offsets[1]).hex() for i in range(9)]
            result['shadow_d_at_block']=[u32(symbols['rt']+offsets[3]+i*4) for i in range(8)]
            break
        if finished and u32(symbols['finished_presentation']):
            result.update(status='complete',finished_frame=finished,finished_result=u32(symbols['finished_result']),finished_presentation=1,fatal_owner=0)
            for name in ('finished_count','finished_timer','finished_vblanks','finished_presentation_vblanks'):result[name]=u32(symbols[name])
            break
        # A ceiling only. Slow frames are retained in the video, never sped up.
        delay=start+ticks/fps-time.monotonic()
        if delay>0:time.sleep(delay)
    else:result.update(status='timeout',last_frame=last_frame)
finally:
    result.update(elapsed_seconds=time.monotonic()-start,video_frames=written,environment_commands=sorted(env_calls),observed_action_frames=sum(bool(s['held'] or s['fresh']) for s in samples),observed_direction_frames=sum(bool(s['direction']) for s in samples))
    if encoder:encoder.stdin.close();assert encoder.wait()==0
    if latest:image().save(a.out/'final.png')
    (a.out/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    core.retro_unload_game();core.retro_deinit()
print(json.dumps({k:v for k,v in result.items() if k not in ('samples','events')},indent=2),flush=True)
raise SystemExit(0 if result.get('status')=='complete' else 1)
