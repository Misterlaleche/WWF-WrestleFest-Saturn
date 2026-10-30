#!/usr/bin/env python3
"""Record a frozen Saturn disc in standalone Mednafen, observing RAM read-only.

No debugger, save state, memory/register write, fast-forward or video retiming
is used. Keyboard actions go through an isolated X11 display and a normal
emulated Saturn gamepad. The QuickTime file is Mednafen's own video+audio.
"""
from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import select
import shutil
import signal
import subprocess
import sys
import time

from observer import ReadOnlyObserver, NotReady, load_symbols, build_watches, find_tool

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
DEPS = Path('/workspace/scratch/8e35fd0cc989/wp02_r02/deps')
HOST = Path(os.environ.get('WP1980_HOST_TOOLS_ROOT', '/workspace/scratch/1e822aeaeac7/wp1979/tooling/host'))
RUNTIME = Path(os.environ.get('WP1980_MEDNAFEN_RUNTIME_ROOT', str(DEPS / 'sysroot')))
SDK = Path('/workspace/scratch/1e822aeaeac7/wp1979/tooling/sh2eb-elf')
BIOS = Path('/workspace/scratch/8e35fd0cc989/wp02_r02/firmware_import/Audio saturn/mpr-17933.bin')
KEYS = {'left': 'a', 'right': 'd', 'up': 'w', 'down': 's',
        'a': 'KP_1', 'b': 'KP_2', 'c': 'KP_3', 'start': 'Return'}
SCANCODES = {'left': 4, 'right': 7, 'up': 26, 'down': 22,
             'a': 89, 'b': 90, 'c': 91, 'start': 40}

# Reuse the already-tested read-only ISO9660/ELF identity gate. Freeze this
# dependency with each capture; no Beetle emulator is loaded or invoked.
_spec = importlib.util.spec_from_file_location('disc_identity', HERE.parent / 'beetle/run_beetle.py')
_identity = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_identity)
sha, disc_tracks, disc_program = _identity.sha, _identity.disc_tracks, _identity.disc_program


def write_json(path, value):
    path = Path(path)
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n')
    temporary.replace(path)


def env_for(prefix):
    result = os.environ.copy()
    result['LD_LIBRARY_PATH'] = str(prefix / 'usr/lib/x86_64-linux-gnu') + ':' + str(prefix / 'lib/x86_64-linux-gnu')
    return result


def probe_media(path):
    return json.loads(subprocess.check_output([
        'ffprobe', '-v', 'error', '-show_streams', '-show_format', '-of', 'json', str(path)], text=True))


def stop_process(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=10)


def start_xvfb(output):
    read_fd, write_fd = os.pipe()
    log = (output / 'xvfb.log').open('wb')
    command = [str(HOST / 'usr/bin/Xvfb'), '-displayfd', str(write_fd),
               '-screen', '0', '1280x720x24', '-nolisten', 'tcp', '-ac']
    environment = env_for(HOST)
    helper = None
    # The supplied Xvfb has a compiled /usr/bin helper directory, while its
    # xkbcomp dependency is bundled read-only beside it. Scope the audited
    # path adapter to this exact host X server; it never reaches Mednafen.
    if not Path('/usr/bin/xkbcomp').exists() and HOST == Path('/workspace/scratch/1e822aeaeac7/wp1979/tooling/host'):
        if environment.get('LD_PRELOAD'):
            raise RuntimeError('Refusing to combine the Xvfb adapter with an existing preload')
        helper = output / 'libxvfb_xkb_path.so'
        shutil.copy2(HERE / helper.name, helper)
        shutil.copy2(HERE / 'xvfb_xkb_path.c', output / 'xvfb_xkb_path.c')
        environment['LD_PRELOAD'] = str(helper)
    write_json(output / 'xvfb-environment.json', dict(
        library_path=environment.get('LD_LIBRARY_PATH'), preload=str(helper) if helper else None,
        preload_sha256=sha(helper) if helper else None, scope='Xvfb host helper only; not supplied to Mednafen'))
    process = subprocess.Popen(command, env=environment, pass_fds=(write_fd,),
                               stdout=log, stderr=subprocess.STDOUT)
    os.close(write_fd)
    try:
        if not select.select([read_fd], [], [], 8)[0]:
            raise RuntimeError('Xvfb did not report a display; inspect xvfb.log')
        number = os.read(read_fd, 64).decode().strip()
        if not number.isdecimal() or process.poll() is not None:
            raise RuntimeError('Xvfb could not create its isolated local display')
        return process, log, ':' + number, command
    except BaseException:
        stop_process(process)
        log.close()
        raise
    finally:
        os.close(read_fd)


