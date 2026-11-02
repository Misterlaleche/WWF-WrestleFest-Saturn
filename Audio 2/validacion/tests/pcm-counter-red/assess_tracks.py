"""Fail-closed gates for a prepared track suite. Audio RMS is not a score oracle."""
import argparse
import json
import sys
from pathlib import Path

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[1]/'emulator'))
from cold_overlay import audio_gate
from compare_cold import stable_span,span_summary

OPTIONS={'beetle_saturn_cpucache_emumode':'full','beetle_saturn_sh2_interleave':'exact',
         'beetle_saturn_sh2_jit':'disabled','beetle_saturn_jit_scu':'disabled','beetle_saturn_jit_scsp':'disabled'}


def assess_profile(report,trace,case,profile):
    blockers=[];rows=report.get('observations',[])
    expected=0x4d46 if profile=='music' else 0x53f1
    ready=[r for r in rows if r.get('native_profile')==expected and r.get('mcu_state')==2]
    if not report.get('load_success'):blockers.append('cold_boot_failed')
    if not report.get('existing_media_unchanged'):blockers.append('original_media_changed_or_unverified')
    if report.get('guest_memory_edits') is not False or report.get('state_loads')!=0:blockers.append('guest_mutation_or_state_load')
    if report.get('disc_images_created') is not False:blockers.append('disc_image_creation_not_excluded')
    if report.get('vfs',{}).get('errors'):blockers.append('vfs_error')
    if any(report.get('accepted_options',{}).get(k)!=v for k,v in OPTIONS.items()):blockers.append('core_options_not_verified')
    gate=audio_gate(rows,trace)
    blockers.extend(gate['blockers'])
    if not ready:blockers.append('required_native_profile_never_ready')
    if any(r.get('fatal_owner',0) for r in trace):blockers.append('guest_fatal')
    for name in (['SCSP.ELF','MUSIC.ELF'] if profile=='music' else ['SCSP.ELF']):
        identity_rows=[r for r in ready if name!='MUSIC.ELF' or r.get('music_state')==2]
        identities=[r.get('loaded_audio_code',{}).get(name,{}).get('executable_samples',[]) for r in identity_rows]
        if not identities or any(not samples or any(not x.get('matches') for x in samples) for samples in identities):blockers.append(name+'_loaded_code_identity_failed')
    game_audio=[r for r in ready if 300<=r.get('game_frame',-1)<=600]
    sfx=[r['accepted_sfx_word'] for r in ready if r.get('game_frame',0)>0 and r.get('accepted_sfx_word') is not None]
    if not sfx:blockers.append('no_observed_accepted_gameplay_sfx_word')
    if profile=='sfx' and not any(r.get('audio_rms',0)>0 for r in game_audio):blockers.append('sfx_output_not_observed_in_comparison_scene')
    result=dict(profile=profile,blockers=blockers,audio_gate=gate,
                ready_checkpoints=len(ready),accepted_gameplay_sfx_words=sorted(set(sfx)),
                nonzero_gameplay_audio_checkpoints=sum(r.get('audio_rms',0)>0 for r in game_audio))
    if profile=='music':
        started=[r for r in ready if r.get('test',{}).get('started')]
        if not started:blockers.append('test_track_never_started')
        if started and any(r.get('music_state')!=2 for r in rows if r['frame']>=started[0]['frame']):blockers.append('music_not_RUNNING')
        if any(r.get('test',{}).get('applied_track')!=case['track'] for r in started):blockers.append('wrong_test_track')
        if any(not r.get('hot_code_identity',{}).get('matches') for r in started):blockers.append('HWRAM_code_identity_failed')
        if any(r.get('music_generation',0)==0 or r.get('music_generation')!=r.get('firmware_generation') or r.get('firmware_generation')!=r.get('firmware_generation_ack') for r in started):blockers.append('music_generation_not_acknowledged')
        for key in ('late_count','late_key_events','dropped_fm','dsp_jump_count'):
            if any(r.get(key,0) for r in started):blockers.append(key)
        if not any(r.get('fm_active_mask',0) for r in started):blockers.append('no_FM_active_mask_observed')
        if not any(r.get('audio_rms',0)>0 for r in started):blockers.append('no_nonzero_output_during_track')
        groups={}
        for r in started:groups.setdefault(r['music_generation'],[]).append(r)
        evidence=[]
        for generation,group in groups.items():
            maximum=max((r.get('score_elapsed_seconds',0) for r in group),default=0)
            active=[r for r in group if r.get('sequence_active')]
            end=[r for r in group if r.get('sequence_active')==0 and r.get('fm_active_mask')==0 and r.get('score_elapsed_seconds',0)>=case.get('host_first_inactive_seconds',0) and active and r['frame']>active[0]['frame']]
            count_progress=max((r.get('music_events',0) for r in group),default=0)>min((r.get('music_events',0) for r in group),default=0)
            required=case['host_required_score_seconds']
            covered=bool(end) if case['required_end_of_track'] else maximum>=required
            evidence.append(dict(generation=generation,maximum_score_seconds=maximum,
                score_duration_covered=covered,event_count_progress=count_progress,
                natural_end_observed=bool(end),active_checkpoints=len(active),
                suppressed={k:max(r.get('test',{}).get(k,0) for r in group) for k in ('suppressed_music','suppressed_stop','suppressed_fade')}))
        if not any(e['score_duration_covered'] and e['event_count_progress'] for e in evidence):blockers.append('track_interrupted_or_insufficient_contiguous_duration_or_events')
        result.update(generations=evidence,coverage_kind=case['coverage_kind'],
            natural_end_rule='Native sequence inactive AND FM mask zero after its host end; zero RMS after a one-shot is allowed.',
            scope='Finite score observation and native delivery; no claim of physical Saturn timing, complete loop for0B/0C/11, or timbral identity from RMS.')
    result['passed']=not blockers
    return result


