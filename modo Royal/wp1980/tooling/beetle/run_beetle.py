#!/usr/bin/env python3
"""Record a native Saturn disc with real BIOS and unchanged strict Beetle timing.

All emulator-memory observations are reads. No RAM/register patches or save
states are supplied. The video stream contains one actual native framebuffer
per retro_run, padded only if smaller than the fixed canvas.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import select
import shutil
import struct
import subprocess
import sys
import time
import wave
from PIL import Image

ROOT = Path(__file__).resolve().parent
CORE_SHA = '9795dccdc55a7ed170140596ffb3398f6f053612a82ca9c0478fadc86f068485'
BIOS_MD5 = '3240872c70984b6cbfda1586cab68dbe'
CARTRIDGE_OPTIONS = {'None': 'None', '4MiB': 'Extended RAM (4MB)'}
CARTRIDGE_DESCRIPTIONS = {'None': 'None', '4MiB': '4MiB Extended RAM'}
DEFAULT_NAMES = (
    'wp1980_started', 'wp1980_draws', 'wp1980_vblanks', 'wp1980_max_gap',
    'wp1980_demo_active', 'video_clock', 'fatal_owner', 'fatal_dependency',
    'fatal_frame', 'finished_frame', 'finished_result', 'finished_count',
    'finished_timer', 'finished_presentation', 'finished_vblanks',
    'finished_presentation_vblanks', 'window_misses', 'window_sectors',
    'wp1979_cd_init_rc', 'wp1979_cdfs_stage', 'wp1979_cdfs_lba', 'wp1979_cdfs_rc',
    'wp1979_started', 'wp1979_draws', 'wp1979_vblanks', 'wp1979_max_gap',
    'wp1979_demo_active', 'boot_seed_observed',
)


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1048576), b''):
            h.update(block)
    return h.hexdigest()


def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n')


def disc_tracks(cue):
    cue = Path(cue).resolve(strict=True)
    content = cue.read_text()
    matches = re.findall(r'^\s*FILE\s+(?:"([^"]+)"|(\S+))\s+BINARY\s*$', content, re.M | re.I)
    if len(matches) != 1 or len(re.findall(r'^\s*TRACK\s+', content, re.M | re.I)) != 1:
        raise ValueError('Expected one BINARY data track')
    name = matches[0][0] or matches[0][1]
    track = (cue.parent / name).resolve(strict=True)
    modes = re.findall(r'^\s*TRACK\s+\d+\s+(MODE1/(?:2352|2048))\s*$', content, re.M | re.I)
    if len(modes) != 1:
        raise ValueError('Expected a single MODE1/2352 or MODE1/2048 track')
    return track, modes[0].upper(), content


def disc_program(cue):
    track, mode, _ = disc_tracks(cue)
    data = track.read_bytes()
    width = 2352 if mode == 'MODE1/2352' else 2048
    if not data or len(data) % width:
        raise ValueError('Incomplete disc sectors')
    if width == 2352:
        data = b''.join(data[i + 16:i + 2064] for i in range(0, len(data), 2352))
    pvd = data[16 * 2048:17 * 2048]
    if len(pvd) != 2048 or pvd[:7] != b'\x01CD001\x01':
        raise ValueError('Invalid ISO9660 primary volume descriptor')
    root = pvd[156:190]
    start = int.from_bytes(root[2:6], 'little') * 2048
    length = int.from_bytes(root[10:14], 'little')
    directory = data[start:start + length]
    offset = 0
    while offset < len(directory):
        size = directory[offset]
        if not size:
            offset = (offset // 2048 + 1) * 2048
            continue
        rec = directory[offset:offset + size]
        if size < 34 or len(rec) != size:
            raise ValueError('Invalid ISO9660 directory record')
        offset += size
        if rec[25] & 2:
            continue
        name = rec[33:33 + rec[32]].decode('ascii').split(';')[0]
        if name == 'A.BIN':
            begin = int.from_bytes(rec[2:6], 'little') * 2048
            size = int.from_bytes(rec[10:14], 'little')
            if begin + size > len(data):
                raise ValueError('A.BIN extends beyond disc')
            return data[begin:begin + size]
    raise ValueError('Disc has no A.BIN')


def resolve_watch(spec, symbols):
    parts = spec.split(':')
    if len(parts) == 1:
        name, expression, size_text = parts[0], parts[0], '4'
    elif len(parts) in (2, 3):
        name, expression = parts[:2]
        size_text = parts[2] if len(parts) == 3 else '4'
    else:
        raise ValueError(f'Invalid watch: {spec}')
    if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]{0,94}', name):
        raise ValueError(f'Invalid watch name: {name}')
    size = int(size_text, 0)
    if not 1 <= size <= 65536:
        raise ValueError('Watch size must be 1..65536 bytes')
    terms = re.fullmatch(r'([^+\s]+)(?:\+(0[xX][0-9a-fA-F]+|[0-9]+))?', expression)
    if not terms:
        raise ValueError(f'Invalid watch address: {expression}')
    base, add = terms.groups()
    try:
        address = int(base, 0)
    except ValueError:
        if base not in symbols:
            raise ValueError(f'Unknown ELF symbol: {base}') from None
        address = symbols[base]
    address += int(add, 0) if add else 0
    if not 0 <= address <= 0xffffffff or address + size > 0x100000000:
        raise ValueError('Watch address range overflows SH2 address space')
    # Cache-through and cached SH2 aliases identify the same physical bytes.
    address &= 0x1fffffff
    return dict(name=name, address=address, size=size)


def find_tool(name, sdk=None):
    if sdk:
        candidate = Path(sdk) / 'bin' / ('sh2eb-elf-' + name)
        if candidate.is_file():
            return str(candidate)
    found = shutil.which('sh2eb-elf-' + name)
    if not found:
        raise ValueError('Set YAUL_INSTALL_ROOT or --sdk to the supplied SH2 toolchain')
    return found


def elf_symbols(elf, nm):
    result = {}
    for line in subprocess.check_output([nm, '-n', str(elf)], text=True).splitlines():
        fields = line.split()
        if len(fields) != 3 or not re.fullmatch(r'[0-9a-fA-F]+', fields[0]):
            continue
        address = int(fields[0], 16)
        name = fields[2]
        result[name] = address
        result.setdefault(name.lstrip('_').split('.lto_priv')[0], address)
    return result


def read_header(pipe, process, timeout):
    deadline = time.monotonic() + timeout
    data = bytearray()
    while len(data) < 8192:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError('Timed out waiting for core AV configuration')
        readable, _, _ = select.select([pipe], [], [], min(remaining, 1))
        if not readable:
            if process.poll() is not None:
                raise RuntimeError('Emulator exited before AV header; inspect emulator.log')
            continue
        char = os.read(pipe.fileno(), 1)
        if not char:
            raise RuntimeError('Emulator closed stream before AV header; inspect emulator.log')
        if char == b'\n':
            header = json.loads(data)
            if not math.isfinite(header['fps']) or header['fps'] <= 0:
                raise ValueError('Core supplied invalid refresh rate')
            return header
        data.extend(char)
    raise ValueError('AV header exceeds length limit')


def terminate(process):
    if process and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('disc', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--bios', type=Path, required=True)
    p.add_argument('--elf', type=Path, help='Verify disc A.BIN against this ELF and resolve symbols')
    p.add_argument('--sdk', type=Path, default=os.environ.get('YAUL_INSTALL_ROOT'))
    p.add_argument('--core', type=Path, default=ROOT / 'mednafen_saturn_libretro.so')
    p.add_argument('--cartridge', choices=CARTRIDGE_OPTIONS, default='None',
                   help='Real emulated cartridge configuration; default is no expansion RAM')
    p.add_argument('--frames', type=int, default=1200)
    p.add_argument('--capture-step', type=int, default=120, help='Native PNG snapshot spacing; video always records every frame')
    p.add_argument('--watch', action='append', default=[], help='NAME:SYMBOL+OFFSET:SIZE or NAME:ADDRESS:SIZE; bare SYMBOL means 4 bytes')
    p.add_argument('--no-default-watches', action='store_true')
    p.add_argument('--actors-layout', type=Path, help='SH2-ABI actors-layout.bin; adds all nine complete actor records (slots 0..8)')
    p.add_argument('--press', action='append', default=[], help='libretro button_id:first_frame:last_frame, inclusive on port 0')
    p.add_argument('--canvas', default='330x240', help='Fixed canvas; black padding only, never scaling or cropping')
    p.add_argument('--no-video', action='store_true', help='Read counters/audio/screenshots without encoding an MP4')
    p.add_argument('--wall-timeout', type=float, default=600)
    a = p.parse_args()
    if a.frames <= 0 or a.capture_step < 0 or a.wall_timeout <= 0:
        p.error('frames/timeout must be positive and capture-step nonnegative')
    geometry = re.fullmatch(r'([1-9][0-9]*)x([1-9][0-9]*)', a.canvas)
    if not geometry or any(int(v) > 4096 or int(v) % 2 for v in geometry.groups()):
        p.error('canvas requires two even dimensions up to 4096')
    for press in a.press:
        match = re.fullmatch(r'(\d+):(\d+):(\d+)', press)
        if not match or not (int(match[1]) < 16 and 1 <= int(match[2]) <= int(match[3])):
            p.error('press must be button_id(0..15):first:last, inclusive, first >= 1')
    if len(a.press) > 64:
        p.error('at most 64 press intervals are supported')
    disc, bios, core = a.disc.resolve(strict=True), a.bios.resolve(strict=True), a.core.resolve(strict=True)
    output = a.output.resolve()
    if output.exists():
        p.error('output must be new; refusing to mix captures')
    bios_bytes = bios.read_bytes()
    if len(bios_bytes) != 524288 or hashlib.md5(bios_bytes).hexdigest() != BIOS_MD5:
        p.error('Expected supplied US/EU mpr-17933.bin BIOS')
    if sha(core) != CORE_SHA:
        p.error('Core differs from the audited unchanged Beetle binary')
    track, _, cue_text = disc_tracks(disc)
    source_inputs = [disc, track]
    input_records = [dict(path=str(f), size=f.stat().st_size, sha256=sha(f)) for f in source_inputs]
    symbols = {}
    elf = a.elf.resolve(strict=True) if a.elf else None
    if elf:
        nm, objcopy = find_tool('nm', a.sdk), find_tool('objcopy', a.sdk)
        symbols = elf_symbols(elf, nm)
    watch_by_name = {}
    if elf and not a.no_default_watches:
        for name in DEFAULT_NAMES:
            if name in symbols:
                watch_by_name[name] = resolve_watch(name, symbols)
        if 'rt' in symbols:
            watch_by_name['game_frame'] = resolve_watch('game_frame:rt+24:4', symbols)
    if a.actors_layout:
        if 'rt' not in symbols:
            p.error('--actors-layout requires ELF symbol rt')
        blob = a.actors_layout.read_bytes()
        if len(blob) < 20:
            p.error('actors-layout binary must contain five SH2 uint32 values')
        offset, stride, rng, registers, inputs = struct.unpack('>5I', blob[:20])
        if stride != 0x10c:
            p.error('actors-layout actor stride differs from expected 0x10c')
        for slot in range(9):
            name = f'actor{slot}'
            watch_by_name[name] = resolve_watch(f'{name}:rt+{offset + slot * stride}:{stride}', symbols)
        watch_by_name['rng_state'] = resolve_watch(f'rng_state:rt+{rng}:4', symbols)
    for spec in a.watch:
        watch = resolve_watch(spec, symbols)
        watch_by_name[watch['name']] = watch
    watches = list(watch_by_name.values())
    if len(watches) > 256:
        p.error('at most 256 watches are supported')
    output.mkdir(parents=True, exist_ok=False)
    # Freeze the exact observer sources before starting; later edits to this
    # independent tool directory cannot change capture provenance.
    shutil.copyfile(ROOT / 'headless_saturn.c', output / 'headless_saturn.c')
    shutil.copyfile(ROOT / 'libretro.h', output / 'libretro.h')
    shutil.copyfile(Path(__file__), output / 'capture_wrapper.py')
    frozen, firmware = output / 'disc', output / 'system'
    frozen.mkdir(); firmware.mkdir()
    # The private BIOS copy is an execution dependency, not a distributable asset.
    shutil.copyfile(bios, firmware / 'mpr-17933.bin')
    shutil.copyfile(track, frozen / track.name)
    staged_cue = re.sub(r'(^\s*FILE\s+)(?:"([^"]+)"|(\S+))', lambda m: m[1] + '"' + track.name + '"', cue_text, flags=re.M | re.I)
    (frozen / disc.name).write_text(staged_cue)
    elf_record = None
    if elf:
        shutil.copyfile(elf, output / 'program.elf')
        subprocess.run([objcopy, '-O', 'binary', str(output / 'program.elf'), str(output / 'program.bin')], check=True)
        if disc_program(frozen / disc.name) != (output / 'program.bin').read_bytes():
            raise ValueError('Disc A.BIN differs from the supplied linked ELF; capture refused')
        if elf.with_suffix('.map').is_file():
            shutil.copyfile(elf.with_suffix('.map'), output / 'program.map')
        elf_record = dict(source=str(elf), sha256=sha(output / 'program.elf'), program_sha256=sha(output / 'program.bin'), a_bin_matches=True)
    write_json(output / 'symbols.json', symbols)
    write_json(output / 'watches.json', watches)
    (output / 'watches.tsv').write_text(''.join(f"{w['name']}\t{w['address']:08x}\t{w['size']}\n" for w in watches))
    binary = output / 'headless_saturn'
    compile_cmd = ['gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-o', str(binary), str(output / 'headless_saturn.c'), '-ldl']
    subprocess.run(compile_cmd, check=True)
    env = os.environ.copy()
    env.update(WP1980_SYSTEM_DIRECTORY=str(firmware), WP1980_WATCH_FILE=str(output / 'watches.tsv'), WP1980_CANVAS=a.canvas,
               WP1980_RECORD_VIDEO='0' if a.no_video else '1', WP1980_CARTRIDGE=a.cartridge)
    command = [str(binary), str(core), str(frozen / disc.name), str(output), str(a.frames), str(a.capture_step), *a.press]
    write_json(output / 'command.json', dict(emulator=command, compile=compile_cmd, watches=watches,
        cartridge=a.cartridge, cartridge_core_option=CARTRIDGE_OPTIONS[a.cartridge]))
    start = time.monotonic()
    process = encoder = None
    encode_command = None
    try:
        with (output / 'emulator.log').open('wb') as log, (output / 'video-encode.log').open('wb') as video_log:
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=log, env=env, bufsize=0)
            header = read_header(process.stdout, process, min(60, a.wall_timeout))
            write_json(output / 'av.json', header)
            if not a.no_video:
                encode_command = ['ffmpeg', '-hide_banner', '-loglevel', 'warning', '-nostdin', '-n',
                    '-f', 'rawvideo', '-pixel_format', 'rgb24', '-video_size', a.canvas,
                    '-framerate', format(header['fps'], '.17g'), '-i', 'pipe:0', '-an',
                    '-c:v', 'libx264', '-preset', 'fast', '-crf', '18', '-pix_fmt', 'yuv420p',
                    '-threads:v', '2', '-fps_mode:v', 'passthrough', '-video_track_timescale', '200000',
                    '-movflags', '+faststart', str(output / 'native-video.mp4')]
                encoder = subprocess.Popen(encode_command, stdin=process.stdout, stdout=video_log, stderr=subprocess.STDOUT)
            process.stdout.close()
            process.wait(timeout=max(1, a.wall_timeout - (time.monotonic() - start)))
            if encoder:
                encoder.wait(timeout=60)
            if process.returncode:
                raise RuntimeError(f'Emulator exited {process.returncode}; inspect emulator.log')
            if encoder and encoder.returncode:
                raise RuntimeError(f'FFmpeg exited {encoder.returncode}; inspect video-encode.log')
    except BaseException:
        terminate(process); terminate(encoder)
        write_json(output / 'failure.json', dict(status='FAIL', wall_seconds=time.monotonic() - start,
                                                emulator_return_code=process.returncode if process else None))
        raise
    wall = time.monotonic() - start
    run = json.loads((output / 'run.json').read_text())
    if (run['cartridge'] != a.cartridge or run['cartridge_core_option'] != CARTRIDGE_OPTIONS[a.cartridge]
            or not run['cartridge_option_advertised'] or not run['cartridge_option_queried']):
        raise ValueError('Observed core cartridge setting differs from the requested configuration')
    core_cartridges = re.findall(r'\bCart:\s*([^\r\n]+?)\.\s*$', (output / 'emulator.log').read_text(), re.M)
    if not core_cartridges or core_cartridges[-1] != CARTRIDGE_DESCRIPTIONS[a.cartridge]:
        raise ValueError('Core cartridge initialization log does not confirm the requested hardware')
    audio = json.loads((output / 'audio.json').read_text())
    samples = [json.loads(line) for line in (output / 'samples.jsonl').read_text().splitlines()]
    with (output / 'frames.csv').open() as f:
        frame_records = list(csv.DictReader(f))
    if len(samples) != a.frames or len(frame_records) != a.frames:
        raise ValueError('Per-frame observations do not cover every retro_run')
    if run['video_callbacks'] != a.frames or any(int(row['video_callbacks']) != 1 for row in frame_records):
        raise ValueError('Expected exactly one native video callback per retro_run; no synthetic frames are accepted')
    if any(sample['emulator_frame'] != index for index, sample in enumerate(samples, 1)):
        raise ValueError('Native observation order is incomplete')
    pcm = output / 'audio_s16le.pcm'
    if pcm.stat().st_size != audio['sample_frames'] * 4:
        raise ValueError('PCM payload length disagrees with callback count')
    for index, rec in enumerate(frame_records):
        if int(rec['audio_sample_start']) != (int(frame_records[index - 1]['audio_sample_end']) if index else 0):
            raise ValueError('Per-frame PCM offsets are discontinuous')
    if int(frame_records[-1]['audio_sample_end']) != audio['sample_frames']:
        raise ValueError('Last PCM offset differs from total callback samples')
    with wave.open(str(output / 'native-audio.wav'), 'wb') as wav, pcm.open('rb') as source:
        wav.setnchannels(2); wav.setsampwidth(2); wav.setframerate(round(audio['sample_rate']))
        for block in iter(lambda: source.read(1048576), b''):
            wav.writeframesraw(block)
    for image in output.glob('frame_*.ppm'):
        with Image.open(image) as im:
            im.save(image.with_suffix('.png'))
        image.unlink()
    media = None
    mux_command = None
    if not a.no_video:
        mux_command = ['ffmpeg', '-hide_banner', '-loglevel', 'warning', '-nostdin', '-n',
            '-i', str(output / 'native-video.mp4'), '-f', 's16le', '-ar', str(round(audio['sample_rate'])),
            '-ac', '2', '-i', str(pcm), '-map', '0:v:0', '-map', '1:a:0', '-c:v', 'copy',
            '-c:a', 'aac', '-b:a', '192k', '-movflags', '+faststart', str(output / 'native-capture.mp4')]
        with (output / 'mux.log').open('wb') as log:
            subprocess.run(mux_command, stdout=log, stderr=subprocess.STDOUT, check=True)
        probe = ['ffprobe', '-v', 'error', '-count_frames', '-show_streams', '-show_format', '-of', 'json', str(output / 'native-capture.mp4')]
        media = json.loads(subprocess.check_output(probe, text=True))
        videos = [s for s in media['streams'] if s['codec_type'] == 'video']
        audios = [s for s in media['streams'] if s['codec_type'] == 'audio']
        if len(videos) != 1 or len(audios) != 1 or int(videos[0]['nb_read_frames']) != a.frames or run['encoded_frames'] != a.frames:
            raise ValueError('Encoded media does not contain every native video frame and one audio stream')
        write_json(output / 'ffprobe.json', media)
    postflight = [dict(path=str(f), size=f.stat().st_size, sha256=sha(f)) for f in source_inputs]
    if postflight != input_records:
        raise ValueError('Source disc changed during capture')
    if sha(core) != CORE_SHA:
        raise ValueError('Core binary changed during capture')
    watch_status = json.loads((output / 'watch_status.json').read_text())
    unmatched = [watch for watch in watch_status if watch['unmapped_samples']]
    provenance = dict(command=command, encode_command=encode_command, mux_command=mux_command,
        return_code=process.returncode, disc_files=input_records, postflight=postflight, disc_unchanged=True,
        elf=elf_record, core_sha256=CORE_SHA, core_path=str(core), harness_source_sha256=sha(output / 'headless_saturn.c'),
        wrapper_source_sha256=sha(output / 'capture_wrapper.py'), bios=dict(source=str(bios), size=len(bios_bytes),
        sha256=hashlib.sha256(bios_bytes).hexdigest(), md5=BIOS_MD5),
        settings=dict(region='North America', cartridge=a.cartridge, cartridge_core_option=run['cartridge_core_option'],
            cartridge_core_description=core_cartridges[-1], expansion_ram_bytes=4194304 if a.cartridge == '4MiB' else 0,
            cpucache_emumode='full', sh2_interleave='exact',
            sh2_jit=False, scu_jit=False, scsp_jit=False, frameskip=False, initial_scanline=0, last_scanline=239),
        memory_reads_only=True, save_states_loaded=False, injected_game_state=False, hardware_test=False,
        video_transform=dict(canvas=a.canvas, pad='right/bottom black only when smaller', scale=False, crop=False,
            temporal_interpolation=False, one_frame_per_retro_run=True, lossy_preview=True))
    write_json(output / 'provenance.json', provenance)
    write_json(output / 'identity.json', dict(a_bin_matches=bool(elf_record), elf=elf_record,
        frozen_disc={f.name: sha(f) for f in frozen.iterdir() if f.is_file()}))
    report = dict(status='PASS' if not unmatched else 'UNMAPPED_WATCHES',
        backend=f"{run['emulator']} {run['version']}", firmware='real supplied mpr-17933.bin',
        hardware_test=False, cartridge=a.cartridge, cartridge_core_option=run['cartridge_core_option'],
        expansion_ram_bytes=4194304 if a.cartridge == '4MiB' else 0, frames=a.frames, video_rate=run['emulated_refresh_hz'],
        emulated_seconds=a.frames / run['emulated_refresh_hz'], wall_seconds=wall,
        emulator_wall_seconds=run['host_elapsed_seconds'], host_frames_per_second=run['host_frames_per_second'],
        video_callbacks=run['video_callbacks'], duplicate_callbacks=run['duplicate_callbacks'],
        changed_framebuffers=run['changed_framebuffers'], audio=audio,
        watches=watches, unmapped_watches=unmatched, samples=samples,
        input_intervals=a.press, core_sha256=CORE_SHA, disc_sha256=sha(track),
        identity='A.BIN compared to objcopy of frozen ELF' if elf_record else 'Disc-only capture; no ELF identity assertion',
        limitations=['Emulator execution is not a physical Saturn performance measurement.',
            'Host elapsed time is independent of emulated time.',
            'MP4 is a lossy display artifact; counters, native PNGs and raw PCM are separate evidence.'])
    write_json(output / 'report.json', report)
    write_json(output / 'capture-commands.json', dict(compile=compile_cmd, emulator=command, encode=encode_command, mux=mux_command))
    print(json.dumps(dict(status=report['status'], output=str(output), frames=a.frames,
        cartridge=a.cartridge,
        video_rate=report['video_rate'], emulated_seconds=report['emulated_seconds'], wall_seconds=wall,
        audio_sample_frames=audio['sample_frames'], watches=len(watches), unmapped_watches=len(unmatched),
        last_observation={k: v for k, v in samples[-1].items() if not isinstance(v, str) or len(v) <= 64}), indent=2))
    return 0 if not unmatched else 1


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, RuntimeError, TimeoutError, subprocess.TimeoutExpired) as exc:
        raise SystemExit(str(exc))
