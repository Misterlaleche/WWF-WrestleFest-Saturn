#!/usr/bin/env python3
"""Capture native Saturn disc execution through a Yabause libretro core.

Uses Yabause's HLE BIOS, interpreter SH2 and software VDP renderer. No BIOS ROM,
save state, CPU/RAM patch, screenshot substitution or game logic is supplied.
Input schedules use emulated frame numbers and ordinary libretro pad callbacks.
The MP4 runs at the core's reported emulated video rate; wall time is reported.
"""
import argparse
import ctypes as C
import hashlib
import json
import re
import subprocess
import time
import wave
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--core', type=Path, default=ROOT/'yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so')
p.add_argument('--cue', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--frames', type=int, default=600)
p.add_argument('--schedule', type=Path, help='JSON list: {frame, port, buttons:[up,down,left,right,a,b,c,x,y,z,l,r,start]}')
p.add_argument('--snapshot-every', type=int, default=60)
p.add_argument('--canvas', default='320x240', help='Fixed MP4 canvas WIDTHxHEIGHT; smaller native frames get black padding, never scaling/cropping')
p.add_argument('--no-video', action='store_true')
p.add_argument('--dump-memory', action='store_true')
p.add_argument('--probe-audio', action='store_true', help='Read-only diagnostic of local Yabause SoundRam and 68000 registers using the core ELF symbols')
p.add_argument('--watch', action='append', default=[], help='NAME:ADDRESS:SIZE, reads only; defaults SIZE=4')
p.add_argument('--watch-every', type=int, default=1)
p.add_argument('--wall-timeout', type=float, default=300)
a = p.parse_args()
canvas_match = re.fullmatch(r'([1-9][0-9]*)x([1-9][0-9]*)', a.canvas)
if not canvas_match:
    p.error('--canvas must be WIDTHxHEIGHT')
canvas_size = tuple(map(int, canvas_match.groups()))
if any(n % 2 for n in canvas_size):
    p.error('--canvas width and height must be even for yuv420p')
a.out.mkdir(parents=True, exist_ok=True)
(a.out/'system').mkdir(exist_ok=True)
(a.out/'saves').mkdir(exist_ok=True)

class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]
class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p), ('size', C.c_size_t), ('meta', C.c_char_p)]
class Info(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p), ('extensions', C.c_char_p), ('fullpath', C.c_bool), ('extract', C.c_bool)]
class Geometry(C.Structure):
    _fields_ = [('bw', C.c_uint), ('bh', C.c_uint), ('mw', C.c_uint), ('mh', C.c_uint), ('aspect', C.c_float)]
class Timing(C.Structure):
    _fields_ = [('fps', C.c_double), ('sample_rate', C.c_double)]
class AV(C.Structure):
    _fields_ = [('geometry', Geometry), ('timing', Timing)]
class MemoryDescriptor(C.Structure):
    _fields_ = [('flags', C.c_uint64), ('ptr', C.c_void_p), ('offset', C.c_size_t), ('start', C.c_size_t), ('select', C.c_size_t), ('disconnect', C.c_size_t), ('length', C.c_size_t), ('name', C.c_char_p)]
class MemoryMap(C.Structure):
    _fields_ = [('descriptors', C.POINTER(MemoryDescriptor)), ('count', C.c_uint)]
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
LOG = C.CFUNCTYPE(None, C.c_int, C.c_char_p)

system = str((a.out/'system').resolve()).encode()
saves = str((a.out/'saves').resolve()).encode()
options = {b'yabause_force_hle_bios': b'enabled', b'yabause_frameskip': b'disabled', b'yabause_addon_cartridge': b'none', b'yabause_multitap_port1': b'disabled', b'yabause_multitap_port2': b'disabled', b'yabause_numthreads': b'1'}
pixel = 2
latest = None
callbacks = 0
audio_frames = 0
memory = []
frame = 0
executed = 0
pad = [set(), set()]
env_calls = set()
wav = wave.open(str(a.out/'native-audio.wav'), 'wb')
wav.setnchannels(2)
wav.setsampwidth(2)
wav.setframerate(44100)

@LOG
def log(level, fmt):
    # The core logger is variadic. Preserve its format string without treating
    # missing Python arguments as data or attempting unsafe format evaluation.
    print('Yabause log format:', fmt.decode('utf-8', 'replace').rstrip(), flush=True)

