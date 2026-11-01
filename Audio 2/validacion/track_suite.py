"""Prepare/build/plan isolated full-score tests. Never creates a disc image.

Preparing, building and planning do not load an emulator. `run --execute` is a
separate, deliberate operation after the final candidate has been approved.
"""
import argparse
import csv
import hashlib
import json
import math
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
CONTROL = HERE.parent
REPAIR = CONTROL.parent
RUNTIME = REPAIR.parents[2]
MODULE = Path('src/audio/native/wf_music_module.c')
HZ = 3579545


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def catalogue(base):
    """Derive intro + one full observed loop; never mistake an idle loop for music."""
    scores = json.loads((base/'template-analysis.json').read_text())['scores']
    handlers = {i: [] for i in range(1, 19)}
    with (base/'handlers.csv').open(newline='') as f:
        for row in csv.DictReader(f):
            handlers[int(row['cmd'])].append(int(row['cycle'])/HZ)
    first_inactive = {}
    with (base/'states.csv').open(newline='') as f:
        for row in csv.DictReader(f):
            if row['active'] == '0':
                first_inactive.setdefault(int(row['cmd']), int(row['cycle'])/HZ)
    result = []
    for score in scores:
        track = score['cmd']; loop = score['loop']; ending = first_inactive.get(track)
        row = dict(track=track, command_hex=f'{track:02X}', host_first_inactive_seconds=ending,
                   host_loop=loop, required_end_of_track=ending is not None)
        if ending is not None:
            covered = ending
            seconds = 30
            kind = 'one_shot_natural_end'
        elif track in (11, 12, 17):
            covered = 600; seconds = 605; kind = 'finite_600_second_observation_no_loop_proof'
        elif track == 4:
            # catalogue180 ends before the complete intro + 166.973s period.
            long_file = CONTROL/'score-analysis/long-4/handlers.csv'
            seen = {}; found = None
            with long_file.open(newline='') as f:
                for item in csv.DictReader(f):
                    t = int(item['cycle'])/HZ
                    if item['ramhash'] in seen:
                        first = seen[item['ramhash']]
                        found = dict(prefix_seconds=first, period_seconds=t-first,
                                     first_repeat_seconds=t)
                        break
                    seen[item['ramhash']] = t
            if found is None:
                raise ValueError('Track04 full-loop evidence is missing')
            row['host_long_loop'] = found
            covered = found['first_repeat_seconds']
            seconds = math.ceil(covered + max(5, covered*.1))
            kind = 'intro_plus_one_observed_loop'
        elif loop is not None:
            intro = handlers[track][loop['prefix']-1]
            covered = intro + loop['seconds']
            row['host_intro_seconds'] = intro
            seconds = max(75, math.ceil(covered + max(5, covered*.1)))
            kind = 'intro_plus_one_observed_loop'
        else:
            raise ValueError(f'No duration evidence for {track}')
        row.update(coverage_kind=kind, host_required_score_seconds=covered,
                   hold_seconds=seconds, checkpoint_frames=60,
                   # Worst-case first command boot allowance plus a final checkpoint.
                   emulated_frames=max(9000,4200+seconds*60+120))
        result.append(row)
    assert [r['track'] for r in result] == list(range(1, 19))
    return result


