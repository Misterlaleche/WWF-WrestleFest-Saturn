#!/usr/bin/env python3
"""Build private Jo audio from the user's identified WWF WrestleFest ROMs.

SFX.DAT preserves the audited Audio 2 command/ADPCM pack. Music is the corrected
full-board software reference (original Z80, Nuked-OPM external clock / 2,
ROM-addressed OKI), converted losslessly to PCM16 mono WAV. These finite captures
are candidates, not physical-PCB recordings or invented seamless loops.
"""
from pathlib import Path
import argparse
import array
import concurrent.futures
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import wave
import zipfile

REFERENCE = Path(__file__).resolve().parent / 'audio_reference'
PIN = '6d6a5705c87e9042529116415770f4bc74c0e2e8'
_RESAMPLER_IDENTITY = None

def reference_module():
    spec = importlib.util.spec_from_file_location('audio2_resources', REFERENCE / 'generate_resources.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def build_sfx(sound, samples):
    return reference_module().generate(sound, samples)



def resampler_identity():
    global _RESAMPLER_IDENTITY
    if _RESAMPLER_IDENTITY is None:
        executable = shutil.which(os.environ.get('FFMPEG', 'ffmpeg'))
        if not executable:
            raise FileNotFoundError('ffmpeg with libsoxr support is required')
        version = subprocess.run([executable, '-version'], check=True, capture_output=True, text=True).stdout.splitlines()[0]
        build = subprocess.run([executable, '-buildconf'], check=True, capture_output=True, text=True).stdout
        if '--enable-libsoxr' not in build:
            raise RuntimeError('ffmpeg was built without libsoxr')
        _RESAMPLER_IDENTITY = dict(executable=executable, sha256=digest_file(Path(executable)), version=version)
    return dict(_RESAMPLER_IDENTITY)


def resample_pcm16be(source, target, in_rate=176400, out_rate=44100):
    """Band-limit a raw mono signed-16 big-endian capture before rate conversion.

    The renderer observes the original chip DACs at a high timeline rate.  A
    direct phase pick aliases energy above the target Nyquist limit.  libsoxr is
    used offline with no dither; Python writes the stable WAV container.
    """
    source = Path(source)
    target = Path(target)
    if in_rate <= out_rate or in_rate % out_rate or source.stat().st_size & 1:
        raise ValueError('invalid PCM resample geometry')
    expected = source.stat().st_size // 2 * out_rate // in_rate
    with tempfile.TemporaryDirectory(prefix='wf-bandlimit-') as temporary:
        raw = Path(temporary) / 'audio.s16le'
        identity = resampler_identity()
        command = [identity['executable'], '-hide_banner', '-loglevel', 'error',
            '-nostdin', '-y', '-f', 's16be', '-ar', str(in_rate), '-ac', '1', '-i', str(source),
            '-af', 'aresample=%d:resampler=soxr:precision=33:cheby=0:dither_method=none' % out_rate,
            '-f', 's16le', '-acodec', 'pcm_s16le', str(raw)]
        subprocess.run(command, check=True)
        data = raw.read_bytes()
    if len(data) != expected * 2:
        raise ValueError('band-limited resampler returned %d frames, expected %d' %
                         (len(data)//2, expected))
    with wave.open(str(target), 'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(out_rate)
        wav.writeframes(data)
    return dict(engine='ffmpeg-libsoxr', precision=33, dither='none',
                input_rate=in_rate, output_rate=out_rate, frames=expected, tool=identity)

def music_plan(profile, seconds):
    if not 1 <= seconds <= 1800:
        raise ValueError('music capture must be 1..1800 seconds')
    if profile == 'none':
        return {}
    # Native TAG progression and post-match owners include07, special card0D,
    # title presenter10, result05/09, and credits11. The unskipped original
    # credits issue1F after2453 arcade ticks (~42.7s), so120s leaves headroom
    # without spending ten minutes of CD space there. No combat cue is cut.
    short=min(seconds,120)
    plan = {4:seconds,5:12,6:seconds,7:seconds,8:8,9:12,10:seconds,
            11:seconds,12:seconds,13:short,15:seconds,16:short,17:short}
    if profile in ('arcade','all'):
        # Complete cabinet also emits01 on title timeout. Include the only
        # remaining music command12 as finite catalog coverage; its gameplay
        # callsite has not been established by the native-source audit.
        plan.update({1:seconds,2:seconds,3:seconds,14:seconds,18:short})
    if profile == 'all':
        for command in range(1,19):
            plan.setdefault(command, 12 if command in (5,9) else min(seconds,120))
    elif profile not in ('tag','arcade'):
        raise ValueError('unknown music profile')
    return dict(sorted(plan.items()))

def digest_file(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()

def verify_sources():
    manifest = json.loads((REFERENCE / 'PROVENANCE.json').read_text())
    for name, identity in manifest['files'].items():
        if digest_file(REFERENCE / name) != identity['sha256']:
            raise ValueError('reference source identity mismatch: ' + name)
    return manifest

def compile_renderer(target):
    subprocess.run([os.environ.get('CC', 'cc'), '-O3', '-std=c99',
        '-I' + str(REFERENCE), str(REFERENCE / 'capture_board_replay.c'),
        str(REFERENCE / 'z80.c'), str(REFERENCE / 'opm.c'), '-lm', '-o', str(target)], check=True)

def render_track(executable, work, out, evidence, command, seconds):
    folder = work / ('music_%02x' % command)
    folder.mkdir()
    (folder / 'schedule.csv').write_text('0,%d\n' % command)
    started = time.monotonic()
    subprocess.run([str(executable), str(work / 'SOUND.ROM'), str(work / 'SAMPLES.ROM'),
        str(folder / 'schedule.csv'), str(seconds), str(folder / 'trace.csv'),
        str(folder / 'audio.pcm'), str(folder / 'capture.json'),
        str(folder / 'deliveries.csv'), 'full', '176400'], check=True)
    capture = json.loads((folder / 'capture.json').read_text())
    if not capture['complete'] or capture.get('output_rate') != 176400 or capture['frames'] != seconds * 176400 or capture['invalid_sample_starts']:
        raise ValueError('full-board capture incomplete: %02x' % command)
    filename = 'MU%02X.WAV' % command
    temporary = folder / filename
    conversion = resample_pcm16be(folder / 'audio.pcm', temporary, 176400, 44100)
    last_nonzero = -1
    frame = 0
    with wave.open(str(temporary), 'rb') as source:
        while True:
            block = source.readframes(1 << 18)
            if not block:
                break
            values = array.array('h', block)
            if sys.byteorder == 'big':
                values.byteswap()
            for i in range(len(values)-1, -1, -1):
                if values[i]:
                    last_nonzero = frame + i
                    break
            frame += len(values)
    row = dict(command=command, filename=filename, seconds=seconds,
        frames=conversion['frames'], capture_frames=capture['frames'], bytes=temporary.stat().st_size,
        sha256=digest_file(temporary), pcm16be_sha256=digest_file(folder / 'audio.pcm'),
        trace_sha256=digest_file(folder / 'trace.csv'), resampler=conversion,
        last_nonzero_frame=last_nonzero,
        behavior='finite natural-end capture' if command in (5,8,9) else 'finite excerpt; stop at EOF, no loop',
        capture=capture, wall_seconds=round(time.monotonic()-started, 3))
    # replace is atomic on this filesystem; a failed render leaves prior files intact.
    staged = out.parent / ('.audio-' + filename + '.tmp')
    shutil.copyfile(temporary, staged)
    staged.replace(out / filename)
    (evidence / ('MU%02X.json' % command)).write_text(json.dumps(row, indent=2) + '\n')
    print('%s: %d seconds, %d bytes, peak %d, clipped %d' %
        (filename,seconds,row['bytes'],capture['peak'],capture['clipped_frames']), flush=True)
    return row

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom-zip', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--evidence', required=True, type=Path)
    p.add_argument('--music', choices=('none','tag','arcade','all'), default='arcade')
    p.add_argument('--seconds', type=int, default=600)
    p.add_argument('--jobs', type=int, default=2)
    p.add_argument('--reuse', action='store_true', help='Reuse outputs only when pinned input/source identities, duration and WAV hash match')
    args = p.parse_args()
    if not 1 <= args.jobs <= 4:
        p.error('--jobs must be 1..4')
    plan = music_plan(args.music,args.seconds)
    if sum(plan.values()) * 88200 > 650_000_000:
        p.error('selected music exceeds 650 MB audio budget for a 700 MB CD-R')
    sources = verify_sources()
    with zipfile.ZipFile(args.rom_zip) as z:
        sound, samples = z.read('31a11-2.ic42'), z.read('31j10.ic73')
    data = build_sfx(sound,samples)
    args.out.mkdir(parents=True, exist_ok=True)
    args.evidence.mkdir(parents=True, exist_ok=True)
    staged = args.out.parent / '.audio-SFX.DAT.tmp'
    staged.write_bytes(data['pack'])
    staged.replace(args.out / 'SFX.DAT')
    catalog = dict(source_sound_sha256=hashlib.sha256(sound).hexdigest(),
        source_samples_sha256=hashlib.sha256(samples).hexdigest(),
        format='WFSX v1', timer_a_numerator=3579545, timer_a_denominator=64*253,
        pcm_rate=8000, pcm_bits=16, pcm_byte_order='big', signal_reset=0, step_index_reset=0,
        bank_policy={'0':'select global bank 0','1':'select global bank 1','2':'retain global bank'},
        event_flags={'0':'new physical note','1':'F1 continuation: no stop/start','3':'new physical note followed by F1'},
        logical_voices=3, commands=data['commands'], samples=data['samples'])
    (args.evidence / 'sfx-catalog.json').write_text(json.dumps(catalog,indent=2)+'\n')
    manifest = dict(audio2_commit=PIN, host_sources=sources, resampler=resampler_identity() if plan else None,
        source_sound_sha256=hashlib.sha256(sound).hexdigest(),
        source_samples_sha256=hashlib.sha256(samples).hexdigest(),
        sfx=dict(filename='SFX.DAT',bytes=len(data['pack']),sha256=hashlib.sha256(data['pack']).hexdigest(),
            commands=98,events=165,physical_starts=100,distinct_samples=95,
            decoded_pcm16be_bytes=len(data['pcm16be']),
            decoded_pcm16be_sha256=hashlib.sha256(data['pcm16be']).hexdigest()),
        music=[], acceptance='software-reference candidate; hardware sound and gameplay timing not accepted',
        limitations=['Finite captures stop at EOF; no imposed loops.',
            'Live 0x1F fade is not reproduced by a static isolated music capture.',
            'Runtime SFX cannot retroactively change a playing predecoded sample when the global OKI bank changes.',
            'Music changes preserve the selected command but not the previous YM chip state.'])
    print('SFX.DAT: %d bytes, all 98 commands and 95 distinct payloads' % len(data['pack']), flush=True)
    reuse = {}
    previous_path = args.evidence / 'audio-manifest.json'
    if args.reuse and previous_path.exists():
        previous = json.loads(previous_path.read_text())
        if all(previous.get(k)==manifest[k] for k in ('audio2_commit','host_sources','source_sound_sha256','source_samples_sha256')):
            for row in previous.get('music',[]):
                path = args.out / row['filename']
                if (plan.get(row['command'])==row['seconds'] and path.is_file() and
                    path.stat().st_size==row['bytes'] and digest_file(path)==row['sha256'] and
                    row['capture']['complete'] and row['capture']['profile']=='full' and
                    row['frames']==row['seconds']*44100):
                    reuse[row['command']]=row
                    manifest['music'].append(row)
                    print('Verified existing '+row['filename'], flush=True)
    if plan:
        with tempfile.TemporaryDirectory(prefix='wf-jo-audio-') as temporary:
            work = Path(temporary)
            (work / 'SOUND.ROM').write_bytes(sound)
            (work / 'SAMPLES.ROM').write_bytes(samples)
            executable = work / 'capture_board_replay'
            compile_renderer(executable)
            with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
                futures = [pool.submit(render_track,executable,work,args.out,args.evidence,c,s)
                           for c,s in plan.items() if c not in reuse]
                for future in concurrent.futures.as_completed(futures):
                    manifest['music'].append(future.result())
    manifest['music'].sort(key=lambda row:row['command'])
    manifest['total_audio_bytes'] = len(data['pack']) + sum(row['bytes'] for row in manifest['music'])
    (args.evidence / 'audio-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Audio payload: %d bytes' % manifest['total_audio_bytes'], flush=True)

if __name__ == '__main__':
    main()
