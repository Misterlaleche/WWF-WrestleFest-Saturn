#!/usr/bin/env python3
"""Capture unmodified standalone Mednafen 1.32.1 with native record/state APIs.

The Xvfb wrapper relocates only Xvfb's xkbcomp lookup. The emulator is never
preloaded, patched, clock-adjusted, or memory-written. Optional WRAM reads use
symbols in the locally built native executable and /proc/<child>/mem.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import time
import wave

from analyze_audio_capture import read_state

ROOT = Path(__file__).resolve().parent


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def finalized_wav(path):
    """Reject incomplete native soundrecord output; never repair its header."""
    path = Path(path)
    with path.open('rb') as source:
        header = source.read(12)
    if (len(header) != 12 or header[:4] != b'RIFF' or header[8:] != b'WAVE'
            or struct.unpack_from('<I', header, 4)[0] + 8 != path.stat().st_size):
        raise ValueError('Native WAV RIFF length/header is not finalized')
    with wave.open(str(path)) as source:
        frames, channels = source.getnframes(), source.getnchannels()
        if (source.getframerate() != 44100 or source.getsampwidth() != 2
                or channels != 2 or not frames
                or len(source.readframes(frames)) != frames * channels * 2):
            raise ValueError('Native WAV PCM span is incomplete or unexpected')
    return dict(sha256=sha256(path), bytes=path.stat().st_size,
                frames=frames, channels=channels, sample_rate=44100,
                bits=16, finalized=True)


def main():
    global ROOT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--environment-root', type=Path, default=ROOT,
                        help='Directory containing mednafen/, host/, and bin/ (default: script directory)')
    parser.add_argument('--cue', required=True, type=Path)
    parser.add_argument('--bios', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--seconds', type=float, default=15)
    parser.add_argument('--display', type=int, default=87)
    parser.add_argument('--watch', action='append', default=[], help='NAME:SH2_ADDRESS:BYTES; width 1, 2, or 4')
    parser.add_argument('--stop-watch', help='Stop when the named watch reaches --stop-value')
    parser.add_argument('--stop-value', type=lambda s: int(s, 0), default=0)
    parser.add_argument('--ready-watch', help='Require this watch to equal --ready-value before threshold stop')
    parser.add_argument('--ready-value', type=lambda s: int(s, 0), default=0)
    parser.add_argument('--snapshot-every', type=float, default=0)
    parser.add_argument('--dump-wram', action='store_true')
    parser.add_argument('--audio-only', action='store_true',
                        help='Keep real soundrecord, native state and screenshot; omit QuickTime video')
    parser.add_argument('--presentation-speed', type=int, choices=(1, 4), default=1,
                        help='Upstream fast-forward multiplier after ready; WAV precedes its resampler and emulated hardware clocks are unchanged')
    parser.add_argument('--cartridge', choices=('none', 'extram4'), default='none',
                        help='Explicit diagnostic cartridge; extra RAM is never enabled implicitly')
    args = parser.parse_args()
    ROOT = args.environment_root.resolve()
    args.cue = args.cue.resolve()
    args.bios = args.bios.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    runtime = out / 'runtime'
    runtime.mkdir(mode=0o700)
    medhome = out / 'mednafen-home'
    medhome.mkdir()
    executable = ROOT / 'mednafen/bin/mednafen'
    watches = []
    for spec in args.watch:
        name, address, width = spec.split(':')
        address, width = int(address, 0), int(width, 0)
        if address & 0xe0000000 == 0x20000000:
            address &= 0x1fffffff
        if width not in (1, 2, 4) or not 0x06000000 <= address <= 0x060FFFFF - width + 1:
            raise ValueError('Only valid high-WRAM byte/word/long watches are supported')
        watches.append((name, address, width))
    if args.stop_watch and args.stop_watch not in [w[0] for w in watches]:
        raise ValueError('--stop-watch must name a --watch')
    if args.ready_watch and args.ready_watch not in [w[0] for w in watches]:
        raise ValueError('--ready-watch must name a --watch')
    if args.presentation_speed != 1 and not args.ready_watch:
        raise ValueError('Presentation fast-forward requires a native --ready-watch')
    env = dict(os.environ)
    env.update(DISPLAY=f':{args.display}', SDL_VIDEODRIVER='x11',
               MEDNAFEN_HOME=str(medhome), XDG_RUNTIME_DIR=str(runtime))
    env['LD_LIBRARY_PATH'] = str(ROOT / 'host/usr/lib/x86_64-linux-gnu') + ':' + env.get('LD_LIBRARY_PATH', '')
    env['PATH'] = str(ROOT / 'bin') + ':' + str(ROOT / 'host/usr/bin') + ':' + env.get('PATH', '')
    # Explicitly prevent any externally inherited preload from changing the emulator.
    env.pop('LD_PRELOAD', None)
    command = [str(executable), '-video.driver', 'softfb', '-video.frameskip', '0',
               '-sound.driver', 'dummy', '-sound', '1', '-sound.rate', '44100',
               '-ss.bios_na_eu', str(args.bios), '-ss.region_autodetect', '0',
               '-ss.region_default', 'na', '-ss.bios_sanity', '1',
               '-ss.cart', args.cartridge,
               '-ss.correct_aspect', '0', '-ss.xscale', '2', '-ss.yscale', '2',
               '-filesys.state_comp_level', '0',
               '-qtrecord.w_double_threshold', '0', '-qtrecord.h_double_threshold', '0',
               '-soundrecord', str(out / 'native.wav'), str(args.cue)]
    if not args.audio_only:
        command[-1:-1] = ['-qtrecord', str(out / 'native.mov')]
    if args.presentation_speed != 1:
        command[-1:-1] = ['-fftoggle', '1', '-ffspeed', str(args.presentation_speed)]
    report = {'emulator': 'standalone Mednafen 1.32.1', 'command': command,
              'capture_tool_sha256': sha256(Path(__file__).resolve()),
              'emulator_sha256': sha256(executable), 'cue_sha256': sha256(args.cue),
              'bios_sha256': sha256(args.bios), 'wall_time_limit_seconds': args.seconds,
              'memory_access': 'read only', 'frame_skip': False,
              'presentation_speed_requested': args.presentation_speed,
              'cartridge': args.cartridge,
              'emulated_hardware_clock_change': False,
              'stop_watch': args.stop_watch, 'stop_value': args.stop_value,
              'ready_watch': args.ready_watch, 'ready_value': args.ready_value,
              'stop_reason': None, 'trace': []}
    report['disc_images'] = []
    for line in args.cue.read_text().splitlines():
        parts = shlex.split(line)
        if parts and parts[0].upper() == 'FILE':
            disc_image = (args.cue.parent / parts[1]).resolve()
            report['disc_images'].append({'path': str(disc_image),
                                          'bytes': disc_image.stat().st_size,
                                          'sha256': sha256(disc_image)})
    symbols = {}
    nm = subprocess.check_output(['nm', '-C', str(executable)], text=True)
    for line in nm.splitlines():
        for name in ('WorkRAMH', 'WorkRAML'):
            if line.endswith('MDFN_IEN_SS::' + name):
                symbols[name] = int(line.split()[0], 16)
    report['native_symbols'] = symbols
    emu = None
    memory = None

    def key(name, release=True):
        subprocess.run(['xdotool', 'keydown', name], env=env, check=True)
        time.sleep(0.08)
        if release:
            subprocess.run(['xdotool', 'keyup', name], env=env, check=True)
            time.sleep(0.08)

    def read_values():
        result = {}
        for name, address, width in watches:
            offset = address - 0x06000000
            if width == 1:
                memory.seek(symbols['WorkRAMH'] + (offset ^ 1))
                result[name] = memory.read(1)[0]
            else:
                memory.seek(symbols['WorkRAMH'] + offset)
                data = memory.read(width)
                ordered = b''.join(data[i:i+2][::-1] for i in range(0, width, 2))
                result[name] = int.from_bytes(ordered, 'big')
        return result

    with (out / 'xvfb.log').open('w') as xlog, (out / 'mednafen.log').open('w') as elog:
        xserver = subprocess.Popen([str(ROOT / 'bin/Xvfb'), f':{args.display}',
                                    '-screen', '0', '1060x800x24', '-nolisten', 'tcp', '-ac'],
                                   env=env, stdout=xlog, stderr=subprocess.STDOUT)
        try:
            for _ in range(30):
                ready = subprocess.run(['xdotool', 'getdisplaygeometry'], env=env,
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                if ready.returncode == 0:
                    break
                if xserver.poll() is not None:
                    raise RuntimeError('Xvfb exited; see xvfb.log')
                time.sleep(0.1)
            else:
                raise RuntimeError('Xvfb did not become ready')
            version = subprocess.run([str(executable), '-help'], env=env, text=True,
                                     capture_output=True, timeout=15)
            (out / 'mednafen-version.txt').write_text(version.stdout + version.stderr)
            # Upstream 1.32.1 intentionally returns -1/255 after displaying help.
            if (version.returncode not in (0, 255)
                    or not version.stdout.startswith('Starting Mednafen 1.32.1\n')
                    or 'Usage:' not in version.stdout or '-qtrecord' not in version.stdout):
                raise RuntimeError('Capture requires standalone Mednafen 1.32.1 exactly')
            emu = subprocess.Popen(command, env=env, stdout=elog, stderr=subprocess.STDOUT)
            started = time.monotonic()
            # Native non-PIE executable gives stable addresses for read-only probes.
            if watches or args.dump_wram:
                time.sleep(0.1)
                memory = open(f'/proc/{emu.pid}/mem', 'rb', buffering=0)
            next_snapshot = args.snapshot_every
            next_status = 5
            fast_forward_started = False
            while time.monotonic() - started < args.seconds:
                if emu.poll() is not None:
                    raise RuntimeError(f'Mednafen exited with {emu.returncode}; see mednafen.log')
                elapsed = time.monotonic() - started
                values = read_values() if watches else {}
                if watches:
                    report['trace'].append({'wall_seconds': round(elapsed, 6), **values})
                if elapsed >= next_status:
                    print(json.dumps({'wall_seconds': round(elapsed, 3), **values}), flush=True)
                    (out / 'report-progress.json').write_text(json.dumps(report, indent=2) + '\n')
                    next_status += 5
                ready = not args.ready_watch or values[args.ready_watch] == args.ready_value
                if ready and args.presentation_speed != 1 and not fast_forward_started:
                    key('grave')
                    fast_forward_started = True
                    report['fast_forward_toggle_wall_seconds'] = time.monotonic() - started
                if args.stop_watch and ready and values[args.stop_watch] >= args.stop_value:
                    report['stop_reason'] = 'watch_threshold_reached'
                    break
                if args.snapshot_every and elapsed >= next_snapshot:
                    key('F9')
                    next_snapshot += args.snapshot_every
                time.sleep(0.02)
            else:
                report['stop_reason'] = 'wall_time_limit'
            key('Pause')
            key('F9')
            key('F5')
            if watches:
                report['final_watch_values'] = read_values()
            if args.dump_wram:
                for name, address in symbols.items():
                    memory.seek(address)
                    data = memory.read(1024 * 1024)
                    (out / (name + '-host-order.bin')).write_bytes(data)
                    # Saturn bytes restored by reversing each native uint16.
                    (out / (name + '-saturn-order.bin')).write_bytes(
                        b''.join(data[i:i+2][::-1] for i in range(0, len(data), 2)))
            # F12 closes the last emulator window. A later keyup can race the
            # X server reset and fail after a fully completed native capture.
            # No release is needed for a process we immediately wait for.
            # Exit status, finalized WAV and the unique F5 state remain gates.
            key('F12', release=False)
            emu.wait(timeout=10)
            report['exit_code'] = emu.returncode
            report['wall_seconds'] = round(time.monotonic() - started, 6)
            try:
                report['wav_validation'] = finalized_wav(out / 'native.wav')
            except (ValueError, wave.Error) as error:
                report['wav_validation'] = {'finalized': False, 'error': str(error)}
                raise
            states = list((medhome / 'mcs').glob('*.mc0'))
            if len(states) != 1:
                raise RuntimeError('Expected exactly one native F5 state; refusing an incomplete capture')
            if len(states) == 1:
                metadata, sections = read_state(states[0])
                report['native_state'] = {'path': str(states[0]), **metadata}
                scsp_ram = sections['SCSP']['RAM']
                (out / 'SCSP-RAM-state-order.bin').write_bytes(scsp_ram)
                if metadata['data_byte_order'] == 'little':
                    scsp_ram = b''.join(scsp_ram[i:i+2][::-1]
                                       for i in range(0, len(scsp_ram), 2))
                (out / 'SCSP-RAM-saturn-order.bin').write_bytes(scsp_ram)
                report['native_state']['scsp_ram_bytes'] = len(scsp_ram)
                report['native_state']['program_counters'] = {
                    name: int.from_bytes(sections[name]['PC'], metadata['data_byte_order'])
                    for name in ('SH2-M', 'SH2-S', 'M68K')}
        finally:
            if memory:
                memory.close()
            if emu is not None and emu.poll() is None:
                emu.terminate()
                emu.wait(timeout=10)
            xserver.terminate()
            xserver.wait(timeout=5)
            (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('trace', 'command')}, indent=2))


if __name__ == '__main__':
    main()
