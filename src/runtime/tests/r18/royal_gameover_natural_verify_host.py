"""Natural Royal match, then original ranking handoff and title on HOST."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT.parents[1]/'validation/local-20261028/royal-natural-post')
out=parser.parse_args().output.resolve()
out.mkdir(parents=True,exist_ok=True)
assert not (out/'host.log').exists(), 'Use a fresh evidence directory'
make=(ROOT/'Makefile').read_text()
core=re.search(r'^CORE=(.*)$',make,re.M)[1].split()
host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
for source in ('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c',
               'src/wf_r17_dependencies.c','src/wf_r17_referee.c'):
    core.remove(source)
wrappers=['tests/r18/allies_cpu_trace.c','tests/r18/allies_dispatch_trace.c',
          'tests/r18/allies_trace.c','tests/r18/transition_trace.c',
          'tests/r18/referee_trace.c','tests/r18/full_flow_combat_probe.c']
binary=out/'royal_natural_post.exe'
with tempfile.TemporaryDirectory(prefix='wf-royal-post-',ignore_cleanup_errors=os.name=='nt') as td:
    frozen=Path(td)
    for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
    for name in (*wrappers,'tests/r18/frontend_driver_host.c','tests/r18/allies_trace.h'):
        p=frozen/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,p)
    probe=frozen/'tests/r18/full_flow_combat_probe.c'
    s=probe.read_text()
    assert s.count('int main(int argc,char**argv)')==1
    s=s.replace('int main(int argc,char**argv)','static int royal_combat_main(int argc,char**argv)')
    join='combat_frontend(royal);combat_setup();';assert s.count(join)==1
    restore='for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){combat_world.cache[side][i].source=read_be(h.ram+0x16d6+side*128u+i*8u,4);combat_world.cache[side][i].destination=read_be(h.ram+0x16da+side*128u+i*8u,4);}'
    s=s.replace(join,join+restore)
    tail=(HERE/'post_tag_host_tail.inc').read_text().split('static int post_loss_resume(void){')[0]
    assert tail.count('owner!=0x11b6')==1
    tail=tail.replace('owner!=0x11b6','owner!=0x1b1e&&owner!=0x11b6')
    tail += r'''
int main(int argc,char**argv){
 int status=royal_combat_main(argc,argv);fflush(stdout);setvbuf(stdout,NULL,_IONBF,0);if(status)return status;
 assert(wf_r18_royal_game_over(&h.runtime));
 size_t cap,count=wf_r18_present_records_count();wf_r18_sprite_record_t*records=wf_r18_present_records_buffer(&cap);assert(count<=cap);
 assert(wf_r18_scene_resume(&h.runtime,h.ram,sizeof h.ram,h.ram,sizeof h.ram,&h.video,&combat_world,records,count,combat_gc.render_queue.count)==WF_OK);
 for(unsigned i=0;i<4;i++)write_be(h.display+i*2u,combat_display.scroll[i],2);
 h.board[0x10]=combat_priority;h.driver.regs=h.runtime.m68k_regs;h.driver.phase=WF_R18_DRIVER_AC0;
 h.video.read_rom=rom;h.video.read_ram=post_ram_read;h.video.read_user=&h;h.driver.platform.bind_world=post_bind;
 unsigned before=h.frames;
 assert(post_owner(0x1b1e,&combat_paid.machine)==WF_OK);
 assert(post_boot_boundary==0x6fc&&post_ranking_calls==1u);
 memcpy(h.driver.a,combat_paid.machine.a,sizeof h.driver.a);
 uint8_t ranked[0x84];memcpy(ranked,h.ram+0xc2,sizeof ranked);
 assert(wf_r18_frontend_driver_resume_boot(&h.driver,0x6fc)==WF_OK);
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_TITLE;i++){
  assert(i<200000u);
  if(wf_r18_frontend_driver_step(&h.driver)<0){fprintf(stderr,"ROYAL REENTRY block pc=%06x dep=%06x\n",h.driver.pc,h.driver.blocked_dependency);return 1;}
 }
 assert(!memcmp(ranked,h.ram+0xc2,sizeof ranked));
 printf("ROYAL POST PASS boundary=0006fc ranking_calls=%u title_phase=%u frontend_frames=%u\n",post_ranking_calls,h.driver.phase,h.frames-before);
 return 0;
}
'''
    probe.write_text(s+'\n'+tail)
    sources={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest()
             for p in sorted(frozen.rglob('*')) if p.is_file()}
    command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
             '-Iinclude',*core,*host,*wrappers,'-o',str(binary)]
    subprocess.run(command,cwd=frozen,check=True)
    (out/'source-manifest.json').write_text(json.dumps({'sources_sha256':sources,
        'compile_command':command,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()},indent=2)+'\n')
env={**os.environ,'WF_ALLIES_OUT':str(out),'WF_TRACE_ALL_ACTORS':'1','WF_HUMAN_FOLLOW_LOSS':'1'}
cmd=[str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK',
     'audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN','6000','basic','1',str(out)]
with (out/'host.log').open('w') as log:
    result=subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
log=(out/'host.log').read_text();print(log[-7000:],end='')
report={'status':'PASS' if result.returncode==0 and 'ROYAL POST PASS' in log else 'ERROR',
        'returncode':result.returncode,'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2',
        'limits':['HOST device tape and adapted test harness, not a Saturn execution.',
                  'No backup-memory save or manual initials interaction is tested.',
                  'No physical Saturn timing or image claim.']}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
raise SystemExit(0 if report['status']=='PASS' else 1)