def compare_pair(music,sfx,mtrace,strace,settings):
    blockers=[]
    if music.get('elf',{}).get('flat_sha256')!=sfx.get('elf',{}).get('flat_sha256'):blockers.append('master_executable_differs')
    mm=music.get('manifest',{});sm=sfx.get('manifest',{})
    changed=sorted(k for k in set(mm)|set(sm) if mm.get(k)!=sm.get(k))
    if changed!=['SCSP.ELF']:blockers.append('filesystem_difference_is_not_SCSP_only')
    for key in ('boot_area_sha256','existing_media_sha256','core_sha256','input_events','accepted_options'):
        if key not in music or music.get(key)!=sfx.get(key):blockers.append(key+'_missing_or_differs')
    start=settings['game_start'];count=settings['game_count']
    spans={'music':stable_span(mtrace,start,count),'sfx':stable_span(strace,start,count)}
    summaries={k:span_summary(v) for k,v in spans.items()}
    scenes={}
    for profile,span in spans.items():
        summary=summaries[profile]
        if not summary['complete']:
            blockers.append(profile+'_stable_span_missing');continue
        if len(summary['frontend_phase_stage_pairs'])!=1:blockers.append(profile+'_scene_changed')
        if summary['fatal_seen'] or summary['backend_counter_resets']:blockers.append(profile+'_fatal_or_backend_reset')
        if summary['diagnostic_render_estimate_median'] is None:blockers.append(profile+'_render_rate_windows_missing')
        final={}
        for r in span:
            if r.get('scene_sha256') and start<=r['game_frame']<start+count:final[r['game_frame']]=r
        if set(final)!=set(range(start,start+count)):blockers.append(profile+'_scene_fingerprint_coverage_incomplete')
        scenes[profile]=final
    compared=0;different=[];workload_different=[]
    if all(k in scenes for k in ('music','sfx')):
        for frame in sorted(set(scenes['music'])&set(scenes['sfx'])):
            a=scenes['music'][frame];b=scenes['sfx'][frame];compared+=1
            if a['scene_sha256']!=b['scene_sha256']:different.append(frame)
            if a.get('emitted_commands')!=b.get('emitted_commands'):workload_different.append(frame)
        if different:blockers.append('gameplay_object_state_differs')
        if workload_different:blockers.append('emitted_render_command_workload_differs')
    ratio=None;render_delta=None
    if all(v.get('complete') for v in summaries.values()):
        m=summaries['music'];s=summaries['sfx']
        if m['frontend_phase_stage_pairs']!=s['frontend_phase_stage_pairs']:blockers.append('frontend_phase_differs')
        if m['boot_seed_observed'] is None or m['boot_seed_observed']!=s['boot_seed_observed']:blockers.append('seed_missing_or_differs')
        ratio=m['video_blanks']/s['video_blanks'] if s['video_blanks'] else None
        mr=m['diagnostic_render_estimate_median'];sr=s['diagnostic_render_estimate_median']
        render_delta=mr-sr if mr is not None and sr is not None else None
        if ratio is None or ratio>settings['max_video_blank_ratio']:blockers.append('music_increases_video_blanks_for_same_game_advances')
        if render_delta is None or render_delta<settings['minimum_render_delta']:blockers.append('music_reduces_render_rate')
    return dict(passed=not blockers,blockers=blockers,changed_files=changed,runs=summaries,
        compared_scene_frames=compared,scene_mismatch_frames=different[:20],render_workload_mismatch_frames=workload_different[:20],
        music_vs_sfx_video_blank_ratio=ratio,music_minus_sfx_median_render_estimate=render_delta,
        scope='Same pinned master, MUSIC, boot, assets and inputs; SCSP profile differs. Gameplay prefix hashes and emitted command counts must match per game frame. Timing applies to this core configuration only.')