@ENV
def environment(cmd, data):
    global pixel, memory
    env_calls.add(cmd)
    if cmd in (9, 31):
        C.cast(data, C.POINTER(C.c_char_p))[0] = system if cmd == 9 else saves
        return True
    if cmd == 3:
        C.cast(data, C.POINTER(C.c_bool))[0] = True
        return True
    if cmd == 10:
        pixel = C.cast(data, C.POINTER(C.c_uint))[0]
        return pixel in (0, 1, 2)
    if cmd == 15:
        v = C.cast(data, C.POINTER(Variable)).contents
        if v.key in options:
            v.value = options[v.key]
            return True
        return False
    if cmd == 17:
        C.cast(data, C.POINTER(C.c_bool))[0] = False
        return True
    if cmd in (52, 57):
        C.cast(data, C.POINTER(C.c_uint))[0] = 0
        return True
    if cmd == 27:
        C.cast(data, C.POINTER(C.c_void_p))[0] = C.cast(log, C.c_void_p).value
        return True
    if cmd == (36 | 0x10000):
        mm = C.cast(data, C.POINTER(MemoryMap)).contents
        memory = [dict(name=(m.name or b'ram').decode(), ptr=m.ptr, start=m.start, offset=m.offset, length=m.length, flags=m.flags) for m in mm.descriptors[:mm.count]]
        return True
    if cmd in (11, 16, 18, 35, 37, 44):
        return True
    return False

@VIDEO
def video(data, w, h, pitch):
    global latest, callbacks
    callbacks += 1
    if data and data != C.c_void_p(-1).value:
        latest = (C.string_at(data, pitch*h), w, h, pitch, pixel)

@AUDIO
def audio(left, right):
    pass

@BATCH
def batch(data, n):
    global audio_frames
    wav.writeframesraw(C.string_at(data, n*4))
    audio_frames += n
    return n

@POLL
def poll():
    pass

# Standard upstream libretro Saturn mapping (unchanged by our core fixes):
# Retro B/A/R -> Saturn A/B/C and Retro Y/X/L -> Saturn X/Y/Z.
buttons = {'up': 4, 'down': 5, 'left': 6, 'right': 7, 'a': 0, 'b': 8, 'c': 11, 'x': 1, 'y': 9, 'z': 10, 'l': 12, 'r': 13, 'start': 3}
@INPUT
def input_state(port, device, index, button):
    return int(port < 2 and device == 1 and button in pad[port])

def current_image():
    raw, w, h, pitch, fmt = latest
    mode = {0:'BGR;15', 1:'BGRX', 2:'BGR;16'}[fmt]
    return Image.frombytes('RGB', (w, h), raw, 'raw', mode, pitch)

def read_memory(address, size):
    for m in memory:
        if m['start'] <= address and address+size <= m['start']+m['length']:
            # WRAM and sound RAM are T2; VDP1 RAM is T1 (native byte order).
            off = address-m['start']+m['offset']
            return bytes(C.c_ubyte.from_address(m['ptr']+((off+i)^m.get('byte_xor', 1))).value for i in range(size))
    return None

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

core = C.CDLL(str(a.core.resolve()))
for name, kind, callback in [('environment', ENV, environment), ('video_refresh', VIDEO, video), ('audio_sample', AUDIO, audio), ('audio_sample_batch', BATCH, batch), ('input_poll', POLL, poll), ('input_state', INPUT, input_state)]:
    f = getattr(core, 'retro_set_'+name)
    f.argtypes = [kind]
    f(callback)
