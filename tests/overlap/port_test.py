#!/usr/bin/env python3
"""Real port lifecycle + overlap closure against two delayed command buffers.

The C06 pipeline suite stays intact; fixture adaptations add wrap-safe mock
ordering, two buffers and the manually delayed display model for the drain.
Production source is never rewritten by this runner.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
B=Path(__file__).resolve().parent;R=B.parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=R);p.add_argument('--output',type=Path,default=R/'build/overlap-recovery/port');a=p.parse_args()
V=a.source.resolve();O=a.output.resolve();O.mkdir(parents=True,exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
fixture=V/'tests/test_jo_pipeline.c';source=fixture.read_text()
changes={
'static wf_jo_command_t built[1000];':'static wf_jo_command_t built_buffers[2][1000];\nstatic wf_jo_command_t *built=built_buffers[0];\nstatic unsigned drain_fixture;',
'if(live[i].tag>=tag)':'if((int32_t)(live[i].tag-tag)>=0)',
'if(!show_valid||live[i].tag>show_tag){display(live[i].tag);return;}':'if((!show_valid||(int32_t)(live[i].tag-show_tag)>0)&&(!drain_fixture||live[i].tag!=build_tag-1u)){display(live[i].tag);return;}',
'assert(!"The test has no pending frame that can release a bank");':'if(drain_fixture){++field_count;in_vblank=1;vblank();in_vblank=0;return;}\n    assert(!"The test has no pending frame that can release a bank");',
'++build_tag;built_count=0;':'++build_tag;built_count=0;\n    if(drain_fixture)built=built==built_buffers[0]?built_buffers[1]:built_buffers[0];',
'assert(wf_jo_video_submit(bank,&command,1)==WF_OK);snapshot_bank=bank;':'if(drain_fixture){wf_jo_command_t *w=wf_jo_video_commands_begin(1);assert(w);*w=command;assert(wf_jo_video_submit(bank,w,1)==WF_OK);}\n    else assert(wf_jo_video_submit(bank,&command,1)==WF_OK);snapshot_bank=bank;',
'int main(void)':'static int pipeline_legacy_main(void)',
}
# publish() has the same submit statement in a later independent sparse test;
# only the first (the helper used by the new drain fixture) is adapted.
for old,new in changes.items():
 count=source.count(old)
 assert count==1 or (old.startswith('assert(wf_jo_video_submit(bank,&command') and count==2),(old,count)
 source=source.replace(old,new,1)
source+='\n'+(B/'port_drain_test.inc').read_text();generated=O/'pipeline-and-drain.c';generated.write_text(source)
inputs=[fixture,B/'port_drain_test.inc',B/'wf_jo_video_test_platform.h',V/'port/wf_jo_video.c',V/'port/wf_jo_video.h',V/'port/wf_jo_backend.c',V/'core/src/wf_saturn_memory.c']
inputs+=[q for q in (V/'port').glob('*.h')];before={str(q):sha(q) for q in inputs}
binary=O/'pipeline-and-drain';command=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-int-to-pointer-cast','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DWF_PLATFORM_SATURN=1','-DWF_JO_ENGINE=1','-DWF_JO_VIDEO_TEST=1','-DJO_COMPILE_WITH_DUAL_CPU_SUPPORT=1','-DWF_JO_DUAL_COMMAND_BUFFERS=1','-I'+str(V/'core/include'),'-I'+str(V/'port'),'-I'+str(B),str(generated),str(V/'port/wf_jo_video.c'),str(V/'port/wf_jo_backend.c'),str(V/'core/src/wf_saturn_memory.c'),'-Wl,--gc-sections','-o',str(binary)]
with(O/'build.log').open('w')as log:build=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
if build.returncode:print((O/'build.log').read_text());raise SystemExit(build.returncode)
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
with(O/'run.log').open('w')as log:run=subprocess.run([str(binary)],env=env,stdout=log,stderr=subprocess.STDOUT)
changed=[str(q) for q in inputs if before[str(q)]!=sha(q)]
result={'status':'PASS'if not changed and run.returncode==0 else'FAIL','returncode':run.returncode,'source_sha256':before,'source_changed':changed,'build_command':command,'fixture_changes':changes,'output':(O/'run.log').read_text()}
(O/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(result['output']);raise SystemExit(run.returncode or bool(changed))
