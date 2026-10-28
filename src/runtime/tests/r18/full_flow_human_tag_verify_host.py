"""Search ordinary pad policies from manual TAG frontend, using frozen owners.

No runtime/actor/seed/result editing. WIN requires original result BYTE.bit6
in one of the four player records; MATCH COMPLETE alone is not a victory.
"""
from pathlib import Path
import argparse,hashlib,itertools,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];PROBE='tests/r18/full_flow_human_tag_probe.c'
p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);p.add_argument('--frames',type=int,default=16000);p.add_argument('--policy',action='append');p.add_argument('--binary',type=Path);p.add_argument('--sweep',action='store_true');p.add_argument('--post-follow',action='store_true');p.add_argument('--loss-follow',action='store_true',help='Follow an actual loss through records and the original6FC reentry; requires --post-follow');p.add_argument('--next-combat-frames',type=int,default=0);args=p.parse_args()
out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);assert not(out/'summary.json').exists();assert 1<=args.frames<=20000
assert 0<=args.next_combat_frames<=20000 and(not args.next_combat_frames or args.post_follow)
assert not args.loss_follow or(args.post_follow and not args.next_combat_frames)
policies=args.policy or ['3,4,2,8,4,0,1']
if args.sweep:policies=[f'{s},{period},{on},{reach},{tol},{bias},1'for s,period,on,reach,tol,bias in itertools.product((0,2,3,4,5),(4,8),(2,4),(8,24),(4,),(0,24))if period-on>=2 and on>=2]
make=(ROOT/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split();binary=args.binary.resolve()if args.binary else out/'human-tag-probe.exe'
if args.post_follow and 'src/wf_r18_post_tag.c'not in host:host.append('src/wf_r18_post_tag.c')
if not args.binary:
 with tempfile.TemporaryDirectory(prefix='wf-human-tag-',ignore_cleanup_errors=os.name=='nt')as t:
  frozen=Path(t)
  for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
  for name in(PROBE,'tests/r18/full_flow_combat_probe.c','tests/r18/frontend_driver_host.c','tests/r18/post_tag_host_tail.inc'):
   dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
  if args.post_follow:
   probe=frozen/'tests/r18/full_flow_combat_probe.c';s=probe.read_text();assert s.count('int main(int argc,char**argv)')==1
   s=s.replace('int main(int argc,char**argv)','static int post_tag_combat_main(int argc,char**argv)')
   a=s.index('   uint32_t before=combat_waits;',s.index('wf_r18_tag_match_complete(&h.runtime)'));b=s.index('\n  }',a);assert 'wf_r18_result_in_ring'in s[a:b]
   s=s[:a]+'''   unsigned winners=0;for(unsigned i=0;i<4;i++)if(wf_u8(&h.runtime.actors[i],WF_A_RESULT)&0x40u)winners|=1u<<i;
   printf("HUMAN_TAG_OUTCOME %s frame=%u winners=%x result=%04x,%04x,%04x,%04x; native post pending\\n",winners?"WIN":"LOSS",h.runtime.g.frame_counter,winners,wf_be16(&h.runtime.actors[0],0xa0),wf_be16(&h.runtime.actors[1],0xa0),wf_be16(&h.runtime.actors[2],0xa0),wf_be16(&h.runtime.actors[3],0xa0));fflush(stdout);if(!winners&&!getenv("WF_HUMAN_FOLLOW_LOSS"))return 2;return 0;'''+s[b:]
   join='combat_frontend(royal);combat_setup();';assert s.count(join)==1
   restore='for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){combat_world.cache[side][i].source=read_be(h.ram+0x16d6+side*128u+i*8u,4);combat_world.cache[side][i].destination=read_be(h.ram+0x16da+side*128u+i*8u,4);}'
   s=s.replace(join,join+restore);s+='\nint human_tag_next_combat(unsigned);\n#include "post_tag_host_tail.inc"\n';probe.write_text(s)
   tail=frozen/'tests/r18/post_tag_host_tail.inc';s=tail.read_text()
   old='if(s.request.kind==WF_R18_POST_CALL)st=post_owner(s.request.owner,m);';assert s.count(old)==1
   new='''if(s.request.kind==WF_R18_POST_CALL){
   if(s.request.owner==0xb608||s.request.owner==0xbda6||s.request.owner==0xae20)printf("POST OWNER %06x call=%06x return=%06x progress=%u seed=%08x\\n",s.request.owner,s.request.call_pc,s.request.return_pc,h.ram[0x163],read_be(h.ram+0x5c,4));
   st=post_owner(s.request.owner,m);
   if(st==WF_OK&&s.request.owner==0x2836&&s.entry_pc==0xbda6){
    static unsigned seen;unsigned phase=read_be(h.ram+0x12fa,2),bit=phase==1?1u:phase==6?2u:phase==16?4u:0u;
    if(bit&&!(seen&bit)){seen|=bit;char name[128];
     snprintf(name,sizeof name,"post-interview-phase-%02u-ram.bin",phase);combat_dump(name,h.ram,sizeof h.ram);
     snprintf(name,sizeof name,"post-interview-phase-%02u-fg.bin",phase);combat_dump(name,h.video.fg0,sizeof h.video.fg0);
     snprintf(name,sizeof name,"post-interview-phase-%02u-background.bin",phase);combat_dump(name,h.video.background,sizeof h.video.background);
     snprintf(name,sizeof name,"post-interview-phase-%02u-palettes.bin",phase);combat_dump(name,h.video.palettes,sizeof h.video.palettes);
     snprintf(name,sizeof name,"post-interview-phase-%02u-fgpalette.bin",phase);combat_dump(name,h.video.fg_palette,sizeof h.video.fg_palette);
     snprintf(name,sizeof name,"post-interview-phase-%02u-bgpalette.bin",phase);combat_dump(name,h.video.bg_palette,sizeof h.video.bg_palette);
     snprintf(name,sizeof name,"post-interview-phase-%02u-dregs.bin",phase);combat_dump(name,m->regs->d,sizeof m->regs->d);
     snprintf(name,sizeof name,"post-interview-phase-%02u-aregs.bin",phase);combat_dump(name,m->a,sizeof m->a);
    }
   }
  }'''
   s=s.replace(old,new);old='combat_dump("next-f8c-ram.bin",h.ram,sizeof h.ram);return 0;';assert s.count(old)==1
   s=s.replace(old,'combat_dump("next-f8c-ram.bin",h.ram,sizeof h.ram);combat_dump("next-f8c-dregs.bin",h.driver.regs.d,sizeof h.driver.regs.d);combat_dump("next-f8c-aregs.bin",h.driver.a,sizeof h.driver.a);printf("NEXT IDENTITY picks=%u,%u,%u,%u progress=%u previous=%u\\n",h.ram[0x607],h.ram[0x713],h.ram[0xa37],h.ram[0xb43],h.ram[0x163],h.ram[0x165]);const char*n=getenv("WF_HUMAN_NEXT_FRAMES");return n?human_tag_next_combat((unsigned)strtoul(n,NULL,10)):0;')
   tail.write_text(s)
  hashes={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
  archive=Path(shutil.make_archive(str(out/'frozen-sources'),'zip',frozen))
  command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,PROBE,'-o',str(binary)]
  subprocess.run(command,cwd=frozen,check=True)
  (out/'source-manifest.json').write_text(json.dumps({'sources_sha256':hashes,'frozen_sources_zip_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'compile_command':command,'post_follow':args.post_follow,'loss_follow':args.loss_follow,'test_only_transformations':['freeze included probe and frontend harness','post-follow only: stop at original terminal before typed131wait segment; original11B6 owns all waits','post-follow only: original world-cache handoff and scene_resume/post_tag_host_tail composition; observational interview snapshots','loss-follow only: continue an actual terminal loss into the original post/records/6FC owners; no result mutation'],'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()},indent=2)+'\n')
results=[]
for i,policy in enumerate(policies):
 dest=out/f'policy-{i:03d}';dest.mkdir(exist_ok=False)
 env={k:v for k,v in os.environ.items()if k not in('WF_CAPTURED_HANDOFF_RAM','WF_CAPTURED_HANDOFF_DREGS','WF_ALLIES_OUT','WF_TRACE_ALL_ACTORS')};env['WF_HUMAN_POLICY']=policy
 if args.loss_follow:env['WF_HUMAN_FOLLOW_LOSS']='1'
 else:env.pop('WF_HUMAN_FOLLOW_LOSS',None)
 if args.next_combat_frames:env['WF_HUMAN_NEXT_FRAMES']=str(args.next_combat_frames)
 else:env.pop('WF_HUMAN_NEXT_FRAMES',None)
 command=[str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN',str(args.frames),'idle','0',str(dest)]
 with(dest/'host.log').open('w')as log:r=subprocess.run(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
 text=(dest/'host.log').read_text();outcome=re.search(r'HUMAN_TAG_OUTCOME (WIN|LOSS).*',text);block=re.search(r'BLOCK frame=.*',text)
 rows=(dest/'human-input.csv').read_text().splitlines()if(dest/'human-input.csv').exists()else[]
 post_ok=bool(re.search(r'NEXT MATCH status=0 .*flow=000f8c world=5',text))and'POST TAG AC0'in text
 second=re.search(r'SECOND COMBAT PASS .*',text)
 loss_return=re.search(r'LOSS RETURN PASS .*',text)
 entry={'policy':policy,'status':'BLOCKED'if block else'LOSS'if outcome and outcome[1]=='LOSS'and r.returncode==2 else'ERROR'if r.returncode!=0 else outcome[1]if outcome else'RUNNING','returncode':r.returncode,'outcome':outcome[0]if outcome else None,'post_follow':args.post_follow,'post_follow_complete':post_ok,'loss_follow':args.loss_follow,'loss_return_pass':loss_return[0]if loss_return else None,'next_combat_frames':args.next_combat_frames,'next_combat_pass':second[0]if second else None,'post_owner_calls':re.findall(r'POST OWNER .*',text),'block':block[0]if block else None,'last_input':rows[-1]if len(rows)>1 else None,'directory':str(dest),'command':command,'input_tape_sha256':hashlib.sha256((dest/'human-input.csv').read_bytes()).hexdigest()if rows else None,'dump_sha256':{f.name:hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(dest.glob('*.bin'))}}
 if args.post_follow and r.returncode==0:
  if outcome and outcome[1]=='LOSS':assert args.loss_follow and loss_return,'Loss follow did not return through original6FC to title'
  else:assert post_ok,'Post follow did not reach original next world5 F8C'
 if args.next_combat_frames and r.returncode==0:assert second,'Second match did not complete requested combat frames'
 (dest/'report.json').write_text(json.dumps(entry,indent=2)+'\n');results.append(entry)
 summary={'results':results,'win':any(x['status']=='WIN'for x in results),'post_follow':args.post_follow,'loss_follow':args.loss_follow,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'limits':['HOST input policy validation, not physical timing','Original manual frontend and paidF8C retained; no direct boot or captured handoff','Policy function runtime purity asserted every new frame','No actor/health/seed/result/config/DIP writes','Original result BYTEA0.bit6 required; optional post-follow uses original controllers with live portable frontend IRQ','No trace wrappers in search binary; blocked native context and raw actors dumped by unchanged probe']}
 (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 print(i,policy,entry['status'],entry['outcome']or entry['block']or entry['last_input'],flush=True)
 if entry['status']in('WIN','ERROR'):break
print('Human TAG winner found:',any(x['status']=='WIN'for x in results))
