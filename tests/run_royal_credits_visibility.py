#!/usr/bin/env python3
"""Host replay of natural Royal/TAG credits; no emulator or game-state writes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from run_tag_campaign_main import input_driver_sources

ROOT=Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir',type=Path,default=ROOT/'build/saturn-campaign-host/runtime')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--driver',type=Path,default=ROOT/'core/src/wf_r18_frontend_driver.c')
    p.add_argument('--mode',choices=('royal','tag'),default='royal')
    p.add_argument('--input-driver',type=Path)
    p.add_argument('--cd-dir',type=Path,default=ROOT/'cd')
    p.add_argument('--pad-tape',action='store_true')
    p.add_argument('--expect-hidden',action='store_true',help='Record the known baseline failure')
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    build=a.build_dir.resolve();manifest=json.loads((build/'runtime-build.json').read_text())
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        obj=build/name;assert sha(obj)==digest,('changed frozen object',name)
        if obj.stem!='wf_r18_frontend_driver':objects.append(str(obj))
    policy=(a.input_driver or ROOT/('tests/policies/royal_closer.h' if a.mode=='royal' else 'tests/saturn/host_policy.h')).resolve()
    sources=[ROOT/'tests/test_royal_credits_visibility.c',ROOT/'tests/test_tag_campaign_main.c',
        ROOT/'tests/test_tag_campaign_game.c',Path(__file__).resolve(),
        ROOT/'tests/native/jo_main_probe.c',ROOT/'port/main.c',ROOT/'port/wf_jo_frontend.c',
        a.driver.resolve(),*input_driver_sources(policy)]
    identities={str(path):sha(path) for path in sources}
    flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE',
        '-DWF_JO_ASSETS_HOST','-DWF_JO_FRONTEND_TEST','-DWF_JO_AUTORUN',
        '-DWF_JO_AUTORUN_MODE='+('2' if a.mode=='royal' else '1'),'-DWF_JO_FRONTEND','-DZ_SOLO','-DNO_GZIP',
        '-DWF_CAMPAIGN_INPUT_DRIVER="'+str(policy)+'"']
    flags+=['-I'+str(ROOT/name) for name in ('tests/native/jo_host','tests','core/include','port','port/zlib','tests/native')]
    command=[os.environ.get('CC','cc'),*flags,*['-Wl,--wrap='+name for name in
        ('wf_jo_frontend_run','wf_jo_frontend_after_combat_at_level','wf_r18_ring_bind','wf_r18_post_tag_step')],
        str(ROOT/'tests/test_royal_credits_visibility.c'),str(ROOT/'tests/native/assets_fixture.c'),
        str(ROOT/'port/wf_jo_frontend.c'),str(ROOT/'port/wf_jo_clock.c'),str(a.driver.resolve()),
        *objects,'-o',str(out/'replay')]
    with (out/'build.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    env=dict(os.environ,WF_VISIBILITY_TRACE=str(out/'visibility.csv'),WF_JO_TEST_CACHE_BYTES='565248',
        WF_POLICY_SWITCH_AT='12000',WF_POLICY_PERIOD='4',WF_POLICY_SPACING='16',
        WF_POLICY_VARIANT='22',WF_POLICY_PIN_HEALTH='24',WF_POLICY_PHASE='0')
    if a.pad_tape:env['WF_POLICY_TAPE']=str(out/'pad-inputs.csv')
    run_command=[str(out/'replay'),*(['1','60000'] if a.mode=='royal' else ['10','300000']),str(out),'0' if a.mode=='royal' else '9','0','0x1b4e']
    with (out/'run.log').open('w') as log:
        result=subprocess.run(run_command,cwd=a.cd_dir.resolve(),env=env,stdout=log,stderr=subprocess.STDOUT)
    text=(out/'run.log').read_text();print(text)
    match=re.search(r'VISIBILITY staff=(\d+) staff_hidden=(\d+) ranking=(\d+) ranking_hidden=(\d+)',text)
    assert result.returncode==0 and match,(result.returncode,'replay did not finish')
    staff,staff_hidden,ranking,ranking_hidden=map(int,match.groups())
    assert staff>0 and ranking>0,'Both original drawing phases must be reached'
    assert all(sha(path)==identities[str(path)] for path in sources),'source changed during replay'
    report=dict(status='BASELINE_DEFECT' if staff_hidden or ranking_hidden else 'PASS',mode=a.mode,
        staff_frames=staff,staff_hidden=staff_hidden,ranking_frames=ranking,ranking_hidden=ranking_hidden,
        sources=identities,command=command,run_command=run_command,
        binary_sha256=sha(out/'replay'),cd_directory=str(a.cd_dir.resolve()),
        input_environment={k:v for k,v in env.items() if k.startswith('WF_POLICY_')},
        events=(out/'campaign-events.log').read_text().splitlines(),
        snapshots_sha256={q.name:sha(q) for q in sorted(out.iterdir()) if q.suffix=='.bin' or q.name.endswith('-state.json') or q.name in ('campaign-events.log','visibility.csv','pad-inputs.csv')},
        runtime_manifest_sha256=sha(build/'runtime-build.json'),
        runtime_source_drift=[name for name,digest in manifest['sources'].items() if sha(ROOT/name)!=digest],
        runtime_scope='Existing immutable host objects; frontend driver compiled explicitly from selected source',
        limitation='Native replay and layer ownership only; no Saturn visual/timing acceptance')
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    assert bool(staff_hidden or ranking_hidden)==a.expect_hidden,report

if __name__=='__main__':main()