core.retro_get_system_info.argtypes = [C.POINTER(Info)]
info = Info()
core.retro_get_system_info(C.byref(info))
core.retro_init()
core.retro_set_controller_port_device.argtypes = [C.c_uint, C.c_uint]
core.retro_set_controller_port_device(0, 1)
core.retro_set_controller_port_device(1, 1)
core.retro_load_game.argtypes = [C.POINTER(Game)]
core.retro_load_game.restype = C.c_bool
path = str(a.cue.resolve()).encode()
assert core.retro_load_game(C.byref(Game(path, None, 0, None))), 'Yabause refused the disc'
core.retro_get_system_av_info.argtypes = [C.POINTER(AV)]
av = AV()
core.retro_get_system_av_info(C.byref(av))
audio_probe_samples = []
audio_probe_registers = None
audio_probe_get_registers = None
audio_probe_base = None
audio_probe_symbols = {}
sh2_probe = {}
if a.probe_audio:
    for line in subprocess.check_output(['nm', '-a', str(a.core)], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3:
            audio_probe_symbols[fields[2]] = int(fields[0], 16)
    audio_probe_base = C.cast(core.retro_api_version, C.c_void_p).value - audio_probe_symbols['retro_api_version']
    sound_ptr = C.c_void_p.from_address(audio_probe_base + audio_probe_symbols['SoundRam']).value
    assert sound_ptr, 'Yabause SoundRam is not initialized'
    memory.append(dict(name='SoundRam', ptr=sound_ptr, start=0x05a00000, offset=0, length=0x80000, flags=4))
    class M68KRegisters(C.Structure):
        _fields_ = [('d', C.c_uint32*8), ('a', C.c_uint32*8), ('sr', C.c_uint32), ('pc', C.c_uint32)]
    audio_probe_registers = M68KRegisters()
    audio_probe_get_registers = C.CFUNCTYPE(None, C.POINTER(M68KRegisters))(audio_probe_base + audio_probe_symbols['M68KGetRegisters'])
    vdp1_ptr = C.c_void_p.from_address(audio_probe_base + audio_probe_symbols['Vdp1Ram']).value
    memory.append(dict(name='Vdp1Ram', ptr=vdp1_ptr, start=0x05c00000, offset=0, length=0x80000, flags=4, byte_xor=0))
    class SH2Registers(C.Structure):
        _fields_ = [('r', C.c_uint32*16), ('sr', C.c_uint32), ('gbr', C.c_uint32), ('vbr', C.c_uint32), ('mach', C.c_uint32), ('macl', C.c_uint32), ('pr', C.c_uint32), ('pc', C.c_uint32)]
    sh2_get_registers = C.CFUNCTYPE(None, C.c_void_p, C.POINTER(SH2Registers))(audio_probe_base + audio_probe_symbols['SH2GetRegisters'])
    for cpu_name in ('MSH2', 'SSH2'):
        sh2_probe[cpu_name] = (C.c_void_p.from_address(audio_probe_base + audio_probe_symbols[cpu_name]).value, SH2Registers())
schedule = sorted(json.loads(a.schedule.read_text()) if a.schedule else [], key=lambda x:x['frame'])
event_at = 0
events = []
samples = []
watch = []
for entry in a.watch:
    parts = entry.split(':')
    watch.append((parts[0], int(parts[1], 0), int(parts[2], 0) if len(parts)>2 else 4))
geometry_changes = []
result = dict(backend=info.name.decode(), core_version=info.version.decode(), firmware='Yabause open source HLE BIOS', hardware_test=False, core_sha256=digest(a.core), cue_sha256=digest(a.cue), video_rate=av.timing.fps, sample_rate=av.timing.sample_rate, duration_basis='emulated frame count at reported video rate; not a hardware performance claim', frames_requested=a.frames, events=events, samples=samples, source_geometry_changes=geometry_changes, video_canvas=list(canvas_size), canvas_method='native pixels at top-left; black padding only; no resizing or cropping')
core_configuration = a.core.with_name(a.core.name + '.configuration.json')
if core_configuration.exists():
    result['core_configuration'] = json.loads(core_configuration.read_text())
if a.probe_audio:
    result['audio_probe'] = audio_probe_samples
    result['audio_probe_method'] = 'Read-only SoundRam/VDP1 pointers and M68KGetRegisters/SH2GetRegisters from core ELF symbols; no register/memory writes'
    result['scsp_engine_selection'] = C.c_int.from_address(audio_probe_base + audio_probe_symbols['use_new_scsp']).value
if a.cue.suffix.lower()=='.cue':
    result['disc_files'] = {n: digest(a.cue.parent/n) for n in re.findall(r'^FILE\s+"([^"]+)"', a.cue.read_text(), re.M)}
else:
    result['disc_files'] = {a.cue.name: digest(a.cue)}
encoder = None
written = 0
unique = set()
last_geometry = None
start = time.monotonic()
last_progress = start
try:
    for frame in range(a.frames):
        if time.monotonic()-start > a.wall_timeout:
            result['status'] = 'wall-timeout'
            break
        while event_at < len(schedule) and schedule[event_at]['frame'] <= frame:
            event = schedule[event_at]
            port = int(event.get('port', 0))
            pad[port] = {buttons[x] for x in event.get('buttons', [])}
            events.append(dict(event, applied_at_frame=frame))
            event_at += 1
        core.retro_run()
        executed += 1
        if audio_probe_get_registers and frame % a.watch_every == 0:
            audio_probe_get_registers(C.byref(audio_probe_registers))
            cpu_registers = {}
            for cpu_name, (cpu_pointer, registers) in sh2_probe.items():
                sh2_get_registers(cpu_pointer, C.byref(registers))
                cpu_registers[cpu_name] = dict(r=list(registers.r), sr=registers.sr, gbr=registers.gbr, vbr=registers.vbr, mach=registers.mach, macl=registers.macl, pr=registers.pr, pc=registers.pc)
            audio_probe_samples.append(dict(emulator_frame=frame,
                pc=audio_probe_registers.pc, sr=audio_probe_registers.sr,
                d=list(audio_probe_registers.d), a=list(audio_probe_registers.a),
                running=C.c_int.from_address(audio_probe_base + audio_probe_symbols['IsM68KRunning']).value,
                mail=read_memory(0x05a0c040, 32).hex(),
                dsp_clock=int.from_bytes(read_memory(0x05a0b010, 2), 'big'),
                music_ring=read_memory(0x05a0b020, 12).hex(), sh2=cpu_registers))
        if watch and frame % a.watch_every == 0:
            sample = {'emulator_frame': frame}
            for name, address, size in watch:
                value = read_memory(address, size)
                sample[name] = int.from_bytes(value, 'big') if value is not None and size <= 8 else value.hex() if value else None
            samples.append(sample)
        if latest:
            im = current_image()
            pixels = im.tobytes()
            unique.add(hashlib.sha256(pixels).hexdigest())
            if im.size != last_geometry:
                geometry_changes.append(dict(emulator_frame=frame, width=im.width, height=im.height))
                last_geometry = im.size
            if not a.no_video:
                if im.width > canvas_size[0] or im.height > canvas_size[1]:
                    raise RuntimeError(f'Native frame {im.width}x{im.height} exceeds --canvas {a.canvas}; choose a larger canvas to preserve all pixels')
                if im.size != canvas_size:
                    canvas = Image.new('RGB', canvas_size, (0, 0, 0))
                    canvas.paste(im, (0, 0))
                    encoded_pixels = canvas.tobytes()
                else:
                    encoded_pixels = pixels
                if encoder is None:
                    encoder = subprocess.Popen(['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pixel_format','rgb24','-video_size',a.canvas,'-framerate',str(av.timing.fps),'-i','pipe:0','-an','-c:v','libx264','-preset','veryfast','-crf','17','-pix_fmt','yuv420p',str(a.out/'native-video.mp4')], stdin=subprocess.PIPE)
                    result['video_dimensions'] = list(canvas_size)
                encoder.stdin.write(encoded_pixels)
                written += 1
            if a.snapshot_every and frame % a.snapshot_every == 0:
                im.save(a.out/f'frame-{frame:06d}.png')
        now = time.monotonic()
        if now-last_progress >= 5:
            print(json.dumps({'emulator_frame':frame, 'wall_seconds':round(now-start, 2), 'unique_video_frames':len(unique)}), flush=True)
            last_progress = now
    else:
        result['status'] = 'complete'
finally:
    if encoder:
        encoder.stdin.close()
        assert encoder.wait() == 0, 'FFmpeg failed'
    wav.close()
    result.update(frames_run=executed, video_callbacks=callbacks, encoded_frames=written, audio_samples_per_channel=audio_frames, unique_video_frames=len(unique), wall_seconds=time.monotonic()-start, emulated_seconds=executed/av.timing.fps, environment_commands=sorted(env_calls), memory_maps=[{k:v for k,v in m.items() if k!='ptr'} for m in memory])
    if latest:
        current_image().save(a.out/'final.png')
    if a.dump_memory:
        for m in memory:
            raw = C.string_at(m['ptr']+m['offset'], m['length'])
            (a.out/(m['name']+'-host-order.bin')).write_bytes(raw)
            swapped = bytearray(raw)
            if m.get('byte_xor', 1):
                swapped[::2], swapped[1::2] = raw[1::2], raw[::2]
            (a.out/(m['name']+'-saturn-order.bin')).write_bytes(swapped)
    (a.out/'report.json').write_text(json.dumps(result, indent=2)+'\n')
    core.retro_unload_game()
    core.retro_deinit()
if written and audio_frames:
    subprocess.run(['ffmpeg','-y','-loglevel','error','-i',str(a.out/'native-video.mp4'),'-i',str(a.out/'native-audio.wav'),'-map','0:v','-map','1:a','-c:v','copy','-c:a','aac','-b:a','192k',str(a.out/'native-capture.mp4')], check=True)
print(json.dumps({k:v for k,v in result.items() if k not in ('samples','events','audio_probe')}, indent=2), flush=True)
raise SystemExit(0 if result.get('status')=='complete' else 1)