def patch_module(source, track, mode, seconds):
    if not 1 <= track <= 18:
        raise ValueError('Track must be in 1..18')
    if not 0 < seconds*60 < 65536:
        raise ValueError('Hold horizon must fit one unsigned master-frame wrap')
    # This one insertion is the ONLY source change in each case repository.
    old = '   if(!command||(command>=1&&command<=18)){'
    if source.count(old) != 1:
        raise ValueError('Final module command-dispatch anchor changed; review required')
    declaration = '''
/* TEST ONLY: no build in this directory is a release MUSIC.ELF. */
volatile uint32_t wf_test_suppressed_music,wf_test_suppressed_stop,wf_test_suppressed_fade;
volatile uint16_t wf_test_started,wf_test_start_frame,wf_test_applied_track;
'''
    anchor = 'static wf_music_seq_t boot_snapshot;'
    if source.count(anchor) != 1:
        raise ValueError('Final module snapshot anchor changed; review required')
    source = source.replace(anchor, declaration + anchor)
    if mode == 'hold-first':
        dispatch = f'''   bool wf_test_suppress=wf_test_started&&
    (uint16_t)(SHARED.master_frame-wf_test_start_frame)<{seconds*60}u;
   if(wf_test_suppress){{
    if(!command)++wf_test_suppressed_stop;
    else if(command==0x1f)++wf_test_suppressed_fade;
    else if(command>=1&&command<=18)++wf_test_suppressed_music;
    else{{fault(7);return;}}
   }}else{{
    if(command>=1&&command<=18){{
     if(!wf_test_started){{wf_test_started=1;wf_test_start_frame=SHARED.master_frame;}}
     command={track}u;wf_test_applied_track={track}u;
    }}
{old}'''
        source = source.replace(old, dispatch)
        end = '   SHARED.consumer=(uint16_t)(consumer+1u);'
        if source.count(end) != 1:
            raise ValueError('Final module consume anchor changed')
        source = source.replace(end, '   } /* TEST ONLY hold-first dispatch */\n'+end)
    else:
        source = source.replace(old, f'''   if(command>=1&&command<=18){{command={track}u;wf_test_applied_track={track}u;
    if(!wf_test_started){{wf_test_started=1;wf_test_start_frame=SHARED.master_frame;}}}}
{old}''')
    # Assertions protect STOP/fade paths against an accidental global substitution.
    assert 'if(playing&&!wf_music_seq_send(&sequence,0x1f))' in source
    assert 'playing=command!=0;' in source
    return source


def prepare(a):
    if a.out.exists():
        raise ValueError('Output already exists; use a fresh directory to preserve evidence')
    repo = a.repo.resolve(); out = a.out.resolve()
    if not repo.is_relative_to(CONTROL.resolve()):
        raise ValueError('Use an isolated final experiment repository, never production')
    required = [MODULE, Path('tools/audio/native_probe/build_music_module.py')]
    for rel in required:
        if not (repo/rel).is_file(): raise ValueError(f'Missing final candidate file: {rel}')
    rows = catalogue(a.catalogue.resolve())
    source = (repo/MODULE).read_text()
    # Freeze the builder and all its sources once; mutable upstream experiments
    # cannot silently change later case builds.
    for folder in ('src/audio/native', 'src/runtime/include', 'tools/audio/native_probe'):
        shutil.copytree(repo/folder, out/'frozen-repo'/folder)
    files = {p.relative_to(out/'frozen-repo').as_posix(): digest(p)
             for p in sorted((out/'frozen-repo').rglob('*')) if p.is_file()}
    for row in rows:
        case = out/f"track-{row['track']:02x}"
        case.mkdir()
        patched = patch_module(source, row['track'], a.mode, row['hold_seconds'])
        (case/'wf_music_module.c').write_text(patched, encoding='utf-8', newline='\n')
        row.update(mode=a.mode, module_source_sha256=digest(case/'wf_music_module.c'))
        write_json(case/'case.json', row)
    write_json(out/'prepared.json', dict(status='PREPARED_NOT_BUILT_NOT_EMULATED',
        experiment_only=True, disc_images_created=False, production_changed=False,
        final_candidate_repo=str(repo), mode=a.mode, frozen_sources=files,
        original_module_sha256=files[MODULE.as_posix()], cases=rows,
        limits=['hold-first deliberately suppresses guest music/STOP/fade commands and is NOT their semantics test.',
                '0B/0C/11 cover600s; no complete-loop proof exists in the supplied1200s host evidence.',
                'All source overrides and binaries remain local test artifacts.']))
    print(json.dumps(dict(output=str(out), cases=len(rows), total_music_seconds=sum(r['hold_seconds'] for r in rows),
                          emulated_frames_per_profile=sum(r['emulated_frames'] for r in rows)), indent=2))


LAYOUT_SOURCE = r'''#include "wf_music_seq.h"
#include "wf_runtime.h"
#include <stddef.h>
const unsigned wf_validation_layout[]={
 sizeof(wf_music_seq_t),offsetof(wf_music_seq_t,ram),offsetof(wf_music_seq_t,cycles),
 offsetof(wf_runtime_t,m68k_regs),offsetof(wf_runtime_t,g.frame_handshake),
 offsetof(wf_runtime_t,g.gameplay_vblank_enable),offsetof(wf_runtime_t,frame_stage),
 offsetof(wf_runtime_t,frame_ready)};
'''
LAYOUT_NAMES = ('sequence_bytes','sequence_ram','sequence_cycles','scene_prefix_bytes',
                'scene_frame_handshake','scene_vblank_enable','runtime_frame_stage','runtime_frame_ready')


