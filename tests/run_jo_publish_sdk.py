#!/usr/bin/env python3
"""Run actual SDK bodies with forced VBlank/DMA progress inside submit.

Reuse sdk_test.py's independently extracted SDK bodies and compile command.
Only generated video code receives two yield/read-only observation hooks;
no scheduling predicate or publication instruction is changed. Negative
irq-mutation deliberately corrupts a ready bit to falsify the seam invariant.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=ROOT);p.add_argument('--setup',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--frames',type=int,default=1200);p.add_argument('--negative-irq-mutation',action='store_true');a=p.parse_args();src=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
setup=json.loads(a.setup.read_text());base=Path(setup['source']);fixture=Path(setup['fixture']);oldout=Path(setup['output_directory'])
video=(src/'port/wf_jo_video.c').read_text();needle='wf_status_t status=append_commands(commands,count);\n    if(status!=WF_OK)return status;';assert video.count(needle)==2
video=video.replace(needle,'wf_test_submit_seam(0,&texture_cache,sizeof texture_cache);\n    '+needle+'\n    wf_test_submit_seam(1,&texture_cache,sizeof texture_cache);',1)
video=video.replace('static wf_jo_texture_cache_t texture_cache;','extern void wf_test_submit_seam(unsigned,const void *,size_t);\nstatic wf_jo_texture_cache_t texture_cache;')
(out/'video-seam.c').write_text(video)
text=(fixture/'sdk_device_test.c').read_text();text=text.replace('int main(int argc,char **argv)','static unsigned seam_calls[2],seam_irqs[2],seam_dma_progress;\nint main(int argc,char **argv)',1)
needle='    assert(present_checks<=displayed&&displayed>=requested_frames&&max_live_maps<=4u);';assert text.count(needle)==1
text=text.replace(needle,needle+'\n    assert(seam_calls[0]&&seam_calls[1]&&seam_irqs[0]&&seam_irqs[1]&&seam_dma_progress);\n    printf("PASS submit seam: calls=%u/%u IRQ=%u/%u DMA-progress=%u\\n",seam_calls[0],seam_calls[1],seam_irqs[0],seam_irqs[1],seam_dma_progress);')
text+='''
void wf_test_submit_seam(unsigned phase,const void *cache,size_t bytes)
{
    uint8_t snapshot[sizeof(wf_jo_texture_cache_t)];
    assert(phase<2u&&bytes==sizeof snapshot);
    memcpy(snapshot,cache,bytes);++seam_calls[phase];
    if(seam_calls[phase]%7u==0u){
        unsigned before=__jo_core_vblank_count,dma_before=dma_checks;
        while(__jo_core_vblank_count==before)host_poll();
        seam_irqs[phase]+=__jo_core_vblank_count-before;
        seam_dma_progress+=dma_checks!=dma_before;
    }
    TEST_MUTATION
    assert(!memcmp(snapshot,cache,bytes));
}
'''
text=text.replace('TEST_MUTATION','((wf_jo_texture_cache_t *)(uintptr_t)cache)->ready[0]^=1u;' if a.negative_irq_mutation else '')
(out/'sdk-seam.c').write_text(text)
command=[]
for arg in setup['build_command']:
 if arg=='-I'+str(oldout):command.append(arg)
 elif arg==str(fixture/'sdk_device_test.c'):command.append(str(out/'sdk-seam.c'))
 elif arg==str(base/'port/wf_jo_video.c'):command.append(str(out/'video-seam.c'))
 elif arg==str(oldout/'sdk-device-test'):command.append(str(out/'sdk-device-test'))
 elif arg.startswith(str(base)+'/'):command.append(str(src)+arg[len(str(base)):])
 elif arg.startswith('-I'+str(base)+'/') and not arg.startswith('-I'+str(fixture)):command.append('-I'+str(src)+arg[len('-I'+str(base)):])
 else:command.append(arg)
# The copied fixture keeps SDK/header include paths and old extraction output.
inputs={str(x):sha(x) for x in (src/'port/wf_jo_video.c',src/'port/wf_jo_texture_cache.h',src/'port/wf_jo_backend.c',src/'sdk/jo_engine/core.c',src/'sdk/jo_engine/vdp1_command_pipeline.c')}
r=subprocess.run(command,capture_output=True,text=True);(out/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
reports=[]
for mode in (['normal'] if a.negative_irq_mutation else ['normal','overlap','drain','edges','lost-frame']):
 run=[str(out/'sdk-device-test'),mode,str(a.frames),'12345'];r=subprocess.run(run,capture_output=True,text=True);(out/(mode+'.log')).write_text(r.stdout+r.stderr)
 expected_negative=a.negative_irq_mutation or mode=='lost-frame'
 assert (r.returncode!=0)==expected_negative,(mode,r.returncode,r.stdout,r.stderr)
 reports.append(dict(mode=mode,expected_negative=expected_negative,returncode=r.returncode,output=r.stdout+r.stderr));print(mode,'EXPECTED-FAIL' if expected_negative else 'PASS',r.stdout[-1000:],flush=True)
assert inputs=={x:sha(Path(x)) for x in inputs},'source changed'
(out/'summary.json').write_text(json.dumps(dict(status='PASS',source=str(src),inputs=inputs,setup_sha256=sha(a.setup),binary_sha256=sha(out/'sdk-device-test'),command=command,negative_irq_mutation=a.negative_irq_mutation,runs=reports),indent=2)+'\n')