def assess(suite):
    prepared=json.loads((suite/'prepared.json').read_text());plan=json.loads((suite/'execution-plan.json').read_text())
    results=[]
    for case in prepared['cases']:
        folder=suite/f"track-{case['track']:02x}";reports={};traces={};profiles={};missing=[]
        for profile in ('music','sfx'):
            run=folder/f'cold-{profile}'
            if not (run/'report.json').is_file() or not (run/'counter-trace.json').is_file():missing.append(profile);continue
            reports[profile]=json.loads((run/'report.json').read_text());traces[profile]=json.loads((run/'counter-trace.json').read_text())
            profiles[profile]=assess_profile(reports[profile],traces[profile],case,profile)
        pair=compare_pair(reports['music'],reports['sfx'],traces['music'],traces['sfx'],plan['comparison']) if not missing else dict(passed=False,blockers=['missing_runs:'+','.join(missing)])
        result=dict(track=case['track'],profiles=profiles,graphics_comparison=pair,passed=not missing and pair['passed'] and all(v['passed'] for v in profiles.values()))
        (folder/'assessment.json').write_text(json.dumps(result,indent=2)+'\n');results.append(result)
    complete=len(results)==18 and all(r['passed'] for r in results)
    summary=dict(status='ALL18_FINITE_EMULATED_CASES_PASS' if complete else 'INCOMPLETE_OR_BLOCKED',passed=complete,
        physical_validation=False,disc_images_created=False,results=results,
        limitations=['hold-first is a TEST ONLY intervention; original STOP/fade semantics require the separate remap case and existing host differential tests.',
                     '0B/0C/11 have finite600s coverage, never a claim of complete loop closure.',
                     'SFX words and callback output are observed in reached gameplay; this does not exhaust every possible SFX or listening/timbre assessment.'])
    (suite/'suite-assessment.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(dict(status=summary['status'],passed_tracks=[r['track'] for r in results if r['passed']]),indent=2))
    return complete


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--suite',type=Path,required=True);args=p.parse_args()
    raise SystemExit(0 if assess(args.suite) else 1)
