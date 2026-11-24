#!/usr/bin/env python3
"""Capture real TAG fourth-match $1F914 entry before $1C1684 guard blocks."""
import json,os,subprocess
from pathlib import Path
from run_tag_campaign_main import ROOT,sha
build=ROOT/'build/tag-policy-search/state5-fixed-native'
output=ROOT/'build/tag-referee-null/capture';output.mkdir(parents=True,exist_ok=True)
fixture=ROOT/'tests/fixtures/tag_referee_null';fixture.mkdir(parents=True,exist_ok=True)
assert not any(fixture.iterdir()),'Never overwrite preserved fixture'
manifest=json.loads((build/'runtime-build.json').read_text());assert not manifest['sources_changed_during_build']
objects=[]
for name,digest in sorted(manifest['objects'].items()):
    assert sha(build/name)==digest;objects.append(str(build/name))
policy=ROOT/'tests/policies/tag_stage4_grapple.h';binary=output/'capture'
flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE','-DWF_JO_ASSETS_HOST',
       '-DWF_JO_FRONTEND_TEST','-DWF_JO_AUTORUN','-DWF_JO_AUTORUN_MODE=1','-DWF_JO_FRONTEND','-DZ_SOLO','-DNO_GZIP',
       '-DWF_CAMPAIGN_INPUT_DRIVER="'+str(policy)+'"']
flags+=['-I'+str(ROOT/p) for p in ('tests/native/jo_host','tests','core/include','port','port/zlib','tests/native')]
command=[os.environ.get('CC','cc'),*flags,*['-Wl,--wrap='+name for name in
         ('wf_jo_frontend_run','wf_jo_frontend_after_combat_at_level','wf_r18_ring_bind','wf_r18_post_tag_step','wf_r17_referee_logic_1f914')],
         str(ROOT/'tests/test_tag_campaign_main.c'),str(ROOT/'tests/capture_tag_referee_null.c'),str(ROOT/'tests/native/assets_fixture.c'),
         str(ROOT/'port/wf_jo_frontend.c'),str(ROOT/'port/wf_jo_clock.c'),*objects,'-o',str(binary)]
subprocess.run(command,cwd=ROOT,check=True)
env=dict(os.environ,WF_CAPTURE_TAG_REFEREE=str(fixture),WF_JO_TEST_CACHE_BYTES='565248',
         WF_POLICY_PERIOD='4',WF_POLICY_SPACING='0',WF_POLICY_VARIANT='3',WF_POLICY_PIN_HEALTH='0',WF_POLICY_PHASE='0')
with (output/'run.log').open('w') as log:
    result=subprocess.run([str(binary),'5','100000',str(output),'4','0','0'],cwd=ROOT/'cd',env=env,stdout=log,stderr=subprocess.STDOUT)
assert result.returncode==2,result.returncode
assert (fixture/'arcade-ram.bin').exists()
sources=[ROOT/'tests/capture_tag_referee_null.c',ROOT/'tests/test_tag_campaign_main.c',ROOT/'port/main.c',policy]
(fixture/'observer-source.c').write_bytes(sources[0].read_bytes())
(fixture/'policy.h').write_bytes(policy.read_bytes())
record=dict(scope='Observed pre-referee owner $1F914, natural TAG stage3 tick1008; controller-only replay from reset.',
            source_trial='build/tag-policy-search/stage4-grapple-grid1/trial004',binary_sha256=sha(binary),runtime_manifest=manifest,
            command=[str(binary),'5','100000',str(output),'4','0','0'],cwd=str(ROOT/'cd'),
            environment={k:v for k,v in env.items() if k.startswith(('WF_POLICY_','WF_JO_TEST_','WF_CAPTURE_'))},
            sources_sha256={str(p.relative_to(ROOT)):sha(p) for p in sources},files={p.name:sha(p) for p in fixture.iterdir()})
(fixture/'manifest.json').write_text(json.dumps(record,indent=2)+'\n');print('CAPTURED',fixture)