def build(a):
    data = json.loads((a.suite/'prepared.json').read_text()); frozen=a.suite/'frozen-repo'
    for rel, sha in data['frozen_sources'].items():
        if digest(frozen/rel) != sha: raise ValueError(f'Frozen source changed: {rel}')
    layout = a.suite/'native_layout.c'; layout.write_text(LAYOUT_SOURCE)
    cc = a.toolchain/'bin/sh2eb-elf-gcc'
    asm = a.suite/'native_layout.s'
    subprocess.run([str(cc), '-m2','-mb','-std=c11','-S',str(layout),
                    '-I',str(frozen/'src/audio/native'),'-I',str(frozen/'src/runtime/include'),
                    '-o',str(asm)], check=True)
    import re
    values = [int(x,0) for x in re.findall(r'^\s*\.long\s+(0x[0-9a-fA-F]+|\d+)\s*$', asm.read_text(), re.M)]
    if len(values) != len(LAYOUT_NAMES): raise ValueError('Unexpected layout assembly')
    write_json(a.suite/'abi-layout.json', dict(zip(LAYOUT_NAMES, values)))
    for row in data['cases']:
        case=a.suite/f"track-{row['track']:02x}"; repo=case/'repo'
        if digest(case/'wf_music_module.c') != row['module_source_sha256']:
            raise ValueError('Prepared override changed')
        if repo.exists(): raise ValueError(f'Case already built/prepared: {repo}')
        shutil.copytree(frozen, repo)
        shutil.copyfile(case/'wf_music_module.c',repo/MODULE)
        command=[sys.executable,'-B',str(repo/'tools/audio/native_probe/build_music_module.py'),
                 '--repo',str(repo),'--toolchain',str(a.toolchain),'--sdk',str(a.sdk),
                 '--rom',str(a.rom),'--out',str(case/'build')]
        with (case/'build.log').open('w') as log:
            subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
        # Initial transport build is insufficient for HWRAM execution. The final
        # builder must already resolve hot code to the pinned master workspace.
        sys.path.insert(0,str(REPAIR/'emulator'))
        from cold_overlay import Elf
        elf=Elf(case/'build/MUSIC.DEBUG.ELF')
        hot=elf.symbols['_wf_hotcode_start'][0]; end=elf.symbols['_wf_hotcode_end'][0]
        if not 0x06004000 <= hot < end <= 0x06100000:
            raise ValueError('Builder produced transport-address module; final HWRAM relink is required')
        write_json(case/'build-identity.json',dict(music_elf_sha256=digest(case/'build/MUSIC.ELF'),
            debug_elf_sha256=digest(case/'build/MUSIC.DEBUG.ELF'),hot_address=hot,hot_bytes=end-hot,
            source_override_only=MODULE.as_posix()))
    print('Built18 isolated test modules; no emulator or image builder invoked.')


