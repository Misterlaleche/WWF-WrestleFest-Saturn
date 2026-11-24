#!/usr/bin/env python3
"""Cold boot a Jo disc with stock Mednafen; read diagnostic RAM, never write it.

SDL dummy and the stock stdin remote interface avoid a display dependency.
The RAM host address is found by matching two immutable sections of this exact
ELF's raw image, then confirming the unique application diagnostic signature.
No emulator patches, checkpoints, or emulated clock changes are used.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
FIELDS = 'magic stage fields ticks draws max_gap debt max_debt fault_owner fault_dependency rng demo cartridge start_field finish_field audio_ok frame_tag g0166_before g0166_ready g0166_committed g0166_ack handoff_debt_total handoff_debt_max handoff_count'.split()
VIDEO = 'submitted_frames repeated_frames displayed_frames texture_bytes texture_transfers last_texture_bytes map_bytes palette_bytes bank_waits bank_wait_fields pipeline_stalled_fields'.split()
VIDEO_OVERLAP = 'overlap_requests overlap_drains overlap_drain_fields overlap_last_target overlap_last_shown_field overlap_last_drain_field'.split()
AUDIO = ('magic initialized fault pack_bytes commands_received commands_applied last_raw last_command queue_depth queue_peak queue_overflows priority_rejects physical_starts f1_skips cleanup_stops selected_voice resident_hits cache_hits cache_misses pcm_underruns decode_samples max_decode_samples_service decode_total_ms decode_max_ms resident_pcm_bytes dynamic_pcm_bytes service_max_gap_ms late_timer_ticks bank_changes active_bank_changes launch_max_delay_ms music_prepared_mask music_missing_mask music_errors music_command music_exhaustions music_natural_ends music_underruns unsupported_fades music_prepare_max_ms music_start_max_delay_ms music_pending_ms music_prefetch_mask music_prefetch_bytes invalid_bank_starts music_command_max_ms music_service_max_ms pcm_underrun_sample pcm_underrun_decoded pcm_underrun_elapsed_frames music_io_errors preserved_primes cooperative_services cooperative_max_ms command_queue_max_ms music_fades music_fade_completions music_fade_volume').split()
PROFILE = 'frame callbacks wait_slave slave vdp1_sort vdp1_copy vdp1_wait vsync vdp1_draw vblanks vdp1_commands polygons polygons3d vertices_count vertices3d polygons_3d clip3d light3d pixels3d audio postack input runtime irq commit automusic sprites'.split()
RENDER_PROFILE = 'parser plan retain begin tiles commands submit unused'.split()
CAMPAIGN = ('magic version bytes sequence policy_mode field tick round world active credited entries terminals '
    'rounds_seen worlds_seen last_owner post_pc credits_entered credits_completed credits_terminal '
    'capture_ready capture_field fault_owner fault_dependency events_written').split()
CAMPAIGN_EVENTS = {1:'entry', 2:'world', 3:'terminal', 4:'credits_enter', 5:'credits_done', 6:'capture_ready', 7:'fault'}
CAMPAIGN_BYTES = 868
CAMPAIGN_OFFSET = 0xa00

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()

def swap(b):
    out = bytearray(len(b)); out[::2] = b[1::2]; out[1::2] = b[::2]
    return bytes(out)

def symbols(elf):
    result = {}
    for line in subprocess.check_output(['nm', '-n', str(elf)], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3:
            result[fields[2].lstrip('_').split('.lto_priv.')[0]] = int(fields[0], 16)
    return result

def symbol_sizes(elf):
    result = {}
    for line in subprocess.check_output(['nm', '-S', '--defined-only', str(elf)], text=True).splitlines():
        fields = line.split()
        if len(fields) == 4:
            result[fields[3].lstrip('_').split('.lto_priv.')[0]] = int(fields[1], 16)
    return result

def state_sections(path):
    data = Path(path).read_bytes()
    if data[:2] == b'\x1f\x8b': data = gzip.decompress(data)
    if data[:8] != b'MDFNSVST': raise ValueError('Unexpected Mednafen state format')
    version, flags, width, height = struct.unpack_from('<4I', data, 16)
    if len(data) != (flags & 0x7fffffff): raise ValueError('Truncated state')
    at = 32 + width * height * 3
    sections = {}
    while at < len(data):
        name = data[at:at+32].split(b'\0', 1)[0].decode()
        length = struct.unpack_from('<I', data, at+32)[0]
        at += 36; end = at + length; fields = {}
        while at < end:
            n = data[at]; field = data[at+1:at+1+n].decode()
            size = struct.unpack_from('<I', data, at+1+n)[0]
            at += 1+n+4
            if at+size > end: raise ValueError('Invalid state boundary')
            fields[field] = data[at:at+size]; at += size
        sections[name] = fields
    return bool(flags & 0x80000000), sections

def locate_ram(pid, mem, raw, diagnostic):
    pattern = swap(raw[0x400:0x500])
    second = swap(raw[0x1800:0x1900])
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        parts = line.split(); permissions = parts[1]
        if permissions[:2] != 'rw': continue
        begin, end = [int(v, 16) for v in parts[0].split('-')]
        # Host read-only observation only. Skip enormous address reservations.
        if end-begin > 256 * 1024 * 1024: continue
        for chunk in range(begin, end, 2 * 1024 * 1024):
            try: data = os.pread(mem, min(2*1024*1024+256, end-chunk), chunk)
            except OSError: continue
            at = data.find(pattern)
            while at >= 0:
                base = chunk+at-0x4400
                try:
                    check = os.pread(mem, len(second), base+0x5800)
                    magic = swap(os.pread(mem, 4, base+diagnostic-0x06000000))
                    if check == second and magic == b'WJ01': return base
                except OSError: pass
                at = data.find(pattern, at+1)
    return None

def decode_campaign(data):
    """Decode the versioned test harness ABI, never adjacent game state."""
    if len(data) != CAMPAIGN_BYTES:
        raise ValueError('Incomplete campaign diagnostic')
    words = struct.unpack('>217I', data)
    result = dict(zip(CAMPAIGN, words))
    if words[:3] != (0x57464350, 1, CAMPAIGN_BYTES) or result['policy_mode'] not in (1, 2):
        raise ValueError('Unknown campaign diagnostic ABI')
    if result['sequence'] & 1:
        raise ValueError('Campaign diagnostic writer is active')
    count = result['events_written']; events = []
    for serial in range(max(0, count-32), count):
        start = 25 + (serial % 32)*6
        event = dict(zip(('kind','field','tick','round','world','value'), words[start:start+6]))
        event['kind'] = CAMPAIGN_EVENTS.get(event['kind'], event['kind'])
        event['serial'] = serial; events.append(event)
    result['events'] = events
    return result

def read_campaign(mem, address, attempts=3):
    """Require an unchanged even sequence around the complete word-swapped copy."""
    for _ in range(attempts):
        try:
            before = os.pread(mem, 4, address+12)
            if len(before) != 4 or int.from_bytes(swap(before), 'big') & 1:
                continue
            data = swap(os.pread(mem, CAMPAIGN_BYTES, address))
            after = os.pread(mem, 4, address+12)
            if before != after or data[12:16] != swap(before):
                continue
            return decode_campaign(data)
        except (OSError, ValueError):
            continue
    return None

def locate_campaign(pid, mem):
    """Find low RAM independently; its host placement need not adjoin high RAM."""
    signature = swap(struct.pack('>3I', 0x57464350, 1, CAMPAIGN_BYTES))
    candidates = set()
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        parts = line.split()
        if parts[1][:2] != 'rw': continue
        begin, end = (int(value, 16) for value in parts[0].split('-'))
        if end-begin > 256*1024*1024: continue
        for chunk in range(begin, end, 2*1024*1024):
            try: data = os.pread(mem, min(2*1024*1024+len(signature), end-chunk), chunk)
            except OSError: continue
            at = data.find(signature)
            while at >= 0:
                address = chunk+at; base = address-CAMPAIGN_OFFSET
                # A candidate must fit the complete 1 MiB low-RAM array.
                if address % 4 == 0 and begin <= base and base+0x100000 <= end:
                    if read_campaign(mem, address) is not None: candidates.add(address)
                at = data.find(signature, at+1)
    if len(candidates) > 1:
        raise RuntimeError('Ambiguous campaign diagnostic: multiple low-RAM candidates')
    return next(iter(candidates), None)

def capture_fault_reason(values):
    # fail(0, 0) used to leave the owner-only observer looking healthy while
    # main deliberately stopped all foreground work. Observe the stop flag.
    if values.get('failed'): return 'main entered failed state'
    if values.get('fault_owner') or (values.get('campaign') or {}).get('fault_owner'):
        return 'game diagnostic fault'
    if (values.get('audio') or {}).get('fault'): return 'audio diagnostic fault'
    if values.get('smpc_timing_fault'): return 'SMPC timing configuration fault'
    if (values.get('null_access') or {}).get('count'): return 'null access diagnostic fault'
    return None

def capture_stop_reason(args, values, title_state=None):
    if getattr(args, 'return_title', False):
        if title_state is None:
            raise ValueError('Title-return validation needs persistent observation state')
        boundary = ((values.get('campaign') or {}).get('capture_ready') if args.campaign
                    else values.get('terminal_completed'))
        front = values.get('frontend') or {}
        field = values.get('live_field')
        tick = front.get('arcade_ticks')
        if (not boundary or front.get('phase') not in (2, 6) or
                not 0x837e <= front.get('pc', 0) <= 0x8482 or field is None or tick is None):
            title_state.clear()
            return None
        if (title_state and tick == title_state['last_tick'] and
                6 < ((field-title_state['last_tick_field']) & 0xffffffff) < 0x80000000):
            title_state.clear()
        if not title_state:
            title_state.update(first_field=field, first_tick=tick, last_tick_field=field, confirmed=False)
        elif tick != title_state['last_tick']:
            title_state['last_tick_field'] = field
        title_state.update(last_field=field, last_tick=tick)
        # Physical VBlank alone also advances while main has stopped. Require
        # progress in the original title controller, then inspect movie frames.
        elapsed_fields = (field-title_state['first_field']) & 0xffffffff
        elapsed_ticks = (tick-title_state['first_tick']) & 0xffffffff
        if 60 <= elapsed_fields < 0x80000000 and 30 <= elapsed_ticks < 0x80000000:
            title_state['confirmed'] = True
            return 'result completed and title observed for 60 fields'
        return None
    if args.campaign:
        if (values.get('campaign') or {}).get('capture_ready'):
            return 'campaign credits and terminal return completed'
        return None
    if args.stop_ticks and values['ticks'] >= args.stop_ticks: return 'requested tick threshold'
    if args.stop_stage and values['stage'] == args.stop_stage: return 'requested stage'
    if args.stop_post_combat and values.get('terminal_completed', 0): return 'frontend result owner completed'
    return None

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cue', type=Path, required=True)
    p.add_argument('--elf', type=Path, required=True)
    p.add_argument('--raw', type=Path, required=True)
    p.add_argument('--bios', type=Path, required=True)
    p.add_argument('--cart', choices=['auto', 'extram4', 'none'], default='extram4',
                   help='Use auto to verify the disc header cart identity.')
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--emulator', type=Path, required=True)
    p.add_argument('--libs', type=Path, required=True)
    p.add_argument('--seconds', type=float, default=120)
    p.add_argument('--stop-ticks', type=int, default=0)
    p.add_argument('--stop-stage', type=int, default=0)
    p.add_argument('--stop-post-combat', action='store_true', help='Stop after the captured frontend result owner returns; preserves WFT1 terminal snapshot')
    p.add_argument('--campaign', action='store_true',
                   help='Test harness only: read WFCP events and stop at capture_ready after complete credits')
    p.add_argument('--return-title', action='store_true',
                   help='Continue beyond the completed result until the original title advances for 60 observed fields')
    p.add_argument('--cold-check', action='store_true',
                   help='Validate inputs and record the cold-boot command without starting the emulator')
    p.add_argument('--video', action='store_true')
    p.add_argument('--video-codec', choices=('cscd', 'png'), default='cscd',
                   help='Lossless recording codec; png trades host CPU for smaller long-campaign captures')
    p.add_argument('--fast', action='store_true')
    p.add_argument('--backup-file', type=Path,
                   help='Seed this fresh private profile with an existing BIOS battery file; never loads a machine state')
    a = p.parse_args()
    if a.campaign and (a.stop_ticks or a.stop_stage or a.stop_post_combat):
        p.error('--campaign cannot be combined with partial-game stop options')
    if a.return_title and (a.stop_ticks or a.stop_stage or a.stop_post_combat):
        p.error('--return-title cannot be combined with partial-game stop options')
    out = a.out.resolve(); out.mkdir(parents=True, exist_ok=False)
    home = out/'profile'; home.mkdir()
    syms = symbols(a.elf); raw = a.raw.read_bytes()
    sizes = symbol_sizes(a.elf)
    video_bytes = sizes.get('wf_jo_video_stats')
    if video_bytes not in (len(VIDEO)*4, (len(VIDEO)+len(VIDEO_OVERLAP))*4):
        p.error('Unknown video diagnostic size; refusing to read adjacent globals')
    video_fields = VIDEO + VIDEO_OVERLAP if video_bytes > len(VIDEO)*4 else VIDEO
    if a.stop_post_combat and 'wf_jo_terminal_completed' not in syms:
        p.error('--stop-post-combat requires a WF_CAPTURE or profiled full-frontend ELF')
    if a.return_title and not all(name in syms for name in ('wf_jo_terminal_completed', 'wf_jo_front_diagnostic')):
        p.error('--return-title requires a captured full-frontend ELF')
    if a.campaign and not all(name in syms for name in
            ('wf_campaign_core_run', 'wf_campaign_key', 'wf_campaign_observer_callback', 'wf_jo_terminal_completed')):
        p.error('--campaign requires the explicit Saturn campaign validation harness ELF')
    command = [str(a.emulator.resolve()), '-remote', 'WFREMOTE:', '-video.driver', 'softfb',
        '-video.frameskip', '0', '-video.fs', '0', '-ss.correct_aspect', '0',
        '-ss.xscale', '1', '-ss.yscale', '1', '-ss.slstart', '0', '-ss.slend', '239',
        '-sound.driver', 'dummy', '-sound', '1', '-sound.rate', '44100',
        '-ss.bios_na_eu', str(a.bios.resolve()), '-ss.region_autodetect', '0',
        '-ss.region_default', 'na', '-ss.bios_sanity', '1', '-ss.cart', a.cart,
        '-ss.dbg_cem', 'full', '-filesys.state_comp_level', '0', '-autosave', '1',
        '-nothrottle', '1' if a.fast else '0', '-soundrecord', str(out/'native.wav')]
    if a.video:
        command += ['-qtrecord.vcodec', a.video_codec,
                    '-qtrecord.w_double_threshold', '0', '-qtrecord.h_double_threshold', '0',
                    '-qtrecord', str(out/'native.mov')]
    command += [str(a.cue.resolve())]
    backup_seed = None
    if a.backup_file is not None:
        # A private profile makes the filename independent of this test disc's
        # checksum while keeping normal Mednafen battery I/O and BIOS access.
        (home/'sav').mkdir()
        shutil.copyfile(a.backup_file, home/'sav'/(a.cue.stem+'.bkr'))
        command[-1:-1] = ['-filesys.fname_sav', '%f.%x']
        backup_seed = {'file': str(a.backup_file.resolve()), 'sha256': sha(a.backup_file)}
    env = dict(os.environ, MEDNAFEN_HOME=str(home), SDL_VIDEODRIVER='dummy',
               SDL_AUDIODRIVER='dummy', LD_LIBRARY_PATH=str(a.libs.resolve()))
    env.pop('LD_PRELOAD', None)
    report = dict(command=command, capture_tool_sha256=sha(Path(__file__)),
        emulator_sha256=sha(a.emulator), elf_sha256=sha(a.elf),
        raw_sha256=sha(a.raw), cue_sha256=sha(a.cue), bios_sha256=sha(a.bios),
        cold_boot=True, guest_writes=False, state_loaded=False, hardware_verified=False, emulator_started=False,
        symbols=syms, diagnostic_symbol_sizes={'wf_jo_video_stats': video_bytes}, trace=[], status='STARTING')
    if backup_seed is not None: report['backup_seed'] = backup_seed
    if a.campaign:
        report['campaign'] = dict(validation_only=True, guest_address='0x00200a00', version=1,
            bytes=CAMPAIGN_BYTES, stop='capture_ready', events_capacity=32,
            limitation='Read-only polling can miss overwritten ring events; counters remain cumulative.')
    # Disc identity is recorded before execution, including the complete raw image.
    import shlex
    report['disc_images'] = [dict(file=t[1], sha256=sha(a.cue.parent/t[1]))
        for line in a.cue.read_text().splitlines() if (t:=shlex.split(line)) and t[0]=='FILE']
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    if a.cold_check:
        report['status'] = 'COLD_CHECKED'
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        print(json.dumps({'status':report['status'], 'command':command}, indent=2))
        return
    proc = None; mem = None; ram = None; campaign_address = None
    title_state = {}
    if a.return_title:
        report['title_return'] = {'required': True, 'minimum_observed_fields': 60, 'confirmed': False}
    controls = out/'control.txt'; controls.write_text('')
    seen = 0
    def send(command):
        proc.stdin.write((command+'\n').encode()); proc.stdin.flush()
    def watch(name, fields):
        address = syms[name]-0x06000000
        values = struct.unpack('>'+str(len(fields))+'I', swap(os.pread(mem, len(fields)*4, ram+address)))
        return dict(zip(fields, values))
    def profile_sample(values):
        if 'jo_profile_cpu' not in syms:return
        sample={k:values.get(k) for k in ('wall_seconds','stage','fields','ticks','live_field')}
        sample['cpu_frt_sum']=watch('jo_profile_cpu',PROFILE)
        if 'wf_jo_render_profile' in syms:
            sample['render_frt_sum']=watch('wf_jo_render_profile',RENDER_PROFILE)
        sample['limitation']='FRT intervals wrap at 65536; a single phase over about78ms is undercounted. Live read is not an atomic guest snapshot.'
        report.setdefault('profile_samples',[]).append(sample)
    try:
        with (out/'mednafen.log').open('wb') as log:
            proc = subprocess.Popen(command, env=env, stdin=subprocess.PIPE, stdout=log, stderr=subprocess.STDOUT)
            report['emulator_started'] = True
            start = time.monotonic(); next_search = 1; next_campaign_search = 1; next_status = 5
            while time.monotonic()-start < a.seconds:
                elapsed = time.monotonic()-start
                if proc.poll() is not None: raise RuntimeError(f'Mednafen exited {proc.returncode}')
                if mem is None: mem = os.open(f'/proc/{proc.pid}/mem', os.O_RDONLY)
                if ram is None and elapsed >= next_search:
                    ram = locate_ram(proc.pid, mem, raw, syms['wf_jo_diagnostic'])
                    next_search = elapsed+3
                    if ram is not None: report['host_ram_base'] = hex(ram)
                if a.campaign and campaign_address is None and elapsed >= next_campaign_search:
                    campaign_address = locate_campaign(proc.pid, mem)
                    next_campaign_search = elapsed+3
                    if campaign_address is not None:
                        report['campaign']['host_address'] = hex(campaign_address)
                commands = controls.read_text().splitlines()
                stop = False
                for c in commands[seen:]:
                    if c.strip() == 'exit': stop = True
                    elif c.strip(): send(c)
                seen = len(commands)
                if stop: report['stop_reason']='control exit'; break
                values = dict(wall_seconds=round(elapsed, 6))
                if ram is not None:
                    values.update(watch('wf_jo_diagnostic', FIELDS))
                    if 'wf_jo_load_diagnostic' in syms:
                        values['loading'] = watch('wf_jo_load_diagnostic', 'bytes total field'.split())
                    values['video'] = watch('wf_jo_video_stats', video_fields)
                    values['audio'] = watch('wf_jo_audio_diagnostic', AUDIO)
                    if 'wf_jo_front_diagnostic' in syms:
                        values['frontend'] = watch('wf_jo_front_diagnostic',
                            'phase pc arcade_ticks physical_fields raster_irqs vblank_irqs commits debt'.split())
                    if 'wf_jo_asset_profile' in syms:
                        values['asset_profile'] = watch('wf_jo_asset_profile', 'count bytes total_ms max_ms last_ms'.split())
                    if 'jo_slave_user_added' in syms and 'jo_slave_user_done' in syms:
                        values['slave_jobs'] = {
                            'user_added': watch('jo_slave_user_added', ['count'])['count'],
                            'user_done': watch('jo_slave_user_done', ['count'])['count']}
                        if 'jo_slave_running' in syms:
                            values['slave_jobs']['running'] = watch('jo_slave_running', ['running'])['running']
                    if 'wf_jo_coin_profile' in syms:
                        values['coin_profile'] = watch('wf_jo_coin_profile', 'calls irq2s total_frt max_frt total_ms max_ms snapshots last_frt'.split())
                    if 'wf_jo_scene_prepare_stats' in syms:
                        values['scene_prepare'] = watch('wf_jo_scene_prepare_stats', 'begun steps decoded_bytes completed hits cancelled errors total_ms max_step_ms'.split())
                    if 'wf_jo_terminal_completed' in syms:
                        values['terminal_completed'] = watch('wf_jo_terminal_completed', ['completed'])['completed']
                    if 'records_save' in syms and sizes.get('records_save') == 164:
                        # Pinned SH-2 ABI: two pointers, 148-byte WFS1 payload,
                        # status enum and writable/cached bytes. Observe only.
                        address = ram+syms['records_save']-0x06000000
                        record = swap(os.pread(mem, 164, address))
                        values['records'] = {'status': int.from_bytes(record[156:160], 'big'),
                                             'writable': record[160], 'cached': record[161]}
                    for name, label in [('failed', 'failed'), ('selected_royal', 'selected_royal'),
                                        ('jo_smpc_timing_fault', 'smpc_timing_fault')]:
                        if name in syms and sizes.get(name) == 4:
                            values[label] = watch(name, ['value'])['value']
                    if 'jo_core_null_access' in syms and sizes.get('jo_core_null_access') == 12:
                        values['null_access'] = watch('jo_core_null_access', ['count', 'pc', 'pr'])
                    values['live_field'] = int.from_bytes(swap(os.pread(mem,4,ram+syms['jo_core_vblank_count']-0x06000000)),'big')
                    if a.campaign:
                        values['campaign'] = read_campaign(mem, campaign_address) if campaign_address is not None else None
                    report['trace'].append(values)
                    fault_reason = capture_fault_reason(values)
                    if fault_reason:
                        report['stop_reason']=fault_reason; report['status']='FAULT'; break
                    reason = capture_stop_reason(a, values, title_state)
                    if a.return_title:
                        report['title_return'] = {'required': True, 'minimum_observed_fields': 60,
                                                  'confirmed': False, **title_state}
                    if reason:
                        report['stop_reason'] = reason; break
                if elapsed >= next_status:
                    if ram is not None:profile_sample(values)
                    print(json.dumps(values), flush=True)
                    (out/'report-progress.json').write_text(json.dumps(report, indent=2)+'\n')
                    next_status += 5
                time.sleep(.05)
            if ram is not None:
                profile_sample(values)
                (out/'WorkRAMH-live.bin').write_bytes(swap(os.pread(mem,0x100000,ram)))
                if 'records_save' in syms and sizes.get('records_save') == 164:
                    (out/'records-context.bin').write_bytes(swap(os.pread(mem,164,
                        ram+syms['records_save']-0x06000000)))
            if campaign_address is not None:
                (out/'WorkRAML-live.bin').write_bytes(swap(os.pread(mem,0x100000,campaign_address-CAMPAIGN_OFFSET)))
            send('exit'); proc.wait(timeout=20)
            report['emulator_exit_code']=proc.returncode
            report['wall_seconds']=time.monotonic()-start
            if report['status'] != 'FAULT':
                if a.return_title and not title_state.get('confirmed'):
                    report['status'] = 'TITLE_RETURN_INCOMPLETE'
                    if 'stop_reason' not in report: report['stop_reason'] = 'title return not confirmed before capture ended'
                elif a.campaign:
                    ready = any((sample.get('campaign') or {}).get('capture_ready') for sample in report['trace'])
                    report['status'] = 'CAPTURED' if ready else 'CAMPAIGN_INCOMPLETE'
                    if 'stop_reason' not in report: report['stop_reason'] = 'campaign wall-time limit before capture_ready'
                else: report['status']='CAPTURED' if ram is not None else 'RAM_NOT_FOUND'
            states = list((home/'mcs').glob('*.mca'))
            if states:
                big, sections = state_sections(states[0])
                for bank, section, field in [('WorkRAMH','MAIN','WorkRAMH'),('WorkRAML','MAIN','WorkRAML'),('VDP1','VDP1','VRAM'),('SCSP','SCSP','RAM')]:
                    data = sections[section][field]
                    (out/(bank+'-state.bin')).write_bytes(data if big else swap(data))
                report['pcs']={n:hex(int.from_bytes(sections[n]['PC'],'big' if big else 'little')) for n in ('SH2-M','SH2-S','M68K')}
    finally:
        if mem is not None: os.close(mem)
        if proc is not None and proc.poll() is None:
            try: send('exit'); proc.wait(timeout=10)
            except Exception: proc.terminate(); proc.wait(timeout=10)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:report.get(k) for k in ('status','stop_reason','wall_seconds','pcs')}, indent=2))

if __name__ == '__main__': main()
