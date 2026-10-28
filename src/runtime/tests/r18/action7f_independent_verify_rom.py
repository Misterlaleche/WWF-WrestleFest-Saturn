"""Read-only independent review: freeze raw7F, rerun author and extra ROM cases.

No production or author-test file is modified. The extra suite is derived only
inside the recorded frozen copy. It executes intact1C03E and its original tick.
"""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,sys,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);frozen=out/'frozen/src/runtime';assert not frozen.exists();frozen.mkdir(parents=True)
for name in ('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
for name in ('Makefile','audit/runtime18-cd/ROM68K.BIN','tests/r18/action7f_verify_rom.py','tests/r18/action7f_oracle_bridge.c','tests/r18/action7f_manifest.json','tests/r18/action7f_snapshot_beetle_stage8b_frame9016.bin'):
 dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest();assert source_hash=='d51b898c821ef3e901b6004057ac50316151f2fcd3186309c6ec007a679e1e72'
body=(frozen/'tests/r18/action7f_verify_rom.py').read_text()
start=body.index('def fixtures():');end=body.index('core=',start)
extra='''def fixtures():
 if args.guards_only:return
 # First-entry latch overrides the old WORD counter before checking BYTEFE.
 for slot in(0,4,8):
  m,a,t=seed(slot);put(m,a+0x1c,0x7f05);put(m,a+0x24,0xfe);put(m,a+0x44,0xffff)
  put(m,a+0x26,0xdeadbeef,4)
  yield 'independent-init-reset-before-unconsumed-link',m,slot,None
 # All high facing bytes, selector aliases, actor slots and self/peer release.
 for high in range(256):
  slot=high%9;m,a,t=seed(slot);other=slot if high%3==0 else (slot+4)%9
  put(m,a+0x2e,(high<<8)|((high*73)&255));put(m,a+0x24,0x00fe);put(m,a+0x44,1)
  put(m,a+0x26,actors[other]|(0,0x40000000,0x80000000,0xff000000)[high%4],4)
  put(m,a+0x60,((high&255)<<8)|0x7f)
  yield 'independent-facing256-self-peer/tagged-bus',m,slot,None
 # Full phase high byte with lowFE; counter byte/word carries and latch masks.
 for high in range(256):
  slot=(high*5)%9;m,a,t=seed(slot)
  put(m,a+0x1c,((0x80|(high&0x7f))<<8)|5);put(m,a+0x24,(high<<8)|0xfe)
  put(m,a+0x22,(0,1,0x100,0x7fff,0x8000,0xff00,0xffff)[high%7])
  put(m,a+0x44,(0,1,2,0xff,0x100,0x7fff,0x8000,0x8001,0xfffe,0xffff)[high%10])
  put(m,a+0x2e,(0x12fe,0x3481,0x567f,0x78ff)[high%4])
  put(m,a+0x26,actors[(slot+high//9)%9]|0x80000000,4)
  yield 'independent-frameBYTE-counterWORD-latch-bit7',m,slot,None
'''
body=body[:start]+extra+body[end:]
body=body.replace('random.Random(0x1a718)','random.Random(0x7f9016a718)')
extra_script=frozen/'tests/r18/action7f_independent_extra_derived.py';extra_script.write_text(body)
hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
 for name in hashes:z.write(frozen/name,name)
reports={}
for name,script in (('author-rerun','action7f_verify_rom.py'),('independent-extra','action7f_independent_extra_derived.py')):
 with(out/(name+'.log')).open('w')as log:r=subprocess.run([sys.executable,str(frozen/'tests/r18'/script),'--report',str(out/(name+'.json'))],cwd=frozen,stdout=log,stderr=subprocess.STDOUT)
 assert r.returncode==0,(name,r.returncode,(out/(name+'.log')).read_text()[-2500:])
 report=json.loads((out/(name+'.json')).read_text());assert report['status']=='PASS'and report['source_sha256']==source_hash
 reports[name]={'cases':report['cases'],'guards':report['retained_guards']};print('PASS',name,reports[name],flush=True)
assert reports=={'author-rerun':{'cases':5551,'guards':31},'independent-extra':{'cases':515,'guards':31}},reports
result={'status':'PASS','production_source_sha256':source_hash,'reports':reports,'sources_sha256':hashes,'frozen_sources_sha256':hashlib.sha256((out/'frozen-sources.zip').read_bytes()).hexdigest(),'limits':['Read-only production and author tests; no new combat/strategy probe','Extra ROM fixtures vary all high facing/phase bytes and WORD counters/aliases/self/peer across9slots','Runtime-export raw16KB/FG16KB/D0-D7/original descriptor compared; address registers/CCR are outside current typed ABI','Native8b capture projection retains9actors and represented globals; does not recreate opaque original RAM']}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sources_sha256'},indent=2))