def plan(a):
    data=json.loads((a.suite/'prepared.json').read_text())
    pins={key:dict(path=str(getattr(a,key).resolve()),sha256=digest(getattr(a,key)))
          for key in ('runtime','scsp','sfx','cue','core','scsp_debug','scsp_layout','sfx_debug','sfx_layout')}
    # CUE data file is pinned too, with explicit read-only sector access at run.
    import re
    name=re.search(r'FILE\s+"([^"]+)"\s+BINARY',a.cue.read_text()).group(1)
    media=a.cue.parent/name;pins['media']=dict(path=str(media.resolve()),sha256=digest(media))
    inputs=[a.suite/'prepared.json',a.suite/'abi-layout.json']
    inputs.extend(p for p in (a.suite/'frozen-repo').rglob('*') if p.is_file())
    inputs.extend(HERE.glob('*.py'))
    inputs.extend([REPAIR/'emulator/cold_overlay.py',REPAIR/'emulator/compare_cold.py',
                   REPAIR/'emulator/observer_identity.py',
                   RUNTIME/'validation/iso_to_bin_exact.py'])
    for row in data['cases']:
        case=a.suite/f"track-{row['track']:02x}"
        inputs.extend([case/'case.json',case/'wf_music_module.c',case/'build-identity.json',
                       case/'build/MUSIC.ELF',case/'build/MUSIC.DEBUG.ELF'])
    # A plan certifies exact per-track artifacts and the code observing them,
    # as well as the common master/core/media. Missing files fail here.
    suite_inputs={str(p.resolve()):digest(p) for p in sorted(set(inputs))}
    jobs=[]
    for row in data['cases']:
        case=a.suite/f"track-{row['track']:02x}"
        for profile,scsp,debug,layout in [('music',a.scsp,a.scsp_debug,a.scsp_layout),
                                         ('sfx',a.sfx,a.sfx_debug,a.sfx_layout)]:
            out=case/f'cold-{profile}'
            command=[sys.executable,'-B',str(HERE/'observe_cold.py'),
                     '--core',str(a.core.resolve()),'--cue',str(a.cue.resolve()),
                     '--runtime',str(a.runtime.resolve()),'--music',str((case/'build/MUSIC.ELF').resolve()),
                     '--scsp',str(scsp.resolve()),'--music-debug',str((case/'build/MUSIC.DEBUG.ELF').resolve()),
                     '--scsp-debug',str(debug.resolve()),'--scsp-layout',str(layout.resolve()),
                     '--layout',str((a.suite/'abi-layout.json').resolve()),'--case',str((case/'case.json').resolve()),
                     '--out',str(out.resolve()),'--frames',str(row['emulated_frames']),
                     '--input',a.input,'--checkpoint',str(row['checkpoint_frames'])]
            if a.wav: command+=['--wav',str((out/'capture.wav').resolve())]
            jobs.append(dict(track=row['track'],profile=profile,command=command,out=str(out.resolve())))
    write_json(a.suite/'execution-plan.json',dict(status='PLANNED_NOT_EXECUTED',pins=pins,
        suite_inputs=suite_inputs,jobs=jobs,
        comparison=dict(game_start=300,game_count=300,max_video_blank_ratio=1.0,minimum_render_delta=0),
        input=a.input,images_created=False,states_loaded=False,guest_memory_edits=False))
    print(f'Planned{len(jobs)} cold boots. No emulator was loaded. Review execution-plan.json.')


def run(a):
    if not a.execute: raise ValueError('Execution requires explicit --execute after root authorization')
    plan=json.loads((a.suite/'execution-plan.json').read_text())
    inputs=plan.get('suite_inputs')
    if not isinstance(inputs,dict) or not inputs:
        raise ValueError('Per-track artifacts and observers are unpinned; regenerate the execution plan')
    for path,expected in inputs.items():
        if not Path(path).is_file() or digest(path)!=expected:
            raise ValueError(f'Pinned suite input changed or is missing: {path}')
    for item in plan['pins'].values():
        if digest(item['path']) != item['sha256']: raise ValueError('Pinned input changed')
    for job in plan['jobs']:
        if a.tracks and job['track'] not in a.tracks: continue
        out=Path(job['out'])
        if out.exists(): raise ValueError(f'Output exists; preserve previous evidence: {out}')
        out.mkdir(parents=True)
        with (out/'run.log').open('w') as log:
            subprocess.run(job['command'],stdout=log,stderr=subprocess.STDOUT,check=True)


def main():
    p=argparse.ArgumentParser(description=__doc__); sub=p.add_subparsers(dest='action',required=True)
    s=sub.add_parser('prepare');s.add_argument('--repo',type=Path,required=True);s.add_argument('--out',type=Path,required=True)
    s.add_argument('--mode',choices=['remap','hold-first'],default='hold-first')
    s.add_argument('--catalogue',type=Path,default=CONTROL/'score-analysis/catalogue180');s.set_defaults(func=prepare)
    s=sub.add_parser('build');s.add_argument('--suite',type=Path,required=True)
    for key in ('toolchain','sdk','rom'):s.add_argument('--'+key,type=Path,required=True)
    s.set_defaults(func=build)
    s=sub.add_parser('plan');s.add_argument('--suite',type=Path,required=True)
    for key in ('runtime','scsp','sfx','cue','core'):s.add_argument('--'+key,type=Path,required=True)
    for key in ('scsp-debug','scsp-layout','sfx-debug','sfx-layout'):s.add_argument('--'+key,type=Path,required=True)
    s.add_argument('--input',choices=['none','tag','royal'],default='tag');s.add_argument('--wav',action='store_true');s.set_defaults(func=plan)
    s=sub.add_parser('run');s.add_argument('--suite',type=Path,required=True);s.add_argument('--tracks',type=int,nargs='*');s.add_argument('--execute',action='store_true');s.set_defaults(func=run)
    a=p.parse_args();a.func(a)


if __name__=='__main__':main()
