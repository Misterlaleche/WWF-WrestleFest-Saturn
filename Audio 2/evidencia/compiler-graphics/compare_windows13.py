from pathlib import Path
import sys,json,hashlib
here=Path(__file__).resolve().parent
sys.path.insert(0,str(here.parents[1]/'emulator'))
from compare_cold import stable_span,span_summary
reports={};traces={};spans={};summaries={}
for label in ('gcc13-sfx','gcc13win-sfx-observed'):
    folder=here/label
    reports[label]=json.loads((folder/'report.json').read_text())
    traces[label]=json.loads((folder/'counter-trace.json').read_text())
    spans[label]=stable_span(traces[label],300,300)
    summaries[label]=span_summary(spans[label])
old,new=(reports[k] for k in ('gcc13-sfx','gcc13win-sfx-observed'))
changed=sorted(k for k in old['manifest'] if old['manifest'][k]!=new['manifest'].get(k))
blockers=[]
if changed!=['A.BIN']:blockers.append('Unexpected asset differences:'+repr(changed))
for label,r in reports.items():
    if not r.get('load_success') or not r.get('existing_media_unchanged'):blockers.append(label+':boot/media')
    if r.get('vfs',{}).get('errors') or r.get('guest_memory_edits') or r.get('state_loads'):blockers.append(label+':mutation/errors')
    if r['audio_gate']['blockers']:blockers.append(label+':audio control:'+repr(r['audio_gate']['blockers']))
    if not summaries[label]['complete']:blockers.append(label+':missing game300..600')
ratio=render_delta=None
if all(s['complete'] for s in summaries.values()):
    a,b=(summaries[k] for k in ('gcc13-sfx','gcc13win-sfx-observed'))
    if a['boot_seed_observed']!=b['boot_seed_observed'] or a['frontend_phase_stage_pairs']!=b['frontend_phase_stage_pairs']:blockers.append('Seed/phase differs')
    ratio=b['video_blanks']/a['video_blanks']
    render_delta=b['diagnostic_render_estimate_median']-a['diagnostic_render_estimate_median']
    if ratio>1: blockers.append('Windows GCC13 slower combat advancement')
    if render_delta<0:blockers.append('Windows GCC13 lower render diagnostic estimate')
result=dict(scope='Compiler feasibility only: same frozen V4 source and SFX control; different master binaries. No music stability proof or current-graphics-source claim.',
    physical_validation=False,disc_images_created=False,changed_files=changed,runs=summaries,
    windows13_to_linux13_video_blank_ratio=ratio,windows13_minus_linux13_render_estimate=render_delta,
    blockers=blockers,feasible_under_observed_keys=not blockers,
    limitations=['Game phase,seed and game frame bounds alone do not prove identical actor state or draw workload.','rate_value is a render diagnostic estimate, not literal measured framebuffer FPS.','Only this cold scripted scene and emulator configuration are covered.'])
(here/'WINDOWS13-RESULT.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
