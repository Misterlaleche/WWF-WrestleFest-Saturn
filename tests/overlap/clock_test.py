#!/usr/bin/env python3
"""Focused actual-main overlap admission, accepted START and original wait gate.

Reuse independently built C06 objects. The video/time shim replaces hardware,
while actual main/frontend/clock/native owners execute. Source and object hashes
are recorded; actual SDK ownership is checked by the separate device fixture.
"""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
B=Path(__file__).resolve().parent;R=B.parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--objects',type=Path,required=True)
    p.add_argument('--object-source',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    for name in ['start-zero','attract','seam','old-reanchor','no-epoch','charge-transport','wait-close','omit-wait-close']:
        p.add_argument('--'+name,action='store_true')
    args=p.parse_args();v=args.source.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    OLD=args.object_source.resolve();FIX=R/'tests/native';main_source=v/'port/main.c'
    source=(FIX/'jo_main_probe.c').read_text()
    source=source[:source.index('int main(int argc,char **argv)')]
    source=source.replace('#include "wf_saturn_yaul_backend.h"', '#include "wf_saturn_yaul_backend.h"\n#include <stdbool.h>\nvoid jo_core_set_frame_overlap(bool);\nvoid jo_core_synch_callbacks(void);')
    source=source.replace('static unsigned host_fields,host_queued,host_commits,host_sounds;', '''static unsigned host_fields,host_queued,host_commits,host_sounds;
static unsigned focal_hint,focal_epoch,focal_allow_calls,focal_explicit_syncs;
static unsigned focal_input_queries,focal_pad_keys,focal_drain_fields=3u;
static unsigned focal_drain_calls,focal_drain_completed,focal_start_8ae_calls;
static unsigned focal_drain_irq2;
static unsigned focal_arm_clock_boundary,focal_injected_boundary;
static unsigned focal_transport_pending,focal_start_drain_seen;
''')
    source=source.replace('unsigned jo_core_get_vblank_count(void){return host_fields;}', '''unsigned jo_core_get_vblank_count(void)
{unsigned value=host_fields;if(focal_arm_clock_boundary){focal_arm_clock_boundary=0;++host_fields;++focal_injected_boundary;}return value;}''')
    source=source.replace('bool jo_is_input_key_pressed(unsigned port,unsigned key){(void)port;(void)key;return false;}',
                         'bool jo_is_input_key_pressed(unsigned port,unsigned key){++focal_input_queries;return !port&&(focal_pad_keys&key)!=0;}')
    source=source.replace('void __jo_core_synch(void)\n{host_queued=0;host_fields+=host_stride;}', '''void jo_core_set_frame_overlap(bool allow){focal_hint=allow;}
void __jo_core_synch(void)
{focal_hint=0;host_queued=0;host_fields+=host_stride;++focal_explicit_syncs;}
void __jo_core_pause(void){assert(!"Unexpected fatal drain pause in clock probe");}
void jo_core_synch_callbacks(void)
{if(focal_hint){focal_hint=0;host_queued=0;}else __jo_core_synch();}
wf_status_t wf_jo_video_allow_overlap(void)
{assert(host_queued&&arcade_clock.pending);++focal_allow_calls;focal_epoch=1;focal_hint=1;return WF_OK;}
wf_status_t wf_jo_video_drain_overlap_fields(wf_status_t (*service)(void *),void *user,uint32_t *elapsed_fields)
{
    if(elapsed_fields)*elapsed_fields=0;
    focal_hint=0;
    focal_start_drain_seen=1;focal_drain_irq2=frontend.combat_coin_irqs;
    if(!focal_epoch)return WF_OK;
    ++focal_drain_calls;focal_drain_irq2=frontend.combat_coin_irqs;
    for(unsigned i=0;i<focal_drain_fields;i++){
        ++host_fields;
        if(service){wf_status_t s=service(user);if(s!=WF_OK)return s;}
    }
    host_queued=0;focal_epoch=0;++focal_drain_completed;
    if(elapsed_fields)*elapsed_fields=focal_drain_fields;
    if(TEST_CLOCK_SEAM)focal_arm_clock_boundary=1u;
    return WF_OK;
}
wf_status_t wf_jo_video_drain_overlap(wf_status_t (*service)(void *),void *user)
{return wf_jo_video_drain_overlap_fields(service,user,NULL);}
wf_status_t __real_wf_r18_scene_resume(const wf_runtime_t *,uint8_t *,size_t,const uint8_t *,size_t,
    wf_r18_video_t *,const wf_r18_world_ctx_t *,const wf_r18_sprite_record_t *,size_t,const uint16_t *);
wf_status_t __wrap_wf_r18_scene_resume(const wf_runtime_t *r,uint8_t *ram,size_t n,const uint8_t *retained,size_t rn,
    wf_r18_video_t *video,const wf_r18_world_ctx_t *world,const wf_r18_sprite_record_t *records,size_t count,const uint16_t *counts)
{
    wf_status_t s=__real_wf_r18_scene_resume(r,ram,n,retained,rn,video,world,records,count,counts);
    if(s==WF_OK&&focal_transport_pending){host_fields+=TEST_START_TRANSPORT;focal_transport_pending=0;}
    return s;
}
wf_status_t __real_wf_r18_frontend_driver_start_8ae(wf_r18_frontend_driver_t *driver);
wf_status_t __wrap_wf_r18_frontend_driver_start_8ae(wf_r18_frontend_driver_t *driver)
{
    assert(driver==&frontend.driver&&focal_start_drain_seen&&!focal_hint&&!focal_epoch);
    assert(frontend.combat_coin_irqs==focal_drain_irq2);
    ++focal_start_8ae_calls;return __real_wf_r18_frontend_driver_start_8ae(driver);
}''')
    # Linker wrapping lets this fixture use either C06 objects or objects
    # built directly from an exported C07 tree, whose video.o also contains
    # the ordinary host implementations of these new APIs.
    for name in ['allow_overlap','drain_overlap','drain_overlap_fields']:
        source=source.replace('wf_status_t wf_jo_video_'+name+'(',
                              'wf_status_t __wrap_wf_jo_video_'+name+'(')
    source+=(B/'clock_body.c').read_text()
    if args.omit_wait_close:
        m=main_source.read_text()
        needle='    wf_status_t closed=wf_jo_video_drain_overlap(service_during_wait,NULL);\n    if(closed!=WF_OK)return closed;\n'
        assert m.count(needle)==1
        main_source=out/'negative-main.c';main_source.write_text(m.replace(needle,''))
    probe=out/'focal-probe.c';probe.write_text(source)
    flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
           '-DWF_JO_ENGINE','-DWF_JO_ASSETS_HOST','-DZ_SOLO','-DNO_GZIP',
           '-DWF_JO_FRONTEND_TEST','-DWF_JO_FOCAL_CLOCK_SHIM','-DWF_JO_AUTORUN_MODE=2','-DWF_JO_FRONTEND',
           '-DTEST_CLOCK_SEAM='+str(int(args.seam)),
           '-DTEST_START_TRANSPORT='+('3' if args.attract else '0'),
           '-DTEST_ATTRACT='+str(int(args.attract)),
           '-DTEST_START_EPOCH='+str(int(not args.no_epoch)),
           '-DTEST_WAIT_CLOSURE='+str(int(args.wait_close)),
           '-DTEST_OVERLAP_VARIANT='+str(int('combat_start_debt' in (v/'port/wf_jo_frontend.h').read_text())),
           '-DWF_JO_PROBE_MAIN="'+str(main_source)+'"']
    if not args.attract:flags+=['-DWF_JO_AUTORUN']
    flags+=['-I'+str(q) for q in [v/'core/include',v/'port',v/'port/zlib',FIX,FIX/'jo_host',R/'tests']]
    objects=sorted(args.objects.resolve().glob('*.o'));assert objects
    core_names=list(dict.fromkeys(re.findall(r'core/(?:src|generated)/\w+\.c',(v/'core_sources.mk').read_text())))
    for name in ['wp1980_input','wp1980_royal_init']:
        q='core/src/'+name+'.c'
        if q not in core_names:core_names.append(q)
    frozen=core_names+['port/'+n for n in ['wf_jo_assets.c','wf_jo_backend.c','wf_jo_scene_prepare.c']]
    frozen+=['port/zlib/'+n for n in ['inflate.c','inffast.c','inftrees.c','adler32.c','zutil.c']]
    changed_core=[n for n in frozen if (v/n).read_bytes()!=(OLD/n).read_bytes()]
    assert set(changed_core)<={'core/src/wf_r18_ring.c'},changed_core
    commands=[]
    for name in changed_core:
        obj=out/(Path(name).stem+'.o');cmd=['cc',*flags,'-c',str(v/name),'-o',str(obj)];commands.append(cmd);subprocess.run(cmd,check=True)
        objects=[q for q in objects if q.stem!=obj.stem]+[obj]
    frontend_source=v/'port/wf_jo_frontend.c'
    if args.old_reanchor or args.charge_transport:
        f=frontend_source.read_text()
        if args.old_reanchor:
            needle='anchored.pending=0;*f->config.clock=anchored;';assert f.count(needle)==1
            f=f.replace(needle,'anchored.last_field=jo_core_get_vblank_count();'+needle)
        if args.charge_transport:
            needle='anchored.last_field=anchor-drain_fields;';assert f.count(needle)==1
            f=f.replace(needle,'/* Negative control: charge old scene transport. */')
        frontend_source=out/'negative-frontend.c';frontend_source.write_text(f)
    inputs=[main_source,frontend_source,v/'port/wf_jo_frontend.h',v/'port/wf_jo_clock.c',v/'port/wf_jo_video.h',probe,B/'clock_body.c',FIX/'assets_fixture.c',FIX/'jo_main_probe.c']+[v/n for n in frozen]
    inputs += list((v/'core/include').glob('*.h'))+list((v/'port').glob('*.h'))
    manifest={str(q):sha(q) for q in inputs};object_hashes={str(q):sha(q) for q in objects}
    binary=out/'focal-main'
    cmd=['cc',*flags,'-Wl,--wrap=wf_jo_video_allow_overlap','-Wl,--wrap=wf_jo_video_drain_overlap','-Wl,--wrap=wf_jo_video_drain_overlap_fields','-Wl,--wrap=wf_jo_frontend_run','-Wl,--wrap=wf_jo_frontend_after_combat_at_level','-Wl,--wrap=wf_r18_frontend_driver_start_8ae','-Wl,--wrap=wf_r18_scene_resume',str(probe),str(FIX/'assets_fixture.c'),str(frontend_source),str(v/'port/wf_jo_clock.c'),*map(str,objects),'-o',str(binary)];commands.append(cmd)
    with(out/'build.log').open('w')as log:built=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
    if built.returncode:print((out/'build.log').read_text()[-10000:]);raise SystemExit(built.returncode)
    env=dict(os.environ,WF_JO_TEST_CACHE_BYTES='565248',WF_JO_TEST_COMMANDS=str(out/'commands.csv'),WF_JO_TEST_STATS=str(out/'assets.csv'))
    with(out/'run.log').open('w')as log:run=subprocess.run([str(binary),str(out)]+(['start-zero']if args.start_zero else []),cwd=v/'cd',env=env,stdout=log,stderr=subprocess.STDOUT)
    changed=[n for n,h in {**manifest,**object_hashes}.items() if sha(Path(n))!=h]
    result={'status':'PASS'if not changed and run.returncode==0 else'FAIL','scope':__doc__,'source':str(v),'source_sha256':manifest,'objects_sha256':object_hashes,'changed':changed,'returncode':run.returncode,'commands':commands,'output':(out/'run.log').read_text(),'negative_control':bool(args.old_reanchor or args.charge_transport or args.omit_wait_close)}
    (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(result['output'][-6000:]);raise SystemExit(run.returncode or bool(changed))
if __name__=='__main__':main()