def keyboard_command(display, *args, check=True):
    env = env_for(HOST)
    env['DISPLAY'] = display
    return subprocess.run([str(HOST / 'usr/bin/xdotool'), *args], env=env,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=10, check=check)


def focus_window(display):
    # This X server was created solely for this emulator. The supplied SDL
    # window has WM_CLASS=mednafen but no WM_NAME, so title matching fails.
    result = keyboard_command(display, 'search', '--onlyvisible', '--class', '^mednafen$', check=False)
    windows = result.stdout.split()
    if len(windows) != 1:
        raise RuntimeError(f'Expected one Mednafen window, found {windows}; search rc={result.returncode}, stderr={result.stderr.strip()}')
    keyboard_command(display, 'windowfocus', '--sync', windows[0])
    return windows[0]


def action_plan(scenario, extra):
    actions = []
    if scenario == 'movement-reset':
        for name, first, last in [('left', 4, 31), ('right', 44, 71),
                                  ('up', 84, 107), ('down', 120, 143),
                                  ('a', 156, 167), ('b', 192, 203), ('c', 228, 239)]:
            actions.append(dict(button=name, first=first, last=last))
        actions += [dict(button=name, first=276, last=287) for name in ('a', 'b', 'c', 'start')]
    elif scenario == 'reset-first':
        actions = [dict(button=name, first=4, last=15) for name in ('a', 'b', 'c', 'start')]
    for text in extra:
        name, first, last = text.split(':')
        if name not in KEYS or not 0 <= int(first) <= int(last):
            raise ValueError('Press requires button:first:last, inclusive nonnegative fields')
        actions.append(dict(button=name, first=int(first), last=int(last)))
    return actions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('disc', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--bios', type=Path, default=BIOS)
    parser.add_argument('--sdk', type=Path, default=SDK)
    parser.add_argument('--mednafen', type=Path, default=DEPS / 'mednafen132/usr/games/mednafen')
    parser.add_argument('--cartridge', choices=['None', '4MiB'], default='None')
    parser.add_argument('--cache', choices=['full', 'data', 'auto'], default='full')
    parser.add_argument('--actors-layout', type=Path, default=PROJECT / 'verification/actors-layout.bin')
    parser.add_argument('--watch', action='append', default=[])
    parser.add_argument('--scenario', choices=['demo', 'movement-reset', 'reset-first'], default='demo')
    parser.add_argument('--press', action='append', default=[])
    parser.add_argument('--stop-game-frame', type=int, help='Smoke test only: stop at this native game update')
    parser.add_argument('--run-fields', type=int, help='Stop this many native fields after first started flag')
    parser.add_argument('--post-fields', type=int, default=240, help='Fields after completed result presentation')
    parser.add_argument('--wall-timeout', type=float, default=900)
    parser.add_argument('--poll-ms', type=float, default=2)
    parser.add_argument('--no-mp4', action='store_true')
    parser.add_argument('--no-movie', action='store_true', help='Observe native counters without qtrecord or video encoding')
    parser.add_argument('--offscreen', action='store_true', help='Internal MOV recording with SDL dummy video; no controller events')
    args = parser.parse_args()
    if args.wall_timeout <= 0 or not 0.5 <= args.poll_ms <= 100 or args.post_fields < 0:
        parser.error('Invalid timeout, polling interval or post-fields')
    actions = action_plan(args.scenario, args.press)
    if actions and args.run_fields is None:
        parser.error('Controller scenarios require --run-fields (normally2300)')
    if actions and args.offscreen:
        parser.error('Controller tests require the isolated X11 display')
    output = args.output.resolve()
    if output.exists():
        parser.error('Output must be new; refusing to mix captures')
    disc, elf, bios, emulator = [path.resolve(strict=True) for path in
                                (args.disc, args.elf, args.bios, args.mednafen)]
    if hashlib.md5(bios.read_bytes()).hexdigest() != '3240872c70984b6cbfda1586cab68dbe':
        parser.error('Expected the supplied US/EU Saturn BIOS')
    track, mode, cue_text = disc_tracks(disc)
    source_records = [dict(path=str(path), sha256=sha(path), bytes=path.stat().st_size)
                      for path in (disc, track, elf, bios, emulator)]
    output.mkdir(parents=True)
    (output / 'disc').mkdir()
    (output / 'home').mkdir()
    (output / 'xdg').mkdir(mode=0o700)
    for filename in ('run_mednafen.py', 'observer.py', 'launch_identity.py'):
        shutil.copy2(HERE / filename, output / filename)
    shutil.copy2(HERE.parent / 'beetle/run_beetle.py', output / 'disc_identity_source.py')
    shutil.copy2(track, output / 'disc' / track.name)
    staged_cue = re.sub(r'(^\s*FILE\s+)(?:"([^"]+)"|(\S+))',
                        lambda match: match[1] + '"' + track.name + '"', cue_text, flags=re.M | re.I)
    frozen_disc = output / 'disc' / disc.name
    frozen_disc.write_text(staged_cue)
    shutil.copy2(elf, output / 'program.elf')
    if elf.with_suffix('.map').is_file():
        shutil.copy2(elf.with_suffix('.map'), output / 'program.map')
    subprocess.run([find_tool('objcopy', args.sdk), '-O', 'binary', str(output / 'program.elf'),
                    str(output / 'program.bin')], check=True)
    program = (output / 'program.bin').read_bytes()
    if disc_program(frozen_disc) != program:
        raise RuntimeError('Frozen disc A.BIN differs from exact ELF; refusing capture')
    symbols = load_symbols(output / 'program.elf', args.sdk)
    extras = list(args.watch)
    if actions:
        extras += ['pad_connected:pad:1', 'pad_level:pad+8:2', 'input0:gameplay:1',
                   'reset_armed:reset_combo:1', 'royal_stage:royal_init_observed:4']
    watches = build_watches(symbols, args.actors_layout, extras)
    write_json(output / 'symbols.json', symbols)
    write_json(output / 'watches.json', watches)
    write_json(output / 'actions.json', dict(scenario=args.scenario, ranges=actions,
               field_origin='first observed started1 and game_frame>=1', keys=KEYS,
               keyboard_scancodes=SCANCODES, mechanism='ordinary X11 keyboard; no RAM writes'))
    xserver = process = observer = None
    xlog = None
    last_sample = None
    sample_count = 0
    native_origin = control_origin = finished_origin = None
    held = set()
    events = []
    reason = None
    successful_exit = False
    started_at = time.monotonic()
    provenance = dict(scope='Standalone Mednafen native emulation, read-only asynchronous /proc observation',
                      source_inputs=source_records, disc_program_matches_elf=True,
                      program_sha256=hashlib.sha256(program).hexdigest(), cartridge=args.cartridge,
                      cache_requested=args.cache, no_save_states=True, no_process_memory_writes=True,
                      timing='Native counters and MOV timestamps; host wall time recorded separately',
                      video='No movie requested' if args.no_movie else 'Mednafen internal QuickTime CSCD+PCM; original source retained')
    write_json(output / 'provenance.json', provenance)
    scalar_names = [watch['name'] for watch in watches if watch['size'] <= 8]
    try:
        if args.offscreen:
            display, xcommand = '', None
        else:
            xserver, xlog, display, xcommand = start_xvfb(output)
        env = env_for(RUNTIME)
        env.update(DISPLAY=display, MEDNAFEN_HOME=str(output / 'home'), SDL_VIDEODRIVER='dummy' if args.offscreen else 'x11',
                   SDL_AUDIODRIVER='dummy', XDG_RUNTIME_DIR=str(output / 'xdg'))
        settings = {'ss.cart': 'extram4' if args.cartridge == '4MiB' else 'none',
                    'ss.region_autodetect': '0', 'ss.region_default': 'na',
                    'ss.dbg_cem': args.cache, 'ss.bios_na_eu': str(bios),
                    'video.driver': 'softfb', 'video.frameskip': '0', 'video.blit_timesync': '0',
                    'nothrottle': '0', 'ffspeed': '1', 'fftoggle': '0',
                    'ss.correct_aspect': '0', 'ss.h_overscan': '1', 'ss.h_blend': '0',
                    'ss.slstart': '0', 'ss.slend': '239', 'ss.xscale': '2', 'ss.yscale': '2',
                    'ss.input.port1': 'gamepad', 'ss.input.port2': 'gamepad',
                    'ss.input.sport1.multitap': '0', 'ss.input.sport2.multitap': '0',
                    'sound': '1', 'sound.driver': 'dummy', 'sound.rate': '44100',
                    'srwautoenable': '0', 'input.ckdelay': '0',
                    'command.exit': 'keyboard 0x0 69',
                    'qtrecord.w_double_threshold': '0', 'qtrecord.h_double_threshold': '0',
                    'qtrecord.vcodec': 'cscd'}
        settings.update({'ss.input.port1.gamepad.' + key: 'keyboard 0x0 ' + str(code)
                         for key, code in SCANCODES.items()})
        command = [str(emulator)]
        for name, value in settings.items():
            command += ['-' + name, value]
        if not args.no_movie:
            command += ['-qtrecord', str(output / 'native.mov')]
        command += [str(frozen_disc)]
        write_json(output / 'command.json', dict(emulator=command, xvfb=xcommand,
                   settings=settings, environment={key: env[key] for key in
                   ('DISPLAY', 'MEDNAFEN_HOME', 'SDL_VIDEODRIVER', 'SDL_AUDIODRIVER', 'XDG_RUNTIME_DIR', 'LD_LIBRARY_PATH')}))
        with (output / 'emulator.log').open('wb') as log, gzip.open(output / 'samples.jsonl.gz', 'wt', compresslevel=3) as samples, (output / 'telemetry.csv').open('w') as counters:
            writer = csv.DictWriter(counters, fieldnames=['host_elapsed_seconds', 'boundary_stable'] + scalar_names)
            writer.writeheader()
            launched_identity = output / 'launched-process.json'
            process = subprocess.Popen([sys.executable, str(output / 'launch_identity.py'),
                                        str(launched_identity), *command], env=env,
                                       stdout=log, stderr=subprocess.STDOUT)
            next_locate = next_progress = 0.0
            last_key = None
            window = None
            while process.poll() is None:
                now = time.monotonic()
                if now - started_at > args.wall_timeout:
                    raise TimeoutError('Standalone capture exceeded wall-timeout')
                if observer is None and launched_identity.exists():
                    identity = json.loads(launched_identity.read_text())
                    proc_pid = identity['proc_pid']
                    try:
                        if sha(f'/proc/{proc_pid}/exe') == source_records[-1]['sha256']:
                            observer = ReadOnlyObserver(process.pid, program, proc_pid=proc_pid)
                    except FileNotFoundError:
                        pass
                if observer is not None and not observer.located and now >= next_locate:
                    next_locate = now + 0.5
                    try:
                        observer.locate()
                        write_json(output / 'memory-identity.json', observer.identity())
                    except NotReady:
                        pass
                if observer is not None and observer.located:
                    sample = observer.sample(watches)
                    sample['host_elapsed_seconds'] = now - started_at
                    last_sample = sample
                    key = tuple(sample.get(name) for name in
                                ('video_clock', 'game_frame', 'wp1980_draws', 'wp1980_started',
                                 'finished_presentation', 'fatal_owner', 'fatal_dependency', 'pad_level',
                                 'frame_ready', 'frame_stage'))
                    if key != last_key:
                        samples.write(json.dumps(sample, separators=(',', ':')) + '\n')
                        writer.writerow({name: sample.get(name) for name in writer.fieldnames})
                        sample_count += 1
                        last_key = key
                    if sample.get('wp1980_started') == 1:
                        clock = sample['video_clock']
                        if native_origin is None:
                            native_origin = clock
                        if control_origin is None and sample.get('game_frame', 0) >= 1:
                            control_origin = clock
                        if sample.get('fatal_owner') or sample.get('fatal_dependency'):
                            raise RuntimeError('Native fatal counter is nonzero')
                        if control_origin is not None and actions:
                            delta = (clock - control_origin) & 0xffffffff
                            desired = {item['button'] for item in actions if item['first'] <= delta <= item['last']}
                            if desired != held:
                                if window is None:
                                    window = focus_window(display)
                                release, press = sorted(held - desired), sorted(desired - held)
                                if release:
                                    keyboard_command(display, 'keyup', '--delay', '0', *[KEYS[name] for name in release])
                                if press:
                                    keyboard_command(display, 'keydown', '--delay', '0', *[KEYS[name] for name in press])
                                events.append(dict(native_clock=clock, relative_field=delta,
                                                   game_frame=sample.get('game_frame'), release=release,
                                                   press=press, host_elapsed_seconds=now - started_at))
                                held = desired
                                write_json(output / 'input-events.json', events)
                        if args.stop_game_frame is not None and sample.get('game_frame', 0) >= args.stop_game_frame:
                            reason = 'smoke_game_frame'
                        if args.run_fields is not None and (clock - native_origin) & 0xffffffff >= args.run_fields:
                            reason = 'requested_native_fields'
                        if sample.get('finished_presentation') == 2:
                            if finished_origin is None:
                                finished_origin = clock
                            if args.run_fields is None and args.stop_game_frame is None and ((clock - finished_origin) & 0xffffffff) >= args.post_fields:
                                reason = 'completed_gameover_and_post_fields'
                        if reason:
                            samples.flush(); counters.flush()
                            write_json(output / 'memory-dump.json', observer.dump_hwram(output))
                            if args.offscreen:
                                process.send_signal(signal.SIGINT)
                            else:
                                focus_window(display)
                                if held:
                                    keyboard_command(display, 'keyup', '--delay', '0', *[KEYS[name] for name in sorted(held)])
                                    held.clear()
                                keyboard_command(display, 'key', 'F12')
                            process.wait(timeout=20)
                            successful_exit = process.returncode == 0
                            if args.offscreen and process.returncode in (1, 255):
                                successful_exit = 'Signal has been caught and dealt with: SIGINT' in (output / 'emulator.log').read_text(errors='replace')
                            break
                    if now >= next_progress:
                        next_progress = now + 1
                        write_json(output / 'progress.json', dict(wall_seconds=now - started_at,
                                   sample_count=sample_count, last_sample={name: sample.get(name) for name in scalar_names},
                                   observer_stats=observer.stats))
                        samples.flush(); counters.flush()
                time.sleep(args.poll_ms / 1000)
        if not successful_exit:
            raise RuntimeError(f'Mednafen did not exit through its planned exit handler: {process.returncode}')
        log_text = (output / 'emulator.log').read_text(errors='replace')
        cache_match = re.search(r'CPU Cache Emulation Mode:\s*([^\r\n]+)', log_text)
        cart_match = re.search(r'^\s*Cart:\s*([^\r\n]+)', log_text, re.M)
        if 'Starting Mednafen 1.32.1' not in log_text:
            raise RuntimeError('Unexpected standalone emulator version')
        if args.cache == 'full' and (not cache_match or cache_match[1].strip() != 'Full'):
            raise RuntimeError('Standalone full-cache selection was not confirmed by emulator log')
        expected_cart = '4MiB Extended RAM' if args.cartridge == '4MiB' else 'None'
        if not cart_match or cart_match[1].strip() != expected_cart:
            raise RuntimeError('Standalone cartridge setting not confirmed by emulator log')
        for record in source_records:
            if sha(record['path']) != record['sha256']:
                raise RuntimeError('Source input changed during capture: ' + record['path'])
        if disc_program(frozen_disc) != program:
            raise RuntimeError('Frozen disc identity changed during capture')
        original_media = None
        if not args.no_movie:
            original_media = probe_media(output / 'native.mov')
            write_json(output / 'native-mov-probe.json', original_media)
        mp4_media = None
        if not args.no_mp4 and not args.no_movie:
            encode = ['ffmpeg', '-hide_banner', '-loglevel', 'warning', '-nostdin', '-n',
                      '-i', str(output / 'native.mov'), '-map', '0:v:0', '-map', '0:a:0',
                      '-c:v', 'libx264', '-preset', 'fast', '-crf', '18', '-pix_fmt', 'yuv420p',
                      '-threads:v', '2', '-fps_mode:v', 'passthrough', '-enc_time_base:v', '-1',
                      '-video_track_timescale', '44100', '-c:a', 'aac', '-b:a', '192k',
                      '-movflags', '+faststart', str(output / 'native.mp4')]
            write_json(output / 'encode-command.json', encode)
            with (output / 'encode.log').open('wb') as log:
                subprocess.run(encode, stdout=log, stderr=subprocess.STDOUT, check=True)
            mp4_media = probe_media(output / 'native.mp4')
            write_json(output / 'native-mp4-probe.json', mp4_media)
            a = next(stream for stream in original_media['streams'] if stream['codec_type'] == 'video')
            b = next(stream for stream in mp4_media['streams'] if stream['codec_type'] == 'video')
            if a['nb_frames'] != b['nb_frames']:
                raise RuntimeError('MP4 frame count differs from original Mednafen MOV')
        report = dict(status='PASS', scope='Identity, native execution, read-only observation and recording; not physical-hardware proof',
                      stop_reason=reason, emulator_version='1.32.1', cache_mode=cache_match[1].strip(),
                      cartridge=cart_match[1].strip(), source_inputs=source_records,
                      exit_method='Mednafen SIGINT handler' if args.offscreen else 'ordinary F12 keyboard command', emulator_return_code=process.returncode,
                      wall_seconds=time.monotonic() - started_at, samples_saved=sample_count,
                      samples_file='samples.jsonl.gz', scalar_file='telemetry.csv',
                      observer_stats=observer.stats, observer_identity=observer.identity(),
                      native_start_clock=native_origin, controller_start_clock=control_origin,
                      finished_presentation_clock=finished_origin, input_events=events,
                      last_sample=last_sample, media_original=original_media, media_mp4=mp4_media,
                      movie_recorded=not args.no_movie,
                      no_memory_writes=True, no_save_states=True, no_overclock=True,
                      no_video_retiming=True, post_fields_requested=args.post_fields)
        write_json(output / 'report.json', report)
        print(json.dumps({key: report[key] for key in ('status', 'stop_reason', 'cache_mode', 'cartridge',
              'samples_saved', 'wall_seconds', 'last_sample')}, indent=2))
    except BaseException as error:
        write_json(output / 'failure.json', dict(status='FAIL', exception=repr(error),
                   wall_seconds=time.monotonic() - started_at, last_sample=last_sample))
        raise
    finally:
        stop_process(process)
        if observer is not None:
            observer.close()
        stop_process(xserver)
        if xlog is not None:
            xlog.close()


if __name__ == '__main__':
    main()
